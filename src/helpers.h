#pragma once
typedef struct _VERSION
{
	WORD major;
	WORD minor;
	WORD build;
	WORD revision;
}VERSION, * PVERSION;

#ifdef _DEBUG
#define DebugOut _DebugOut
#else
#define DebugOut(...)
#endif
//
#define MAX_PATH_LENGTH       32768

#define PATH_LEN (MAX_PATH_LENGTH + MAX_PATH + 1)
#define SETLIMITS(x,lolimit,hilimit) (x)<(lolimit)? (lolimit) : (x)>(hilimit)? (hilimit) : (x)
#define MKBYTE(l,h) ((l)&0xf | ((h)&0xf)<<4)
#define LONIBBLE(b) ((b)&0xf)
#define HINIBBLE(b) (((b)>>4)&0xf)
#define LODWORD(qw) ((DWORD)(qw))
TCHAR* LTrim(TCHAR* s);
void GetMyVersion(PVERSION pver, BOOL bFileVer, HMODULE hm = NULL);
BOOL FileExist(TCHAR* fn);
void _DebugOut(const TCHAR* msg, ...);
BOOL GetParentFolder(TCHAR* path);
void StopThread(HANDLE thread, DWORD timeout = 0);
