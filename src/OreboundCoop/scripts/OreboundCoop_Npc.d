META
{
    Parser = Game;
};

// Own empty instance: never clone PC_HERO or replace its parser binding.
instance ZK_COOP_PARTNER(C_NPC)
{
    name[0] = "Co-op partner";
    npcType = 0;
    guild = 0;
    level = 0;
    attribute[0] = 100;
    attribute[1] = 100;
    attribute[4] = 10;
    attribute[5] = 10;
    noFocus = 0;
    Mdl_SetVisual(self, "HUMANS.MDS");
};

func void ZK_COOP_PREPARE_TRADE()
{
    B_GiveTradeInv(other);
};

// Queried dynamically by the adapted Ninja Quickloot 1.9.5 frame function.
var int ZK_COOP_QOL_ACTIVE;
var int ZK_COOP_QOL_GUEST;
func int ZK_COOP_QOL_BLOCK_QUICKLOOT()
{
    return ZK_COOP_QOL_ACTIVE;
};
func int ZK_COOP_QOL_BLOCK_PARTYSPRINT()
{
    // The hero keeps upstream sprint; partner overlays come from the plugin.
    // Do not pass the AI-less remote actor into the upstream party loop.
    return ZK_COOP_QOL_ACTIVE;
};
func void ZK_COOP_PREPARE_QUICKLOOT()
{
    B_ClearRuneInv(other);
};

var string ZK_COOP_CAV_TEXT_ACTIVE;
var string ZK_COOP_CAV_TEXT_DONE;
var string ZK_COOP_CAV_TEXT_LETTER;

// Narrow first-quest adapter. Native host dialogue remains unchanged.
var int ZK_COOP_QUEST_XP;
var int ZK_COOP_CAV_LOG_STATE;
var int ZK_COOP_CAV_LOG_LETTER;
func int ZK_COOP_CAVALORN_WAIT()
{
    return FALSE;
};
func void ZK_COOP_APPLY_QUEST_XP()
{
    B_GivePlayerXP(ZK_COOP_QUEST_XP);
};
func void ZK_COOP_CAVALORN_LOG()
{
    if (MIS_Addon_Cavalorn_KillBrago != ZK_COOP_CAV_LOG_STATE)
    {
        if (MIS_Addon_Cavalorn_KillBrago == LOG_RUNNING)
        {
            Log_CreateTopic(TOPIC_ADDON_KILLBRAGO, LOG_MISSION);
            Log_SetTopicStatus(TOPIC_ADDON_KILLBRAGO, LOG_RUNNING);
            Log_AddEntry(TOPIC_ADDON_KILLBRAGO, ZK_COOP_CAV_TEXT_ACTIVE);
        };
        if (MIS_Addon_Cavalorn_KillBrago == LOG_SUCCESS)
        {
            Log_CreateTopic(TOPIC_ADDON_KILLBRAGO, LOG_MISSION);
            Log_SetTopicStatus(TOPIC_ADDON_KILLBRAGO, LOG_SUCCESS);
            Log_AddEntry(TOPIC_ADDON_KILLBRAGO, ZK_COOP_CAV_TEXT_DONE);
        };
        ZK_COOP_CAV_LOG_STATE = MIS_Addon_Cavalorn_KillBrago;
    };
    if ((MIS_Addon_Cavalorn_Letter2Vatras == LOG_RUNNING) && (ZK_COOP_CAV_LOG_LETTER == 0))
    {
        Log_CreateTopic(TOPIC_ADDON_KDW, LOG_MISSION);
        Log_SetTopicStatus(TOPIC_ADDON_KDW, LOG_RUNNING);
        Log_AddEntry(TOPIC_ADDON_KDW, ZK_COOP_CAV_TEXT_LETTER);
        ZK_COOP_CAV_LOG_LETTER = 1;
    };
};


// Spawn only the creature. The casting player's native spell already paid
// mana/scroll costs; never replay Spell_Cast_* on the other PC.
var int ZK_COOP_SUMMON_SPELL;
func void ZK_COOP_REPLAY_SUMMON()
{
    if (ZK_COOP_SUMMON_SPELL == 22) { Wld_SpawnNpcRange(self, SUMMONED_GOBBO_SKELETON, 1, 500.0); };
    if (ZK_COOP_SUMMON_SPELL == 25) { Wld_SpawnNpcRange(self, SUMMONED_WOLF, 1, 500.0); };
    if (ZK_COOP_SUMMON_SPELL == 31) { Wld_SpawnNpcRange(self, SUMMONED_SKELETON, 1, 500.0); };
    if (ZK_COOP_SUMMON_SPELL == 35) { Wld_SpawnNpcRange(self, SUMMONED_GOLEM, 1, 500.0); };
    if (ZK_COOP_SUMMON_SPELL == 40) { Wld_SpawnNpcRange(self, SUMMONED_DEMON, 1, 500.0); };
    if (ZK_COOP_SUMMON_SPELL == 45) { Wld_SpawnNpcRange(self, SUMMONED_SKELETON, 6, 800.0); };
    if (ZK_COOP_SUMMON_SPELL == 84) { Wld_SpawnNpcRange(self, SUMMONED_GUARDIAN, 1, 500.0); };
    if (ZK_COOP_SUMMON_SPELL == 88) { Wld_SpawnNpcRange(self, SUMMONED_ZOMBIE, 1, 500.0); };
};
