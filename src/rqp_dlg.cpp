#include "stdafx.h"
#include <assert.h>
#include <shellapi.h>
#include <stdio.h>
#include "rqpf.h"
#include "rqp_dlg.h"
#include "rqp_bass.h"
#include "plugin.hpp"
#include "farcolor.hpp"
#include "registry.h"
#include "helpers.h"
#include "guids.h"
#include "unidefs.h"
#include "keys.h"
#include "rqp_dlg_priv.h"
#include "onkey_cases.h"
#include "Farsettings.h"
#include "descriptions.h"
#include <vector>
#include <iterator>

using namespace std;

static FarDialogItem g_dlgitems[] =
{
   SETFARDIALOGITEM(DI_DOUBLEBOX,   0,0,DLG_WIDTH - 1,DLG_HEIGHT - 1, DIF_SHOWAMPERSAND, _T("RQP")),

   SETFARDIALOGITEM2(DI_TEXT,       TXT_BTNMAX_X,0,               0,                _T("[\x1b\x1a]")),  // max button

   SETFARDIALOGITEM2(DI_TEXT,       TXT_VOLBARS_X,1,              DIF_SHOWAMPERSAND, _T("")),  // vol level bars DIF_SETCOLOR|0x0a

   SETFARDIALOGITEM2(DI_TEXT,       TXT_TIME_X,2,                 0,                 _T("")),//   0:00.000")},
   SETFARDIALOGITEM2(DI_TEXT,       TXT_POSBAR_X,2,               0,                 _T("")),//[|===================================================]")},

   SETFARDIALOGITEM2(DI_BUTTON,     BTN_PREV_X,4,                 DIF_NOBRACKETS | DIF_NOFOCUS, _T("")),// < ")},    |DIF_SETCOLOR|0x7
   SETFARDIALOGITEM2(DI_BUTTON,     BTN_PLAYPAUSE_X,4,            DIF_NOBRACKETS | DIF_NOFOCUS, _T("")),// \x10 ")}, |DIF_SETCOLOR|0x7
   SETFARDIALOGITEM2(DI_BUTTON,     BTN_NEXT_X,4,                 DIF_NOBRACKETS | DIF_NOFOCUS, _T("")),// > ")},    |DIF_SETCOLOR|0x7
   SETFARDIALOGITEM2(DI_BUTTON,     BTN_STOP_X,4,                 DIF_NOBRACKETS | DIF_NOFOCUS, _T("")),// \x16 ")}, |DIF_SETCOLOR|0x7
   SETFARDIALOGITEM2(DI_BUTTON,     BTN_DEMO_X,4,                 DIF_NOBRACKETS | DIF_NOFOCUS, _T("")),// Demo      |DIF_SETCOLOR|0x7
   SETFARDIALOGITEM2(DI_BUTTON,     BTN_AUTONEXT_X,4,             DIF_NOBRACKETS | DIF_NOFOCUS, _T("")),// Auto-Next |DIF_SETCOLOR|0x7
   SETFARDIALOGITEM2(DI_TEXT,       TXT_VOLUME_X,4,               DIF_SHOWAMPERSAND, _T("")),//.....:::::IIIII")},
   SETFARDIALOGITEM2(DI_TEXT,       TXT_BALANCE_X,4,              DIF_SHOWAMPERSAND, _T("")),//L=====C=====R")},

   SETFARDIALOGITEM2(DI_TEXT,       TXT_DESCRMARK_X,6,            DIF_SHOWAMPERSAND, _T("")), // description mark
   SETFARDIALOGITEM2(DI_TEXT,        TXT_TAGS_X,6,                 DIF_SHOWAMPERSAND, _T("")), // tags
   SETFARDIALOGITEM2(DI_TEXT,       TXT_TAGSARROW_X,6,            DIF_SHOWAMPERSAND, _T("")), // arrow

   SETFARDIALOGITEM2(DI_TEXT,       TXT_INFO_X,7,                 DIF_SHOWAMPERSAND, _T("")),  // info

   SETFARDIALOGITEM2(DI_BUTTON,      BTN_EXTPLAY_X,8,             DIF_NOBRACKETS | DIF_NOFOCUS, _T(""))// Ext. Player |DIF_SETCOLOR|0x7
};

vector <vector<int>> g_dlglines(DLG_HEIGHT - 2);

struct DLG_STATE g_dlgstate = {
   NULL, NULL, NULL, NULL,
   0, VOLSLIDER_LEN, PANSLIDER_LEN, VOLBARS_LEN,
   1., /*1., */ VOLSLIDER_LEN / 2 - 1, PANSLIDER_LEN / 2 - 1,
   FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE,
   0xffffffff, 0xffffffff, {DLG_WIDTH,DLG_HEIGHT},{DLG_WIDTH,DLG_HEIGHT},{DLG_WIDTH,DLG_HEIGHT},-1,0xff//, 0
};

DLG_COLORS g_dlcolrs = {
   MKBYTE(CLR_WHITE,CLR_DGRAY),    // bg
   MKBYTE(CLR_BROWN,CLR_BLACK),    // butn
   MKBYTE(CLR_YELLOW,CLR_BROWN),   // hbutn
   MKBYTE(CLR_WHITE,CLR_DGRAY),    // disbutn
   MKBYTE(CLR_LCYAN,CLR_CYAN),     // text
   MKBYTE(CLR_GREEN,CLR_BLACK),    // htext
   MKBYTE(CLR_LCYAN,CLR_DGRAY),    // invtext
   MKBYTE(CLR_GRAY,CLR_DGRAY),     // panslider
   MKBYTE(CLR_LGREEN,CLR_DGRAY),   // posslider
   MKBYTE(CLR_YELLOW,CLR_DGRAY),   // volslider
   MKBYTE(CLR_GRAY,CLR_DGRAY),     // volbars
   MKBYTE(CLR_WHITE,CLR_DGRAY),    // time
   MKBYTE(CLR_WHITE,CLR_DGRAY),    // box
   MKBYTE(CLR_WHITE,CLR_DGRAY)     // boxtitle
};

#define SET_POSVAL(x) g_dlgstate.poscurval=(double)(x);

#define TEXTPTR Text

static TCHAR g_poschars[] = _T("[=|-]");
static TCHAR g_volchars[VOLSLIDER_LEN + 1] = _T(".:|_");  //_T("0123456789ABCDE ");
static TCHAR g_buttonschars[] = _T("<\x25ba>\x25a0\x2551");
static TCHAR g_panchars[] = _T("L=<C>R");
static TCHAR g_butns[_countof(g_buttonschars)][PLAYBACK_BTN_LEN] = { NULL };
static TCHAR g_plrpath[PATH_LEN];

static TCHAR g_plrparms[2][128];
static TCHAR* g_extplraction[2] = { 0,0 };
static char g_tagtemplate[200] = "%IFV1(%ARTI,%ARTI)%IFV1(%TITL, - %TITL)%IFV1(%ALBM, - %ALBM)";
static HANDLE g_hExitEvent = NULL;
static HANDLE g_hMetaEvent = NULL;
static HANDLE g_hThread = NULL;
CRITICAL_SECTION g_refreshcrits;

static HSYNC g_demomark[MAX_DEMO_MARK] = { NULL };
static int g_demotimes[_countof(g_demomark)] = { 20,20,20 };
static int g_demostarts[] = { 0,5,10,20,30,60 };
static int g_demostartidx = 0;
static LONG g_showinfodelay = 0;
TRACK_INFO g_ti = { 0 };
static HSTREAM g_hsnd = NULL;
static int g_fadecurval = FADE_DEF_VAL, g_fadedefval = FADE_DEF_VAL;
static int g_PanelHeight;
static HSYNC g_endoftracksync = NULL;
static HANDLE g_hdlg = NULL;

void CALLBACK OnEndOfTrack(HSYNC handle, DWORD channel, DWORD data, void* user);
FINT LoadAndPrepareTrack(HANDLE hdlg, TRACK_INFO* ti, HSTREAM* h_snd, int delta = 0);
BOOL SearchAndSetOnPanel(const TCHAR* fn);
static void ClearTrackEndHandler(HSTREAM hsnd);
static void SetTrackEndHandler(HANDLE hdlg, HSTREAM hsnd);
void Dlg_OnClose(HANDLE hdlg);
void _RefreshDlg(HANDLE hdlg);

//HANDLE g_hWaitKeyThread = NULL, g_hWaitKeyEvent = NULL;
//INPUT_RECORD g_WaitKey;
//
//DWORD WINAPI WaitForKey(LPVOID param)
//{
//	INPUT_RECORD* ir = (INPUT_RECORD*)param;
//
//	return 0;
//}
//
//void StartWaitForKey(WORD keycode, DWORD mod)
//{
//	g_WaitKey.EventType = KEY_EVENT;
//	g_WaitKey.Event.KeyEvent.bKeyDown = TRUE;
//	g_WaitKey.Event.KeyEvent.wVirtualKeyCode = keycode;
//	g_WaitKey.Event.KeyEvent.dwControlKeyState = mod;
//}
//
//void StopWaitForKey()
//{
//	if (g_hWaitKeyThread) {
//		StopThread(g_hWaitKeyThread, 0);
//	}
//
//	g_hWaitKeyThread = NULL;
//}
//
//BOOL IsKeyPressed()
//{
//	return TRUE;
//	//return WaitForSingleObject(g_hWaitKeyEvent,0)!=WAIT_TIMEOUT;
//}

void MakePosLine(int len, int curpos, TCHAR* line)
{
	assert(line);
	//   assert(curpos>0 && curpos<len-2);

	curpos = SETLIMITS(curpos, 1, len - 3);

	for (int i = 1; i < curpos; i++) line[i] = g_poschars[1];
	for (int i = curpos + 1; i < len - 2; i++) line[i] = g_poschars[3];
	line[0] = g_poschars[0];
	line[len - 2] = g_poschars[4];
	line[len - 1] = 0;

	line[curpos] = g_poschars[2];
}

void MakeBalanceLine(int len, int curpos, TCHAR* line)
{
	assert(line);
	//assert(curpos>0 && curpos<len-1);

	curpos = SETLIMITS(curpos, 1, len - 3);
	for (int i = 1; i < len - 2; i++) line[i] = g_panchars[1];
	line[0] = g_panchars[0];
	line[len - 2] = g_panchars[5];
	line[len - 1] = 0;

	line[curpos] = curpos<len / 2. - 1 ? g_panchars[2] : curpos>len / 2. - 1 ? g_panchars[4] : g_panchars[3];
}

