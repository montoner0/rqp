#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>
#include <io.h>
#include <map>

#include "bass.h"
#include "rqp_bass.h"
#include "tags.h"
#include "helpers.h"
#include "rqpf.h"
#include "unidefs.h"
#include "descriptions.h"

#ifdef _UNICODE
#define _BASS_UNICODE BASS_UNICODE
#else
#define _BASS_UNICODE 0
#endif

#define ROUND_DIV(x,y,r) ( (int) ( ( (double)(x)/(double)(y) ) * (int)(r)  +.5 ) / (double)(r) )

#define MAX_HPLUGS 150
static HPLUGIN g_hplugs[MAX_HPLUGS];
static DWORD g_plugsloaded = 0;

static float g_vollevel = 1.;
static float g_panning = 0.;
static int g_Fade = 200;
TCHAR g_descrfile[PATH_LEN] = _T("descript.ion");
static HSYNC g_metasync = NULL;
static char* g_tagsfmt = NULL;
static HANDLE g_hMetaEvent = NULL;

typedef struct
{
	union
	{
		HMUSIC  hmus;
		HSTREAM hstr;
	};
	SOUND_TYPE type;
} HANDLES;

#define MAX_HANDLES 50
static HANDLES hndls[MAX_HANDLES] = { 0 };

const std::map<int, char*> CTypes = {
	{ BASS_CTYPE_STREAM_OGG,       "Ogg Vorbis" },
	{ BASS_CTYPE_STREAM_MP1,       "MPEG layer 1" },
	{ BASS_CTYPE_STREAM_MP2,       "MPEG layer 2" },
	{ BASS_CTYPE_STREAM_MP3,       "MPEG layer 3" },
	{ BASS_CTYPE_STREAM_AIFF,      "Audio IFF" },
	{ BASS_CTYPE_STREAM_CA,        "CoreAudio codec" },
	{ BASS_CTYPE_STREAM_WAV_FLOAT, "WAVE" },
	{ BASS_CTYPE_STREAM_WAV_PCM,   "WAVE" },
	{ BASS_CTYPE_MUSIC_MOD,        "Generic MOD" },
	{ BASS_CTYPE_MUSIC_MTM,        "MultiTracker" },
	{ BASS_CTYPE_MUSIC_S3M,        "ScreamTracker 3" },
	{ BASS_CTYPE_MUSIC_XM,         "FastTracker 2" },
	{ BASS_CTYPE_MUSIC_IT,         "Impulse Tracker" },
	{ BASS_CTYPE_STREAM_MF,        "Media Foundation codec" },
	{ BASS_CTYPE_STREAM_AM,		   "Android media codec"},
};

const char* GetCType(BASS_CHANNELINFO ci)
{
	if (ci.plugin) {
		const BASS_PLUGININFO* info = BASS_PluginGetInfo(ci.plugin); // get the plugin info
		for (size_t i = 0; i < info->formatc; i++) { // display the array of formats...
			if (ci.ctype == info->formats[i].ctype)
				return info->formats[i].name;
		}
	} else {
		auto f = CTypes.find(ci.ctype);
		if (f == CTypes.end()) {
			if (ci.ctype & BASS_CTYPE_STREAM_WAV)
				return "WAVE";
			else
				if (ci.ctype & BASS_CTYPE_MUSIC_MO3)
					return "MO3";
		} else {
			return f->second;
		}
	}

	return "N/A";
}

