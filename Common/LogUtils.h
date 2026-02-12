#pragma once
// LogUtils.h - Logging to CEdit control and optional file

#include <afxwin.h>

class CLogUtils
{
public:
    CLogUtils();
    ~CLogUtils();

    // Set the edit control to receive log messages
    void SetLogControl(CEdit* pEdit);

    // Log a message with a prefix
    void Log(LPCTSTR message);
    void LogStep(int step, int total, LPCTSTR message);
    void LogSuccess(LPCTSTR message);
    void LogWarning(LPCTSTR message);
    void LogError(LPCTSTR message);
    void LogInfo(LPCTSTR message);
    void LogSeparator();
    void Clear();

    // Enable/disable file logging
    void EnableFileLog(LPCTSTR filePath);
    void DisableFileLog();

private:
    void AppendToEdit(LPCTSTR text);
    void AppendToFile(LPCTSTR text);

    CEdit*  m_pEdit;
    CString m_logFilePath;
    bool    m_fileLogEnabled;
};
