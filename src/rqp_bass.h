#pragma once
#include <Windows.h>
#include "rqpf.h"
#include "bass.h"

typedef enum
{
	SOUND_UNK,
	SOUND_FILE,
	SOUND_URL,
	SOUND_MOD
} SOUND_TYPE;

#define DESCR_LEN 2048

typedef struct _TRACK_INFO
{
	QWORD lenbytes;
	double lensecs;
	TCHAR info[100];
	TCHAR* tags;
	int tagslen;
	TCHAR* fname;
	TCHAR descr[DESCR_LEN];
	BOOL bLoop;
	SOUND_TYPE fType;
} TRACK_INFO;

extern TCHAR g_descrfile[PATH_LEN];

int Init_Sound(TCHAR* plugsdir);
HSTREAM Open_Sound(TCHAR* fileurl, SOUND_TYPE* fType);
BOOL Close_Sound(HSTREAM hstr);
HSTREAM LoadSound(TRACK_INFO* ti);
int Play_Sound(HSTREAM chan, BOOL bRestart = FALSE, int Fadeinms = -1);
int Pause_Sound(HSTREAM chan, int Fadeoutms = -1);
int Stop_Sound(HSTREAM chan, int Fadeoutms = -1);
int PlaySoundFromPercent(HSTREAM chan, double frompcent, BOOL bRestart = FALSE, int Fadeinms = -1);
int PlaySoundFromTime(HSTREAM chan, double fromtime, BOOL bRestart = FALSE, int Fadeinms = -1);
int SetSoundVol(HSTREAM chan, double vol);
int SetSoundPan(HSTREAM chan, double pan);
int SetSoundPosTime(HSTREAM chan, double time);
int SetSoundPosPercent(HSTREAM chan, double pcent);
void GetSoundTimeStr(HSTREAM chan, TCHAR* str, int strsize);
void GetSoundPosStr(HSTREAM chan, TCHAR* str, int strsize, BOOL bRemainTime/*=FALSE*/, BOOL bShowms, BOOL bShowHours);
void SetFadeInOut(int fade);
void SetTagsFormat(const char TagsFmt[]);
int Done_Sound();
