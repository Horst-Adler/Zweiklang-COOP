META
{
    Parser = Menu;
    After = zUnionMenu.d;
};

instance MenuItem_Union_Auto_ZweiklangCoop(C_MENU_ITEM_UNION_DEF)
{
    text[0] = "Zweiklang Coop";
    text[1] = "Verbindung, Spielername und Diagnose einstellen.";
    onselaction[0] = SEL_ACTION_STARTMENU;
    onselaction_s[0] = "MENU_ZWEIKLANG_COOP";
};

instance MENU_ZWEIKLANG_COOP(C_MENU_DEF)
{
    backpic = MENU_BACK_PIC;
    items[0] = "MENUITEM_ZK_TITLE";
    items[1] = "MENUITEM_ZK_NAME_LABEL";
    items[2] = "MENUITEM_ZK_NAME_EDIT";
    items[3] = "MENUITEM_ZK_NAME_INPUT";
    items[4] = "MENUITEM_ZK_HOST_LABEL";
    items[5] = "MENUITEM_ZK_HOST_EDIT";
    items[6] = "MENUITEM_ZK_HOST_INPUT";
    items[7] = "MENUITEM_ZK_CLIENT_LABEL";
    items[8] = "MENUITEM_ZK_CLIENT_EDIT";
    items[9] = "MENUITEM_ZK_CLIENT_INPUT";
    items[10] = "MENUITEM_ZK_PORT_LABEL";
    items[11] = "MENUITEM_ZK_PORT_EDIT";
    items[12] = "MENUITEM_ZK_PORT_INPUT";
    items[13] = "MENUITEM_ZK_ROLE_LABEL";
    items[14] = "MENUITEM_ZK_ROLE_CHOICE";
    items[15] = "MENUITEM_ZK_GENDER_LABEL";
    items[16] = "MENUITEM_ZK_GENDER_CHOICE";
    items[17] = "MENUITEM_ZK_LOG_LABEL";
    items[18] = "MENUITEM_ZK_LOG_CHOICE";
    items[19] = "MENUITEM_ZK_ENGINE_LABEL";
    items[20] = "MENUITEM_ZK_ENGINE_CHOICE";
    items[21] = "MENUITEM_ZK_APPLY";
    items[22] = "MENUITEM_ZK_START";
    items[23] = "MENUITEM_ZK_LEAVE";
    items[24] = "MENUITEM_ZK_STATUS";
    items[25] = "MENUITEM_ZK_FF_LABEL";
    items[26] = "MENUITEM_ZK_FF_CHOICE";
    items[100] = "UNION_MENUITEM_BACK";
    defaultoutgame = 2;
    defaultingame = 2;
    flags = flags | MENU_SHOW_INFO;
};

prototype ZK_MENU_LABEL(C_MENU_ITEM)
{
    C_MENU_ITEM_DEF();
    type = MENU_ITEM_TEXT;
    posx = 400;
    dimx = 3000;
    dimy = 350;
    fontname = MENU_FONT_SMALL;
    flags = flags & ~IT_SELECTABLE;
};


prototype ZK_MENU_INPUT(C_MENU_ITEM)
{
    C_MENU_ITEM_DEF();
    type = MENU_ITEM_INPUT;
    posx = 3450;
    dimx = 4100;
    dimy = 350;
    fontname = MENU_FONT_SMALL;
    onchgsetoptionsection = "ZWEIKLANGCOOP_MENU";
    flags = IT_SELECTABLE;
    onselaction[0] = SEL_ACTION_UNDEF;
};

prototype ZK_MENU_CHOICE(C_MENU_ITEM)
{
    C_MENU_ITEM_DEF();
    type = MENU_ITEM_CHOICEBOX;
    posx = 3450;
    dimx = 4100;
    dimy = 350;
    fontname = MENU_FONT_SMALL;
    onchgsetoptionsection = "ZWEIKLANGCOOP_MENU";
    flags = IT_SELECTABLE;
    onselaction[0] = SEL_ACTION_UNDEF;
};

instance MENUITEM_ZK_CLIENT_LABEL(ZK_MENU_LABEL) { text[0] = "Client-IP (IPv4)"; posy = 1900; };
instance MENUITEM_ZK_CLIENT_INPUT(ZK_MENU_INPUT)
{
    posy = 1900;
    onchgsetoption = "Client";
    text[1] = "IP des Client-PCs. 0.0.0.0: Windows waehlt die Netzwerkschnittstelle.";
};
instance MENUITEM_ZK_CLIENT_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 1900; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Rolle Client: eigene primaere IPv4 automatisch. Enter: manuell bearbeiten.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_CLIENT";
};
instance MENUITEM_ZK_ROLE_LABEL(ZK_MENU_LABEL) { text[0] = "Rolle dieses PCs"; posy = 2700; };
instance MENUITEM_ZK_ROLE_CHOICE(ZK_MENU_CHOICE)
{
    posy = 2700;
    text[0] = "Host|Client";
    text[1] = "Links/Rechts: Rolle waehlen. Die zugehoerige eigene IP wird automatisch eingetragen.";
    onchgsetoption = "Role";
};
instance MENUITEM_ZK_START(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 5300; dimx = 8100; dimy = 350;
    flags = flags | IT_TXT_CENTER;
    text[0] = "Sitzung starten";
    text[1] = "Einstellungen speichern und mit der ausgewaehlten Rolle starten. Welt vorher laden.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_START";
};
instance MENUITEM_ZK_LEAVE(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 5800; dimx = 8100; dimy = 350;
    flags = flags | IT_TXT_CENTER;
    text[0] = "Sitzung beenden";
    text[1] = "Aktuelle Coop-Verbindung trennen.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_LEAVE";
};

