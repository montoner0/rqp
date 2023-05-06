#pragma once

enum
{
    ID_BOX = 0,
    ID_MAXBUTN,
    ID_VOLBARS,
    ID_TIME,
    ID_POS,
    ID_PREV,
    ID_PLAYPAUSE,
    ID_NEXT,
    ID_STOP,
    ID_DEMO,
    ID_AUTONEXT,
    ID_VOL,
    ID_BALANCE,
    ID_DESCRMARK,
    ID_INFO_TAGS,
    ID_ARROW,
    ID_INFO,
    ID_EXTPLAYER
} DialogControls;

enum
{
    CLR_BLACK = 0,
    CLR_BLUE,
    CLR_GREEN,
    CLR_CYAN,
    CLR_RED,
    CLR_PURPLE,
    CLR_BROWN,
    CLR_GRAY,
    CLR_DGRAY,
    CLR_LBLUE,
    CLR_LGREEN,
    CLR_LCYAN,
    CLR_LRED,
    CLR_LPURPLE,
    CLR_YELLOW,
    CLR_WHITE
} Colors16;

typedef struct
{
    BYTE bg;
    BYTE butn;
    BYTE hbutn;
    BYTE text;
    BYTE htext;
    BYTE volbars;
    BYTE volslider;
    BYTE panslider;
    BYTE posslider;
    BYTE time;
    BYTE box;
    BYTE boxtitle;
    BYTE reserved1;
    BYTE reserved2;
} DLG_COLORS;

#define MAX_DEMO_MARK 3

#define REFRESH_FREQ 30

#define MIN_BUTTON _T("[\x1a\x1b]")
#define MAX_BUTTON _T("[\x1b\x1a]")

#define PLAYBACK_BTN_LEN 4

#define TXT_VOLBARS_X   2
#define TXT_TIME_X      2
#define TXT_POSBAR_X    TXT_TIME_X+TIME_LEN

#define BTN_PREV_X      2
#define BTN_PLAYPAUSE_X BTN_PREV_X+PLAYBACK_BTN_LEN
#define BTN_NEXT_X      BTN_PLAYPAUSE_X+PLAYBACK_BTN_LEN
#define BTN_STOP_X      BTN_NEXT_X+PLAYBACK_BTN_LEN
#define BTN_DEMO_X      BTN_STOP_X+PLAYBACK_BTN_LEN
#define BTN_AUTONEXT_X  BTN_DEMO_X+7
#define TXT_VOLUME_X    BTN_AUTONEXT_X+15
#define TXT_BALANCE_X   TXT_VOLUME_X+VOLSLIDER_LEN+1

#define TXT_DESCRMARK_X 1
#define TXT_TAGS_X      2
#define TXT_TAGSARROW_X DLG_WIDTH-2
#define TXT_INFO_X      2
#define TXT_BTNMAX_X    DLG_WIDTH-6
#define BTN_EXTPLAY_X   2

#define FDFILENAME FileName
#define FDFILEATTR FileAttributes
#define FDFILESIZE FileSize
#define REDRAWALL    {EnterCriticalSection(&g_refreshcrits); \
                     DebugOut(_T("RedrawEnter")); \
                     Info.AdvControl(PLUGIN_ID,ACTL_REDRAWALL,0,0); \
                     DebugOut(_T("RedrawAbouttoLeave")); \
                     LeaveCriticalSection(&g_refreshcrits); \
                     DebugOut(_T("RedrawLeave"));}
#define MENUID(...) Info.Menu(PLUGIN_ID, &g_menuguid, __VA_ARGS__)

extern struct PluginStartupInfo Info;
extern TCHAR g_cmdline[PATH_LEN];
extern BOOL g_bSelfExecute;
extern TCHAR PluginRootKey[];