void MakeVolLine(int len, int curpos, TCHAR* line)
{
	assert(line);
	//assert(curpos>=0 && curpos<len);
	int charnum = (int)_tcslen(g_volchars) - 1;

	curpos = SETLIMITS(curpos, 0, len - 2);

	double chars = (double)(len - 1) / charnum;
	int i = 0;

	for (int j = 0; j < charnum; j++) {
		for (; i < (int)(chars * (j + 1) + .01); i++) {
			line[i] = g_volchars[j];
		}
	}

	for (int i = curpos + 1; i < len - 1; i++) {
		line[i] = g_volchars[charnum];
	}
	line[len - 1] = 0;
}

void MakeVolBars(TCHAR* line, int len, int curpos1, int curpos2)
{
	TCHAR vollevchars[] =
		_T("\x2580\x2584\x2588 ");
	//assert(line);
	curpos1 = SETLIMITS(curpos1, 0, len - 1);
	curpos2 = SETLIMITS(curpos2, 0, len - 1);

	int maxlevidx, minlev, maxlev;
	if (curpos1 > curpos2) {
		maxlevidx = 0;
		minlev = curpos2;
		maxlev = curpos1;
	} else
		if (curpos1 < curpos2) {
			maxlevidx = 1;
			minlev = curpos1;
			maxlev = curpos2;
		} else {
			maxlevidx = 3;
			minlev = maxlev = curpos1;
		}

	_tmemset(line, vollevchars[3], len - 1);
	line[len - 1] = 0;
	_tcsnset_s(line, len, vollevchars[2], minlev);
	_tcsnset_s(&line[minlev], len - minlev, vollevchars[maxlevidx], maxlev - minlev);
}

static void Set_Vol(HANDLE hdlg, HSTREAM hsnd)
{
	assert(hsnd);
	MakeVolLine(g_dlgstate.volsliderlen, g_dlgstate.volcurval, g_dlgstate.volslider);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_VOL, (FPAR2)g_dlgstate.volslider);
	SetSoundVol(hsnd, (float)g_dlgstate.volcurval / (g_dlgstate.volsliderlen - 2));
}

static void Set_Pan(HANDLE hdlg, HSTREAM hsnd)
{
	assert(hsnd);
	MakeBalanceLine(g_dlgstate.pansliderlen, g_dlgstate.pancurval, g_dlgstate.panslider);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_BALANCE, (FPAR2)g_dlgstate.panslider);
	SetSoundPan(hsnd, ROUND_DIV(g_dlgstate.pancurval - 1, (g_dlgstate.pansliderlen - 4), 10) * 2 - 1);
}

static void Set_Pos(HANDLE hdlg, HSTREAM hsnd/*, BOOL bFade=TRUE*/)
{
	assert(hsnd);
	BASS_ChannelLock(hsnd, TRUE);
	MakePosLine(g_dlgstate.possliderlen, (int)g_dlgstate.poscurval, g_dlgstate.posslider);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_POS, (FPAR2)g_dlgstate.posslider);
	g_dlgstate.bRestart = FALSE;
	if (g_dlgstate.bPlay) {
		PlaySoundFromPercent(hsnd, (float)(g_dlgstate.poscurval - 1) / (g_dlgstate.possliderlen - 3) * 100.);
	} else {
		SetSoundPosPercent(hsnd, (float)(g_dlgstate.poscurval - 1) / (g_dlgstate.possliderlen - 3) * 100.);
	}
	BASS_ChannelLock(hsnd, FALSE);
}

void CALLBACK OnDemoJump(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	INT_PTR i = (INT_PTR)user;

	DebugOut(_T("OnDemoJump<"));
	QWORD demopart = g_ti.lenbytes / MAX_DEMO_MARK;
	QWORD pos = BASS_ChannelGetPosition(g_hsnd, BASS_POS_BYTE);
	QWORD halfsec = BASS_ChannelSeconds2Bytes(g_hsnd, .5);

	if (pos < demopart * (i + 1) - halfsec)
		if (!BASS_ChannelSetPosition(g_hsnd, demopart * (i + 1) - halfsec, BASS_POS_BYTE))
			assert(BASS_ErrorGetCode() == BASS_OK);

	DebugOut(_T(">OnDemoJump"));
}

static void Set_Demo(HANDLE hdlg, HSTREAM hsnd, TRACK_INFO* ti, int offsets = 0)
{
	assert(ti && hsnd);
	if (g_dlgstate.bDemo && ti->fType != SOUND_URL) {
		QWORD offsetb = BASS_ChannelSeconds2Bytes(hsnd, offsets);
		QWORD demopart = (ti->lenbytes - offsetb) / MAX_DEMO_MARK;
		for (int i = 0; i < MAX_DEMO_MARK; i++) {
			QWORD demotime = BASS_ChannelSeconds2Bytes(hsnd, g_demotimes[i]);

			if (demopart < demotime) {
				break; // track length is less than demopart length * demotime
			}

			g_demomark[i] = BASS_ChannelSetSync(hsnd, BASS_SYNC_POS, demopart * i + demotime + offsetb, OnDemoJump, (void*)(INT_PTR)i);
		}
	} else {
		for (int i = 0; i < MAX_DEMO_MARK; i++) {
			BASS_ChannelRemoveSync(hsnd, g_demomark[i]);
			g_demomark[i] = NULL;
		}
	}
}

static void SetLoopMode(HANDLE hdlg, HSTREAM hsnd)
{
	if (g_dlgstate.bLoopEnabled) {
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_AUTONEXT, (FPAR2)GetLocalizedMsg(MLoop));
	} else {
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_AUTONEXT, (FPAR2)GetLocalizedMsg(MAutonext));
	}

	BASS_ChannelLock(hsnd, TRUE);
	if (g_dlgstate.bLoopEnabled && g_dlgstate.bAutoNextOrLoopOn) {
		BASS_ChannelFlags(hsnd, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP | BASS_MUSIC_STOPBACK);
		ClearTrackEndHandler(hsnd);
	} else {
		BASS_ChannelFlags(hsnd, BASS_MUSIC_STOPBACK, BASS_MUSIC_STOPBACK | BASS_SAMPLE_LOOP);
		ClearTrackEndHandler(hsnd);
		SetTrackEndHandler(hdlg, hsnd);
	}
	BASS_ChannelLock(hsnd, FALSE);
}

static void NextDemoStart(HANDLE hdlg)
{
	if (g_showinfodelay != 0) {
		g_demostartidx++;
		if (g_demostartidx >= _countof(g_demostarts)) {
			g_demostartidx = 0;
		}
	}

	TCHAR s[7];

	_stprintf_s(s, _countof(s), _T("  %02d  "), g_demostarts[g_demostartidx]);

	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_DEMO, (FPAR2)s);

	g_showinfodelay = REFRESH_FREQ * 2;
}

void ScrollTags(HANDLE hdlg, BOOL bReset, TCHAR* _tags, int _tagslen)
{
	TCHAR c;
	static int tagspos = 0, directn = -1, waitcounter = 0, scrollcounter = 0, tagslen = 0;
	static TCHAR* tags = NULL;

	if (bReset) {
		tagspos = 0, directn = -1, waitcounter = 0, scrollcounter = 0;
		tags = _tags;
		tagslen = _tagslen;
	}
	assert(tags && tagslen >= 0);

	int end = g_dlgstate.dlgsize.X - 4 + tagspos;
	if (end > tagslen) end = tagslen;
	c = tags[end];
	tags[end] = 0;
	int r = Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_INFO_TAGS, (FPAR2)&tags[tagspos]);
	tags[end] = c;

	if (tagspos > 0 && tagspos < tagslen - g_dlgstate.dlgsize.X + 4) {
		if (scrollcounter > REFRESH_FREQ / 12) {
			tagspos += directn;
			scrollcounter = 0;
		} else
			scrollcounter++;
	} else {
		waitcounter++;
		if (waitcounter > (int)(REFRESH_FREQ * 1.5)) {
			directn *= -1;
			tagspos += directn;
			waitcounter = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Update track info and scrolling
void UpdateTrackInfo(HANDLE hdlg, TRACK_INFO* ti, BOOL bScroll = TRUE)
{
	assert(ti && ti->fname && ti->tags);
	Info.SendDlgMessage(hdlg, DM_ENABLEREDRAW, FALSE, 0);

	TCHAR* tag = NULL;
	size_t taglen;

	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_INFO, (FPAR2)ti->info);

	if (g_dlgstate.bShowDesc) {
		tag = ti->descr;
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_DESCRMARK, (FPAR2)_T("*"));
	} else {
		tag = ti->tags;
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_DESCRMARK, (FPAR2)_T(" "));
	}
	taglen = _tcslen(tag);

	TCHAR* c = _tcsrchr(ti->fname, _T('\\'));
	if (c)
		c++;
	else
		c = ti->fname;

	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_BOX, (FPAR2)c);

	g_dlgstate.bScrolltags = (bScroll && taglen > (unsigned)(g_dlgstate.dlgsize.X - 4));
	if (g_dlgstate.bScrolltags) {
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_ARROW, (FPAR2)_T("\x1a"));
	} else {
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_ARROW, (FPAR2)_T(" "));
	}
	ScrollTags(hdlg, TRUE, tag, (int)taglen);

	Info.SendDlgMessage(hdlg, DM_ENABLEREDRAW, TRUE, 0);
}

static void SetTrackEndHandler(HANDLE hdlg, HSTREAM hsnd)
{
	if (!(g_dlgstate.bLoopEnabled && g_dlgstate.bAutoNextOrLoopOn))
		g_endoftracksync = BASS_ChannelSetSync(hsnd, BASS_SYNC_END, 0, OnEndOfTrack, hdlg);
}

static void ClearTrackEndHandler(HSTREAM hsnd)
{
	BASS_ChannelRemoveSync(hsnd, g_endoftracksync);
	g_endoftracksync = NULL;
}