//////////////////////////////////////////////////////////////////////////
static void LoadPlugins(TCHAR* PlugsDir)
{
	WIN32_FIND_DATA FileData;
	TCHAR pdir[PATH_LEN], * c = NULL;
	HANDLE hSearch;
	DWORD LastErr = 0, i = 0, j;
	BOOL res = FALSE;
	HPLUGIN hplug;

	assert(PlugsDir && PlugsDir[0]);

	g_plugsloaded = 0;

	_tcscpy_s(pdir, _countof(pdir), PlugsDir);
	c = &pdir[_tcslen(pdir)];
	c[0] = _T('\\');
	c++;
	c[0] = 0;

	_tcscat_s(pdir, _countof(pdir), _T("*.dll"));

	hSearch = FindFirstFile(pdir, &FileData);
	if (hSearch == INVALID_HANDLE_VALUE) {
		LastErr = GetLastError();
		return;
	}

	do {
		if (!((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))) {
			c[0] = 0;
			_tcscat_s(pdir, _countof(pdir), FileData.cFileName);
			if (hplug = BASS_PluginLoad((char*)pdir, _BASS_UNICODE)) {
				g_hplugs[i++] = hplug;
			} else {
				j = BASS_ErrorGetCode();
			}
		}

		if (!FindNextFile(hSearch, &FileData))
			LastErr = GetLastError();
	} while (LastErr == 0 && i < MAX_HPLUGS);

	g_plugsloaded = i;

	FindClose(hSearch);
}
//////////////////////////////////////////////////////////////////////////
static void UnloadPlugins()
{
	for (UINT i = 0; i < g_plugsloaded; i++) {
		BASS_PluginFree(g_hplugs[i]);
	}
}
//////////////////////////////////////////////////////////////////////////
int Init_Sound(TCHAR* plugsdir)
{
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		return BASS_ERROR_VERSION;
	}

	BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT, TRUE);
	BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1);
	// setup output - default device
	if (!BASS_Init(-1, 48000, 0, 0, NULL))
		return BASS_ErrorGetCode();
	LoadPlugins(plugsdir);
	for (int i = 0; i < MAX_HANDLES; i++) {
		hndls[i].hmus = 0;
		hndls[i].type = SOUND_UNK;
	}

	g_hMetaEvent = CreateEvent(NULL, TRUE, FALSE, _T("Local\\RQPMetaChange"));
	assert(g_hMetaEvent);
	ResetEvent(g_hMetaEvent);

	return BASS_OK;
}
//////////////////////////////////////////////////////////////////////////
int Done_Sound()
{
	UnloadPlugins();

	BASS_Free();

	if (g_hMetaEvent) {
		CloseHandle(g_hMetaEvent);
		g_hMetaEvent = NULL;
	}

	return BASS_OK;
}
//////////////////////////////////////////////////////////////////////////
static int AddHandle(HSTREAM hnd, SOUND_TYPE type)
{
	UINT i = 0;
	while (i < MAX_HANDLES && hndls[i].hmus) i++;

	if (i >= MAX_HANDLES) return -1;

	switch (type) {
		case SOUND_URL:
		case SOUND_FILE:
			hndls[i].hstr = (HSTREAM)hnd;
			break;
		case SOUND_MOD:
			hndls[i].hmus = (HMUSIC)hnd;
			break;
		default:
			return -2;
			break;
	}

	hndls[i].type = type;

	return i;
}
//////////////////////////////////////////////////////////////////////////
static int FreeHandle(HSTREAM hnd)
{
	UINT i = 0;
	while (i < MAX_HANDLES && hndls[i].hmus != (HMUSIC)hnd) i++;

	if (i >= MAX_HANDLES) return -1;

	switch (hndls[i].type) {
		case SOUND_URL:
		case SOUND_FILE:
			BASS_StreamFree(hndls[i].hstr);
			hndls[i].hstr = NULL;
			break;
		case SOUND_MOD:
			BASS_MusicFree(hndls[i].hmus);
			hndls[i].hmus = NULL;
			break;
		default:
			assert(_T("Somethin' f*cked up") && 0);
			break;
	}
	hndls[i].type = SOUND_UNK;

	return i;
}
//////////////////////////////////////////////////////////////////////////
HSTREAM Open_Sound(TCHAR* fileurl, SOUND_TYPE* fType)
{
	HSTREAM chan;
	assert(fileurl && fileurl[0]);

	// try loading the MOD
	if ((chan = BASS_MusicLoad(FALSE, (char*)fileurl, 0, 0, BASS_MUSIC_STOPBACK | BASS_MUSIC_POSRESETEX | BASS_MUSIC_RAMPS | BASS_MUSIC_PRESCAN | _BASS_UNICODE, 0))) {
		*fType = SOUND_MOD;
	} else
		// try streaming the file/url
		if ((chan = BASS_StreamCreateFile(FALSE, (char*)fileurl, 0, 0, _BASS_UNICODE))) {
			if (BASS_StreamGetFilePosition(chan, BASS_FILEPOS_DOWNLOAD) != -1) {
				// streaming from the internet
				*fType = SOUND_URL;
			} else
				*fType = SOUND_FILE;
		} else {
			if ((chan = BASS_StreamCreateURL((char*)fileurl, 0, BASS_STREAM_RESTRATE | _BASS_UNICODE, NULL, NULL))) {
				*fType = SOUND_URL;
			} else
				return 0;
		}

	int res = AddHandle(chan, *fType);
	assert(res >= 0);

	return chan;
}
//////////////////////////////////////////////////////////////////////////
BOOL Close_Sound(HSTREAM hstr)
{
	return FreeHandle(hstr) >= 0;
}
//////////////////////////////////////////////////////////////////////////
int Play_Sound(HSTREAM chan, BOOL bRestart/*=FALSE*/, int Fadeinms/*=-1*/)
{
	assert(chan);

	if (Fadeinms < 0)
		Fadeinms = g_Fade;

	if (Fadeinms > 0) {
		BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, 0);
	} else {
		BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, g_vollevel);
	}

	int res = BASS_OK;

	if (BASS_ChannelPlay(chan, bRestart)) {
		DebugOut(_T("Play_Sound>BASS_ChannelPlay"));
		if (Fadeinms > 0) {
			DebugOut(_T("Play_Sound>BASS_ChannelSlideAttribute< vollevel=%f fadein=%d issliding=%d"), g_vollevel, Fadeinms, BASS_ChannelIsSliding(chan, BASS_ATTRIB_VOL));
			if (!BASS_ChannelSlideAttribute(chan, BASS_ATTRIB_VOL, g_vollevel, Fadeinms))
				assert(BASS_ErrorGetCode());
			DebugOut(_T(">Play_Sound>BASS_ChannelSlideAttribute"));
		}
	} else
		res = BASS_ErrorGetCode();

	return res;
}
//////////////////////////////////////////////////////////////////////////
int Pause_Sound(HSTREAM chan, int Fadeoutms/*=-1*/)
{
	assert(chan);

	DebugOut(_T("Pause_Sound<"));
	if (Fadeoutms < 0)
		Fadeoutms = g_Fade;

	if (Fadeoutms > 0) {
		DebugOut(_T("Pause_Sound>BASS_ChannelSlideAttribute< fadeout=%d issliding=%d"), Fadeoutms, BASS_ChannelIsSliding(chan, BASS_ATTRIB_VOL));
		if (!BASS_ChannelSlideAttribute(chan, BASS_ATTRIB_VOL, 0, Fadeoutms))
			assert(BASS_ErrorGetCode());
		// wait for slide to finish
		int i = 0;
		while (BASS_ChannelIsSliding(chan, BASS_ATTRIB_VOL) && i++ < Fadeoutms) Sleep(1);
		DebugOut(_T(">Pause_Sound>BASS_ChannelSlideAttribute %d"), i);
	}
	BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, 0);

	int res = BASS_OK;
	if (!BASS_ChannelPause(chan))
		res = BASS_ErrorGetCode();

	DebugOut(_T(">Pause_Sound"));
	return res;
}
//////////////////////////////////////////////////////////////////////////
int Stop_Sound(HSTREAM chan, int Fadeoutms/*=-1*/)
{
	int res = BASS_OK;

	assert(chan);
	DebugOut(_T("Stop_Sound<"));
	if (Fadeoutms < 0)
		Fadeoutms = g_Fade;

	if (BASS_ChannelIsActive(chan)) {
		DebugOut(_T("Stop_Sound>Pause_Sound"));
		Pause_Sound(chan, Fadeoutms);
	}

	if (!BASS_ChannelStop(chan))
		res = BASS_ErrorGetCode();

	DebugOut(_T(">Stop_Sound"));
	return res;
}
//////////////////////////////////////////////////////////////////////////
static int PlaySoundFromByte(HSTREAM chan, QWORD frompos, BOOL bRestart/*=FALSE*/, int Fadeinms/*=-1*/)
{
	int res;

	assert(chan);

	if (frompos < 0)
		return BASS_ERROR_POSITION;

	if (Fadeinms < 0)
		Fadeinms = g_Fade;

	if (Fadeinms > 0 && BASS_ChannelIsActive(chan)) {
		DebugOut(_T("PlaySoundFromByte>Pause_Sound"));
		if ((res = Pause_Sound(chan, Fadeinms)) != BASS_OK)
			return res;
	}

	if (!BASS_ChannelSetPosition(chan, frompos, BASS_POS_BYTE))
		return BASS_ErrorGetCode();

	res = Play_Sound(chan, bRestart, Fadeinms);

	return res;
}
//////////////////////////////////////////////////////////////////////////
int PlaySoundFromPercent(HSTREAM chan, double frompos, BOOL bRestart/*=FALSE*/, int Fadeinms/*=-1*/)
{
	QWORD len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);

	if (Fadeinms < 0)
		Fadeinms = g_Fade;

	return PlaySoundFromByte(chan, (QWORD)(len / 100. * frompos), bRestart, Fadeinms);
}
//////////////////////////////////////////////////////////////////////////
int PlaySoundFromTime(HSTREAM chan, double frompos, BOOL bRestart/*=FALSE*/, int Fadeinms/*=-1*/)
{
	QWORD pos = BASS_ChannelSeconds2Bytes(chan, frompos);

	if (Fadeinms < 0)
		Fadeinms = g_Fade;

	return PlaySoundFromByte(chan, pos, bRestart, Fadeinms);
}
//////////////////////////////////////////////////////////////////////////
int SetSoundVol(HSTREAM chan, double vol)
{
	assert(chan);

	if (!BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, (float)vol))
		return BASS_ErrorGetCode();

	g_vollevel = (float)vol;
	return BASS_OK;
}
//////////////////////////////////////////////////////////////////////////
int SetSoundPan(HSTREAM chan, double pan)
{
	assert(chan);

	if (!BASS_ChannelSetAttribute(chan, BASS_ATTRIB_PAN, (float)pan))
		return BASS_ErrorGetCode();

	g_panning = (float)pan;
	return BASS_OK;
}
//////////////////////////////////////////////////////////////////////////
static int SetSoundPosByte(HSTREAM chan, QWORD pos)
{
	assert(chan);

	if (pos < 0)
		return BASS_ErrorGetCode();

	if (!BASS_ChannelSetPosition(chan, pos, BASS_POS_BYTE))
		return BASS_ErrorGetCode();

	return BASS_OK;
}
//////////////////////////////////////////////////////////////////////////
int SetSoundPosTime(HSTREAM chan, double time)
{
	QWORD pos = BASS_ChannelSeconds2Bytes(chan, time);

	return SetSoundPosByte(chan, pos);
}
//////////////////////////////////////////////////////////////////////////
int SetSoundPosPercent(HSTREAM chan, double pcent)
{
	QWORD len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);

	return SetSoundPosByte(chan, (QWORD)(len / 100. * pcent));
}
//////////////////////////////////////////////////////////////////////////
void MakeTimeStr(double time, TCHAR* str, int strsize, BOOL bShowms, BOOL bShowHours)
{
	if (time < 0) time = 0;

	int seconds = (int)time % 60;
	int minutes = (int)time / 60;
	int hours = minutes / 60;

	if (bShowHours && hours > 0)  //hh:mm:ss.uuu
		_stprintf_s(str, strsize, _T("%3d:%02u:%02u"), hours, minutes % 60, seconds);
	else
		_stprintf_s(str, strsize, _T("%6d:%02u"), minutes, seconds);

	if (bShowms)
		_stprintf_s(str, strsize, _T("%s.%03u"), str, (int)((time - (int)time) * 1000));
	else
		_tcscat_s(str, strsize, _T("    ")); // 4 spaces to pad out msecs
}
//////////////////////////////////////////////////////////////////////////
void GetSoundTimeStr(HSTREAM chan, TCHAR* str, int strsize)
{
	assert(str && strsize);
	assert(chan);

	QWORD pos = BASS_ChannelGetLength(chan, BASS_POS_BYTE);
	// display the time length
	if (pos != -1) {
		double time = BASS_ChannelBytes2Seconds(chan, pos);
		MakeTimeStr(time, str, strsize, TRUE, TRUE);
	} else // no time length available
		str[0] = 0;
}
//////////////////////////////////////////////////////////////////////////
void GetSoundPosStr(HSTREAM chan, TCHAR* str, int strsize, BOOL bRemainTime/*=FALSE*/, BOOL bShowms, BOOL bShowHours)
{
	assert(str && strsize);
	assert(chan);

	QWORD len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);
	QWORD pos = BASS_ChannelGetPosition(chan, BASS_POS_BYTE) % len;

	if (bRemainTime) {
		pos = len - pos;
	}

	double time = BASS_ChannelBytes2Seconds(chan, pos);
	MakeTimeStr(time, str, strsize, bShowms, bShowHours);

	if (bRemainTime) {
		int i = 1;
		while (str[i] == _T(' ')) i++;
		str[i - 1] = _T('-');
	}
}
//////////////////////////////////////////////////////////////////////////
void GetTags(HSTREAM hsnd, TRACK_INFO* ti, DWORD ctype, TCHAR* deftag);

