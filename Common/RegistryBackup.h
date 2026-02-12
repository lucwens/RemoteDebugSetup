#pragma once
// RegistryBackup.h - Save/restore system state using INI file

#include <afxwin.h>
#include <map>

class CRegistryBackup
{
public:
    CRegistryBackup();
    ~CRegistryBackup();

    // Initialize with a specific state file name (e.g., "state_develop" or "state_test")
    void Initialize(LPCTSTR stateFileName);

    // Save a key-value pair to the state file
    void SaveState(LPCTSTR key, LPCTSTR value);
    void SaveState(LPCTSTR key, bool value);
    void SaveState(LPCTSTR key, int value);

    // Load a value from the state file
    CString LoadState(LPCTSTR key, LPCTSTR defaultValue = _T(""));
    bool    LoadStateBool(LPCTSTR key, bool defaultValue = false);
    int     LoadStateInt(LPCTSTR key, int defaultValue = 0);

    // Check if a saved state exists
    bool HasSavedState();

    // Delete the state file
    void ClearState();

    // Get the full path to the state file
    CString GetStateFilePath() const;

private:
    CString m_stateFilePath;
    static constexpr LPCTSTR SECTION_NAME = _T("State");
};