static void Do_PlayPause(HANDLE hdlg, HSTREAM hsnd)
{
	assert(hsnd);

	g_dlgstate.bPlay = !g_dlgstate.bPlay;
	g_dlgstate.bStop = FALSE;

	if (g_dlgstate.bPlay) {
		if (g_ti.bLoop && g_dlgstate.bAutoLoop) {
			g_dlgstate.bLoopEnabled = g_dlgstate.bAutoNextOrLoopOn = TRUE;
		}
		SetLoopMode(hdlg, hsnd);
		if (g_dlgstate.bDemo) {
			if (PlaySoundFromTime(hsnd, (double)g_demostarts[g_demostartidx], FALSE) != BASS_OK)
				Play_Sound(hsnd, g_dlgstate.bRestart);
		} else
			Play_Sound(hsnd, g_dlgstate.bRestart);
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_PLAYPAUSE, (FPAR2)g_butns[4]);
		g_dlgstate.bRestart = FALSE;
		ClearTrackEndHandler(hsnd);
		SetTrackEndHandler(hdlg, hsnd);
	} else {
		ClearTrackEndHandler(hsnd);
		Pause_Sound(hsnd);
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_PLAYPAUSE, (FPAR2)g_butns[1]);
	}
}

static void Do_Stop(HANDLE hdlg, HSTREAM hsnd)
{
	assert(hsnd);
	g_dlgstate.bPlay = FALSE;
	g_dlgstate.bStop = TRUE;
	ClearTrackEndHandler(hsnd);
	Stop_Sound(hsnd);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_PLAYPAUSE, (FPAR2)g_butns[1]);
	MakeVolBars(g_dlgstate.vollevbars, (int)g_dlgstate.vollevbarslen, 0, 0);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_VOLBARS, (FPAR2)g_dlgstate.vollevbars);
	g_dlgstate.bRestart = TRUE;
	SET_POSVAL(1.);
	Set_Pos(hdlg, hsnd);
}

void NextPrevTrack(HANDLE hdlg, TRACK_INFO* ti, int nextprev, BOOL bFade = TRUE, BOOL bPlayIt = TRUE)
{
	FINT res, i = 0;
	HSTREAM hsnd = 0;

	assert(ti);
	DebugOut(_T("NextPrevTrack fade=%d<"), bFade);
	if (!bFade)
		SetFadeInOut(0);
	Do_Stop(hdlg, g_hsnd);

	if (g_dlgstate.bIsRealFile) {
		Info.PanelControl(PANEL_ACTIVE, FCTL_UPDATEPANEL, 1, 0);

		PluginPanelItem* ppilast = NULL;
		res = GetPanelItem(&Info, &ppilast, 0); // memorize last good filename
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_BOX, (FPAR2)GetLocalizedMsg(MSearching));
		do {
			res = LoadAndPrepareTrack(hdlg, &g_ti, &hsnd, nextprev);
			if (hsnd) {
				Close_Sound(g_hsnd);
				InterlockedExchange((LONG*)&g_hsnd, hsnd);
				break;
			}
			i++;
		} while (res > 0);

		if (!hsnd) { // slide back to first/last track on panel
			if (ppilast) {
				SearchAndSetOnPanel((TCHAR*)ppilast->FDFILENAME);
				Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_BOX, (FPAR2)ppilast->FDFILENAME);
			}
			res = -1;
		}
		if (bPlayIt && g_hsnd && res != -1) {
			Do_PlayPause(hdlg, g_hsnd);
			if (g_dlgstate.bRedrawPanels) REDRAWALL;
		}
	} else { // trying to process plugin panels (like archives and such)
		PluginPanelItem* ppi = NULL, * ppilast = NULL;
		uintptr_t attr = 0;

		DebugOut(_T("NextPrevTrack>NotRealFile"));
		res = GetPanelItem(&Info, &ppilast, 0); // memorize last good filename
		if (res > 0) {
			do {
				res = GetPanelItem(&Info, &ppi, nextprev);
				if (res > 0) {
					attr = ppi->FDFILEATTR;
					free(ppi);
				}
			} while (res > 0 && (attr & FILE_ATTRIBUTE_DIRECTORY));

			if (res > 0 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
				if (SendKeys())
					Info.SendDlgMessage(hdlg, DM_CLOSE, 0, 0);
			} else {
				if (!SearchAndSetOnPanel((TCHAR*)ppilast->FDFILENAME)) {
					Info.SendDlgMessage(hdlg, DM_CLOSE, 0, 0);
				}
			}
			free(ppilast);
		}
	}

	if (!bFade)
		SetFadeInOut(g_fadecurval);
	DebugOut(_T(">NextPrevTrack fade=%d"), bFade);
}

void Do_Next(HANDLE hdlg, TRACK_INFO* ti, BOOL bFade = TRUE)
{
	assert(ti);
	NextPrevTrack(hdlg, ti, 1, bFade);
}

void Do_Prev(HANDLE hdlg, TRACK_INFO* ti, BOOL bFade = TRUE)
{
	assert(ti);
	NextPrevTrack(hdlg, ti, -1, bFade);
}

void CALLBACK OnEndOfTrack(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	HANDLE hdlg = (HANDLE)user;

	assert(hdlg);
	if (g_dlgstate.bAutoNextOrLoopOn) {
		if (g_dlgstate.bLoopEnabled) return;
		DebugOut(_T("OnEndOfTrack>Do_Next"));
		Info.AdvControl(&g_plguid, ACTL_SYNCHRO, 0, (FPAR2)1);
	} else {
		DebugOut(_T("OnEndOfTrack>Do_Stop"));
		Info.AdvControl(&g_plguid, ACTL_SYNCHRO, 0, (FPAR2)2);
	}
}

FINT WINAPI ProcessSynchroEventW(const struct ProcessSynchroEventInfo* Info)
{
	if (Info->Event == SE_COMMONSYNCHRO) {
		//DebugOut(_T("ProcessSynchroEventW"));
		switch ((int)(Info->Param)) {
			case 0:
				_RefreshDlg(g_hdlg);
				break;
			case 1:
				Do_Next(g_hdlg, &g_ti, FALSE);
				break;
			case 2:
				Do_Stop(g_hdlg, g_hsnd);
				break;
			case 3:
				UpdateTrackInfo(g_hdlg, &g_ti);
				break;
		}
	}

	return 0;
}

DWORD WINAPI RefreshDlg(LPVOID param)
{
	HANDLE hdlg = (HANDLE)param;

	assert(hdlg);
	while (g_hExitEvent && WaitForSingleObject(g_hExitEvent, 1000 / REFRESH_FREQ) == WAIT_TIMEOUT) {
		Info.AdvControl(&g_plguid, ACTL_SYNCHRO, 0, (FPAR2)0);
	}

	Dlg_OnClose((FPAR2)hdlg);

	DebugOut(_T(">RefreshDlg"));

	return 0;
}

void _RefreshDlg(HANDLE hdlg)
{
	TCHAR time[TIME_LEN];
	static BOOL bShowTime = TRUE;
	static int timeblinkcounter = 0, incdec = 1;

	//DebugOut(_T("Refresh0"));
	if (g_hsnd && TryEnterCriticalSection(&g_refreshcrits)) {
		Info.SendDlgMessage(hdlg, DM_ENABLEREDRAW, FALSE, 0);
		if (g_dlgstate.bMaximized) { // костыль
			Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_MAXBUTN, (FPAR2)MIN_BUTTON);
		} else {
			Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_MAXBUTN, (FPAR2)MAX_BUTTON);
		}

		if (bShowTime) {
			GetSoundPosStr(g_hsnd, time, _countof(time), g_dlgstate.bRemainTime, g_dlgstate.bShowms, g_dlgstate.bShowHours);
		} else {
			_tmemset(time, _T(' '), _countof(time));
			time[_countof(time) - 1] = 0;
		}
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_TIME, (FPAR2)time);

		if (g_showinfodelay) {
			InterlockedDecrement(&g_showinfodelay);
			if (g_showinfodelay == 0)
				Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_DEMO, (FPAR2)GetLocalizedMsg(MDemo));
		}

		if (g_dlgstate.bScrolltags) {
			ScrollTags(hdlg, FALSE, NULL, NULL);
		}

		if (g_hMetaEvent && WaitForSingleObject(g_hMetaEvent, 0) == WAIT_OBJECT_0) {
			UpdateTrackInfo(hdlg, &g_ti);
			ResetEvent(g_hMetaEvent);
		}

		if (g_dlgstate.bStop)
			bShowTime = TRUE;
		else
			if (g_dlgstate.bPlay) {
				bShowTime = TRUE;
				BASS_ChannelLock(g_hsnd, TRUE);
				// MODs can report position beyond of the end of track in a loop mode
				QWORD pos = BASS_ChannelGetPosition(g_hsnd, BASS_POS_BYTE) % g_ti.lenbytes;
				DWORD lev = g_dlgstate.bVolbars ? BASS_ChannelGetLevel(g_hsnd) : 0;
				BASS_ChannelLock(g_hsnd, FALSE);

				if (pos == g_ti.lenbytes) {  // end of track
					DebugOut(_T("RefreshDlg>End of track"));
				} else {
					SET_POSVAL((double)pos / g_ti.lenbytes * (g_dlgstate.possliderlen - 3) + 1);

					MakePosLine(g_dlgstate.possliderlen, (int)g_dlgstate.poscurval, g_dlgstate.posslider);
					Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_POS, (FPAR2)g_dlgstate.posslider);

					MakeVolBars(g_dlgstate.vollevbars, (int)g_dlgstate.vollevbarslen, (int)ROUND_DIV(LOWORD(lev) * (g_dlgstate.vollevbarslen - 1), 32768, 1),
								(int)ROUND_DIV(HIWORD(lev) * (g_dlgstate.vollevbarslen - 1), 32768, 1));
					Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_VOLBARS, (FPAR2)g_dlgstate.vollevbars);
				}
			} else { // pause
				timeblinkcounter += incdec;
				if (timeblinkcounter > (int)(REFRESH_FREQ * .7) || timeblinkcounter < 0) {
					bShowTime = !bShowTime;
					incdec *= -1;
				}
			}
		Info.SendDlgMessage(hdlg, DM_ENABLEREDRAW, TRUE, 0);
		LeaveCriticalSection(&g_refreshcrits);
	}
}

