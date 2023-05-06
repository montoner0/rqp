#include "descriptions.h"
#include <assert.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////
void GetDescriptionFName(TCHAR* sndfname, TCHAR* descfname, int descfnlen)
{
	assert(sndfname && descfname && descfnlen >= PATH_LEN);

	ExpandEnvironmentStrings(g_descrfile, descfname, descfnlen);

	TCHAR* c = _tcsrchr(descfname, _T('\\'));

	if (!c) { // description file have no full path. trying to use sound file's path
		c = _tcsrchr(sndfname, _T('\\'));
		if (c) {
			TCHAR tmp[PATH_LEN];
			_tcsncpy_s(tmp, _countof(tmp), sndfname, c - sndfname + 1);
			_tcscat_s(tmp, _countof(tmp), descfname);
			_tcscpy_s(descfname, descfnlen, tmp);
		}
	}
}
//////////////////////////////////////////////////////////////////////////
BOOL SearchDescription(TCHAR* fname, TCHAR* fdescr, DWORD descrlen)
{
	assert(fname && fdescr && descrlen);

	FILE* f;
	TCHAR descrfile[PATH_LEN];

	GetDescriptionFName(fname, descrfile, _countof(descrfile));

	_tfopen_s(&f, descrfile, _T("rt, ccs=UTF-8"));
	if (!f) return FALSE;

	TCHAR fn[PATH_LEN], descr[DESCR_LEN];

	fdescr[0] = 0;
	while (!feof(f)) {
		_ftscanf_s(f, _T("%[^|]|%[^\r\n]%*[\r\n]"), fn, _countof(fn), descr, _countof(descr));
		if (_tcsicmp(fn, fname) == 0) {
			_tcscpy_s(fdescr, descrlen, descr);
			fclose(f);
			return TRUE;
		}
	}
	fclose(f);
	return FALSE;
}
//////////////////////////////////////////////////////////////////////////
BOOL SearchDescription(TRACK_INFO* ti)
{
	return SearchDescription(ti->fname, ti->descr, _countof(ti->descr));
}
//////////////////////////////////////////////////////////////////////////
BOOL ReplaceDescription(TCHAR* fname, TCHAR* fdescr)
{
	assert(fname && fdescr);

	TCHAR templ[PATH_LEN] = { _T("rqpdescXXXXXX") };
	if (_tmktemp_s(templ, _countof(templ)) != 0)
		return FALSE;

	TCHAR descrfile[PATH_LEN];

	GetDescriptionFName(fname, descrfile, _countof(descrfile));

	TCHAR* c = _tcsrchr(descrfile, _T('\\'));
	TCHAR outfile[PATH_LEN] = { 0 };

	if (c)
		_tcsncpy_s(outfile, _countof(outfile), descrfile, c - descrfile + 1);

	_tcscat_s(outfile, _countof(outfile), templ);

	FILE* f = NULL, * f2 = NULL;

	__try {
		_tfopen_s(&f, descrfile, _T("rt, ccs=UTF-8"));
		if (!f) return FALSE;

		_tfopen_s(&f2, outfile, _T("wt, ccs=UTF-8"));
		if (!f2) return FALSE;

		TCHAR fn[PATH_LEN], descr[DESCR_LEN];

		while (!feof(f)) {
			_ftscanf_s(f, _T("%[^|]|%[^\r\n]%*[\r\n]"), fn, _countof(fn), descr, _countof(descr));
			if (_tcsicmp(fn, fname) == 0) {
				if (fdescr[0]) {
					_tcscpy_s(descr, _countof(descr), fdescr);
				} else {
					continue;
				}
			}
			if (_ftprintf_s(f2, _T("%s|%s\n"), fn, descr) <= 0)
				return FALSE;
		}

		fclose(f);
		fclose(f2);
		f = f2 = NULL;

		if (_tremove(descrfile) != 0)
			return FALSE;
		else
			if (_trename(outfile, descrfile) != 0)
				return FALSE;

		return TRUE;
	} __finally {
		if (f) fclose(f);
		if (f2) fclose(f2);
	}
}
//////////////////////////////////////////////////////////////////////////
BOOL ReplaceDescription(TRACK_INFO* ti)
{
	return ReplaceDescription(ti->fname, ti->descr);
}
//////////////////////////////////////////////////////////////////////////
BOOL AddDescription(TCHAR* fname, TCHAR* fdescr)
{
	assert(fname && fdescr);

	FILE* f = NULL;
	BOOL bOk = FALSE;

	TCHAR descrfile[PATH_LEN];

	GetDescriptionFName(fname, descrfile, _countof(descrfile));

	_tfopen_s(&f, descrfile, _T("a+t, ccs=UTF-8"));
	if (!f) return FALSE;

	__try {
		if (_ftprintf_s(f, _T("%s|%s\n"), fname, fdescr) <= 0)
			return FALSE;

		return TRUE;
	} __finally {
		if (f) fclose(f);
	}
}
//////////////////////////////////////////////////////////////////////////
BOOL AddDescription(TRACK_INFO* ti)
{
	return AddDescription(ti->fname, ti->descr);
}
//////////////////////////////////////////////////////////////////////////
void EnterDescription(TRACK_INFO* ti, struct PluginStartupInfo* Info)
{
	TCHAR descr[DESCR_LEN] = { 0 };

	BOOL bIsDescrPresent = SearchDescription(ti);

	if (PINPUTBOXID(GetLocalizedMsg(MEnterDescr), NULL, _T("RQPDescriptions"), ti->descr, descr, _countof(descr), NULL, FIB_BUTTONS | FIB_ENABLEEMPTY)) {
		_tcscpy_s(ti->descr, DESCR_LEN, descr);
		if (bIsDescrPresent) { // remove/replace existing description
			if (!ReplaceDescription(ti))
				ErrorMsg(MDescUpdateFail, 0);
		} else {
			if (ti->descr) {
				if (!AddDescription(ti))
					ErrorMsg(MDescUpdateFail, 1);
			}
		}
	}
}