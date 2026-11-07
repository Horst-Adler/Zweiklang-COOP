META
{
    Parser = Game;
};

// Own empty instance: never clone PC_HERO or replace its parser binding.
instance ZK_COOP_PARTNER(C_NPC)
{
    name[0] = "Koop-Partner";
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
    return ZK_COOP_QOL_GUEST;
};
func void ZK_COOP_PREPARE_QUICKLOOT()
{
    B_ClearRuneInv(other);
};

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
            Log_AddEntry(TOPIC_ADDON_KILLBRAGO, "Koop: Wir helfen Cavalorn gegen die Banditen bei der Hoehle.");
        };
        if (MIS_Addon_Cavalorn_KillBrago == LOG_SUCCESS)
        {
            Log_CreateTopic(TOPIC_ADDON_KILLBRAGO, LOG_MISSION);
            Log_SetTopicStatus(TOPIC_ADDON_KILLBRAGO, LOG_SUCCESS);
            Log_AddEntry(TOPIC_ADDON_KILLBRAGO, "Koop: Die Banditen sind besiegt und Cavalorn hat die Aufgabe abgeschlossen.");
        };
        ZK_COOP_CAV_LOG_STATE = MIS_Addon_Cavalorn_KillBrago;
    };
    if ((MIS_Addon_Cavalorn_Letter2Vatras == LOG_RUNNING) && (ZK_COOP_CAV_LOG_LETTER == 0))
    {
        Log_CreateTopic(TOPIC_ADDON_KDW, LOG_MISSION);
        Log_SetTopicStatus(TOPIC_ADDON_KDW, LOG_RUNNING);
        Log_AddEntry(TOPIC_ADDON_KDW, "Koop: Cavalorn hat uns auf Vatras in Khorinis hingewiesen. Diese Folgequest ist noch kein gemeinsamer Testfall.");
        ZK_COOP_CAV_LOG_LETTER = 1;
    };
};