BOOL SearchAndSetOnPanel(const TCHAR* fn)
{
	struct PanelInfo pi;

	assert(fn);
	PluginPanelItem* ppi;
	F3STRUCTSIZE(pi);
	if (Info._PControl(PANEL_ACTIVE, FCTL_GETPANELINFO, sizeof(pi), (FPAR2)&pi)) {
		if (pi.PanelType == PTYPE_FILEPANEL && pi.ItemsNumber > 0) {
			for (FSIZET i = 0; i < pi.ItemsNumber; i++) {
				FINT res = Info._PControl(PANEL_ACTIVE, FCTL_GETPANELITEM, i, NULL);
				ppi = (PluginPanelItem*)calloc(1, res);
				if (ppi) {
					INITPANELITEMSTRUCT(ppi, res, pnlitem);
					if (Info._PControl(PANEL_ACTIVE, FCTL_GETPANELITEM, i, pnlitem)) {
						if (!(ppi->FDFILEATTR & FILE_ATTRIBUTE_DIRECTORY) && _tcsicmp(ppi->FDFILENAME, fn) == 0) {
							PanelRedrawInfo pri;
							F3STRUCTSIZE(pri);
							pri.CurrentItem = i;
							pri.TopPanelItem = pi.TopPanelItem;
							Info._PControl(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, 0, (FPAR2)&pri);
							free(ppi);
							return TRUE;
						}
					}
					free(ppi);
				}
			}
		}
	}

	return FALSE;
}

FINT LoadAndPrepareTrack(HANDLE hdlg, TRACK_INFO* ti, HSTREAM* h_snd, int delta)
{
	PluginPanelItem* ppi;
	HSTREAM hsnd = 0;
	FINT res = 1;
	TCHAR* filepath = NULL;
	WIN32_FIND_DATA fd;
	__int64 fsize;

	assert(ti);

	if (g_cmdline[0]) {
		Info.FSF->Unquote(g_cmdline);
		TCHAR* c = _tcsrchr(g_cmdline, _T('\\'));
		if (c)
			c++;
		else
			c = g_cmdline;
		if (!SearchAndSetOnPanel(c)) {
			if (!SearchAndSetOnPanel(g_cmdline)) {
				//            Info.SendDlgMessage(hdlg,DM_CLOSE,0,0);
				//            return FALSE;
			}
		}
		if (g_cmdline[1] == _T(':') || (g_cmdline[0] == _T('\\') && g_cmdline[1] == _T('\\'))) {
			HANDLE h = FindFirstFile(g_cmdline, &fd);
			if (h != INVALID_HANDLE_VALUE) {
				fsize = ((__int64)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
				filepath = _tcsdup(g_cmdline);
				assert(filepath);
				g_dlgstate.bSelectedTrack = IsSelected(&Info);
				FindClose(h);
			}
		} else {
			if (_tcsstr(g_cmdline, _T("://"))) { // URL
				fsize = -1;
				filepath = _tcsdup(g_cmdline);
				assert(filepath);
			}
		}
		g_cmdline[0] = 0;
	}

	if (!filepath && (res = GetPanelItem(&Info, &ppi, delta)) > 0) {   // if cmdline empty or not valid file path try to get it from panel
		if (!(ppi->FDFILEATTR & FILE_ATTRIBUTE_DIRECTORY) && ppi->FDFILENAME && ppi->FDFILENAME[0]) {
			filepath = (TCHAR*)malloc(sizeof(TCHAR) * PATH_LEN);
			assert(filepath);
			filepath[0] = 0;

			g_dlgstate.bSelectedTrack = (BOOL)(ppi->Flags & PPIF_SELECTED);

			if (_tcschr(ppi->FDFILENAME, _T('\\'))) { // is path present
				_tcscpy_s(filepath, PATH_LEN, ppi->FDFILENAME);
			} else { // else take current panel path
				GetPanelDir(filepath, sizeof(TCHAR) * PATH_LEN);
				if (filepath[0]) {
					Info.FSF->AddEndSlash(filepath);
					_tcscat_s(filepath, PATH_LEN, ppi->FDFILENAME);
				} else {
					_tcscpy_s(filepath, PATH_LEN, ppi->FDFILENAME); // got no dir - write filename only
				}
			}
			fsize = ppi->FDFILESIZE;
		}
		free(ppi);
	}

	if (res > 0 && filepath) {
		if (ti->fname)
			free(ti->fname);
		ti->fname = filepath;
		SetTagsFormat(g_tagtemplate);
		hsnd = LoadSound(/*filepath,fsize,*/ ti);

		if (hsnd) {
			UpdateTrackInfo(hdlg, ti);
			if (ti->lensecs <= 5) {
				g_fadecurval = 0;
			} else {
				g_fadecurval = g_fadedefval;
			}
			SetFadeInOut(g_fadecurval);
			Set_Demo(hdlg, hsnd, ti, g_demostarts[g_demostartidx]);
			*h_snd = hsnd;
		}
	}

	return res;
}

void LoadOptions(DLG_STATE* dlgs)
{
	assert(dlgs);

	try {
		FarSettings fs(*PLUGIN_ID, &Info);
		if (fs.GetNumber(L"FirstRun", 42) != 42) {
			dlgs->bDemo = (BOOL)fs.GetNumber(L"DemoMode", FALSE);
			dlgs->bAutoNextOrLoopOn = (BOOL)fs.GetNumber(L"Autonext", FALSE);
			dlgs->bRemainTime = (BOOL)fs.GetNumber(L"RemainTime", FALSE);
			dlgs->bMaximized = (BOOL)fs.GetNumber(L"Maximized", FALSE);
			dlgs->bLoopEnabled = (BOOL)fs.GetNumber(L"Loop", FALSE);
			dlgs->bAutoLoop = (BOOL)fs.GetNumber(L"AutoLoop", FALSE);
			dlgs->bVolbars = (BOOL)fs.GetNumber(L"VolumeBars", TRUE);
			dlgs->bRedrawPanels = (BOOL)fs.GetNumber(L"RedrawPanels", TRUE);
			dlgs->bShowms = (BOOL)fs.GetNumber(L"Showms", TRUE);
			dlgs->bShowHours = (BOOL)fs.GetNumber(L"ShowHours", TRUE);
			dlgs->dwdlgpos = dlgs->dwolddlgpos = (DWORD)fs.GetNumber(L"Dlgpos", 0xffffffff);
			dlgs->volcurval = (int)fs.GetNumber(L"Volume", dlgs->volsliderlen / 2);
			dlgs->extplrdefactn = (int)fs.GetNumber(L"ExtPlayerDefAction", -1);
			dlgs->visiblelinesmask = (DWORD)fs.GetNumber(L"VisibleLinesMask", 0xff);

			fs.GetString(L"PosSliderChars", g_poschars, _countof(g_poschars) , g_poschars);
			fs.GetString(L"VolSliderChars", g_volchars, _countof(g_volchars), g_volchars);
			fs.GetString(L"ButtonsChars", g_buttonschars, _countof(g_buttonschars), g_buttonschars);
			fs.GetString(L"PanSliderChars", g_panchars, _countof(g_panchars), g_panchars);
			fs.GetString(L"ExtPlayerPath", g_plrpath, _countof(g_plrpath), L"");
			fs.GetString(L"ExtPlayerPlay", g_plrparms[0], _countof(g_plrparms[0]), L"");
			fs.GetString(L"ExtPlayerAddToPlaylist", g_plrparms[1], _countof(g_plrparms[1]), L"");
			fs.GetString(L"DescriptionFile", g_descrfile, _countof(g_descrfile), L"descript.ion");

			const WCHAR* s = fs.GetString(L"TagsTemplate", L"");
			if (wcslen(s) > 0) {
				WideCharToMultiByte(CP_ACP, 0, s, -1, g_tagtemplate, sizeof(g_tagtemplate), NULL, NULL);
			}

			fs.GetBinary(L"Colors", (BYTE*)&g_dlcolrs, sizeof(g_dlcolrs), (BYTE*)&g_dlcolrs, sizeof(g_dlcolrs));
		} else {
			CRegistry reg(PluginRootKey);
			dlgs->bDemo = reg.GetDword(_T("DemoMode"), FALSE);
			dlgs->bAutoNextOrLoopOn = reg.GetDword(_T("Autonext"), FALSE);
			dlgs->bRemainTime = reg.GetDword(_T("RemainTime"), FALSE);
			dlgs->bMaximized = reg.GetDword(_T("Maximized"), FALSE);
			dlgs->bLoopEnabled = reg.GetDword(_T("Loop"), FALSE);
			dlgs->bAutoLoop = reg.GetDword(_T("AutoLoop"), FALSE);
			dlgs->bVolbars = reg.GetDword(_T("VolumeBars"), TRUE);
			dlgs->bRedrawPanels = reg.GetDword(_T("RedrawPanels"), TRUE);
			dlgs->bShowms = reg.GetDword(_T("Showms"), TRUE);
			dlgs->bShowHours = reg.GetDword(_T("ShowHours"), TRUE);
			dlgs->dwdlgpos = dlgs->dwolddlgpos = reg.GetDword(_T("Dlgpos"), 0xffffffff);
			dlgs->volcurval = reg.GetDword(_T("Volume"), dlgs->volsliderlen / 2);
			dlgs->extplrdefactn = reg.GetDword(_T("ExtPlayerDefAction"), -1);
			dlgs->visiblelinesmask = reg.GetDword(_T("VisibleLinesMask"), 0xff);

			reg.GetString(_T("PosSliderChars"), g_poschars, sizeof(g_poschars), g_poschars);
			reg.GetString(_T("VolSliderChars"), g_volchars, sizeof(g_volchars), g_volchars);
			reg.GetString(_T("ButtonsChars"), g_buttonschars, sizeof(g_buttonschars), g_buttonschars);
			reg.GetString(_T("PanSliderChars"), g_panchars, sizeof(g_panchars), g_panchars);
			reg.GetString(_T("ExtPlayerPath"), g_plrpath, sizeof(g_plrpath), _T(""));
			reg.GetString(_T("ExtPlayerPlay"), g_plrparms[0], sizeof(g_plrparms[0]), _T(""));
			reg.GetString(_T("ExtPlayerAddToPlaylist"), g_plrparms[1], sizeof(g_plrparms[1]), _T(""));
			reg.GetString(_T("DescriptionFile"), g_descrfile, sizeof(g_descrfile), _T("descript.ion"));

			const TCHAR* s = reg.GetString(_T("TagsTemplate"), _T(""));
			if (_tcslen(s) > 0) {
				WideCharToMultiByte(CP_ACP, 0, s, -1, g_tagtemplate, sizeof(g_tagtemplate), NULL, NULL);
				delete[]s;
			}

			reg.GetBinary(_T("Colors"), (BYTE*)&g_dlcolrs, sizeof(g_dlcolrs), (BYTE*)&g_dlcolrs, sizeof(g_dlcolrs));

		}
	} catch (...) {
	}

	dlgs->dwdlgsize = MAKELONG(DLG_WIDTH, DLG_HEIGHT);
	dlgs->smallsize = dlgs->dlgsize;

	if (dlgs->volcurval > dlgs->volsliderlen - 2)
		dlgs->volcurval = dlgs->volsliderlen - 2;
	else
		if (dlgs->volcurval < 0)
			dlgs->volcurval = 0;
}

void SaveOptions(DLG_STATE* dlgs)
{
	assert(dlgs);

	dlgs->dlgsize.Y = 10;

	try {
		FarSettings fs(*PLUGIN_ID, &Info);

		fs.SetNumber(L"DemoMode", dlgs->bDemo);
		fs.SetNumber(L"Autonext", dlgs->bAutoNextOrLoopOn);
		fs.SetNumber(L"RemainTime", dlgs->bRemainTime);
		fs.SetNumber(L"Dlgpos", dlgs->dwdlgpos);
		fs.SetNumber(L"Volume", dlgs->volcurval);
		fs.SetNumber(L"Maximized", dlgs->bMaximized);
		fs.SetNumber(L"VolumeBars", dlgs->bVolbars);
		fs.SetNumber(L"ExtPlayerDefAction", dlgs->extplrdefactn);
		fs.SetNumber(L"Showms", dlgs->bShowms);
		fs.SetNumber(L"ShowHours", dlgs->bShowHours);
		fs.SetNumber(L"Loop", dlgs->bLoopEnabled);
		fs.SetNumber(L"RedrawPanels", dlgs->bRedrawPanels);
		fs.SetNumber(L"VisibleLinesMask", dlgs->visiblelinesmask);
		fs.SetNumber(L"AutoLoop", dlgs->bAutoLoop);
		fs.SetNumber(L"FirstRun", 0);
		fs.SetString(L"PosSliderChars", g_poschars);
		fs.SetString(L"VolSliderChars", g_volchars);
		fs.SetString(L"ButtonsChars", g_buttonschars);
		fs.SetString(L"PanSliderChars", g_panchars);
		fs.SetString(L"DescriptionFile", g_descrfile);
		if (g_plrpath[0])     fs.SetString(L"ExtPlayerPath", g_plrpath);
		if (g_plrparms[0][0]) fs.SetString(L"ExtPlayerPlay", g_plrparms[0]);
		if (g_plrparms[1][0]) fs.SetString(L"ExtPlayerAddToPlaylist", g_plrparms[1]);

		int l = strlen(g_tagtemplate) + 1;
		WCHAR* s = new WCHAR[l];
		MultiByteToWideChar(CP_ACP, 0, g_tagtemplate, -1, s, l);
		fs.SetString(L"TagsTemplate", s);
		delete[]s;

		fs.SetBinary(L"Colors", &g_dlcolrs, sizeof(g_dlcolrs));
	} catch (...) {
	}
}

void RunPlayerWithParams(const TCHAR* plrpath, const TCHAR* plrparm, const TCHAR* fname)
{
	TCHAR* params = NULL;
	assert(plrparm && g_plrparms && fname);

	size_t a = _tcslen(plrparm) + _tcslen(fname) + 4;

	if (plrpath[0] == 0 || plrparm[0] == 0) return;

	params = new TCHAR[a];

	_stprintf_s(params, a, plrparm, fname);

	TCHAR* e1 = new TCHAR[PATH_LEN], * e2 = new TCHAR[PATH_LEN];
	assert(e1 && e2);
	e1[0] = e2[0] = 0;
	ExpandEnvironmentStrings(plrpath, e1, PATH_LEN);
	ExpandEnvironmentStrings(params, e2, PATH_LEN);

	int res = (long)(LONG_PTR)ShellExecute(NULL, _T("open"), e1, e2, _T("."), SW_SHOWNOACTIVATE);
	if (res <= 32)
		ErrorMsg(MExtPlayerFail, res);
	delete[]params;
	delete[]e1;
	delete[]e2;
}

void ExtPlayerMenu(HANDLE hdlg)
{
	FARMENUITEM menuitems[6] = { 0 };
	SMALL_RECT ipos;
	TCHAR* tmp = new TCHAR[_countof(g_plrpath)];
	FINT BreakCode = -1, res = -1;
	FarKey BreakKeys[] = { {VK_SPACE},{VK_RBUTTON},{0} };

	Info.SendDlgMessage(hdlg, DM_GETITEMPOSITION, ID_EXTPLAYER, (FPAR2)&ipos);
	menuitems[0].TEXTPTR = g_extplraction[0];
	menuitems[1].TEXTPTR = g_extplraction[1];
	menuitems[2].TEXTPTR = NULL;
	menuitems[3].TEXTPTR = GetLocalizedMsg(MSetPlayerPath);
	menuitems[4].TEXTPTR = GetLocalizedMsg(MSetPlayParms);
	menuitems[5].TEXTPTR = GetLocalizedMsg(MSetAddParms);

	while (1) {
		menuitems[0].Flags |= (g_plrpath[0] == 0 || g_plrparms[0][0] == 0) ? MIF_GRAYED : 0;
		menuitems[1].Flags |= (g_plrpath[0] == 0 || g_plrparms[1][0] == 0) ? MIF_GRAYED : 0;
		menuitems[2].Flags = MIF_SEPARATOR;

		if (g_dlgstate.extplrdefactn >= 0)
			menuitems[g_dlgstate.extplrdefactn].Flags |= MIF_CHECKED;

		res = MENUID(ipos.Left + g_dlgstate.dlgpos.X, ipos.Top + g_dlgstate.dlgpos.Y - _countof(menuitems) - 3, 0,
					 FMENU_USEEXT | FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT, NULL, GetLocalizedMsg(MSpaceDefault), NULL,
					 BreakKeys, &BreakCode, (const struct FarMenuItem*)menuitems, _countof(menuitems));

		for (int i = 0; i < _countof(menuitems); i++) {
			menuitems[i].Flags = 0;
		}
		if (res >= 0) menuitems[res].Flags |= MIF_SELECTED;

		if (BreakCode >= 0) {
			if (g_plrpath[0] != 0)
				switch (res) {
					case 0:
					case 1:
						if (g_plrparms[res][0] == 0) break;

						if (g_dlgstate.extplrdefactn != res)
							g_dlgstate.extplrdefactn = (int)res;
						else
							g_dlgstate.extplrdefactn = -1;

						break;
					default:;
				}
			continue;
		} else {
			switch (res) {
				case 0:
				case 1:
					RunPlayerWithParams(g_plrpath, g_plrparms[res], g_ti.fname);
					break;
				case 3:
					if (INPUTBOXID(GetLocalizedMsg(MPlayerPath), GetLocalizedMsg(MPath), NULL, g_plrpath, tmp, _countof(g_plrpath), NULL, FIB_BUTTONS | FIB_ENABLEEMPTY))
						_tcscpy_s(g_plrpath, _countof(g_plrpath), tmp);  //ExpandEnvironmentStrings
					continue;
					break;
				case 4:
					if (INPUTBOXID(GetLocalizedMsg(MPlayParms), GetLocalizedMsg(MParms), NULL, g_plrparms[0], tmp, _countof(g_plrparms[0]), NULL, FIB_BUTTONS | FIB_ENABLEEMPTY))
						_tcscpy_s(g_plrparms[0], _countof(g_plrparms[0]), tmp);
					continue;
					break;
				case 5:
					if (INPUTBOXID(GetLocalizedMsg(MAddParms), GetLocalizedMsg(MParms), NULL, g_plrparms[1], tmp, _countof(g_plrparms[1]), NULL, FIB_BUTTONS | FIB_ENABLEEMPTY))
						_tcscpy_s(g_plrparms[1], _countof(g_plrparms[1]), tmp);
					continue;
					break;
				default:
					break;
			}
		}
		break;
	}

	delete[]tmp;

	if (g_dlgstate.extplrdefactn >= 0)
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_EXTPLAYER, (FPAR2)g_extplraction[g_dlgstate.extplrdefactn]);
	else
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_EXTPLAYER, (FPAR2)GetLocalizedMsg(MExtPlayer));
}

