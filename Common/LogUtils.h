#pragma once
// LogUtils.h - Logging to CRichEditCtrl control and NDJSON file

#include <afxwin.h>
#include <afxcmn.h>

class CLogUtils
{
public:
    CLogUtils();
    ~CLogUtils();

    // Set the rich-edit control to receive log messages
    void SetLogControl(CRichEditCtrl* pEdit);

    // Initialize NDJSON file logging.
    // Creates a "Log" folder next to the executable and opens a timestamped file.
    // appName: e.g. "SetupDevelop" or "SetupTest"
    void InitFileLog(LPCTSTR appName);

    // Set the current operation context (e.g. "setup", "restore")
    void SetOperation(LPCTSTR operation);

    // Log a message with a prefix
    void Log(LPCTSTR message);
    void LogStep(int step, int total, LPCTSTR message);
    void LogSuccess(LPCTSTR message);
    void LogWarning(LPCTSTR message);
    void LogError(LPCTSTR message);
    void LogInfo(LPCTSTR message);
    void LogSeparator();
    void Clear();

    // Get the current log file path (empty if file logging not active)
    CString GetLogFilePath() const { return m_logFilePath; }

private:
    void AppendToEdit(LPCTSTR text, COLORREF color = RGB(0, 0, 0));
    void WriteJsonLine(LPCTSTR level, LPCTSTR message, int step = -1, int total = -1);

    static CString JsonEscape(LPCTSTR input);
    static CString GetISOTimestamp();

    CRichEditCtrl* m_pEdit;
    CString m_logFilePath;
    bool    m_fileLogEnabled;
    CString m_appName;
    CString m_hostname;
    CString m_operation;
};
