META
{
    Parser = Menu;
    After = zUnionMenu.d;
};

instance MenuItem_Union_Auto_ZweiklangCoop(C_MENU_ITEM_UNION_DEF)
{
    text[0] = "Orebound Co-op";
    text[1] = "Configure connection, character and diagnostics.";
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
    items[27] = "MENUITEM_ZK_LANGUAGE_LABEL";
    items[28] = "MENUITEM_ZK_LANGUAGE_CHOICE";
    items[100] = "MENUITEM_ZK_BACK";
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

instance MENUITEM_ZK_CLIENT_LABEL(ZK_MENU_LABEL)
{ text[0] = "Client IP (IPv4)"; posy = 1760; };
instance MENUITEM_ZK_CLIENT_INPUT(ZK_MENU_INPUT)
{
    posy = 1760;
    onchgsetoption = "Client";
    text[1] = "Client PC address. Use 0.0.0.0 for automatic interface selection.";
};
instance MENUITEM_ZK_CLIENT_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 1760; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Client role: primary local IPv4 is detected automatically. Enter: edit manually.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_CLIENT";
};
instance MENUITEM_ZK_ROLE_LABEL(ZK_MENU_LABEL)
{ text[0] = "Role of this PC"; posy = 2520; };
instance MENUITEM_ZK_ROLE_CHOICE(ZK_MENU_CHOICE)
{
    posy = 2520;
    text[0] = "Host|Client";
    text[1] = "Left/Right: select role. Its local IP is filled in automatically.";
    onchgsetoption = "Role";
};
instance MENUITEM_ZK_START(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 5320; dimx = 8100; dimy = 350;
    flags = flags | IT_TXT_CENTER;
    text[0] = "Start session";
    text[1] = "Save settings and start with the selected role. Load a world first.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_START";
};
instance MENUITEM_ZK_LEAVE(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 5740; dimx = 8100; dimy = 350;
    flags = flags | IT_TXT_CENTER;
    text[0] = "End session";
    text[1] = "Disconnect the current co-op session.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_LEAVE";
};

instance MENUITEM_ZK_TITLE(C_MENU_ITEM_DEF)
{
    text[0] = "OREBOUND CO-OP";
    type = MENU_ITEM_TEXT;
    posx = 0; posy = 500; dimx = 8100; dimy = 400;
    flags = (flags & ~IT_SELECTABLE) | IT_TXT_CENTER;
};

instance MENUITEM_ZK_GENDER_LABEL(ZK_MENU_LABEL)
{ text[0] = "Character / gender"; posy = 2900; };
instance MENUITEM_ZK_GENDER_CHOICE(ZK_MENU_CHOICE)
{
    posy = 2900;
    text[0] = "Male (Hero)|Female (Velaya)";
    text[1] = "Left/Right: choose a character and save. Native G2 body and face; outfit planned for beta.";
    onchgsetoption = "Gender";
};

instance MENUITEM_ZK_NAME_LABEL(ZK_MENU_LABEL)
{ text[0] = "Player name"; posy = 1000; };
instance MENUITEM_ZK_NAME_INPUT(ZK_MENU_INPUT)
{
    posy = 1000;
    onchgsetoption = "Name";
    text[1] = "Enter: edit name. Enter confirms; Escape cancels.";
};
instance MENUITEM_ZK_HOST_LABEL(ZK_MENU_LABEL)
{ text[0] = "Host IP (IPv4)"; posy = 1380; };
instance MENUITEM_ZK_HOST_INPUT(ZK_MENU_INPUT)
{
    posy = 1380;
    onchgsetoption = "Host";
    text[1] = "Host role: automatic local IPv4. Client role: address of the host PC.";
};
instance MENUITEM_ZK_PORT_LABEL(ZK_MENU_LABEL)
{ text[0] = "UDP port"; posy = 2140; };
instance MENUITEM_ZK_PORT_INPUT(ZK_MENU_INPUT)
{
    posy = 2140;
    onchgsetoption = "Port";
    text[1] = "Enter: port 1024 to 65535. Default: 12345.";
};
instance MENUITEM_ZK_LOG_LABEL(ZK_MENU_LABEL)
{ text[0] = "Logging"; posy = 3280; };
instance MENUITEM_ZK_LOG_CHOICE(ZK_MENU_CHOICE)
{
    posy = 3280;
    text[0] = "Off|Normal|Debug";
    text[1] = "Left/Right: log level. Debug records packets and game state.";
    onchgsetoption = "LogLevel";
};
instance MENUITEM_ZK_ENGINE_LABEL(ZK_MENU_LABEL)
{ text[0] = "Engine messages"; posy = 3660; };
instance MENUITEM_ZK_ENGINE_CHOICE(ZK_MENU_CHOICE)
{
    posy = 3660;
    text[0] = "Off|On";
    text[1] = "Left/Right: capture additional engine diagnostic messages.";
    onchgsetoption = "EngineMessages";
};
instance MENUITEM_ZK_APPLY(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 4900; dimx = 8100; dimy = 350;
    flags = flags | IT_TXT_CENTER;
    text[0] = "Save settings";
    text[1] = "Save name, IP addresses, port, role and language locally.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_APPLY";
};
instance MENUITEM_ZK_STATUS(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 6160; dimx = 8100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    flags = (flags & ~IT_SELECTABLE) | IT_TXT_CENTER;
    text[0] = "Use Save settings to keep your changes.";
};

// Gothic hides MENU_ITEM_INPUT outside its native editing loop. These visible
// text controls display the value and explicitly start that loop on Enter.
instance MENUITEM_ZK_NAME_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 1000; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Enter: edit. Enter confirms; Escape cancels. Then save settings.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_NAME";
};
instance MENUITEM_ZK_HOST_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 1380; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Enter: edit IPv4 address. Enter confirms; Escape cancels.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_HOST";
};
instance MENUITEM_ZK_PORT_EDIT(C_MENU_ITEM_DEF)
{
    posx = 3450; posy = 2140; dimx = 4100; dimy = 350;
    fontname = MENU_FONT_SMALL;
    text[0] = "";
    text[1] = "Enter: edit port 1024 to 65535. Default: 12345.";
    onselaction[0] = SEL_ACTION_EXECCOMMANDS;
    onselaction_s[0] = "ZWEIKLANG_EDIT_PORT";
};






instance MENUITEM_ZK_FF_LABEL(ZK_MENU_LABEL)
{ text[0] = "Friendly fire"; posy = 4040; };
instance MENUITEM_ZK_FF_CHOICE(ZK_MENU_CHOICE)
{
    posy = 4040;
    text[0] = "Off|On";
    text[1] = "Host rule. Off: no partner damage or combat targeting. Client follows the host rule.";
    onchgsetoption = "FriendlyFire";
};

instance MENUITEM_ZK_LANGUAGE_LABEL(ZK_MENU_LABEL) { text[0] = "Language / Sprache"; posy = 4420; };
instance MENUITEM_ZK_LANGUAGE_CHOICE(ZK_MENU_CHOICE)
{
    posy = 4420;
    text[0] = "English|Deutsch";
    text[1] = "Left/Right: choose your local language. Save settings to keep it.";
    onchgsetoption = "Language";
};
instance MENUITEM_ZK_BACK(C_MENU_ITEM_DEF)
{
    posx = 0; posy = 6892; dimx = 8100; dimy = 350;
    flags = flags | IT_TXT_CENTER;
    text[0] = "Back";
    onselaction[0] = SEL_ACTION_BACK;
};
