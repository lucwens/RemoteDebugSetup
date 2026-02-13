#pragma once
// SettingsUtils.h - Simple JSON settings persistence

#include <afxwin.h>
#include <map>

class CSettingsUtils
{
public:
    // Load key-value pairs from a JSON file. Returns true if file was read.
    static bool Load(LPCTSTR filePath, std::map<CString, CString>& settings);

    // Save key-value pairs to a JSON file. Creates directories as needed.
    static bool Save(LPCTSTR filePath, const std::map<CString, CString>& settings);

    // Get the Settings directory next to the executable.
    static CString GetSettingsDir();
};