void ExtPlayerAction(HANDLE hdlg)
{
	switch (g_dlgstate.extplrdefactn) {
		case 0:
		case 1:
			RunPlayerWithParams(g_plrpath, g_plrparms[g_dlgstate.extplrdefactn], g_ti.fname);
			break;

		default:
			ExtPlayerMenu(hdlg);
	}
}
//////////////////////////////////////////////////////////////////////////
void ResizeDlg(HANDLE hdlg, COORD newdlgpos, COORD newdlgsize)
{
	SMALL_RECT sr;

	assert(newdlgpos.X >= -1 && newdlgpos.Y >= -1 && newdlgsize.X >= -1 && newdlgsize.Y >= -1);
	if (newdlgsize.X == g_dlgstate.smallsize.X && newdlgsize.X >= g_dlgstate.largesize.X) return;

	Info.SendDlgMessage(hdlg, DM_ENABLEREDRAW, FALSE, 0);

	Info.SendDlgMessage(hdlg, DM_RESIZEDIALOG, 0, (FPAR2)&newdlgsize);
	Info.SendDlgMessage(hdlg, DM_MOVEDIALOG, TRUE, (FPAR2)&newdlgpos);
	g_dlgstate.dlgsize = newdlgsize;
	if (!g_dlgstate.bMaximized) {
		g_dlgstate.olddlgpos = newdlgpos;
	}
	g_dlgstate.dlgpos = newdlgpos;
	g_dlgstate.largesize.Y = g_dlgstate.smallsize.Y = newdlgsize.Y;

	Info.SendDlgMessage(hdlg, DM_GETITEMPOSITION, ID_BOX, (FPAR2)&sr);
	sr.Right = newdlgsize.X - 1;
	sr.Bottom = newdlgsize.Y - 1;
	Info.SendDlgMessage(hdlg, DM_SETITEMPOSITION, ID_BOX, (FPAR2)&sr);

	Info.SendDlgMessage(hdlg, DM_GETITEMPOSITION, ID_MAXBUTN, (FPAR2)&sr);
	sr.Left = sr.Right = newdlgsize.X - 6;
	Info.SendDlgMessage(hdlg, DM_SETITEMPOSITION, ID_MAXBUTN, (FPAR2)&sr);

	Info.SendDlgMessage(hdlg, DM_GETITEMPOSITION, ID_ARROW, (FPAR2)&sr);
	sr.Left = sr.Right = newdlgsize.X - 2;
	Info.SendDlgMessage(hdlg, DM_SETITEMPOSITION, ID_ARROW, (FPAR2)&sr);

	QWORD pos = BASS_ChannelGetPosition(g_hsnd, BASS_POS_BYTE);

	g_dlgstate.possliderlen = g_dlgstate.dlgsize.X - TIME_LEN - 3;
	if (g_dlgstate.posslider) delete[]g_dlgstate.posslider;
	g_dlgstate.posslider = new TCHAR[g_dlgstate.possliderlen];
	assert(g_dlgstate.posslider);

	SET_POSVAL((double)pos / g_ti.lenbytes * (g_dlgstate.possliderlen - 3) + 1);

	MakePosLine(g_dlgstate.possliderlen, (int)g_dlgstate.poscurval, g_dlgstate.posslider);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_POS, (FPAR2)g_dlgstate.posslider);

	UpdateTrackInfo(hdlg, &g_ti);
	Info.SendDlgMessage(hdlg, DM_ENABLEREDRAW, TRUE, 0);
}

