// Zweiklang Coop alpha 0.1: original two-player presence adapter.
// No NPC instances, AI routines, quests or item scripts are replaced.
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <atomic>
#include <map>
#include <deque>
#include <memory>
#include "coop/CombatProtocol.h"
#include "coop/ActiveAnimations.h"
#include "coop/NetworkMotion.h"
#include "coop/LootProtocol.h"
#include "coop/Recovery.h"
#include "coop/WorldProtocol.h"
#include "coop/SessionProtocol.h"
#include "coop/CavalornPolicy.h"
#include "coop/QolPolicy.h"
#include "coop/Network.h"
#include "coop/Protocol.h"
#include "coop/Diagnostics.h"
#include "coop/Settings.h"
#include "coop/KeyboardText.h"
#include "coop/LocalAddress.h"
#include "UnionAfx.h"
#include "coop/EngineAdapter.h"

#if defined(__G1)
namespace gothic = Gothic_I_Classic;
static constexpr unsigned int kEngine = Engine_G1;
#elif defined(__G2A)
namespace gothic = Gothic_II_Addon;
static constexpr unsigned int kEngine = Engine_G2A;
#else
#error Build the adapter with __G1 or __G2A.
#endif

namespace zweiklang::engine {
using namespace gothic;

namespace {
Diagnostics diagnostics;
net::Network network;
Options options;
bool coopTextEditing=false;
int menuAddressRole=-1;
oCNpc* appearancePlayer=nullptr;
int appliedAppearance=-1;
net::Appearance remoteAppearance=net::Appearance::Nameless;
std::atomic<bool> diagnosticsReady{false},captureEngineMessages{true};
std::atomic<int> activeLogLevel{1};
unsigned long long lastStateLog = 0;
int previousHp = -1, previousMana = -1, previousDialog = -1;
std::string previousPeerName;
bool haveState = false;
std::string systemDirectory;
std::string iniPath;
std::string hostAddress = "127.0.0.1";
std::string contentId;
std::string meshName = "HUM_BODY_NAKED0.ASC";
std::string headName = "HUM_HEAD_PONY.MMB";
std::string notice = "Ready: F6 host / F7 join / F8 leave";
std::string lastLoggedStatus;
unsigned short port = 12345;
int hostKey = KEY_F6;
int joinKey = KEY_F7;
int leaveKey = KEY_F8;
bool loading = true;
bool saving = false;
bool proxyFailed = false;
zCVob* remote = nullptr;
zCWorld* remoteWorld = nullptr;
zCModel* remoteModel = nullptr;
net::Snapshot target{};
bool haveTarget = false;
bool isMoving = false;
unsigned long long lastFrame = 0;
unsigned long long lastSend = 0;
unsigned long long movingUntil = 0;
unsigned long long lastRemoteLog=0,lastSnapshotAt=0;
int remoteAnimation=-1;
HANDLE faultFile=INVALID_HANDLE_VALUE;
PVOID faultObserver=nullptr;
volatile LONG faultCount=0;
std::string sessionWorld;
session::Ready peerReadiness;
bool havePeerReadiness=false;
bool diagnosticOverlay=true;
unsigned markerStep=1;

unsigned long long Now() { return GetTickCount64(); }

std::string ConvertText(const std::string& value,unsigned int from,unsigned int to) {
    if(value.empty())return {};
    const int count=MultiByteToWideChar(from,0,value.data(),static_cast<int>(value.size()),nullptr,0);
    if(count<=0)return value;
    std::wstring wide(count,L'\0');MultiByteToWideChar(from,0,value.data(),static_cast<int>(value.size()),&wide[0],count);
    const int bytes=WideCharToMultiByte(to,0,wide.data(),count,nullptr,0,nullptr,nullptr);
    if(bytes<=0)return value;
    std::string out(bytes,'\0');WideCharToMultiByte(to,0,wide.data(),count,&out[0],bytes,nullptr,nullptr);return out;
}

std::string ReadIni(const char* section, const char* key, const char* fallback) {
    char buffer[1024]{};
    GetPrivateProfileStringA(section, key, fallback, buffer, sizeof(buffer), iniPath.c_str());
    return buffer;
}

void Log(const std::string& message) {
    diagnostics.Write(1,"lifecycle",message);
}

// First-chance observation only: never handle an exception or replace Union's
// exception filter. Use a separate pre-opened file, no engine access or log mutex.
LONG CALLBACK ObserveFault(EXCEPTION_POINTERS* p) {
    if(!p||!p->ExceptionRecord||!p->ContextRecord||faultFile==INVALID_HANDLE_VALUE||activeLogLevel.load()==0)return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code=p->ExceptionRecord->ExceptionCode;
    if(code!=EXCEPTION_ACCESS_VIOLATION&&code!=EXCEPTION_ILLEGAL_INSTRUCTION&&code!=EXCEPTION_INT_DIVIDE_BY_ZERO&&code!=EXCEPTION_IN_PAGE_ERROR)return EXCEPTION_CONTINUE_SEARCH;
    if(InterlockedIncrement(&faultCount)>16)return EXCEPTION_CONTINUE_SEARCH;
    char line[768]{};const auto* c=p->ContextRecord;
    const int count=sprintf_s(line,"{\"event\":\"first_chance_fault\",\"tick_ms\":%llu,\"pid\":%lu,\"thread\":%lu,\"code\":\"0x%08lX\",\"eip\":\"0x%08lX\",\"esp\":\"0x%08lX\",\"ebp\":\"0x%08lX\",\"eax\":\"0x%08lX\",\"ecx\":\"0x%08lX\",\"edx\":\"0x%08lX\",\"access\":%llu,\"address\":%llu}\n",
        GetTickCount64(),GetCurrentProcessId(),GetCurrentThreadId(),code,c->Eip,c->Esp,c->Ebp,c->Eax,c->Ecx,c->Edx,
        p->ExceptionRecord->NumberParameters>0?static_cast<unsigned long long>(p->ExceptionRecord->ExceptionInformation[0]):0,
        p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long long>(p->ExceptionRecord->ExceptionInformation[1]):0);
    if(count>0){DWORD written=0;WriteFile(faultFile,line,count,&written,nullptr);FlushFileBuffers(faultFile);}
    return EXCEPTION_CONTINUE_SEARCH;
}

void LogRuntime() {
    char exe[MAX_PATH]{};GetModuleFileNameA(nullptr,exe,MAX_PATH);
    Log(std::string("Runtime exe=")+exe+" plugin="+systemDirectory+" log="+diagnostics.Path()+" content="+contentId);
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
    if(snapshot==INVALID_HANDLE_VALUE)return;
    MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);
    if(Module32FirstW(snapshot,&entry))do {
        char path[MAX_PATH*4]{};WideCharToMultiByte(CP_UTF8,0,entry.szExePath,-1,path,sizeof(path),nullptr,nullptr);
        std::ostringstream text;text<<"base="<<static_cast<void*>(entry.modBaseAddr)<<" bytes="<<entry.modBaseSize<<" path="<<path;
        diagnostics.Write(2,"module",text.str());
    }while(Module32NextW(snapshot,&entry));
    CloseHandle(snapshot);
}

int ParseKey(std::string value, int fallback) {
    for (char& c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (value.size() < 2 || value[0] != 'F') return fallback;
    char* end = nullptr;
    const long number = std::strtol(value.c_str() + 1, &end, 10);
    if (!end || *end != '\0') return fallback;
    if (number >= 1 && number <= 10) return KEY_F1 + static_cast<int>(number) - 1;
    if (number == 11) return KEY_F11;
    if (number == 12) return KEY_F12;
    return fallback;
}

void ReadConfiguration() {
    hostAddress = ReadIni("Network", "Host", "127.0.0.1");
    options.client=ReadIni("Network","Client","0.0.0.0");
    options.asHost=_stricmp(ReadIni("Network","Role","Host").c_str(),"Client")!=0;
    options.female=GetPrivateProfileIntA("Avatar","Gender",0,iniPath.c_str())==1;
    options.friendlyFire=GetPrivateProfileIntA("Combat","FriendlyFire",0,iniPath.c_str())!=0;
    contentId = ReadIni("Network", "ContentId", "");
    options.name=ReadIni("Player","Name","Spieler");
    if(!ValidName(options.name))options.name="Spieler";
    const unsigned int configuredPort = GetPrivateProfileIntA("Network", "Port", 12345, iniPath.c_str());
    port = configuredPort >= 1024 && configuredPort <= 65535 ? static_cast<unsigned short>(configuredPort) : 12345;
    options.host=hostAddress; options.port=port;
    diagnosticOverlay=GetPrivateProfileIntA("Diagnostics","Overlay",1,iniPath.c_str())!=0;
    markerStep=GetPrivateProfileIntA("Diagnostics","NextTestStep",1,iniPath.c_str());if(markerStep<1||markerStep>65)markerStep=1;
    options.logLevel=static_cast<int>(GetPrivateProfileIntA("Logging","Level",1,iniPath.c_str()));
    if(options.logLevel<0||options.logLevel>2)options.logLevel=1;
    activeLogLevel=options.logLevel;
    options.engineLog=GetPrivateProfileIntA("Logging","EngineMessages",1,iniPath.c_str())!=0;
    captureEngineMessages=options.engineLog;
    diagnostics.Configure(options.logLevel,options.name);
    network.SetPlayerName(options.name);
    network.SetAppearance(options.female?net::Appearance::Velaya:net::Appearance::Nameless);
    meshName = ReadIni("Avatar", "Mesh", "HUM_BODY_NAKED0.ASC");
    headName = ReadIni("Avatar", "Head", "HUM_HEAD_PONY.MMB");
    if (headName.find('.') == std::string::npos) headName += ".MMB";
    hostKey = ParseKey(ReadIni("Controls", "HostKey", "F6"), KEY_F6);
    joinKey = ParseKey(ReadIni("Controls", "JoinKey", "F7"), KEY_F7);
    leaveKey = ParseKey(ReadIni("Controls", "LeaveKey", "F8"), KEY_F8);
    diagnostics.Write(1,"configuration","host="+options.host+" client="+options.client+" port="+std::to_string(options.port)+" role="+(options.asHost?"Host":"Client")+" gender="+std::to_string(options.female)+" log_level="+std::to_string(options.logLevel)+" engine_log="+std::to_string(options.engineLog)+" content="+contentId);
}

std::string CurrentWorld() {
    if (!ogame || !ogame->GetWorld()) return {};
    auto* world = ogame->GetGameWorld();
    std::string value = world->worldFilename.ToChar();
    if (value.empty()) value = world->GetWorldName().ToChar();
    const auto slash = value.find_last_of("/\\");
    if (slash != std::string::npos) value.erase(0, slash + 1);
    for (char& c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}

void LootReceive(const std::vector<std::uint8_t>&);
void LootReset();
bool RecoveryTransfer(oCItemContainer*,int);
bool RecoveryCanLoot();
void RecoveryReceive(const std::vector<std::uint8_t>&);
void RecoverySessionReset();
struct NpcBinding;
void WorldReset();
void WorldReceive(const std::vector<std::uint8_t>&);
void WorldNpcVisual(combat::Npc&,oCNpc*);
void WorldApplyNpc(NpcBinding&,const combat::Npc&);
NpcBinding* WorldCreateNpc(const combat::Npc&);
bool WorldTradePaused(oCNpc*);
bool WorldLocalTalksTo(oCNpc*);
unsigned WorldLocalInteraction();
bool WorldRecoveryEffect(const recovery::Record&,bool,bool&);
bool WorldHostRequest(recovery::Message&);
oCItemContainer* WorldTradeUI(oCNpc*);
bool WorldTradeBusy(oCNpc*);
void WorldTradeClose();
void WorldTradeSnapshot(bool);
void WorldPrepareMerchant(oCNpc*);
bool WorldValidatePrice(const recovery::Message&);
void SessionReset();
void SessionReceive(const std::vector<std::uint8_t>&);
void SessionFrame(unsigned long long);
void QuestSessionReset();
bool QuestRecoveryEffect(const recovery::Record&,bool,bool&);
void QuestReceiveReward(const recovery::Message&);
void QuestFrame(unsigned long long);
bool QuestAllowInfo(oCInfo*);
unsigned QolSprint(oCNpc*);
void QolApplySprint(oCNpc*,unsigned);
bool QolPrepareCorpse(oCNpc*);
void QolNotify(unsigned,unsigned);
#include "CombatBridge.inl"
#include "LootBridge.inl"
#include "RecoveryBridge.inl"
#include "WorldBridge.inl"
#include "CavalornBridge.inl"
#include "SessionBridge.inl"
#include "QolBridge.inl"

void RemoveProxy() {
    if (remote) {
        diagnostics.Write(1,"proxy_remove","Removing partner visual from world");
        // Own creation reference remains held until after removal from the world.
        if (remote->homeWorld && remote->homeWorld == remoteWorld) remote->RemoveVobFromWorld();
        remote->Release();
    }
    remote = nullptr;
    remoteWorld = nullptr;
    remoteModel = nullptr;
    haveTarget = false;
    isMoving = false;
    remoteAnimation=-1;movingUntil=0;lastRemoteLog=0;
}

void Leave(const char* reason) {
    Log(reason);
    network.Stop();
    QolGuard(false);
    CombatReset();
    RemoveProxy();
    sessionWorld.clear();
    proxyFailed = false;
    notice = reason;
    Log(notice);
    diagnostics.Role("OFFLINE");haveState=false;previousPeerName.clear();
}

void Animate(bool moving);
bool CreateProxy() {
    if (remote || proxyFailed || !haveTarget || !ogame || !player) return remote != nullptr;
    remote = CreateCombatActor();
    if(!remote)return false;
    remoteAppearance=network.PeerAppearance();
    const bool female=remoteAppearance==net::Appearance::Velaya;
    meshName=female?"HUM_BODY_BABE0.ASC":"HUM_BODY_NAKED0.ASC";
    headName=female?"HUM_HEAD_BABE.MMB":"HUM_HEAD_PONY.MMB";
    remote->SetObjectName(zSTRING("ZWEIKLANG_REMOTE_VISUAL"));
    remote->dontWriteIntoArchive = TRUE;
    remote->showVisual = TRUE;
    remote->ignoredByTraceRay = FALSE;
    remote->SetCollDetStat(FALSE);
    remote->SetCollDetDyn(TRUE);
    remote->SetPhysicsEnabled(FALSE);
    remote->SetVisual(zSTRING("HUMANS.MDS"));
    remoteModel = zDYNAMIC_CAST<zCModel>(remote->GetVisual());
    diagnostics.Write(1,"proxy_create","model="+std::to_string(reinterpret_cast<uintptr_t>(remoteModel))+" mesh="+meshName+" head="+headName+" gender="+std::to_string(static_cast<int>(remoteAppearance)));
    if (!remoteModel || !remoteModel->ApplyMeshLib(zSTRING(meshName.c_str()))) {
        RemoveProxy();
        proxyFailed = true;
        notice = "Avatar unavailable; connection remains active. Check log.";
        Log("Avatar creation failed: HUMANS.MDS / " + meshName);
        return false;
    }
    auto* headNode=remoteModel->SearchNode(zSTRING("BIP01 HEAD"));
    // MMB is a compiled morph mesh, not a generic zCVisual file extension.
    auto* head=headNode?zCMorphMesh::Load(zSTRING(female?"HUM_HEAD_BABE":"HUM_HEAD_PONY")):nullptr;
    diagnostics.Write(1,"proxy_head","node="+std::to_string(reinterpret_cast<uintptr_t>(headNode))+" morph="+std::to_string(reinterpret_cast<uintptr_t>(head)));
    if(!headNode||!head){
        RemoveProxy();proxyFailed=true;notice="Partnerkopf konnte nicht geladen werden; siehe Diagnose.";Log(notice);return false;
    }
    // MorphMesh::Render applies its OWN texAniState after the model node's
    // state. Both must agree; setting only the node is overwritten by face 0.
    zSTRING faceFilter("HUM_HEAD");
    head->SetTextureVar(0,0,nullptr);
    head->SetTextureVar(1,0,nullptr);
    head->SetTextureVar(0,female?137:18,&faceFilter);
    diagnostics.Write(1,"proxy_morph_material","head="+headName+" requested_face="+std::to_string(female?137:18)+" textures="+std::to_string(head->texAniState.numNodeTex));
    for(int i=0;head->texAniState.nodeTexList&&i<(std::min)(4,head->texAniState.numNodeTex);++i){
        auto* texture=head->texAniState.nodeTexList[i];if(!texture)continue;
        diagnostics.Write(1,"proxy_morph_material","base="+std::string(texture->GetObjectName().ToChar())+" variant="+std::to_string(head->texAniState.actAniFrames[0][i])+" color="+std::to_string(head->texAniState.actAniFrames[1][i]));
    }
    // TRUE builds the node's texture animation list, required for face variants.
    remoteModel->SetNodeVisual(headNode,head,TRUE);head->Release();
    const int bodyResult=remoteModel->SetMeshLibTexture(zSTRING(meshName.c_str()),0,female?5:9,nullptr);
    remoteModel->SetMeshLibTexture(zSTRING(meshName.c_str()),1,0,nullptr);
    remoteModel->SetNodeMeshTexture(zSTRING("BIP01 HEAD"),0,0,nullptr);
    remoteModel->SetNodeMeshTexture(zSTRING("BIP01 HEAD"),1,0,nullptr);
    const int faceResult=remoteModel->SetNodeMeshTexture(zSTRING("BIP01 HEAD"),0,female?137:18,&faceFilter);
    diagnostics.Write(1,"proxy_texture","body_channel=0 variant="+std::to_string(female?5:9)+" result="+std::to_string(bodyResult)+" head_channel=0 variant="+std::to_string(female?137:18)+" result="+std::to_string(faceResult));
    remoteModel->SetRandAnisEnabled(FALSE);
    remoteModel->doVobRot = FALSE;
    // Engine animation updates advance awake Vobs. Root motion is disabled:
    // only the network interpolation owns this independent proxy's position.
    remoteModel->aniTransScale=zVEC3(0,0,0);
    remote->SetPositionWorld(zVEC3(target.x, target.y, target.z));
    remote->ResetRotationsWorld();
    remote->RotateWorldY(target.yaw);
    remoteWorld = ogame->GetWorld();
    remoteWorld->AddVob(remote);
    remote->SetSleepingMode(zVOB_AWAKE);
    diagnostics.Write(1,"proxy_animation_ids","S_RUN="+std::to_string(remoteModel->GetAniIDFromAniName(zSTRING("S_RUN")))+" S_RUNL="+std::to_string(remoteModel->GetAniIDFromAniName(zSTRING("S_RUNL"))));
    for(int i=0;i<remoteModel->meshLibList.GetNumInList();++i){
        auto* entry=remoteModel->meshLibList[i];if(!entry)continue;
        auto& t=entry->texAniState;
        for(int j=0;t.nodeTexList&&j<(std::min)(4,t.numNodeTex);++j){
            if(t.nodeTexList[j])diagnostics.Write(1,"proxy_material","base="+std::string(t.nodeTexList[j]->GetObjectName().ToChar())+" variant="+std::to_string(t.actAniFrames[0][j])+" color="+std::to_string(t.actAniFrames[1][j]));
        }
    }
    Animate(false);
    Log("Independent remote visual created in " + sessionWorld);
    return true;
}

void Animate(bool moving) {
    if (!remoteModel) return;
    const zSTRING animation(moving ? "S_RUNL" : "S_RUN");
    const int id = remoteModel->GetAniIDFromAniName(animation);
    if(id<0&&remoteAnimation==-2&&moving==isMoving)return;
    if(moving==isMoving&&id==remoteAnimation&&id>=0&&remoteModel->GetActiveAni(id))return;
    isMoving=moving;
    diagnostics.Write(1,"proxy_animation","requested="+std::string(animation.ToChar())+" id="+std::to_string(id));
    if (id >= 0) {
        remoteModel->StopAnisLayerRange(0, 1);
        remoteModel->StartAni(id, FALSE);
        remoteAnimation=id;
        if(!remoteModel->GetActiveAni(id))diagnostics.Write(1,"proxy_animation_error","Requested animation did not become active");
    } else if(remoteAnimation!=-2) {
        diagnostics.Write(1,"proxy_animation_error","Animation missing from loaded model");remoteAnimation=-2;
    }
}

void ApplyRemote(unsigned long long now, float seconds) {
    if(remote&&remoteAppearance!=network.PeerAppearance()){
        // Keep the combat actor's identity and any enemy references intact.
        remoteAppearance=network.PeerAppearance();const bool female=remoteAppearance==net::Appearance::Velaya;
        if(auto* npc=PartnerNpc()){
            zSTRING body(female?"HUM_BODY_BABE0":"HUM_BODY_NAKED0"),head(female?"HUM_HEAD_BABE":"HUM_HEAD_PONY");
            npc->SetAdditionalVisuals(body,female?5:9,0,head,female?137:18,0,-1);
            remoteModel=npc->GetModel();if(remoteModel){remoteModel->doVobRot=FALSE;remoteModel->aniTransScale=zVEC3(0,0,0);}
            remoteAnimation=-1;
        }
    }
    if (!haveTarget || !CreateProxy()) return;
    const zVEC3 old = remote->GetPositionWorld();
    const float dx = target.x - old[0], dy = target.y - old[1], dz = target.z - old[2];
    const float distanceSquared = dx * dx + dy * dy + dz * dz;
    const float fraction = distanceSquared > 4000000.0f ? 1.0f : 1.0f - std::exp(-seconds * 12.0f);
    remote->SetSleepingMode(zVOB_AWAKE);
    const auto direction = remote->GetAtVectorWorld();
    const float oldYaw = std::atan2(direction[0], direction[2]) * 57.295779513f;
    float delta = std::fmod(target.yaw - oldYaw + 540.0f, 360.0f) - 180.0f;
    const auto placed=PlaceNetworkActor(*remote,zVEC3(old[0]+dx*fraction,old[1]+dy*fraction,old[2]+dz*fraction),oldYaw+delta*fraction);
    if(havePeerCombat)QolApplySprint(PartnerNpc(),peerCombat.sprint);
    if(remoteModel){remoteModel->aniTransScale=zVEC3(0,0,0);remoteModel->doVobRot=FALSE;}
    if(!havePeerCombat||!SetCombatAnimation(remoteModel,peerCombat.animation))Animate(now < movingUntil);
    if(now-lastRemoteLog>=1000){
        lastRemoteLog=now;auto* active=remoteModel->numActiveAnis>0&&remoteModel->numActiveAnis<=6?remoteModel->aniChannels[0]:nullptr;
        std::ostringstream text;text<<"target="<<target.x<<","<<target.y<<","<<target.z<<" yaw="<<target.yaw<<" error="<<std::sqrt(distanceSquared)
            <<" actual="<<remote->GetPositionWorld()[0]<<","<<remote->GetPositionWorld()[1]<<","<<remote->GetPositionWorld()[2]
            <<" placed="<<placed<<" movement_block="<<int(remote->isInMovementMode)<<" sleep="<<int(remote->sleepingMode)
            <<" moving="<<isMoving<<" snapshot_age_ms="<<(now-lastSnapshotAt)<<" active_anis="<<remoteModel->numActiveAnis<<" ani="<<remoteAnimation<<" frame="<<(active?active->actFrame:-1.0f);
        diagnostics.Write(2,"proxy_state",text.str());
    }
}

void RenderStatus() {
    if (!screen||zCMenu::GetActive()) return;
    const auto oldFont = screen->GetFontName();
    const auto oldColor = screen->fontColor;
    screen->SetFont(zSTRING("FONT_OLD_10_WHITE.TGA"));
    screen->SetFontColor(zCOLOR(210, 225, 255));
    const std::string role = network.GetRole() == net::Role::Host ? "HOST" :
                             network.GetRole() == net::Role::Guest ? "GUEST" : "OFFLINE";
    screen->Print(180, 180, zSTRING(("Zweiklang Coop Alpha.26 | " + role + " | " + network.LastStatus()).c_str()));
    screen->Print(180, 390, zSTRING(notice.c_str()));
    if(!network.PeerName().empty())screen->Print(180,600,zSTRING(("Partner: "+ConvertText(network.PeerName(),CP_UTF8,CP_ACP)).c_str()));
    int y=810;for(const auto& line:SessionStatusLines()){screen->Print(180,y,zSTRING(line.c_str()));y+=210;}
    QolRender();
    screen->SetFont(oldFont);
    screen->SetFontColor(oldColor);
}

void ReportNetworkStatus() {
    if (lastLoggedStatus != network.LastStatus()) {
        lastLoggedStatus = network.LastStatus();
        Log("Network: " + lastLoggedStatus);
    }
}

void Start(bool asHost) {
    Leave("Preparing session");
    ReadConfiguration();
    if (contentId.empty()) {
        notice = "Missing ContentId. Run Prepare-Alpha.ps1 before connecting.";
        Log(notice);
        return;
    }
    sessionWorld = CurrentWorld();
    if (sessionWorld.empty()) { notice = "Zuerst eine Spielwelt laden, dann Sitzung starten."; return; }
    if(!parser||parser->GetIndex(zSTRING("ZK_COOP_PARTNER"))<0){notice="ZweiklangCoop_Npc.d fehlt oder wurde nicht geladen. Beide PCs aktualisieren.";Log(notice);return;}
    for(const auto* name:{"ZK_COOP_QOL_BLOCK_QUICKLOOT","ZK_COOP_QOL_BLOCK_PARTYSPRINT","ZK_COOP_PREPARE_QUICKLOOT"}){
        auto* helper=parser->GetSymbol(zSTRING(name));if(!helper||helper->type!=5||helper->ele!=0){notice="Geladene Koop-Helfer veraltet; Gothic nach Update neu starten.";Log(notice);return;}
    }
    for(const auto* name:{"ZK_COOP_QOL_ACTIVE","ZK_COOP_QOL_GUEST"}){
        auto* value=parser->GetSymbol(zSTRING(name));if(!value||value->type!=2){notice="Koop-QoL-Schutzvariablen fehlen; Gothic nach Update neu starten.";Log(notice);return;}
    }
    if(QuestInt("NINJA_QUICKLOOT_ZK_BRIDGE_REV",-1)!=1){notice="Quickloot fehlt oder hat keine Koop-Bruecke; passende Alpha-Dateien und Ninja pruefen.";Log(notice);return;}
    if(QuestInt("NINJA_SPRINT_ZK_BRIDGE_REV",-1)!=1){notice="Sprint fehlt oder hat keine Koop-Bruecke; passende Alpha-Dateien und Ninja pruefen.";Log(notice);return;}
    diagnostics.Write(1,"qol_profile","quickloot=1.9.5 bridge=1 sprint=2.4.0 party_guard=1 protocol=9 journal=4");
    const net::Compatibility compatibility{kEngine, net::HashString(contentId), net::HashString(sessionWorld)};
    diagnostics.Role(asHost?"HOST":"CLIENT");
    diagnostics.Write(1,"session_start","name="+options.name+" host="+hostAddress+" port="+std::to_string(port)+" world="+sessionWorld+" content="+contentId+" protocol=9");
    const bool started = asHost ? network.StartHost(port, compatibility, Now(),hostAddress) :
                                 network.Connect(hostAddress, port, compatibility, Now(),options.client);
    Log(std::string("Session role=")+(asHost?"Host":"Client")+" bind="+(asHost?hostAddress:options.client));
    notice = started ? "Koop: Cavalorn-Dialoge beim Host; Speichern trennt die Sitzung." : network.LastStatus();
    QolGuard(started);
    ReportNetworkStatus();
}

const zSTRING kMenuSection("ZWEIKLANGCOOP_MENU");

void ApplyLocalAppearance() {
    const int wanted=options.female?1:0;
    if(!player||!player->GetModel()||(appearancePlayer==player&&appliedAppearance==wanted))return;
    zSTRING body(options.female?"HUM_BODY_BABE0":"HUM_BODY_NAKED0");
    zSTRING head(options.female?"HUM_HEAD_BABE":"HUM_HEAD_PONY");
    // -1 creates/equips no item; retain the currently equipped armor.
    player->SetAdditionalVisuals(body,options.female?5:9,0,head,options.female?137:18,0,-1);
    appearancePlayer=player;appliedAppearance=wanted;
    Log(options.female?"Local appearance applied: native G2 female body 5 / HUM_HEAD_BABE / face 137; no custom texture.":"Local appearance applied: Nameless Hero.");
}

void SeedUnionMenu() {
    if(!zoptions)return;
    zoptions->WriteString(kMenuSection,"Name",zSTRING(ConvertText(options.name,CP_UTF8,CP_ACP).c_str()),TRUE);
    zoptions->WriteString(kMenuSection,"Host",zSTRING(options.host.c_str()),TRUE);
    zoptions->WriteString(kMenuSection,"Client",zSTRING(options.client.c_str()),TRUE);
    zoptions->WriteInt(kMenuSection,"Role",options.asHost?0:1,TRUE);
    zoptions->WriteInt(kMenuSection,"Gender",options.female?1:0,TRUE);
    zoptions->WriteInt(kMenuSection,"FriendlyFire",options.friendlyFire?1:0,TRUE);
    zoptions->WriteString(kMenuSection,"Port",zSTRING(std::to_string(options.port).c_str()),TRUE);
    zoptions->WriteInt(kMenuSection,"LogLevel",options.logLevel,TRUE);
    zoptions->WriteInt(kMenuSection,"EngineMessages",options.engineLog?1:0,TRUE);
}

void UnionMenuEnter() {
    if(network.GetState()!=net::State::Idle&&network.GetState()!=net::State::Error)
        Leave("Union options opened: session closed; reconnect afterwards.");
    ReadConfiguration();SeedUnionMenu();menuAddressRole=-1;
    Log("Native Union plugin menu opened.");
}

zCMenuItem* CoopMenuItem(zCMenu* menu,const char* name) {
    if(menu)for(int i=0;i<menu->m_listItems.GetNum();++i){
        auto* item=menu->m_listItems[i];
        if(item&&_stricmp(item->GetName().ToChar(),name)==0)return item;
    }
    return nullptr;
}

void RefreshUnionInputs(zCMenu* menu) {
    if(!zoptions)return;
    const char* fields[]={"NAME","HOST","CLIENT","PORT"};
    const char* keys[]={"Name","Host","Client","Port"};
    for(int i=0;i<4;++i){
        const std::string prefix=std::string("MENUITEM_ZK_")+fields[i];
        auto* input=CoopMenuItem(menu,(prefix+"_INPUT").c_str());
        auto* display=CoopMenuItem(menu,(prefix+"_EDIT").c_str());
        const zSTRING value=zoptions->ReadString(kMenuSection,keys[i],"");
        if(input&&input->GetType()==MENU_ITEM_INPUT){
              input->SetText(zSTRING(""),0,FALSE);
            input->SetIsSelectable(FALSE);
            // SetText refreshes the native view. Only the EDIT control should
            // be visible outside Run(), including after confirmation/cancel.
            input->Hide();
        }
        if(display){
            // Remove cached print lines as well as replacing the logical value.
            // Otherwise Gothic can retain the old text behind the new string.
            display->ClrPrintwin();
            if(display->m_pInnerWindow)display->m_pInnerWindow->ClrPrintwin();
            display->m_parText[0]=value;
            display->SetText(value,0,FALSE);
        }
    }
}

void SyncRoleAddress(zCMenu* menu) {
    if(!menu||!zoptions||coopTextEditing)return;
    const int role=zoptions->ReadInt(kMenuSection,"Role",0)==0?0:1;
    if(role==menuAddressRole)return;
    menuAddressRole=role;
    const auto address=PrimaryIPv4();
    if(address.empty()){Log("Primary IPv4 unavailable; keeping manual menu address.");return;}
    zoptions->WriteString(kMenuSection,role==0?"Host":"Client",zSTRING(address.c_str()),TRUE);
    RefreshUnionInputs(menu);
    Log(std::string("Menu primary IPv4: role=")+(role==0?"Host":"Client")+" address="+address);
}

bool UnionMenuEdit(zCMenu* menu,const zSTRING& command) {
    const char* fields[]={"NAME","HOST","CLIENT","PORT"};
    const char* keys[]={"Name","Host","Client","Port"};
    for(int i=0;i<4;++i){
        if(_stricmp(command.ToChar(),(std::string("ZWEIKLANG_EDIT_")+fields[i]).c_str())!=0)continue;
        const std::string prefix=std::string("MENUITEM_ZK_")+fields[i];
        auto* item=CoopMenuItem(menu,(prefix+"_INPUT").c_str());
        auto* display=CoopMenuItem(menu,(prefix+"_EDIT").c_str());
        if(!item||item->GetType()!=MENU_ITEM_INPUT||!zoptions)return true;
        auto* input=static_cast<zCMenuItemInput*>(item);
        const zSTRING previous=zoptions->ReadString(kMenuSection,keys[i],"");
        // Each edit replaces the entire value. The saved value is kept separately
        // for Escape; it must never become an uneditable prefix in the editor.
        input->ClrPrintwin();
        if(input->m_pInnerWindow)input->m_pInnerWindow->ClrPrintwin();
        input->m_parText[0]=zSTRING("");
        input->SetText(zSTRING(""),0,FALSE);
        if(display)display->Hide();
        // INPUT was hidden between edits. Run() does not show it itself.
        input->SetIsSelectable(TRUE);
        input->m_bDontRender=FALSE;
        input->Show();
        // Use Gothic's existing text editor; no keyboard capture or global menu patch.
        coopTextEditing=true;
        input->Run();
        coopTextEditing=false;
        const zSTRING value=input->HasBeenCanceled()?previous:input->GetValue();
        zoptions->WriteString(kMenuSection,keys[i],value,TRUE);
        input->Hide();
        if(display){display->Show();menu->SetActiveItem(display);}
        if(zinput)zinput->ClearKeyBuffer();
        RefreshUnionInputs(menu);
        Log(std::string("Union menu text editor closed: ")+keys[i]);
        return true;
    }
    return false;
}

void UnionMenuStatus(zCMenu* menu,const std::string& text) {
    if(!menu)return;
    for(int i=0;i<menu->m_listItems.GetNum();++i){
        auto* item=menu->m_listItems[i];
        if(item&&_stricmp(item->GetName().ToChar(),"MENUITEM_ZK_STATUS")==0)
            item->SetText(zSTRING(text.c_str()),0,FALSE);
    }
}

bool UnionMenuApply(zCMenu* menu) {
    if(!zoptions){UnionMenuStatus(menu,"Spieloptionen sind noch nicht bereit.");return false;}
    Options candidate=options;
    candidate.name=ConvertText(zoptions->ReadString(kMenuSection,"Name","").ToChar(),CP_ACP,CP_UTF8);
    candidate.host=zoptions->ReadString(kMenuSection,"Host","").ToChar();
    candidate.client=zoptions->ReadString(kMenuSection,"Client","").ToChar();
    candidate.asHost=zoptions->ReadInt(kMenuSection,"Role",0)==0;
    candidate.female=zoptions->ReadInt(kMenuSection,"Gender",0)==1;
    candidate.friendlyFire=zoptions->ReadInt(kMenuSection,"FriendlyFire",0)!=0;
    const std::string portText=zoptions->ReadString(kMenuSection,"Port","").ToChar();
    if(!ValidName(candidate.name)){
        UnionMenuStatus(menu,"Name: 1 bis 32 UTF-8-Bytes, keine Steuerzeichen.");Log("Union menu validation failed: name");return false;
    }
    if(!ValidHost(candidate.host)){
        UnionMenuStatus(menu,"Bitte eine gueltige Host-IPv4-Adresse eingeben.");Log("Union menu validation failed: host");return false;
    }
    if(candidate.client!="0.0.0.0"&&!ValidHost(candidate.client)){
        UnionMenuStatus(menu,"Client-IP: eigene IPv4 oder 0.0.0.0 fuer automatisch.");return false;
    }
    if(!ParsePort(portText,candidate.port)){
        UnionMenuStatus(menu,"Port muss zwischen 1024 und 65535 liegen.");Log("Union menu validation failed: port");return false;
    }
    candidate.logLevel=zoptions->ReadInt(kMenuSection,"LogLevel",1);
    if(candidate.logLevel<0||candidate.logLevel>2)candidate.logLevel=1;
    candidate.engineLog=zoptions->ReadInt(kMenuSection,"EngineMessages",1)!=0;
    if(!WritePrivateProfileStringA("Player","Name",candidate.name.c_str(),iniPath.c_str()) ||
       !WritePrivateProfileStringA("Avatar","Gender",candidate.female?"1":"0",iniPath.c_str()) ||
       !WritePrivateProfileStringA("Combat","FriendlyFire",candidate.friendlyFire?"1":"0",iniPath.c_str()) ||
       !WritePrivateProfileStringA("Network","Client",candidate.client.c_str(),iniPath.c_str()) ||
       !WritePrivateProfileStringA("Network","Role",candidate.asHost?"Host":"Client",iniPath.c_str()) ||
       !WritePrivateProfileStringA("Network","Host",candidate.host.c_str(),iniPath.c_str()) ||
       !WritePrivateProfileStringA("Network","Port",std::to_string(candidate.port).c_str(),iniPath.c_str()) ||
       !WritePrivateProfileStringA("Logging","Level",std::to_string(candidate.logLevel).c_str(),iniPath.c_str()) ||
       !WritePrivateProfileStringA("Logging","EngineMessages",candidate.engineLog?"1":"0",iniPath.c_str())) {
        UnionMenuStatus(menu,"Speichern fehlgeschlagen: Schreibrechte pruefen.");Log("Configuration write failed");return false;
    }
    ReadConfiguration();
    UnionMenuStatus(menu,candidate.asHost?"Gespeichert. Rolle: Host.":"Gespeichert. Rolle: Client.");
    Log("Union options saved: name="+options.name+" host="+hostAddress+" port="+std::to_string(port)+" logLevel="+std::to_string(options.logLevel));
    return true;
}

// Observe state only; no changes to single-player simulation or scripts.
void ObserveState(unsigned long long now) {
    const int hp=player->GetAttribute(NPC_ATR_HITPOINTS),mana=player->GetAttribute(NPC_ATR_MANA);
    const int dialog=player->talkOther?1:0;
    if(!haveState||hp!=previousHp||mana!=previousMana||dialog!=previousDialog) {
        diagnostics.Write(1,"player_event","hp="+std::to_string(hp)+" mana="+std::to_string(mana)+" dialog="+std::to_string(dialog));
        previousHp=hp;previousMana=mana;previousDialog=dialog;haveState=true;
    }
    if(previousPeerName!=network.PeerName()) {previousPeerName=network.PeerName();diagnostics.Write(1,"peer_name",previousPeerName);}
    if(now-lastStateLog>=1000) {
        lastStateLog=now;const auto p=player->GetPositionWorld();const auto& stats=network.Stats();
        std::ostringstream text;text<<"world="<<CurrentWorld()<<" x="<<p[0]<<" y="<<p[1]<<" z="<<p[2]<<" hp="<<hp<<" mana="<<mana
          <<" rx="<<stats.received<<" malformed="<<stats.malformed<<" ignored="<<stats.ignored<<" snapshots="<<stats.snapshots;
        diagnostics.Write(2,"game_state",text.str());
    }
}
}

void Initialize() {
    char modulePath[MAX_PATH]{};
    if (!GetModuleFileNameA(moduleHandle, modulePath, MAX_PATH)) return;
    systemDirectory = modulePath;
    auto slash = systemDirectory.find_last_of("/\\");
    if (slash == std::string::npos) return;
    systemDirectory.erase(slash);
    const auto leafStart = systemDirectory.find_last_of("/\\");
    const auto leaf = systemDirectory.substr(leafStart == std::string::npos ? 0 : leafStart + 1);
    if (_stricmp(leaf.c_str(), "autorun") == 0 && leafStart != std::string::npos) systemDirectory.erase(leafStart);
    iniPath = systemDirectory + "\\ZweiklangCoop.ini";
    diagnostics.Open(systemDirectory+"\\ZweiklangLogs");diagnosticsReady=true;
    RecoveryInit();
    network.SetTraceSink([](const std::string& detail){diagnostics.Write(2,"packet",detail);});
    ReadConfiguration();
    faultFile=CreateFileA((diagnostics.Path()+".faults.jsonl").c_str(),FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(faultFile!=INVALID_HANDLE_VALUE)faultObserver=AddVectoredExceptionHandler(1,ObserveFault);
    LogRuntime();
    lastFrame = Now();
    Log("Zweiklang Coop 0.1.0-alpha.26 initialized; protocol=9; engine=G2NotR; no auto-connect.");
}

void Shutdown() {
    Leave("Session closed");diagnostics.Flush();diagnosticsReady=false;
    if(faultObserver){RemoveVectoredExceptionHandler(faultObserver);faultObserver=nullptr;}
    if(faultFile!=INVALID_HANDLE_VALUE){CloseHandle(faultFile);faultFile=INVALID_HANDLE_VALUE;}
}

void Frame() {
    if (loading || saving || !player || !ogame || !ogame->GetWorld()) return;
    ApplyLocalAppearance();
    RecoveryFrame();
    const auto now = Now();
    const float elapsed = static_cast<float>((now - lastFrame) > 250 ? 250 : now - lastFrame) / 1000.0f;
    lastFrame = now;
    network.Poll(now);
    ReportNetworkStatus();
    ObserveState(now);
    if(network.GetState()!=net::State::Connected&&combatSession)CombatReset();
    if (network.GetState() != net::State::Idle && network.GetState() != net::State::Error &&
        !sessionWorld.empty() && CurrentWorld() != sessionWorld) Leave("World changed; reconnect in the same world.");
    if (zinput) {
        if (zinput->KeyToggled(leaveKey)) Leave("Disconnected. F6 host / F7 join");
        else if (zinput->KeyToggled(hostKey)) Start(true);
        else if (zinput->KeyToggled(joinKey)) Start(false);
    }
    if (network.GetState() == net::State::Connected) {
        net::Snapshot incoming{};
        if (network.ConsumeSnapshot(incoming)) {
            if(haveTarget){const float dx=incoming.x-target.x,dy=incoming.y-target.y,dz=incoming.z-target.z;if(dx*dx+dy*dy+dz*dz>1.0f)movingUntil=now+200;}
            target = incoming; haveTarget = true;lastSnapshotAt=now;
        }
        if (now - lastSend >= 50) {
            const auto position = player->GetPositionWorld();
            const auto direction = player->GetAtVectorWorld();
            const net::Snapshot snapshot{position[0], position[1], position[2],
                std::atan2(direction[0], direction[2]) * 57.295779513f};
            network.SendSnapshot(snapshot, now);
            lastSend = now;
        }
        CombatFrame(now,elapsed);
        LootFrame(now);
        ApplyRemote(now, elapsed);
        WorldFrame(now);
        if(!combatFailure.empty()){const auto reason=combatFailure;Leave(reason.c_str());}
    } else if (remote) RemoveProxy();
    QolFrame(now);
    SessionFrame(now);
    QuestFrame(now);
    RenderStatus();
}

void MenuFrame() {
    // Keep transport responsive in menus without touching world/visual objects.
    network.Poll(Now());
    ReportNetworkStatus();
    auto* menu=zCMenu::GetActive();
    if(menu&&_stricmp(menu->GetName().ToChar(),"MENU_ZWEIKLANG_COOP")==0)SyncRoleAddress(menu);
}

void BeforeSave() {
    QuestRestoreConditions();
    saving = true;
    Leave("Save started: presence session closed; reconnect after saving.");
}
void AfterSave() { saving = false; lastFrame = Now();notice="Speichern beendet. Sitzung bei Bedarf erneut starten.";Log("Save completed"); }
void BeforeLoad() {
    QuestRestoreConditions();
    questError.clear();questSessionBound=false;
    questCheckedParser=nullptr;questCheckedInfoManager=nullptr;
    loading = true;
    Leave("Loading: presence session closed; reconnect after loading.");
    QolReset();
}
void BeforeNewGame(){BeforeLoad();recoveryTimeline.clear();recoveryEffects.clear();recoveryReady=false;recoveryLastReplay=0;recoveryLastWorld.clear();if(recoveryStoreReady){recoveryError.clear();recoveryBlocked=false;}}
void AfterLoad() { QolReset();appearancePlayer=nullptr;appliedAppearance=-1; loading = false; saving = false; lastFrame = Now();notice="Bereit: Sitzung im Union-Menue oder mit F6/F7 starten.";Log("Load completed: world="+CurrentWorld());haveState=false; }
}

// The exact G2 2.6-fix methods come from the external Union/ZenGin SDK.
namespace {
using namespace gothic;
void __fastcall SharedReadSave(CGameManager*,void*,int);
CInvoke<void(__thiscall*)(CGameManager*,int)> originalReadSave(0x0042A040,&SharedReadSave);
void __fastcall SharedReadSave(CGameManager* manager,void*,int slot){if(slot<0){originalReadSave(manager,slot);return;}zweiklang::engine::RecoveryBeginLoad(slot);originalReadSave(manager,slot);zweiklang::engine::RecoveryEndLoad();}
void __fastcall SharedWriteSave(CGameManager*,void*,int);
CInvoke<void(__thiscall*)(CGameManager*,int)> originalWriteSave(0x0042A2D0,&SharedWriteSave);
void __fastcall SharedWriteSave(CGameManager* manager,void*,int slot){
    using namespace zweiklang::engine;
    if(!recoveryError.empty()||recoveryBlocked){notice="Speichern gesperrt: erst den Inventar-/Questabgleich klaeren.";LootLog("save_blocked",recoveryError);return;}
    recoverySaveSlot=slot;recoverySaveStarted=false;
    originalWriteSave(manager,slot);
    if(recoverySaveStarted)RecoveryAfterSave(slot,recoverySavingCheckpoint);
    recoverySaveSlot=-1;recoverySaveStarted=false;
}
void __fastcall SharedNativeWriteSave(oCGame*,void*,int,int);
CInvoke<void(__thiscall*)(oCGame*,int,int)> originalNativeWriteSave(0x006C5250,&SharedNativeWriteSave);
void __fastcall SharedNativeWriteSave(oCGame* game,void*,int slot,int globals){
    using namespace zweiklang::engine;
    QuestRestoreConditions();
    if(recoverySaveSlot>=0&&!recoverySaveStarted){recoverySavingCheckpoint=RecoveryBeforeSave(recoverySaveSlot);recoverySaveStarted=true;}
    if(!recoveryError.empty()){recoverySaveStarted=false;LootLog("save_blocked","Recovery guard failed before native write");return;}
    originalNativeWriteSave(game,slot,globals);
}
int __fastcall CoopSessionInput(CGameManager*,void*,int);
CInvoke<int(__thiscall*)(CGameManager*,int)> originalSessionInput(0x0042AD50,&CoopSessionInput);
int __fastcall CoopSessionInput(CGameManager* manager,void*,int key){if(zweiklang::engine::SessionInput(key))return TRUE;return originalSessionInput(manager,key);}
int __fastcall CoopGameSessionInput(oCGame*,void*,int);
CInvoke<int(__thiscall*)(oCGame*,int)> originalGameSessionInput(0x006FC170,&CoopGameSessionInput);
int __fastcall CoopGameSessionInput(oCGame* game,void*,int key){if(zweiklang::engine::SessionInput(key))return TRUE;return originalGameSessionInput(game,key);}
int __fastcall CoopQuestCondition(oCInfo*,void*);
CInvoke<int(__thiscall*)(oCInfo*)> originalQuestCondition(0x00703950,&CoopQuestCondition);
int __fastcall CoopQuestCondition(oCInfo* info,void*){if(!zweiklang::engine::QuestAllowInfo(info))return FALSE;return originalQuestCondition(info);}
void __fastcall CoopQuestInformation(oCInfo*,void*);
CInvoke<void(__thiscall*)(oCInfo*)> originalQuestInformation(0x00703970,&CoopQuestInformation);
void __fastcall CoopQuestInformation(oCInfo* info,void*){using namespace zweiklang::engine;if(!QuestAllowInfo(info))return;const auto before=QuestBeforeInfo(info);originalQuestInformation(info);QuestAfterInfo(before);}
void __fastcall CoopInfoManagerDestroy(oCInfoManager*,void*);
CInvoke<void(__thiscall*)(oCInfoManager*)> originalInfoManagerDestroy(0x00702720,&CoopInfoManagerDestroy);
void __fastcall CoopInfoManagerDestroy(oCInfoManager* manager,void*){using namespace zweiklang::engine;if(manager==questCheckedInfoManager){questOriginalConditions.clear();questCheckedInfoManager=nullptr;questCheckedParser=nullptr;}originalInfoManagerDestroy(manager);}
void __fastcall SharedUnlock(oCMobLockable*,void*,oCNpc*,int);
CInvoke<void(__thiscall*)(oCMobLockable*,oCNpc*,int)> originalUnlock(0x00724A70,&SharedUnlock);
void __fastcall SharedUnlock(oCMobLockable* lock,void*,oCNpc* who,int mode){auto tx=zweiklang::engine::RecoveryUnlockBegin(lock,who);originalUnlock(lock,who,mode);zweiklang::engine::RecoveryUnlockEnd(lock,tx);}
// Full damage entry point: blocked hits never reach scripts, HP, knockback or
// the hit-stage observer. Network replay deliberately forwards exactly once.
void __fastcall SharedDamage(oCNpc*,void*,oCNpc::oSDamageDescriptor&);
CInvoke<void(__thiscall*)(oCNpc*,oCNpc::oSDamageDescriptor&)> originalSharedDamage(0x006660E0,&SharedDamage);
void __fastcall SharedDamage(oCNpc* npc,void*,oCNpc::oSDamageDescriptor& damage){
    if(!zweiklang::engine::InterceptCombatDamage(npc,damage))originalSharedDamage(npc,damage);
}
int __fastcall SharedAIState(oCNpc_States*,void*);
CInvoke<int(__thiscall*)(oCNpc_States*)> originalAIState(0x0076D1A0,&SharedAIState);
int __fastcall SharedAIState(oCNpc_States* state,void*){if(zweiklang::engine::CombatMirror(state->npc))return FALSE;return originalAIState(state);}
void __fastcall SharedHumanAI(oCAIHuman*,void*,zCVob*,int&);
CInvoke<void(__thiscall*)(oCAIHuman*,zCVob*,int&)> originalHumanAI(0x0069BAB0,&SharedHumanAI);
void __fastcall SharedHumanAI(oCAIHuman* ai,void*,zCVob* vob,int& remove){
    if(zweiklang::engine::CombatMirror(zDYNAMIC_CAST<oCNpc>(vob))){remove=FALSE;return;}originalHumanAI(ai,vob,remove);
}
void __fastcall SharedPerception(oCNpc*,void*);
CInvoke<void(__thiscall*)(oCNpc*)> originalPerception(0x0075DD30,&SharedPerception);
void __fastcall SharedPerception(oCNpc* npc,void*){if(!zweiklang::engine::CombatMirror(npc))originalPerception(npc);}
void __fastcall SharedFocus(oCNpc*,void*,zCVob*);
CInvoke<void(__thiscall*)(oCNpc*,zCVob*)> originalFocus(0x00732B60,&SharedFocus);
void __fastcall SharedFocus(oCNpc* npc,void*,zCVob* target){
    using namespace zweiklang::engine;
    if(CombatConnected()&&npc==player&&target==remote&&co::BlockPartnerFocus(sessionFriendlyFire,npc->GetWeaponMode()))target=nullptr;
    const bool newPartnerFocus=CombatConnected()&&npc==player&&target==remote&&target&&npc->GetFocusVob()!=target;
    originalFocus(npc,target);
    if(newPartnerFocus&&npc->GetFocusVob()==target)CombatLog("partner_focus","name="+network.PeerName()+" weapon_mode="+std::to_string(npc->GetWeaponMode()));
}
int __fastcall SharedFocusCheck(oCNpc*,void*,const zCVob*,int,int,float,float&);
CInvoke<int(__thiscall*)(oCNpc*,const zCVob*,int,int,float,float&)> originalFocusCheck(0x007331C0,&SharedFocusCheck);
int __fastcall SharedFocusCheck(oCNpc* npc,void*,const zCVob* target,int category,int flags,float range,float& score){
    using namespace zweiklang::engine;
    if(CombatConnected()&&npc==player&&target==remote&&co::BlockPartnerFocus(sessionFriendlyFire,npc->GetWeaponMode()))return FALSE;
    return originalFocusCheck(npc,target,category,flags,range,score);
}
int __fastcall SharedFocusBBox(oCNpc*,void*,const zCVob*,int,int,float,float&);
CInvoke<int(__thiscall*)(oCNpc*,const zCVob*,int,int,float,float&)> originalFocusBBox(0x00732F40,&SharedFocusBBox);
int __fastcall SharedFocusBBox(oCNpc* npc,void*,const zCVob* target,int category,int flags,float range,float& score){
    using namespace zweiklang::engine;
    if(CombatConnected()&&npc==player&&target==remote&&co::BlockPartnerFocus(sessionFriendlyFire,npc->GetWeaponMode()))return FALSE;
    return originalFocusBBox(npc,target,category,flags,range,score);
}
void __fastcall SharedEnemy(oCNpc*,void*,oCNpc*);
CInvoke<void(__thiscall*)(oCNpc*,oCNpc*)> originalEnemy(0x00734BC0,&SharedEnemy);
void __fastcall SharedEnemy(oCNpc* npc,void*,oCNpc* target){
    using namespace zweiklang::engine;
    if(CombatConnected()&&npc==player&&target==PartnerNpc()&&!sessionFriendlyFire)target=nullptr;
    originalEnemy(npc,target);
}
void __fastcall SharedLoot(oCNpc*,void*);
CInvoke<void(__thiscall*)(oCNpc*)> originalLoot(0x00762970,&SharedLoot);
void __fastcall SharedLoot(oCNpc* npc,void*){
    using namespace zweiklang::engine;
    if(npc==player&&LootBeforeCorpseOpen())return;
    originalLoot(npc);
}
void __fastcall SharedChestOpen(oCMobContainer*,void*,oCNpc*);
CInvoke<void(__thiscall*)(oCMobContainer*,oCNpc*)> originalChestOpen(0x00726500,&SharedChestOpen);
void __fastcall SharedChestOpen(oCMobContainer* chest,void*,oCNpc* who){
    if(!zweiklang::engine::LootBeforeChestOpen(chest,who))originalChestOpen(chest,who);
}
int __fastcall SharedItemTransfer(oCItemContainer*,void*,int,int);
CInvoke<int(__thiscall*)(oCItemContainer*,int,int)> originalItemTransfer(0x00709F40,&SharedItemTransfer);
int __fastcall SharedItemTransfer(oCItemContainer* container,void*,int direction,int amount){
    if(zweiklang::engine::LootInterceptTransfer(container,direction,amount))return TRUE;
    return originalItemTransfer(container,direction,amount);
}
unsigned char __cdecl CoopKeyCharacter(int scan,int shift,int alt);
CInvoke<unsigned char(__cdecl*)(int,int,int)> originalKeyCharacter(0x004D2130,&CoopKeyCharacter);
unsigned char __cdecl CoopKeyCharacter(int scan,int shift,int alt) {
    if(!zweiklang::engine::coopTextEditing)return originalKeyCharacter(scan,shift,alt);
    BYTE state[256]{};
    state[VK_CAPITAL]=static_cast<BYTE>(GetKeyState(VK_CAPITAL)&1);
    state[VK_NUMLOCK]=static_cast<BYTE>(GetKeyState(VK_NUMLOCK)&1);
    if(shift)state[VK_SHIFT]=0x80;
    const bool rightAlt=zinput&&zinput->KeyPressed(0xB8);
    if(alt||rightAlt)state[VK_MENU]=0x80;
    if(rightAlt||(zinput&&(zinput->KeyPressed(0x1D)||zinput->KeyPressed(0x9D))))state[VK_CONTROL]=0x80;
    return zweiklang::MenuCharacter(static_cast<unsigned int>(scan),GetKeyboardLayout(0),state);
}
void __fastcall MenuInit(zCMenu*,void*);
void __fastcall MenuDone(zCMenu*,void*);
int __fastcall MenuAction(zCMenu*,void*,zTMenuItemSelAction,zSTRING const&,zCMenuItem*);
CInvoke<void(__thiscall*)(zCMenu*)> originalMenuInit(0x004DCDE0,&MenuInit);
CInvoke<void(__thiscall*)(zCMenu*)> originalMenuDone(0x004DD9E0,&MenuDone);
CInvoke<int(__thiscall*)(zCMenu*,zTMenuItemSelAction,zSTRING const&,zCMenuItem*)> originalMenuAction(0x004DC430,&MenuAction);
bool IsCoopMenu(zCMenu* menu) {
    return menu&&_stricmp(menu->GetName().ToChar(),"MENU_ZWEIKLANG_COOP")==0;
}
void __fastcall MenuInit(zCMenu* menu,void*) {
    // The Union script creates every item. No construction, insertion or layout
    // mutation in C++; only seed our own option section before native init.
    if(IsCoopMenu(menu))zweiklang::engine::UnionMenuEnter();
    originalMenuInit(menu);
    if(IsCoopMenu(menu)){
        zweiklang::engine::RefreshUnionInputs(menu);
        zweiklang::engine::SyncRoleAddress(menu);
    }
}
void __fastcall MenuDone(zCMenu* menu,void*) {
    if(IsCoopMenu(menu)){
        zweiklang::engine::Log("Native Union plugin menu closed.");
        zweiklang::engine::ReadConfiguration();
    }
    originalMenuDone(menu);
}
int __fastcall MenuAction(zCMenu* menu,void*,zTMenuItemSelAction action,zSTRING const& command,zCMenuItem* item) {
      if(IsCoopMenu(menu)&&action==SEL_ACTION_EXECCOMMANDS&&_stricmp(command.ToChar(),"ZWEIKLANG_START")==0){
          using namespace zweiklang::engine;
          if(UnionMenuApply(menu)){Start(options.asHost);UnionMenuStatus(menu,notice);}
          menu->ClearAllEvents();menu->m_exitState=zCMenu::NONE;return 0;
      }
      if(IsCoopMenu(menu)&&action==SEL_ACTION_EXECCOMMANDS&&_stricmp(command.ToChar(),"ZWEIKLANG_LEAVE")==0){
          zweiklang::engine::Leave("Disconnected from Union menu");
          zweiklang::engine::UnionMenuStatus(menu,"Sitzung beendet.");
          menu->ClearAllEvents();menu->m_exitState=zCMenu::NONE;return 0;
      }
      if(IsCoopMenu(menu)&&action==SEL_ACTION_EXECCOMMANDS&&zweiklang::engine::UnionMenuEdit(menu,command)){
          // HandleAction propagates a nonzero result to Run() as "close menu".
          // A completed local command must return zero to keep this menu open.
          menu->ClearAllEvents();menu->m_exitState=zCMenu::NONE;return 0;
      }
    if(IsCoopMenu(menu)&&action==SEL_ACTION_EXECCOMMANDS&&_stricmp(command.ToChar(),"ZWEIKLANG_APPLY")==0){
          zweiklang::engine::UnionMenuApply(menu);menu->ClearAllEvents();menu->m_exitState=zCMenu::NONE;return 0;
    }
    return originalMenuAction(menu,action,command,item);
}
int __fastcall EngineReport(zERROR*,void*,zERROR_TYPE,int,zSTRING const&,signed char,unsigned int,int,char*,char*);
CInvoke<int(__thiscall*)(zERROR*,zERROR_TYPE,int,zSTRING const&,signed char,unsigned int,int,char*,char*)> originalReport(0x0044C8D0,&EngineReport);
int __fastcall EngineReport(zERROR* self,void*,zERROR_TYPE type,int id,zSTRING const& text,signed char level,unsigned int flags,int line,char* file,char* function) {
    using namespace zweiklang::engine;
    if(diagnosticsReady&&captureEngineMessages&&(type>=zERR_TYPE_WARN||activeLogLevel==2)) {
        std::string detail="type="+std::to_string(type)+" id="+std::to_string(id)+" level="+std::to_string(level)+" "+text.ToChar();
        if(file)detail+=" file="+std::string(file)+":"+std::to_string(line);
        diagnostics.Write(type>=zERR_TYPE_WARN?1:2,"engine",ConvertText(detail,CP_ACP,CP_UTF8));
    }
    return originalReport(self,type,id,text,level,flags,line,file,function);
}

// Passive observation: always forward the original engine calls and return values.
void __fastcall DamageEvent(oCNpc*,void*,oCNpc::oSDamageDescriptor&);
CInvoke<void(__thiscall*)(oCNpc*,oCNpc::oSDamageDescriptor&)> originalDamage(0x00666610,&DamageEvent);
void __fastcall DamageEvent(oCNpc* npc,void*,oCNpc::oSDamageDescriptor& damage) {
    using namespace zweiklang::engine;
    const bool observe=diagnosticsReady&&activeLogLevel>0&&npc;
    const int before=observe?npc->GetAttribute(NPC_ATR_HITPOINTS):0;
    if(observe){
        npc->AddRef();
        std::string text="target="+std::string(npc->GetObjectName().ToChar());
        if(damage.pNpcAttacker)text+=" attacker="+std::string(damage.pNpcAttacker->GetObjectName().ToChar());
        diagnostics.Write(1,"damage",text);
    }
    originalDamage(npc,damage);
    if(observe){diagnostics.Write(1,"damage_result","target="+std::string(npc->GetObjectName().ToChar())+" hp_before="+std::to_string(before)+" hp_after="+std::to_string(npc->GetAttribute(NPC_ATR_HITPOINTS))+" effective="+std::to_string(damage.fDamageEffective));npc->Release();}
}
int __fastcall TakeEvent(oCNpc*,void*,zCVob*);
int __fastcall DropEvent(oCNpc*,void*,zCVob*);
CInvoke<int(__thiscall*)(oCNpc*,zCVob*)> originalTake(0x007449C0,&TakeEvent);
CInvoke<int(__thiscall*)(oCNpc*,zCVob*)> originalDrop(0x00744DD0,&DropEvent);
void ItemEvent(const char* event,oCNpc* npc,zCVob* item,int result) {
    using namespace zweiklang::engine;
    if(diagnosticsReady&&activeLogLevel>0&&npc&&item)
        diagnostics.Write(1,event,"npc="+std::string(npc->GetObjectName().ToChar())+" item="+item->GetObjectName().ToChar()+" result="+std::to_string(result));
}
int __fastcall TakeEvent(oCNpc* npc,void*,zCVob* item) {
    // Log before execution: engine may destroy or merge the world item.
    ItemEvent("take_attempt",npc,item,0);if(zweiklang::engine::WorldGroundAction(npc,item,false))return TRUE;
    auto* held=zDYNAMIC_CAST<oCItem>(item);if(held)held->AddRef();const auto result=originalTake(npc,item);if(held){zweiklang::engine::WorldGroundChanged(held);held->Release();}return result;
}
int __fastcall DropEvent(oCNpc* npc,void*,zCVob* item) {
    ItemEvent("drop_attempt",npc,item,0);auto* held=zDYNAMIC_CAST<oCItem>(item);if(held)held->AddRef();const bool handled=zweiklang::engine::WorldGroundAction(npc,item,true);const int result=handled?TRUE:originalDrop(npc,item);if(held)held->Release();return result;
}
void __fastcall SharedSpawnPass(oCSpawnManager*,void*);
CInvoke<void(__thiscall*)(oCSpawnManager*)> originalSpawnPass(0x007780B0,&SharedSpawnPass);
void __fastcall SharedSpawnPass(oCSpawnManager* manager,void*){
    using namespace zweiklang::engine;
    const bool cullingBefore=worldCulling;worldCulling=true;originalSpawnPass(manager);worldCulling=cullingBefore;
    if(!CombatConnected()||!HostCombat()||!haveTarget||worldGuestPass)return;
    worldGuestPass=true;worldCulling=true;
    worldTimersRestored+=zweiklang::worldsync::GuestSpawnPass(*manager,zVEC3(target.x,target.y,target.z),[&](){originalSpawnPass(manager);});
    worldCulling=cullingBefore;worldGuestPass=false;++worldDualPasses;
}
int __fastcall SharedRemoveNpc(oCSpawnManager*,void*,oCNpc*);
CInvoke<int(__thiscall*)(oCSpawnManager*,oCNpc*)> originalRemoveNpc(0x007792E0,&SharedRemoveNpc);
int __fastcall SharedRemoveNpc(oCSpawnManager* manager,void*,oCNpc* npc){
    using namespace zweiklang::engine;
    if(CombatConnected()&&HostCombat()&&npc&&haveTarget){const auto range=oCSpawnManager::GetRemoveRange();if(Distance2(npc->GetPositionWorld(),zVEC3(target.x,target.y,target.z))<=range*range||Distance2(npc->GetPositionWorld(),player->GetPositionWorld())<=range*range)return FALSE;}
    const bool previous=worldCulling;worldCulling=true;const int result=originalRemoveNpc(manager,npc);worldCulling=previous;return result;
}
void __fastcall SharedDeleteNpc(oCSpawnManager*,void*,oCNpc*);
CInvoke<void(__thiscall*)(oCSpawnManager*,oCNpc*)> originalDeleteNpc(0x00779690,&SharedDeleteNpc);
void __fastcall SharedDeleteNpc(oCSpawnManager* manager,void*,oCNpc* npc){using namespace zweiklang::engine;if(CombatConnected()&&!HostCombat()){auto it=npcByPointer.find(npc);if(it!=npcByPointer.end()&&it->second->mirror)return;}WorldLife(npc,zweiklang::worldsync::RemovalLife(worldCulling));originalDeleteNpc(manager,npc);}
void __fastcall SharedNpcDisable(oCNpc*,void*);
CInvoke<void(__thiscall*)(oCNpc*)> originalNpcDisable(0x00745A20,&SharedNpcDisable);
void __fastcall SharedNpcDisable(oCNpc* npc,void*){using namespace zweiklang::engine;WorldLife(npc,zweiklang::worldsync::RemovalLife(worldCulling));originalNpcDisable(npc);}
void __fastcall SharedNpcEnable(oCNpc*,void*,zVEC3&);
CInvoke<void(__thiscall*)(oCNpc*,zVEC3&)> originalNpcEnable(0x00745D40,&SharedNpcEnable);
void __fastcall SharedNpcEnable(oCNpc* npc,void*,zVEC3& position){using namespace zweiklang::engine;if(!worldConstructing&&CombatConnected()&&!HostCombat()){auto it=npcByPointer.find(npc);if(it!=npcByPointer.end()&&it->second->mirror&&it->second->life!=1)return;}originalNpcEnable(npc,position);WorldLife(npc,1);}
void __fastcall SharedNpcProcess(oCNpc*,void*);
CInvoke<void(__thiscall*)(oCNpc*)> originalNpcProcess(0x0073E480,&SharedNpcProcess);
void __fastcall SharedNpcProcess(oCNpc* npc,void*){using namespace zweiklang::engine;if(CombatConnected()&&HostCombat()&&haveTarget&&npc!=player&&Distance2(npc->GetPositionWorld(),zVEC3(target.x,target.y,target.z))<36000000.0f)++worldGuestTicks;originalNpcProcess(npc);}
void __fastcall SharedTradeBegin(oCInformationManager*);
CInvoke<void(__fastcall*)(oCInformationManager*)> originalTradeBegin(0x00662E60,&SharedTradeBegin);
void __fastcall SharedTradeBegin(oCInformationManager* info){originalTradeBegin(info);zweiklang::engine::WorldTradeBegin(info);}
int __fastcall SharedSell(oCViewDialogTrade*,short);
int __fastcall SharedBuy(oCViewDialogTrade*,short);
CInvoke<int(__fastcall*)(oCViewDialogTrade*,short)> originalSell(0x0068B840,&SharedSell);
CInvoke<int(__fastcall*)(oCViewDialogTrade*,short)> originalBuy(0x0068BB10,&SharedBuy);
int __fastcall SharedSell(oCViewDialogTrade* trade,short amount){if(zweiklang::engine::WorldTradeTransfer(trade,true,amount))return TRUE;return originalSell(trade,amount);}
int __fastcall SharedBuy(oCViewDialogTrade* trade,short amount){if(zweiklang::engine::WorldTradeTransfer(trade,false,amount))return TRUE;return originalBuy(trade,amount);}
}

namespace {
using UnionCore::CApplication;
// Register only for the exact engine this binary was compiled against.
CApplication* application = Union.GetEngineVersion() != kEngine ? nullptr :
    CApplication::CreateRefApplication(nullptr, zweiklang::engine::Initialize,
        zweiklang::engine::Shutdown, nullptr, zweiklang::engine::Frame, nullptr,
        zweiklang::engine::MenuFrame, zweiklang::engine::BeforeSave,
        zweiklang::engine::AfterSave, zweiklang::engine::BeforeNewGame,
        zweiklang::engine::AfterLoad, zweiklang::engine::BeforeLoad,
        zweiklang::engine::AfterLoad, zweiklang::engine::BeforeLoad,
        zweiklang::engine::AfterLoad, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr);
}
