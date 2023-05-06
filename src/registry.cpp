#include "registry.h"
#include <tchar.h>

CRegistry::CRegistry(const TCHAR* baseKey, HKEY hRoot)
{
	BaseKey = _tcsdup(baseKey);
	Root = hRoot;
}

CRegistry::~CRegistry()
{
	free(BaseKey);
}

HKEY CRegistry::OpenBaseKey()
{
	HKEY hKey;
	return RegOpenKeyEx(Root, BaseKey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS ? hKey : NULL;
}

HKEY CRegistry::CreateBaseKey()
{
	HKEY hKey;
	return RegCreateKeyEx(Root, BaseKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS ? hKey : NULL;
}

DWORD CRegistry::GetDword(const TCHAR* name, DWORD defValue)
{
	DWORD data = 0, size = sizeof(data);

	return GetValue(name, (BYTE*)&data, &size) ? data : defValue;
}

const TCHAR* CRegistry::GetString(const TCHAR* name, const TCHAR* defValue)
{
	return (TCHAR*)GetBinary(name, defValue);
}

BOOL CRegistry::GetString(const TCHAR* name, TCHAR* data, DWORD* dataSize, const TCHAR* defValue)
{
	return GetBinary(name, (BYTE*)data, dataSize, (BYTE*)defValue, (_tcslen(defValue) + 1) * sizeof(TCHAR) / sizeof(BYTE));
}

BOOL CRegistry::GetString(const TCHAR* name, TCHAR* data, DWORD dataSize, const TCHAR* defValue)
{
	return GetBinary(name, (BYTE*)data, &dataSize, (BYTE*)defValue, (_tcslen(defValue) + 1) * sizeof(TCHAR) / sizeof(BYTE));
}

const void* CRegistry::GetBinary(const TCHAR* name, const void* defValue)
{
	DWORD size;

	if (GetValue(name, NULL, &size)) {
		BYTE* val = new BYTE[size];
		if (GetValue(name, val, &size)) {
			return val;
		}
	}

	return defValue;
}

BOOL CRegistry::GetBinary(const TCHAR* name, BYTE* data, DWORD dataSize, const BYTE* defValue, DWORD defSize)
{
	return GetBinary(name, data, &dataSize, defValue, defSize);
}

BOOL CRegistry::GetBinary(const TCHAR* name, BYTE* data, DWORD* dataSize, const BYTE* defValue, DWORD defSize)
{
	DWORD size = *dataSize;
	if (GetValue(name, data, dataSize)) {
		return TRUE;
	}

	*dataSize = size;
	memcpy_s(data, *dataSize, defValue, defSize);
	return FALSE;
}

BOOL CRegistry::GetValue(const TCHAR* name, BYTE* data, DWORD* dataSize)
{
	HKEY hKey = OpenBaseKey();
	if (hKey == NULL) return FALSE;

	int res = RegQueryValueEx(hKey, name, 0, NULL, data, dataSize);
	RegCloseKey(hKey);

	return res == ERROR_SUCCESS;
}

BOOL CRegistry::SetDword(const TCHAR* name, DWORD data)
{
	return SetValue(name, &data, sizeof(data), REG_DWORD);
}

BOOL CRegistry::SetString(const TCHAR* name, TCHAR* data, DWORD type)
{
	return SetValue(name, data, (_tcslen(data) + 1) * sizeof(TCHAR) / sizeof(BYTE), type);
}

BOOL CRegistry::SetBinary(const TCHAR* name, void* data, DWORD dataSize)
{
	return SetValue(name, data, dataSize, REG_BINARY);
}

BOOL CRegistry::SetValue(const TCHAR* name, void* data, DWORD dataSize, DWORD type)
{
	HKEY hKey = CreateBaseKey();
	if (hKey == NULL) return FALSE;

	int res = RegSetValueEx(hKey, name, 0, type, (BYTE*)data, dataSize);
	RegCloseKey(hKey);

	return res == ERROR_SUCCESS;
}