#pragma once
#ifndef __REGISTRY_H__
#define __REGISTRY_H__
#include <windows.h>
#include <tchar.h>

class CRegistry
{
public:
	CRegistry(const TCHAR* baseKey, HKEY hRoot = HKEY_CURRENT_USER);
	~CRegistry();

	DWORD GetDword(const TCHAR* name, DWORD defValue);
	const TCHAR* GetString(const TCHAR* name, const TCHAR* defValue);
	/// <summary>
	///
	/// </summary>
	/// <param name="name"></param>
	/// <param name="data"></param>
	/// <param name="dataSize">in bytes</param>
	/// <param name="defValue"></param>
	/// <returns></returns>
	BOOL GetString(const TCHAR* name, TCHAR* data, DWORD* dataSize, const TCHAR* defValue);
	/// <summary>
	///
	/// </summary>
	/// <param name="name"></param>
	/// <param name="data"></param>
	/// <param name="dataSize">in bytes</param>
	/// <param name="defValue"></param>
	/// <returns></returns>
	BOOL GetString(const TCHAR* name, TCHAR* data, DWORD dataSize, const TCHAR* defValue);
	const void* GetBinary(const TCHAR* name, const void* defValue);
	BOOL GetBinary(const TCHAR* name, BYTE* data, DWORD dataSize, const BYTE* defValue, DWORD defSize);
	BOOL GetBinary(const TCHAR* name, BYTE* data, DWORD* dataSize, const BYTE* defValue, DWORD defSize);
	BOOL GetValue(const TCHAR* name, BYTE* data, DWORD* dataSize);

	BOOL SetDword(const TCHAR* name, DWORD data);
	BOOL SetString(const TCHAR* name, TCHAR* data, DWORD type = REG_SZ);
	BOOL SetBinary(const TCHAR* name, void* data, DWORD dataSize);
	BOOL SetValue(const TCHAR* name, void* data, DWORD dataSize, DWORD type);

private:
	TCHAR* BaseKey;
	HKEY Root;

	HKEY OpenBaseKey();
	HKEY CreateBaseKey();
};
#endif