void MaximizeDlg(HANDLE hdlg, BOOL bMaximize)
{
	COORD dlgpos, dlgsize;

	SMALL_RECT Rect;
	Info.SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (FPAR2)&Rect);
	dlgpos.Y = Rect.Top;

	if (bMaximize) {
		g_dlgstate.olddlgpos.X = Rect.Left;
		dlgpos.X = 0;
		dlgsize = g_dlgstate.largesize;
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_MAXBUTN, (FPAR2)MIN_BUTTON);
	} else {
		dlgpos.X = g_dlgstate.olddlgpos.X;
		dlgsize = g_dlgstate.smallsize;
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_MAXBUTN, (FPAR2)MAX_BUTTON);
	}

	ResizeDlg(hdlg, dlgpos, dlgsize);
}

// fill matrix with dialog control's ids by lines
void InitDlgLines()
{
	for (int i = 0; i < _countof(g_dlgitems); i++) {
		if (g_dlgitems[i].Type != DI_DOUBLEBOX && g_dlgitems[i].Y1 > 0 && g_dlgitems[i].Y1 < DLG_HEIGHT - 1) {
			g_dlglines[g_dlgitems[i].Y1 - 1].push_back(i);
		}
	}
}

// Hide/Show dialog lines
void SetDlgVisibleLines(HANDLE hdlg, DWORD linesmask, BOOL bShowInvisible = FALSE)
{
	int hiddenlinesnum = 0;
	SMALL_RECT sr;

	Info.SendDlgMessage(hdlg, DM_SHOWDIALOG, FALSE, 0);

	for each (vector<int> v in g_dlglines) {
		if ((linesmask & 1) == 0) {
			for each (int id in v) {
				Info.SendDlgMessage(hdlg, DM_SHOWITEM, id, (FPAR2)bShowInvisible);
			}
			hiddenlinesnum++;
		} else {
			if (hiddenlinesnum > 0) {
				for each (int id in v) {
					Info.SendDlgMessage(hdlg, DM_GETITEMPOSITION, id, (FPAR2)&sr);
					sr.Top = sr.Bottom -= hiddenlinesnum * (bShowInvisible ? -1 : 1);
					sr.Right = sr.Left;
					Info.SendDlgMessage(hdlg, DM_SETITEMPOSITION, id, (FPAR2)&sr);
				}
			}
		}
		linesmask >>= 1;
	}

	COORD dlgpos, dlgsize;

	Info.SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (FPAR2)&sr);
	dlgpos.X = sr.Left;
	dlgpos.Y = sr.Top;

	dlgsize.X = sr.Right - sr.Left + 1;
	dlgsize.Y = sr.Bottom - sr.Top + 1 - (bShowInvisible ? -hiddenlinesnum : hiddenlinesnum);

	if (bShowInvisible) {
		g_dlgstate.olddlgpos.Y = dlgpos.Y;
		if (dlgpos.Y + dlgsize.Y > g_PanelHeight)
			dlgpos.Y = g_PanelHeight - dlgsize.Y;
	} else {
		dlgpos.Y = g_dlgstate.olddlgpos.Y;
	}

	ResizeDlg(hdlg, dlgpos, dlgsize);

	Info.SendDlgMessage(hdlg, DM_SHOWDIALOG, TRUE, 0);
}

BOOL Dlg_OnInit(HANDLE hdlg, FINT focused_id, FPAR2 param)
{
	LoadOptions(&g_dlgstate);

	PanelInfo pia, pip;
	F3STRUCTSIZE(pia);
	F3STRUCTSIZE(pip);
	Info._PControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (FPAR2)&pia);
	Info._PControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (FPAR2)&pip);
	g_dlgstate.largesize.X = (SHORT)(pia.PanelRect.right - pia.PanelRect.left) + (SHORT)(pip.PanelRect.right - pip.PanelRect.left) + 2;
	g_dlgstate.bIsRealFile = pia.Flags & PFLAGS_REALNAMES;
	g_dlgstate.bRestart = TRUE;
	g_dlgstate.bStop = TRUE;
	g_dlgstate.bPlay = FALSE;
	g_dlgstate.bInited = FALSE;
	g_dlgstate.bShowDesc = FALSE;

	SMALL_RECT sr;

	if (Info.AdvControl(PLUGIN_ID, ACTL_GETFARRECT, 0, &sr))
		g_PanelHeight = sr.Bottom - sr.Top + 1;
	else
		g_PanelHeight = pia.PanelRect.bottom - pia.PanelRect.top + 2;

	if (!g_dlgstate.bIsRealFile && (int)param != 1) {
		SendKeys();
		Info.SendDlgMessage(hdlg, DM_CLOSE, 0, 0);
		return FALSE;
	}

	FINT res = LoadAndPrepareTrack(hdlg, &g_ti, &g_hsnd);

	if (!g_hsnd) {
		if (!param) ErrorMsg(MBadFile, BASS_ErrorGetCode());
		Info.SendDlgMessage(hdlg, DM_CLOSE, 0, 0);
		return FALSE;
	}

	g_dlgstate.pancurval = g_dlgstate.pansliderlen / 2 - 1;
	SET_POSVAL(1.);

	assert(!g_dlgstate.volslider && !g_dlgstate.posslider && !g_dlgstate.panslider);
	g_dlgstate.volslider = new TCHAR[g_dlgstate.volsliderlen];
	g_dlgstate.panslider = new TCHAR[g_dlgstate.pansliderlen];
	g_dlgstate.vollevbars = new TCHAR[g_dlgstate.vollevbarslen];

	for (int i = 0; i < _countof(g_butns); i++) {
		_stprintf_s(g_butns[i], _countof(g_butns[0]), _T(" %c "), g_buttonschars[i]);
	}

	COORD consize = { sr.Right - sr.Left + 1,sr.Bottom - sr.Top + 1 };
	Info.SendDlgMessage(hdlg, DN_RESIZECONSOLE, 0, &consize);
	MaximizeDlg(hdlg, g_dlgstate.bMaximized);

	MakeVolLine(g_dlgstate.volsliderlen, g_dlgstate.volcurval, g_dlgstate.volslider);
	MakeBalanceLine(g_dlgstate.pansliderlen, g_dlgstate.pancurval, g_dlgstate.panslider);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_VOL, (FPAR2)g_dlgstate.volslider);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_BALANCE, (FPAR2)g_dlgstate.panslider);

	MakeVolBars(g_dlgstate.vollevbars, (int)g_dlgstate.vollevbarslen, 0, 0);
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_VOLBARS, (FPAR2)g_dlgstate.vollevbars);

	for (int i = 0; i < _countof(g_butns) - 1; i++) {
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_PREV + i, (FPAR2)g_butns[i]);
	}

	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_DEMO, (FPAR2)GetLocalizedMsg(MDemo));
	Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_AUTONEXT, (FPAR2)GetLocalizedMsg(MAutonext));

	g_extplraction[0] = _tcsdup(GetLocalizedMsg(MPlay));
	g_extplraction[1] = _tcsdup(GetLocalizedMsg(MAdd));

	assert(g_extplraction[0] && g_extplraction[1]);

	if (g_plrpath[0] && g_plrparms[0][0] && g_plrparms[1][0] && g_dlgstate.extplrdefactn >= 0)
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_EXTPLAYER, (FPAR2)g_extplraction[g_dlgstate.extplrdefactn]);
	else {
		Info.SendDlgMessage(hdlg, DM_SETTEXTPTR, ID_EXTPLAYER, (FPAR2)GetLocalizedMsg(MExtPlayer));
		g_dlgstate.extplrdefactn = -1;
	}

	SetDlgVisibleLines(hdlg, g_dlgstate.visiblelinesmask);

	Set_Vol(hdlg, g_hsnd);

	g_hExitEvent = CreateEvent(NULL, TRUE, FALSE, _T("Local\\RQPThread"));
	assert(g_hExitEvent);
	ResetEvent(g_hExitEvent);

	g_hMetaEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("Local\\RQPMetaChange"));

	g_hThread = CreateThread(NULL, 0, RefreshDlg, hdlg, 0, NULL);
	assert(g_hThread);

	Do_PlayPause(hdlg, g_hsnd);
	g_dlgstate.bInited = TRUE;
	return TRUE;
}

