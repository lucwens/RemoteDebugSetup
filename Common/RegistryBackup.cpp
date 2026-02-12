#include "pch.h"
#include "RegistryBackup.h"
#include <ShlObj.h>

CRegistryBackup::CRegistryBackup()
{
}

CRegistryBackup::~CRegistryBackup()
{
}

void CRegistryBackup::Initialize(LPCTSTR stateFileName)
{
    // Build path: %APPDATA%\RemoteDebugSetup\<stateFileName>.ini
    TCHAR appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath)))
    {
        m_stateFilePath.Format(_T("%s\\RemoteDebugSetup\\%s.ini"), appDataPath, stateFileName);

        // Ensure directory exists
        CString dir;
        dir.Format(_T("%s\\RemoteDebugSetup"), appDataPath);
        ::CreateDirectory(dir, nullptr);
    }
}

void CRegistryBackup::SaveState(LPCTSTR key, LPCTSTR value)
{
    if (m_stateFilePath.IsEmpty())
        return;
    ::WritePrivateProfileString(SECTION_NAME, key, value, m_stateFilePath);
}

void CRegistryBackup::SaveState(LPCTSTR key, bool value)
{
    SaveState(key, value ? _T("1") : _T("0"));
}

void CRegistryBackup::SaveState(LPCTSTR key, int value)
{
    CString str;
    str.Format(_T("%d"), value);
    SaveState(key, str);
}

CString CRegistryBackup::LoadState(LPCTSTR key, LPCTSTR defaultValue)
{
    if (m_stateFilePath.IsEmpty())
        return defaultValue;

    TCHAR buffer[1024];
    ::GetPrivateProfileString(SECTION_NAME, key, defaultValue, buffer, _countof(buffer), m_stateFilePath);
    return CString(buffer);
}

bool CRegistryBackup::LoadStateBool(LPCTSTR key, bool defaultValue)
{
    CString val = LoadState(key, defaultValue ? _T("1") : _T("0"));
    return val == _T("1");
}

int CRegistryBackup::LoadStateInt(LPCTSTR key, int defaultValue)
{
    CString defStr;
    defStr.Format(_T("%d"), defaultValue);
    CString val = LoadState(key, defStr);
    return _ttoi(val);
}

bool CRegistryBackup::HasSavedState()
{
    if (m_stateFilePath.IsEmpty())
        return false;
    return (::GetFileAttributes(m_stateFilePath) != INVALID_FILE_ATTRIBUTES);
}

void CRegistryBackup::ClearState()
{
    if (!m_stateFilePath.IsEmpty())
    {
        ::DeleteFile(m_stateFilePath);
    }
}

CString CRegistryBackup::GetStateFilePath() const
{
    return m_stateFilePath;
}
