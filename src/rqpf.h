#pragma once
#include "stdafx.h"
#include "plugin.hpp"
#include "unidefs.h"

#define ROUND_DIV(x,y,r) ( (int) ( ( (double)(x)/(double)(y) ) * (int)(r)  +.5 ) / (double)(r) )

#define MAX_PATH_LENGTH       32768

#define PATH_LEN (MAX_PATH_LENGTH + MAX_PATH + 1)

typedef enum
{
	MTitle,
	MEmptyLine,
	MBASSFailed,
	MExtPlayerFail,
	MExtPlayer,
	MPlayParms,
	MAddParms,
	MParms,
	MPath,
	MPlayerPath,
	MSpaceDefault,
	MSetAddParms,
	MSetPlayParms,
	MSetPlayerPath,
	MPlay,
	MAdd,
	MDemo,
	MAutonext,
	MLoop,
	MButOk,
	MBadFile,
	MEnterDescr,
	MDescUpdateFail,
	MSearching
} LOC_MESSAGES;

FINT GetPanelItem(struct PluginStartupInfo* Info, PluginPanelItem** ppi, int delta = 0);
void GetPanelDir(TCHAR* buf, FINT bufsize);
void SelectPanelItem(struct PluginStartupInfo* Info, BOOL bSelect = TRUE, FINT i = -1);
BOOL IsSelected(struct PluginStartupInfo* Info, int i = -1);
void ErrorMsg(LOC_MESSAGES err, int code);
const TCHAR* GetLocalizedMsg(int MsgId);
BOOL SendKeys(/*HANDLE hdlg*/);