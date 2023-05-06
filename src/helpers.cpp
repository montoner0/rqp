#include <windows.h>
#include <tchar.h>
#include <assert.h>

#include "helpers.h"

#pragma comment(lib,"version.lib")

TCHAR* LTrim(TCHAR* s)
{
	TCHAR* b;

	b = s;
	while (*b == _T(' ') && *b != 0) b++;
	if (*b != 0) memmove(s, b, (_tcslen(b) + 1) * sizeof(*b));
	return s;
}

//////////////////////////////////////////////////////////////////////////
BOOL FileExist(TCHAR* fn)
{
	//WIN32_FIND_DATA FileData;
	//HANDLE hSearch;

	return (GetFileAttributes(fn) != -1);
	/*   hSearch = FindFirstFile(fn, &FileData);
	   if (hSearch==INVALID_HANDLE_VALUE)
		  return FALSE;
	   FindClose(hSearch);
	   if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		  return FALSE;
	   return TRUE;*/
}
//////////////////////////////////////////////////////////////////////////
void _DebugOut(const TCHAR* format, ...)
{
	va_list args;
	int     len;
	TCHAR* buffer;

	// retrieve the variable arguments
	va_start(args, format);

	len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'

	buffer = new TCHAR[len * sizeof(TCHAR)];

	_vstprintf_s(buffer, len * sizeof(TCHAR), format, args);

#ifdef DEBUGOUT
	OutputDebugString(buffer);
	OutputDebugString(_T("\n"));
#endif
#ifdef CONOUT
	_putts(buffer);
#endif
#ifdef FILEOUT
	FILE* f;
	fopen_s(f, _T(""), _T("at"));
	_ftprintf_s(f, _T("%s\n"), buffer);
	fclose(f);
#endif

	delete[]buffer;
}
//////////////////////////////////////////////////////////////////////////
void GetMyVersion(PVERSION pver, BOOL bFileVer, HMODULE hm)
{
	TCHAR fn[PATH_LEN];
	DWORD d, sz;
	BYTE* pb;
	VS_FIXEDFILEINFO* fi;
	UINT bsize;

	assert(pver);
	GetModuleFileName(hm, fn, _countof(fn));
	sz = GetFileVersionInfoSize(fn, &d);
	if (sz) {
		pb = new BYTE[sz];

		assert(pb);

		GetFileVersionInfo(fn, 0, sz, pb);

		if (VerQueryValue(pb, _T("\\"), (LPVOID*)&fi, &bsize)) {
			if (bFileVer) {
				pver->major = HIWORD(fi->dwFileVersionMS);
				pver->minor = LOWORD(fi->dwFileVersionMS);
				pver->build = HIWORD(fi->dwFileVersionLS);
				pver->revision = LOWORD(fi->dwFileVersionLS);
			} else {
				pver->major = HIWORD(fi->dwProductVersionMS);
				pver->minor = LOWORD(fi->dwProductVersionMS);
				pver->build = HIWORD(fi->dwProductVersionLS);
				pver->revision = LOWORD(fi->dwProductVersionLS);
			}
		}
		delete[]pb;
	}
}
//////////////////////////////////////////////////////////////////////////
BOOL GetParentFolder(TCHAR* path)
{
	TCHAR* c = _tcsrchr(path, _T('\\'));
	if (c) {
		*c = 0;
		return TRUE;
	}
	return FALSE;
}
//////////////////////////////////////////////////////////////////////////
void StopThread(HANDLE thread, DWORD timeout)
{
	DebugOut(_T("StopThread<"));

	if (WaitForSingleObject(thread, timeout) == WAIT_TIMEOUT) {
		DebugOut(_T("StopThread< TerminateThread"));
		TerminateThread(thread, 0);
	}
	DebugOut(_T("StopThread< 1"));
	CloseHandle(thread);
	thread = NULL;
	DebugOut(_T(">StopThread"));
}
///EOF////////////////////////////////////////////////////////////////////