instance MENUITEM_ZK_TITLE(C_MENU_ITEM_DEF)
{
    text[0] = "ZWEIKLANG COOP";
    type = MENU_ITEM_TEXT;
    posx = 0; posy = 700; dimx = 8100; dimy = 400;
    flags = (flags & ~IT_SELECTABLE) | IT_TXT_CENTER;
};

instance MENUITEM_ZK_GENDER_LABEL(ZK_MENU_LABEL) { text[0] = "Geschlecht / Figur"; posy = 3100; };
instance MENUITEM_ZK_GENDER_CHOICE(ZK_MENU_CHOICE)
{
    posy = 3100;
    text[0] = "Maennlich (Held)|Weiblich (Velaya)";
    text[1] = "Links/Rechts: Figur waehlen und speichern. Native G2-Figur mit Velayas Kopf/Gesicht; Outfit erst Beta.";
    onchgsetoption = "Gender";
};

instance MENUITEM_ZK_NAME_LABEL(ZK_MENU_LABEL) { text[0] = "Spielername"; posy = 1100; };
instance MENUITEM_ZK_NAME_INPUT(ZK_MENU_INPUT)
{
    posy = 1100;
    onchgsetoption = "Name";
    text[1] = "Enter: Name bearbeiten. Enter bestaetigt, Escape verwirft die Eingabe.";
};
instance MENUITEM_ZK_HOST_LABEL(ZK_MENU_LABEL) { text[0] = "Host-IP (IPv4)"; posy = 1500; };
instance MENUITEM_ZK_HOST_INPUT(ZK_MENU_INPUT)
{
    posy = 1500;
    onchgsetoption = "Host";
    text[1] = "Rolle Host: eigene primaere IPv4 automatisch. Als Client: Adresse des Host-PCs.";
};
instance MENUITEM_ZK_PORT_LABEL(ZK_MENU_LABEL) { text[0] = "UDP-Port"; posy = 2300; };
instance MENUITEM_ZK_PORT_INPUT(ZK_MENU_INPUT)
{
    posy = 2300;
    onchgsetoption = "Port";
    text[1] = "Enter: Port 1024 bis 65535 eingeben. Standard: 12345.";
};
instance MENUITEM_ZK_LOG_LABEL(ZK_MENU_LABEL) { text[0] = "Protokollierung"; posy = 3500; };
instance MENUITEM_ZK_LOG_CHOICE(ZK_MENU_CHOICE)
{
    posy = 3500;
    text[0] = "Aus|Normal|Debug";
    text[1] = "Links/Rechts: Logstufe. Debug erfasst Pakete und Spielzustand.";
    onchgsetoption = "LogLevel";
};
instance MENUITEM_ZK_ENGINE_LABEL(ZK_MENU_LABEL) { text[0] = "Engine-Meldungen"; posy = 3900; };
instance MENUITEM_ZK_ENGINE_CHOICE(ZK_MENU_CHOICE)
{
    posy = 3900;
    text[0] = "Aus|Ein";
    text[1] = "Links/Rechts: Engine-Diagnosemeldungen zusaetzlich erfassen.";
    onchgsetoption = "EngineMessages";
};
instance MENUITEM_ZK_APPLY(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 4800; dimx = 8100; dimy = 350;
    flags = flags | IT_TXT_CENTER;
    text[0] = "Einstellungen speichern";
    text[1] = "Name, IP-Adressen, Port und Rolle lokal speichern.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_APPLY";
};
instance MENUITEM_ZK_STATUS(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 6300; dimx = 8100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    flags = (flags & ~IT_SELECTABLE) | IT_TXT_CENTER;
    text[0] = "Aenderungen erst mit Einstellungen speichern uebernehmen.";
};

// Gothic hides MENU_ITEM_INPUT outside its native editing loop. These visible
// text controls display the value and explicitly start that loop on Enter.
instance MENUITEM_ZK_NAME_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 1100; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Enter: bearbeiten. Enter bestaetigt, Escape verwirft. Danach Einstellungen speichern.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_NAME";
};
instance MENUITEM_ZK_HOST_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 1500; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Enter: IPv4-Adresse bearbeiten. Enter bestaetigt, Escape verwirft.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_HOST";
};
instance MENUITEM_ZK_PORT_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 2300; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Enter: Port 1024 bis 65535 bearbeiten. Standard: 12345.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_PORT";
};






instance MENUITEM_ZK_FF_LABEL(ZK_MENU_LABEL) { text[0] = "Friendly Fire"; posy = 4300; };
instance MENUITEM_ZK_FF_CHOICE(ZK_MENU_CHOICE)
{
    posy = 4300;
    text[0] = "Aus|Ein";
    text[1] = "Host-Regel. Aus: kein Partnerschaden und kein Partner-Kampffokus. Client uebernimmt Host-Regel.";
    onchgsetoption = "FriendlyFire";
};
