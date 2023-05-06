#pragma once
#ifndef __FARSETTINGS__
#define __FARSETTINGS__
#include <wtypes.h>
#include <plugin.hpp>
#include <exception>

class FarSettings
{
public:
	FarSettings(GUID pluginGuid, PluginStartupInfo* pluginStartupInfo);
	~FarSettings();

	UINT64 GetNumber(const WCHAR* name, UINT64 defValue);
	const WCHAR* GetString(const WCHAR* name, const WCHAR* defValue);
	/// <summary>
	///
	/// </summary>
	/// <param name="name"></param>
	/// <param name="data"></param>
	/// <param name="dataSize">in wchars</param>
	/// <param name="defValue"></param>
	/// <returns></returns>
	BOOL GetString(const WCHAR* name, WCHAR* data, DWORD dataSize, const WCHAR* defValue);
	const void* GetBinary(const WCHAR* name, DWORD* dataSize, const void* defValue);
	BOOL GetBinary(const WCHAR* name, BYTE* data, DWORD dataSize, const BYTE* defValue, DWORD defSize);

	BOOL SetNumber(const WCHAR* name, UINT64 data);
	BOOL SetString(const WCHAR* name, const WCHAR* data);
	BOOL SetBinary(const WCHAR* name, const void* data, DWORD dataSize);

private:
	GUID _pluginId;
	HANDLE _handle;
	PluginStartupInfo* _info;
	FarSettingsItem _fsi;

	intptr_t InvokeCmd(FAR_SETTINGS_CONTROL_COMMANDS cmd, void* param);
};
#endif