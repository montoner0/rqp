// rqpf.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <assert.h>
#include <stdio.h>
#include <shlwapi.h>
#include "plugin.hpp"
#include "unidefs.h"
#include "rqp_bass.h"
#include "tags.h"
#include "rqpf.h"
#include "rqp_dlg.h"
#include "guids.h"
#include "helpers.h"
#include "keys.h"

#ifdef _MANAGED
#pragma managed(push, off)
#endif

VERSION g_ver;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	GetMyVersion(&g_ver, TRUE, hModule);
	return TRUE;
}

struct PluginStartupInfo Info;
static struct FarStandardFunctions FSF;

static struct PluginPanelItem* g_ppi = NULL;
TCHAR PluginRootKey[PATH_LEN];
TCHAR g_cmdline[PATH_LEN] = { 0 };
BOOL g_bSelfExecute = FALSE;

void ErrorMsg(const TCHAR* title, const TCHAR* button, const TCHAR* fmt, ...)
{
	va_list args;
	int     len;
	TCHAR* buffer;

	// retrieve the variable arguments
	va_start(args, fmt);
	len = _vsctprintf(fmt, args) + 1; // _vscprintf doesn't count terminating '\0'
	buffer = new TCHAR[len * sizeof(TCHAR)];
	_vstprintf_s(buffer, len * sizeof(TCHAR), fmt, args);

	size_t msglen = _tcslen(title) + _tcslen(buffer) + _tcslen(button) + 3 + 3 + 1;
	TCHAR* msg = new TCHAR[msglen];
	_stprintf_s(msg, msglen, _T("%s\n\n%s\n\n\x01\n%s"), title, buffer, button);
	Info.Message(PLUGIN_ID, &g_miscguid, FMSG_WARNING | FMSG_LEFTALIGN | FMSG_ALLINONE, _T("Contents"), (TCHAR**)msg, 0, 1);
	delete[]msg;
	delete[]buffer;
}

const TCHAR* GetLocalizedMsg(int MsgId)
{
	const TCHAR* msg = Info.GetMsg(PLUGIN_ID, MsgId);
	if (!msg)
		ErrorMsg(_T("Error"), _T("OK"), _T("Localized string %d not found"), MsgId);
	return msg;
}

//////////////////////////////////////////////////////////////////////////
void ErrorMsg(LOC_MESSAGES err, int code)
{
	ErrorMsg(GetLocalizedMsg(MTitle), GetLocalizedMsg(MButOk), _T("%s\n%d"), GetLocalizedMsg(err), code);
}

void WINAPI GetGlobalInfoW(struct GlobalInfo* GInfo)
{
	GInfo->StructSize = sizeof(struct GlobalInfo);
	GInfo->MinFarVersion = MAKEFARVERSION(3, 0, 0, 4400, VS_RELEASE); //FARMANAGERVERSION
	GInfo->Version = MAKEFARVERSION(g_ver.major, g_ver.minor, g_ver.build, g_ver.revision, VS_RC);
	GInfo->Guid = g_plguid;
	GInfo->Title = _T("RQP");
	GInfo->Description = _T("Simple audio player for Far Manager");
	GInfo->Author = _T("montonero");
}

void WINAPI _SetStartupInfo(struct PluginStartupInfo* psInfo)
{
	int res;
	TCHAR* s = NULL, * c = NULL;
	TCHAR plugdir[] = _T("bassplugs");

	Info = *psInfo;
	FSF = *psInfo->FSF;
	Info.FSF = &FSF; // скорректируем адрес в локальной структуре

	s = _tcsdup(Info.ModuleName);
	assert(s);
	c = _tcsrchr(s, _T('\\'));
	if (c)
		*c = 0;
	c = (TCHAR*)malloc((_tcslen(s) + _countof(plugdir) + 2) * sizeof(TCHAR));
	assert(c);
	_stprintf_s(c, _tcslen(s) + _countof(plugdir) + 2, _T("%s\\%s"), s, plugdir);

	res = Init_Sound(c);
	if (res != BASS_OK && res != BASS_ERROR_ALREADY) {
		ErrorMsg(MBASSFailed, res);
	}
	free(s);
	free(c);

	InitializeCriticalSection(&g_refreshcrits);

	InitDialog();

	_stprintf_s(PluginRootKey, _countof(PluginRootKey), _T("%s\\RQP"), _T("Software\\far2\\plugins"));
}

void WINAPI _GetPluginInfo(struct PluginInfo* PInfo)
{
	PInfo->StructSize = sizeof(struct PluginInfo);
	PInfo->Flags = 0;

	static TCHAR* PluginMenuStrings[1];
	PluginMenuStrings[0] = (TCHAR*)GetLocalizedMsg(MTitle);

	PInfo->PluginMenu.Guids = &g_menuguid;
	PInfo->PluginMenu.Strings = PluginMenuStrings;
	PInfo->PluginMenu.Count = sizeof(PluginMenuStrings) / sizeof(PluginMenuStrings[0]);

	PInfo->CommandPrefix = _T("rqp");
}

HANDLE WINAPI AnalyseW(const struct AnalyseInfo* AInfo)
{
	assert(AInfo->StructSize >= sizeof(AnalyseInfo));
	const TCHAR* pFileName = AInfo->FileName;

	if (g_bSelfExecute) {
		g_bSelfExecute = FALSE;
		_tcscpy_s(g_cmdline, _countof(g_cmdline), pFileName);

		ShowDialog(PLUGIN_ID);
	}
	return NULL;
}

