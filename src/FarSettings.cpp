#include "FarSettings.h"
#include <stdexcept>

FarSettings::FarSettings(GUID pluginGuid, PluginStartupInfo* pluginStartupInfo)
{
	_pluginId = pluginGuid;
	_handle = INVALID_HANDLE_VALUE;
	_info = pluginStartupInfo;
	FarSettingsCreate fsc = { sizeof(fsc), _pluginId };
	_fsi = { sizeof(_fsi), 0 };

	if (InvokeCmd(SCTL_CREATE, &fsc)) {
		_handle = fsc.Handle;
	} else {
		throw std::invalid_argument("Can't create settings");
	}
}

FarSettings::~FarSettings()
{
	InvokeCmd(SCTL_FREE, 0);
}

UINT64 FarSettings::GetNumber(const WCHAR* name, UINT64 defValue)
{
	_fsi.Name = name;
	_fsi.Type = FST_QWORD;

	return InvokeCmd(SCTL_GET, &_fsi) ? _fsi.Number : defValue;
}

const WCHAR* FarSettings::GetString(const WCHAR* name, const WCHAR* defValue)
{
	_fsi.Name = name;
	_fsi.Type = FST_STRING;

	return InvokeCmd(SCTL_GET, &_fsi) ? _fsi.String : defValue;
}

BOOL FarSettings::GetString(const WCHAR* name, WCHAR* data, DWORD dataSize, const WCHAR* defValue)
{
	return wcsncpy_s(data, dataSize, GetString(name, defValue), _TRUNCATE) == 0;
}

const void* FarSettings::GetBinary(const WCHAR* name, DWORD* dataSize, const void* defValue)
{
	_fsi.Name = name;
	_fsi.Type = FST_DATA;
	const BOOL res = InvokeCmd(SCTL_GET, &_fsi);
	*dataSize = _fsi.Data.Size;

	return res ? _fsi.Data.Data : defValue;
}

BOOL FarSettings::GetBinary(const WCHAR* name, BYTE* data, DWORD dataSize, const BYTE* defValue, DWORD defSize)
{
	DWORD size;
	return GetBinary(name, &size, defValue)
		? memcpy_s(data, dataSize, _fsi.Data.Data, size) == 0
		: memcpy_s(data, dataSize, defValue, defSize) == 0;
}

BOOL FarSettings::SetNumber(const WCHAR* name, UINT64 data)
{
	_fsi.Name = name;
	_fsi.Type = FST_QWORD;
	_fsi.Number = data;

	return InvokeCmd(SCTL_SET, &_fsi);
}

BOOL FarSettings::SetString(const WCHAR* name, const WCHAR* data)
{
	_fsi.Name = name;
	_fsi.Type = FST_STRING;
	_fsi.String = data;

	return InvokeCmd(SCTL_SET, &_fsi);
}

BOOL FarSettings::SetBinary(const WCHAR* name, const void* data, DWORD dataSize)
{
	_fsi.Name = name;
	_fsi.Type = FST_DATA;
	_fsi.Data.Data = data;
	_fsi.Data.Size = dataSize;

	return InvokeCmd(SCTL_SET, &_fsi);
}

intptr_t FarSettings::InvokeCmd(enum FAR_SETTINGS_CONTROL_COMMANDS cmd, void* settingsInfo)
{
	return _info ? _info->SettingsControl(_handle, cmd, PSL_ROAMING, settingsInfo) : FALSE;
}