void Dlg_OnClose(HANDLE hdlg)
{
	DebugOut(_T("Dlg_OnClose<"));
	if (!g_dlgstate.bInited) return;

	DebugOut(_T(">Dlg_OnClose 1"));
	ClearTrackEndHandler(g_hsnd);
	g_dlgstate.bPlay = FALSE;
	g_dlgstate.bStop = TRUE;
	if (g_hsnd) {
		Stop_Sound(g_hsnd);
		Close_Sound(g_hsnd);
		g_hsnd = 0;
	}

	DebugOut(_T(">Dlg_OnClose 2"));
	assert(g_dlgstate.panslider && g_dlgstate.posslider && g_dlgstate.volslider && g_dlgstate.vollevbars);
	delete[]g_dlgstate.panslider;
	delete[]g_dlgstate.posslider;
	delete[]g_dlgstate.volslider;
	delete[]g_dlgstate.vollevbars;

	g_dlgstate.volslider = g_dlgstate.posslider = g_dlgstate.panslider = g_dlgstate.vollevbars = NULL;
	if (g_hExitEvent) {
		CloseHandle(g_hExitEvent);
		g_hExitEvent = NULL;
	}

	if (g_hMetaEvent) {
		CloseHandle(g_hMetaEvent);
		g_hMetaEvent = NULL;
	}

	DebugOut(_T(">Dlg_OnClose 3"));

	if (!g_dlgstate.bIsRealFile) { // clean up after plugin
		if (FileExist(g_ti.fname)) {
			DebugOut(_T(">Dlg_OnClose>Removing temp file %s"), g_ti.fname);
			DeleteFile(g_ti.fname);
			if (GetParentFolder(g_ti.fname)) RemoveDirectory(g_ti.fname);
		}
	}

	if (g_extplraction[0]) {
		free(g_extplraction[0]);
		g_extplraction[0] = NULL;
	}
	if (g_extplraction[1]) {
		free(g_extplraction[1]);
		g_extplraction[1] = NULL;
	}

	if (g_ti.fname) {
		free(g_ti.fname);
		g_ti.fname = NULL;
	}

	DebugOut(_T(">Dlg_OnClose"));
}

void ToggleTimeLook()
{
	if (g_dlgstate.bShowms && g_dlgstate.bShowHours) {
		g_dlgstate.bShowms = FALSE;
	} else {
		if (g_dlgstate.bShowms) {
			g_dlgstate.bShowHours = TRUE;
		} else {
			if (g_dlgstate.bShowHours) {
				g_dlgstate.bShowms = FALSE;
				g_dlgstate.bShowHours = FALSE;
			} else
				g_dlgstate.bShowms = TRUE;
		}
	}
}

BOOL Dlg_OnKey(HANDLE hdlg, FINT id, LONG_PTR keycode)
{
	double divider = 1, posstep = 1;


	switch (keycode) {
		CASE_VOL_UP
			g_dlgstate.volcurval++;
			if (g_dlgstate.volcurval > g_dlgstate.volsliderlen - 2) {
				g_dlgstate.volcurval = g_dlgstate.volsliderlen - 2;
			}
			Set_Vol(hdlg, g_hsnd);
		break;
		CASE_VOL_DOWN
			g_dlgstate.volcurval--;
			if (g_dlgstate.volcurval < 0) {
				g_dlgstate.volcurval = 0;
			}
			Set_Vol(hdlg, g_hsnd);
		break;
		CASE_PREV_POS_FINEST
			divider = 10;
		CASE_PREV_POS_FINE
			divider *= 10;
		CASE_PREV_POS
			posstep /= divider;
			SetFadeInOut(0);
		SET_POSVAL(g_dlgstate.poscurval - posstep);
			if (g_dlgstate.poscurval < 1) {
				SET_POSVAL(1);
			}
			Set_Pos(hdlg, g_hsnd);
			SetFadeInOut(g_fadecurval);
		break;
		CASE_NEXT_POS_FINEST
			divider = 10;
		CASE_NEXT_POS_FINE
			divider *= 10;
		CASE_NEXT_POS
			posstep /= divider;
		SET_POSVAL(g_dlgstate.poscurval + posstep);
			SetFadeInOut(0);
			if (g_dlgstate.poscurval >= g_dlgstate.possliderlen - 2) {
				SET_POSVAL(g_dlgstate.possliderlen - 3);
			}
			Set_Pos(hdlg, g_hsnd);
			SetFadeInOut(g_fadecurval);
		break;
		CASE_PAN_RIGHT
			g_dlgstate.pancurval++;
			if (g_dlgstate.pancurval >= g_dlgstate.pansliderlen - 2) {
				g_dlgstate.pancurval = g_dlgstate.pansliderlen - 3;
			}
			Set_Pan(hdlg, g_hsnd);
		break;
		CASE_PAN_LEFT
			g_dlgstate.pancurval--;
			if (g_dlgstate.pancurval < 1) {
				g_dlgstate.pancurval = 1;
			}
			Set_Pan(hdlg, g_hsnd);
		break;
		CASE_PREV_TRACK
			Do_Prev(hdlg, &g_ti, FALSE);
		break;

		CASE_PLAY_PAUSE
			Do_PlayPause(hdlg, g_hsnd);
		break;

		CASE_NEXT_TRACK
			Do_Next(hdlg, &g_ti, FALSE);
		break;

		CASE_STOP
			Do_Stop(hdlg, g_hsnd);
		break;

		CASE_TOGGLE_REMAIN_TIME
			g_dlgstate.bRemainTime = !g_dlgstate.bRemainTime;
		break;

		CASE_TOGGLE_TIME_LOOK
			ToggleTimeLook();
		break;

		CASE_DEMO
			g_dlgstate.bDemo = !g_dlgstate.bDemo;
			Set_Demo(hdlg, g_hsnd, &g_ti, g_demostarts[g_demostartidx]);
		break;

		CASE_CHANGE_DEMO_TIME
			NextDemoStart(hdlg);
		break;

		CASE_AUTO_NEXT
			g_dlgstate.bAutoNextOrLoopOn = !g_dlgstate.bAutoNextOrLoopOn;
			SetLoopMode(hdlg, g_hsnd);
		break;

		CASE_LOOP
			g_dlgstate.bLoopEnabled = !g_dlgstate.bLoopEnabled;
			SetLoopMode(hdlg, g_hsnd);
		break;

		CASE_EXT_PLAYER_ACTION
			ExtPlayerAction(hdlg);
		break;

		CASE_TOGGLE_VOLUMEBARS
			g_dlgstate.bVolbars = !g_dlgstate.bVolbars;
		break;

		CASE_ENTER_DESCRIPTION
			EnterDescription(&g_ti, &Info);
			if (g_dlgstate.bShowDesc)
				UpdateTrackInfo(hdlg, &g_ti);
		break;

		CASE_TOGGLE_DESCRIPTION
			g_dlgstate.bShowDesc = !g_dlgstate.bShowDesc;
			UpdateTrackInfo(hdlg, &g_ti);
		break;

		CASE_SELECT
			g_dlgstate.bSelectedTrack = !g_dlgstate.bSelectedTrack;
			SelectPanelItem(&Info, g_dlgstate.bSelectedTrack);
			if (g_dlgstate.bRedrawPanels) REDRAWALL;
		break;

		CASE_TOGGLE_MINIMODE
			if (LOBYTE(g_dlgstate.visiblelinesmask) != 0xff) {
				SMALL_RECT dlgr;
				Info.SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (FPAR2)&dlgr);
				SetDlgVisibleLines(hdlg, g_dlgstate.visiblelinesmask, dlgr.Bottom - dlgr.Top < 9);
			}
		break;

		CASE_TOGGLE_MAXIMIZE
			g_dlgstate.bMaximized = !g_dlgstate.bMaximized;
			MaximizeDlg(hdlg, g_dlgstate.bMaximized);
		break;
		default:
			return FALSE;
			break;
	}
	return TRUE;
}

BOOL Dlg_OnMouseClick(HANDLE hdlg, FINT id, MOUSE_EVENT_RECORD* mer)
{
	SMALL_RECT dlgr, ipos;

	g_dlgstate.bDragging = id == ID_BOX;

	Info.SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (FPAR2)&dlgr);
	Info.SendDlgMessage(hdlg, DM_GETITEMPOSITION, id, (FPAR2)&ipos);

	int ix = mer->dwMousePosition.X - dlgr.Left - ipos.Left;
	int iy = mer->dwMousePosition.Y - dlgr.Top - ipos.Top;

	switch (id) {
		case ID_POS:
			SET_POSVAL(ix <= 0 ? 1 : ix >= g_dlgstate.possliderlen - 2 ? g_dlgstate.possliderlen - 3 : ix);
			Set_Pos(hdlg, g_hsnd);
			break;
		case ID_VOL:
			g_dlgstate.volcurval = ix;
			Set_Vol(hdlg, g_hsnd);
			break;
		case ID_BALANCE:
			g_dlgstate.pancurval = ix <= 0 ? 1 : ix >= g_dlgstate.pansliderlen - 2 ? g_dlgstate.pansliderlen - 3 : ix;
			Set_Pan(hdlg, g_hsnd);
			break;
		case ID_TIME:
			if (mer->dwButtonState & RIGHTMOST_BUTTON_PRESSED)
				ToggleTimeLook();
			else
				g_dlgstate.bRemainTime = !g_dlgstate.bRemainTime;
			break;
		case ID_INFO_TAGS:
			if (mer->dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
				g_dlgstate.bShowDesc = !g_dlgstate.bShowDesc;
				UpdateTrackInfo(hdlg, &g_ti);
			} else
				UpdateTrackInfo(hdlg, &g_ti, !g_dlgstate.bScrolltags);
			break;
		case ID_MAXBUTN: {
			g_dlgstate.bMaximized = !g_dlgstate.bMaximized;

			MaximizeDlg(hdlg, g_dlgstate.bMaximized);
			break;
		}
		case ID_VOLBARS:
			g_dlgstate.bVolbars = !g_dlgstate.bVolbars;

			break;
		case ID_EXTPLAYER:
			if (mer->dwButtonState & RIGHTMOST_BUTTON_PRESSED)
				ExtPlayerMenu(hdlg);
			break;
		case ID_DEMO:
			if (mer->dwButtonState & RIGHTMOST_BUTTON_PRESSED)
				NextDemoStart(hdlg);
			break;
		case ID_AUTONEXT:
			if (mer->dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
				g_dlgstate.bLoopEnabled = !g_dlgstate.bLoopEnabled;
				SetLoopMode(hdlg, g_hsnd);
			}
			break;
		default:

			break;
	}
	return FALSE;
}

