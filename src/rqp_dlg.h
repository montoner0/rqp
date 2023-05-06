#pragma once
#include "stdafx.h"

//#define POSSLIDER_LEN 56
#define VOLSLIDER_LEN 16
#define PANSLIDER_LEN 14
#define VOLBARS_LEN 14
#define TIME_LEN 14

#define DLG_WIDTH  72
#define DLG_HEIGHT 10

#define FADE_DEF_VAL 200

struct DLG_STATE
{
	TCHAR* posslider;
	TCHAR* volslider;
	TCHAR* panslider;
	TCHAR* vollevbars;
	int possliderlen;
	int volsliderlen;
	int pansliderlen;
	int vollevbarslen;
	double poscurval;
	//   double posstep;
	int volcurval;
	int pancurval;
	BOOL bPlay;
	BOOL bStop;
	BOOL bDemo;
	BOOL bAutoNextOrLoopOn;
	BOOL bDragging;
	BOOL bRestart;
	BOOL bRemainTime;
	BOOL bScrolltags;
	BOOL bSelectedTrack;
	BOOL bMaximized;
	BOOL bVolbars;
	BOOL bRedrawPanels;
	BOOL bInited;
	BOOL bIsRealFile;
	BOOL bShowms;
	BOOL bShowHours;
	BOOL bShowDesc;
	BOOL bLoopEnabled;
	BOOL bAutoLoop;
	union
	{
		DWORD dwdlgpos;
		COORD dlgpos;
	};
	union
	{
		DWORD dwolddlgpos;
		COORD olddlgpos;
	};
	COORD smallsize;
	COORD largesize;
	union
	{
		COORD dlgsize;
		DWORD dwdlgsize;
	};
	int extplrdefactn; // Default action for an external player
	DWORD visiblelinesmask;
};

extern CRITICAL_SECTION g_refreshcrits;

void ShowDialog(const PLUGIN_ID_TYPE pluginid, BOOL bSuppressOpenError = TRUE);
BOOL InitDialog();