HANDLE WINAPI _OpenPlugin(const struct OpenInfo* OInfo)
{
	g_bSelfExecute = FALSE;
	g_cmdline[0] = 0;
	if (OInfo->StructSize >= sizeof(OpenInfo)) {
		switch (OInfo->OpenFrom) {
			case OPEN_COMMANDLINE:
				_tcscpy_s(g_cmdline, _countof(g_cmdline), (TCHAR*)(((struct OpenCommandLineInfo*)OInfo->Data)->CommandLine));
				break;
		}

		ShowDialog(PLUGIN_ID, FALSE);
	}

	return NULL;
}

FINT WINAPI _Configure(CONFIGUREPARMS)
{
	return(FALSE);
}

void WINAPI _ExitFAR(EXITFARPARMS)
{
	DeleteCriticalSection(&g_refreshcrits);
	Done_Sound();
}

//////////////////////////////////////////////////////////////////////////
BOOL IsSelected(struct PluginStartupInfo* Info, int i/*=-1*/)
{
	FINT res = 0;

	assert(Info);
	if (i < 0) {
		res = Info->_PControl(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, NULL);
	} else {
		res = Info->_PControl(PANEL_ACTIVE, FCTL_GETPANELITEM, i, NULL);
	}

	PluginPanelItem* ppi;

	ppi = (PluginPanelItem*)calloc(1, res);
	if (ppi) {
		;
		INITPANELITEMSTRUCT(ppi, res, pnlitem);
		if (i < 0) {
			Info->_PControl(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, pnlitem);
		} else {
			Info->_PControl(PANEL_ACTIVE, FCTL_GETPANELITEM, i, pnlitem);
		}

		res = ppi->Flags & PPIF_SELECTED;
		free(ppi);
	}

	return (BOOL)res;
}

void SelectPanelItem(struct PluginStartupInfo* Info, BOOL bSelect/*=TRUE*/, FINT i/*=-1*/)
{
	assert(Info);
	if (i < 0) {
		struct PanelInfo pi;
		F3STRUCTSIZE(pi);
		if (Info->_PControl(PANEL_ACTIVE, FCTL_GETPANELINFO, sizeof(pi), (FPAR2)&pi)) {
			i = pi.CurrentItem;
		} else {
			return;
		}
	}
	Info->_PControl(PANEL_ACTIVE, FCTL_BEGINSELECTION, 0, 0);
	Info->_PControl(PANEL_ACTIVE, FCTL_SETSELECTION, i, (FPAR2)(bSelect ? 1 : 0));
	Info->_PControl(PANEL_ACTIVE, FCTL_ENDSELECTION, 0, 0);

	Info->_PControl(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, NULL);
}
// returns 1 if ok, -1 if out of items, 0 if fail
FINT GetPanelItem(struct PluginStartupInfo* Info, PluginPanelItem * *ppi, int delta /*=0*/)
{
	struct PanelInfo pi;
	FINT bres = 0;
	FSIZET itemidx;
	F3STRUCTSIZE(pi);

	assert(Info);
	if (Info->_PControl(PANEL_ACTIVE, FCTL_GETPANELINFO, sizeof(pi), (FPAR2)&pi)) {
		if (pi.PanelType == PTYPE_FILEPANEL && pi.ItemsNumber > 0 && pi.CurrentItem >= 0) {
			delta = (delta > 0) - (delta < 0);   // get 1 if delta >0 and -1 if delta <0 (signum)

			itemidx = pi.CurrentItem + delta;
			if (itemidx >= pi.ItemsNumber || itemidx < 0) {
				return -1;
			}

			FINT res = Info->_PControl(PANEL_ACTIVE, FCTL_GETPANELITEM, itemidx, NULL);
			*ppi = (PluginPanelItem*)calloc(1, res);
			if (*ppi) {
				INITPANELITEMSTRUCT(*ppi, res, pnlitem);
				if (Info->_PControl(PANEL_ACTIVE, FCTL_GETPANELITEM, itemidx, pnlitem)) {
					bres = 1;
					// set cursor to newitem
					PanelRedrawInfo pri;
					F3STRUCTSIZE(pri);
					pri.CurrentItem = itemidx;
					pri.TopPanelItem = pi.TopPanelItem;
					Info->_PControl(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (FPAR2)&pri);
				}
				//free(ppi); no free requires as we return the pointer
			}
		}
	}
	return bres;
}

void GetPanelDir(TCHAR * buf, FINT bufsize)
{
	FINT size = Info._PControl(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, NULL);
	if (size > bufsize) realloc(buf, size);
	F3STRUCTSIZE(*(FarPanelDirectory*)buf);
	Info._PControl(PANEL_ACTIVE, FCTL_GETPANELDIR, PATH_LEN, (FPAR2)buf);
	memmove_s(buf, size, ((FarPanelDirectory*)buf)->Name, (_tcslen(((FarPanelDirectory*)buf)->Name) + 1) * sizeof(TCHAR));
}

BOOL SendKeys()
{
	DebugOut(_T("SendKeys>SendCtrlPgDn"));
	MacroSendMacroText msmt = { sizeof(msmt),KMFLAGS_SILENTCHECK,{0},_T("Keys \"CtrlPgDn\"") };
	if (Info.MacroControl(PLUGIN_ID, MCTL_SENDSTRING, MSSC_POST, &msmt)) {
		g_bSelfExecute = TRUE;
		DebugOut(_T("SendKeys>SentCtrlPgDn>Success"));
		return TRUE;
	}
	return FALSE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