BOOL Dlg_OnControlInput(HANDLE hdlg, FINT id, INPUT_RECORD* ir)
{
	DebugOut(_T("Dlg_OnControlInput<"));

	DWORD modState = 0;
	WORD keycode = 0;
	KEY_EVENT_RECORD ker;
	MOUSE_EVENT_RECORD mer;

	switch (ir->EventType) {
		case KEY_EVENT:
			DebugOut(_T("Dlg_OnControlInput:KEY_EVENT"));
			ker = ir->Event.KeyEvent;
			if (ker.bKeyDown) {
				modState = (ker.dwControlKeyState & LEFT_CTRL_PRESSED || ker.dwControlKeyState & RIGHT_CTRL_PRESSED) ? KEY_CTRL : 0;
				modState += ker.dwControlKeyState & SHIFT_PRESSED ? KEY_SHIFT : 0;
				modState += (ker.dwControlKeyState & LEFT_ALT_PRESSED || ker.dwControlKeyState & RIGHT_ALT_PRESSED) ? KEY_ALT : 0;
				keycode = ker.wVirtualKeyCode;
			} else { // key depressed
			}
			break;
		case MOUSE_EVENT:
			DebugOut(_T("Dlg_OnControlInput:MOUSE_EVENT"));
			mer = ir->Event.MouseEvent;
			if (mer.dwEventFlags == MOUSE_WHEELED) {
				modState = (mer.dwControlKeyState & LEFT_CTRL_PRESSED || mer.dwControlKeyState & RIGHT_CTRL_PRESSED) ? KEY_CTRL : 0;
				modState += mer.dwControlKeyState & SHIFT_PRESSED ? KEY_SHIFT : 0;
				modState += (mer.dwControlKeyState & LEFT_ALT_PRESSED || mer.dwControlKeyState & RIGHT_ALT_PRESSED) ? KEY_ALT : 0;
				keycode = ((short)HIWORD(mer.dwButtonState) > 0) ? KEY_MSWHEEL_UP : KEY_MSWHEEL_DOWN;
			} else { // click
				Dlg_OnMouseClick(hdlg, id, &mer);
			}
			break;
	}
	if (keycode)
		return Dlg_OnKey(hdlg, id, keycode + modState);

	return FALSE;
}

BOOL Dlg_OnButtonClick(HANDLE hdlg, FINT id, FPAR2 state)
{
	switch (id) {
		case ID_PREV:
			Do_Prev(hdlg, &g_ti);
			break;
		case ID_PLAYPAUSE:
			Do_PlayPause(hdlg, g_hsnd);
			break;
		case ID_NEXT:
			Do_Next(hdlg, &g_ti);
			break;
		case ID_STOP:
			Do_Stop(hdlg, g_hsnd);
			break;
		case ID_DEMO:
			g_dlgstate.bDemo = !g_dlgstate.bDemo;
			Set_Demo(hdlg, g_hsnd, &g_ti, g_demostarts[g_demostartidx]);
			break;
		case ID_AUTONEXT:
			g_dlgstate.bAutoNextOrLoopOn = !g_dlgstate.bAutoNextOrLoopOn;
			SetLoopMode(hdlg, g_hsnd);
			break;
		case ID_EXTPLAYER:
			ExtPlayerAction(hdlg);
			break;
		default:
			assert(_T("Unknown button") && 0);
			break;
	}
	return TRUE;
}

DLGPROCRETVAL Dlg_OnCtlColorDlgItem(HANDLE hdlg, FINT id, FPAR2 par2)
{
	WORD loattr = 0, hiattr = 0;
	DLGPROCRETVAL res;
	FarDialogItemColors* fdic = (FarDialogItemColors*)par2;
	res = 0;

	switch (id) {
		case ID_PLAYPAUSE:
			if (g_dlgstate.bStop)
				res |= g_dlcolrs.butn;
			else
				res |= g_dlcolrs.hbutn;
			break;
		case ID_STOP:
			if (g_dlgstate.bStop)
				res |= g_dlcolrs.hbutn;
			else
				res |= g_dlcolrs.butn;
			break;
		case ID_DEMO:
			if (g_dlgstate.bDemo)
				res |= g_dlcolrs.hbutn;
			else
				res |= g_dlcolrs.butn;
			break;
		case ID_AUTONEXT:
			if (g_dlgstate.bAutoNextOrLoopOn)
				res |= g_dlcolrs.hbutn;
			else
				res |= g_dlcolrs.butn;
			break;
		case ID_INFO_TAGS:
			if (g_dlgstate.bSelectedTrack)
				res |= g_dlcolrs.htext;
			else
				res |= g_dlcolrs.text;
			break;
		case ID_VOLBARS:
			if (g_dlgstate.bVolbars)
				res |= g_dlcolrs.volbars;
			else
				res |= MKBYTE(g_dlcolrs.bg >> 4, g_dlcolrs.bg >> 4);
			break;
		case ID_VOL:
			res |= g_dlcolrs.volslider;
			break;
		case ID_BALANCE:
			res |= g_dlcolrs.panslider;
			break;
		case ID_POS:
			res |= g_dlcolrs.posslider;
			break;
		case ID_TIME:
			res |= g_dlcolrs.time;
			break;
		case ID_BOX:
			res = g_dlcolrs.boxtitle;
			fdic->Colors[2].ForegroundColor = LONIBBLE(g_dlcolrs.box);
			fdic->Colors[2].BackgroundColor = HINIBBLE(g_dlcolrs.box);
			break;
		case ID_DESCRMARK:
		case ID_INFO:
		case ID_ARROW:
		case ID_MAXBUTN:
			res |= g_dlcolrs.text;
			break;
		case ID_PREV:
		case ID_NEXT:
		case ID_EXTPLAYER:
			res |= g_dlcolrs.butn;
			break;

		default:
			res = FALSE;
			break;
	}
	if (res) {
		fdic->Flags |= FCF_4BITMASK;
		fdic->Colors[0].ForegroundColor = LONIBBLE(res);
		fdic->Colors[0].BackgroundColor = HINIBBLE(res);
	}

	return res;
}

DLGPROCRETVAL Dlg_OnCtlColorDialog(FPAR2 par2)
{
	FarColor* fc = (FarColor*)par2;
	fc->Flags |= FCF_4BITMASK;
	fc->ForegroundColor = LONIBBLE(g_dlcolrs.bg);
	fc->BackgroundColor = HINIBBLE(g_dlcolrs.bg);
	return TRUE;
}

DLGPROCRETVAL WINAPI dlgProc(HANDLE hdlg, FINT msg, FINT param1, FPAR2 param2)
{
	FINT id = param1;
	LONG_PTR p = 0;
	DLGPROCRETVAL res = 0;

	//DebugOut(_T("DlgProc>msg=%d"), msg);
	switch (msg) {
		case DN_INITDIALOG:
			res = Dlg_OnInit(hdlg, param1, param2);
			break;
		case DN_GETDIALOGINFO:
		{
			DialogInfo* di = (DialogInfo*)param2;
			di->StructSize = sizeof(DialogInfo);
			di->Id = g_dlguid;
		}
		res = TRUE;
		break;
		case DN_CONTROLINPUT:
			res = Dlg_OnControlInput(hdlg, param1, (INPUT_RECORD*)param2);
			break;
		case DN_BTNCLICK:
			res = Dlg_OnButtonClick(hdlg, param1, param2);
			break;
		case DN_CTLCOLORDLGITEM:
			res = Dlg_OnCtlColorDlgItem(hdlg, param1, param2);
			break;
		case DN_CTLCOLORDIALOG:
			res = Dlg_OnCtlColorDialog(param2);
			break;
			//  case DN_DRAGGED: // is it really needed?
			if (param1 == 0) {
				if (!g_dlgstate.bDragging) res = FALSE;
			} else {
				SMALL_RECT Rect;
				Info.SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (FPAR2)&Rect);
				g_dlgstate.dlgpos.X = Rect.Left;
				g_dlgstate.dlgpos.Y = Rect.Top;
			}
			break;
			//case DN_ENTERIDLE: //obsolete
			g_dlgstate.bDragging = TRUE;
			break;
		case DN_CLOSE:
			if (g_dlgstate.bInited) {
				SMALL_RECT Rect;
				Info.SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (FPAR2)&Rect);
				g_dlgstate.dlgpos.X = Rect.Left;
				g_dlgstate.dlgpos.Y = Rect.Top;
				SaveOptions(&g_dlgstate);
			}
			SetEvent(g_hExitEvent);
			res = TRUE;
			break;
		case DM_USER:
			switch (param1) {
				case 0:
					_RefreshDlg((HANDLE)param2);
					break;
				case 1:
					Do_Next(hdlg, &g_ti, FALSE);
					break;
				case 2:
					Do_Stop(hdlg, g_hsnd);
					break;
			}

			break;
		case DN_RESIZECONSOLE: {
			COORD* coord = (COORD*)param2;
			g_dlgstate.largesize.X = coord->X;

			if (g_dlgstate.smallsize.X > g_dlgstate.largesize.X) {
				g_dlgstate.largesize.X = g_dlgstate.smallsize.X;
			}

			COORD newpos = g_dlgstate.dlgpos, newsize = g_dlgstate.dlgsize;

			if (g_dlgstate.bMaximized) {
				newsize.X = g_dlgstate.largesize.X;
			} else {
				if (g_dlgstate.dlgpos.X > coord->X || g_dlgstate.dlgpos.Y > coord->Y) {
					newpos.X = coord->X - g_dlgstate.smallsize.X;
					newpos.Y = coord->Y - g_dlgstate.smallsize.Y;
				}
			}

			ResizeDlg(hdlg, newpos, newsize);

			res = TRUE;
			break;
		}
		default:
			res = Info.DefDlgProc(hdlg, msg, param1, (FPAR2)param2);
	}

	return res;
}

void ShowDialog(const PLUGIN_ID_TYPE pluginid, BOOL bSuppressOpenError)
{
#ifdef UNICODE
	g_hdlg = DIALOGINIT(pluginid, g_dlguid, g_dlgitems, bSuppressOpenError);

	assert(g_hdlg != INVALID_HANDLE_VALUE);
	FINT r = Info.DialogRun(g_hdlg);
	Info.DialogFree(g_hdlg);

	g_hdlg = NULL;

#else
	Info.DialogEx(pluginid, -1, -1, 70, 10, _T("Contents"), g_dlgitems, _countof(g_dlgitems), 0, FDLG_SMALLDIALOG, dlgProc, 0);
#endif
}

BOOL InitDialog()
{
	InitDlgLines();

	return TRUE;
}