void CALLBACK OnMetaChange(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	TRACK_INFO* ti = (TRACK_INFO*)user;

	BASS_CHANNELINFO ci;
	BASS_ChannelGetInfo(channel, &ci);

	GetTags(channel, ti, ci.ctype, ti->fname);

	SetEvent(g_hMetaEvent);
}

static void SetMetaChangeHandler(HSTREAM hsnd, TRACK_INFO* ti, DWORD ctype)
{
	DWORD synctype = BASS_SYNC_META;
	if (ctype == BASS_CTYPE_STREAM_OGG) {
		synctype = BASS_SYNC_OGG_CHANGE;
	}

	g_metasync = BASS_ChannelSetSync(hsnd, synctype, 0, OnMetaChange, ti);
}

static void ClearMetaChangeHandler(HSTREAM hsnd)
{
	BASS_ChannelRemoveSync(hsnd, g_metasync);
	g_metasync = NULL;
}
//////////////////////////////////////////////////////////////////////////
char* GetShoutMeta(HSTREAM hsnd)
{
	const char* meta;
	meta = BASS_ChannelGetTags(hsnd, BASS_TAG_META);
	if (meta) {
		meta = strstr(meta, "StreamTitle='"); // look for title
		if (meta) { // found it, copy it...
			char* m;
			meta += strlen("StreamTitle='");
			m = _strdup(meta);
			strchr(m, ';')[-1] = 0;
			return m;
		}
	}
	return NULL;
}
//////////////////////////////////////////////////////////////////////////
void GetTags(HSTREAM hsnd, TRACK_INFO* ti, DWORD ctype, TCHAR* deftag)
{
	const char* lTAGS_Read = NULL;
	BOOL freetag = FALSE;

#ifdef _UNICODE
	TAGS_SetUTF8(TRUE);
#endif
	lTAGS_Read = TAGS_Read(hsnd, g_tagsfmt);
	if (ti->fType == SOUND_URL) {
		if (lTAGS_Read[0] == 0) {
			lTAGS_Read = GetShoutMeta(hsnd);
			freetag = TRUE;
		}
		ClearMetaChangeHandler(hsnd);
		SetMetaChangeHandler(hsnd, ti, ctype);
	}

#ifdef _UNICODE
	if (ti->tags) {
		free(ti->tags);
	}
	int d = MultiByteToWideChar(CP_UTF8, 0, lTAGS_Read, -1, NULL, 0);
	ti->tags = (TCHAR*)malloc(sizeof(TCHAR) * d);
	MultiByteToWideChar(CP_UTF8, 0, lTAGS_Read, -1, ti->tags, d);
#else
	ti->tags = _tcsdup(lTAGS_Read);
	CharToOem(ti->tags, ti->tags);
#endif
	if (freetag && lTAGS_Read) {
		free((void*)lTAGS_Read);
	}

	if (ti->tags[0] == 0) {
		free(ti->tags);
		TCHAR* c = _tcsrchr(deftag, _T('\\'));
		if (!c)
			c = deftag - 1;
		ti->tags = _tcsdup(c + 1);
	}

	assert(ti->tags);

	ti->tagslen = (int)_tcslen(ti->tags);
}
//////////////////////////////////////////////////////////////////////////
void SetTagsFormat(const char TagsFmt[])
{
	if (g_tagsfmt) free(g_tagsfmt);
	g_tagsfmt = _strdup(TagsFmt);
}
//////////////////////////////////////////////////////////////////////////
HSTREAM LoadSound(TRACK_INFO* ti)
{
	HSTREAM hsnd;
	SOUND_TYPE fType;

	assert(ti->fname);

	hsnd = Open_Sound(ti->fname, &fType);
	if (!hsnd) return NULL;
	ti->fType = fType;
	ti->lenbytes = BASS_ChannelGetLength(hsnd, BASS_POS_BYTE);
	ti->lensecs = BASS_ChannelBytes2Seconds(hsnd, ti->lenbytes);
	BASS_CHANNELINFO ci;
	BASS_ChannelGetInfo(hsnd, &ci);
	if (((ci.freq == 0) || (ti->lensecs < 1. / ci.freq)) && (fType != SOUND_URL)) { // zero-length track - skip
		Close_Sound(hsnd);
		return NULL;
	}
	TCHAR ts[14];
	MakeTimeStr(ti->lensecs, ts, _countof(ts), TRUE, TRUE);
	LTrim(ts);
	QWORD fsize = BASS_StreamGetFilePosition(hsnd, BASS_FILEPOS_SIZE);
	if (fsize == ULLONG_MAX)
		fsize = 0;//BASS_StreamGetFilePosition(hsnd,BASS_FILEPOS_END);
	int BitRate = ti->lensecs > 1e-3 ? (int)ROUND_DIV(fsize / ti->lensecs * 8, 1000, 1) : 0;
	if (fType == SOUND_URL) {
		if (BitRate == 0)
			BitRate = (int)(BASS_StreamGetFilePosition(hsnd, BASS_FILEPOS_END) * 8 / BASS_GetConfig(BASS_CONFIG_NET_BUFFER));

		TCHAR* s = NULL;
		if (ci.filename[1] == 0) {
			s = _tcsdup((TCHAR*)(ci.filename));
		} else {
			int d = MultiByteToWideChar(CP_UTF8, 0, ci.filename, -1, NULL, 0);
			s = (TCHAR*)malloc(sizeof(TCHAR) * d);
			MultiByteToWideChar(CP_UTF8, 0, ci.filename, -1, s, d);
		}
		if (_tcsicmp(ti->fname, s) != 0) {
			free(ti->fname);
			ti->fname = s;
		} else {
			free(s);
		}
	}
	_stprintf_s(ti->info, _countof(ti->info), _T("%S, %d Hz, %d ch, %d kbps, %s"),
				GetCType(ci), ci.freq, ci.chans, BitRate, ts);

	ti->bLoop = FALSE;
	if (ci.ctype == BASS_CTYPE_STREAM_OGG) {
		const char* comments = BASS_ChannelGetTags(hsnd, BASS_TAG_OGG);
		if (comments)
			while (*comments && !ti->bLoop) {
				ti->bLoop = _stricmp(comments, "ANDROID_LOOP=TRUE") == 0;
				//DebugOut(_T("%S"), comments);
				comments += strlen(comments) + 1;
			}
	}

	GetTags(hsnd, ti, ci.ctype, ti->fname);

	SearchDescription(ti);

	return hsnd;
}
//////////////////////////////////////////////////////////////////////////
void SetFadeInOut(int fade)
{
	DebugOut(_T("SetFadeInOut( %d )"), fade);
	g_Fade = fade;
}
//////////////////////////////////////////////////////////////////////////