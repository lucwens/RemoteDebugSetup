#include "pch.h"
#include "LogUtils.h"
#include <ShlObj.h>

CLogUtils::CLogUtils()
    : m_pEdit(nullptr)
    , m_fileLogEnabled(false)
{
}

CLogUtils::~CLogUtils()
{
}

void CLogUtils::SetLogControl(CEdit* pEdit)
{
    m_pEdit = pEdit;
}

void CLogUtils::InitFileLog(LPCTSTR appName)
{
    m_appName = appName;

    // Get hostname once
    TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerName(computerName, &size);
    m_hostname = computerName;

    // Build "Log" folder path next to the executable
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    CString exeDir(exePath);
    exeDir = exeDir.Left(exeDir.ReverseFind(_T('\\')));
    CString logDir = exeDir + _T("\\Log");

    // Create directory (including parents)
    SHCreateDirectoryEx(nullptr, logDir, nullptr);

    // Build filename: AppName_YYYY-MM-DD_HHmmss.jsonl
    SYSTEMTIME st;
    GetLocalTime(&st);
    CString fileName;
    fileName.Format(_T("%s_%04d-%02d-%02d_%02d%02d%02d.jsonl"),
                    appName, st.wYear, st.wMonth, st.wDay,
                    st.wHour, st.wMinute, st.wSecond);

    m_logFilePath = logDir + _T("\\") + fileName;
    m_fileLogEnabled = true;

    // Write opening system info entry
    CString osInfo;
    OSVERSIONINFOEX ovi = {};
    ovi.dwOSVersionInfoSize = sizeof(ovi);
#pragma warning(push)
#pragma warning(disable: 4996)  // GetVersionEx deprecated but sufficient here
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&ovi));
#pragma warning(pop)
    osInfo.Format(_T("Windows %d.%d.%d"),
                  ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);

    CString sysMsg;
    sysMsg.Format(_T("Log started. Host: %s, OS: %s, Admin: %s"),
                  (LPCTSTR)m_hostname, (LPCTSTR)osInfo,
                  IsUserAnAdmin() ? _T("yes") : _T("no"));
    WriteJsonLine(_T("SYSTEM"), sysMsg);
}

void CLogUtils::SetOperation(LPCTSTR operation)
{
    m_operation = operation;
}

void CLogUtils::Log(LPCTSTR message)
{
    CString text;
    text.Format(_T("%s\r\n"), message);
    AppendToEdit(text);

    // Skip separator-only and blank lines for JSON
    CString trimmed(message);
    trimmed.Trim();
    if (!trimmed.IsEmpty() && trimmed[0] != _T('='))
    {
        WriteJsonLine(_T("INFO"), trimmed);
    }
}

void CLogUtils::LogStep(int step, int total, LPCTSTR message)
{
    CString text;
    text.Format(_T("[%d/%d] %s\r\n"), step, total, message);
    AppendToEdit(text);
    WriteJsonLine(_T("STEP"), message, step, total);
}

void CLogUtils::LogSuccess(LPCTSTR message)
{
    CString text;
    text.Format(_T("  OK: %s\r\n"), message);
    AppendToEdit(text);
    WriteJsonLine(_T("SUCCESS"), message);
}

void CLogUtils::LogWarning(LPCTSTR message)
{
    CString text;
    text.Format(_T("  WARNING: %s\r\n"), message);
    AppendToEdit(text);
    WriteJsonLine(_T("WARNING"), message);
}

void CLogUtils::LogError(LPCTSTR message)
{
    CString text;
    text.Format(_T("  ERROR: %s\r\n"), message);
    AppendToEdit(text);
    WriteJsonLine(_T("ERROR"), message);
}

void CLogUtils::LogInfo(LPCTSTR message)
{
    CString text;
    text.Format(_T("  %s\r\n"), message);
    AppendToEdit(text);

    CString trimmed(message);
    trimmed.Trim();
    if (!trimmed.IsEmpty())
    {
        WriteJsonLine(_T("INFO"), trimmed);
    }
}

void CLogUtils::LogSeparator()
{
    CString text(_T("======================================================\r\n"));
    AppendToEdit(text);
    // Separators are visual-only, not written to JSON
}

void CLogUtils::Clear()
{
    if (m_pEdit && ::IsWindow(m_pEdit->GetSafeHwnd()))
    {
        m_pEdit->SetWindowText(_T(""));
    }
}

void CLogUtils::AppendToEdit(LPCTSTR text)
{
    if (!m_pEdit || !::IsWindow(m_pEdit->GetSafeHwnd()))
        return;

    int len = m_pEdit->GetWindowTextLength();
    m_pEdit->SetSel(len, len);
    m_pEdit->ReplaceSel(text);

    // Auto-scroll to bottom
    m_pEdit->LineScroll(m_pEdit->GetLineCount());

    // Process pending messages so the UI updates
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

void CLogUtils::WriteJsonLine(LPCTSTR level, LPCTSTR message, int step, int total)
{
    if (!m_fileLogEnabled || m_logFilePath.IsEmpty())
        return;

    CString timestamp = GetISOTimestamp();
    CString escapedMsg = JsonEscape(message);
    CString escapedOp  = JsonEscape(m_operation);

    // Build JSON line
    CString json;
    json.Format(
        _T("{\"Timestamp\":\"%s\",\"Level\":\"%s\",\"Message\":\"%s\"")
        _T(",\"Logger\":\"%s\",\"Hostname\":\"%s\""),
        (LPCTSTR)timestamp, level, (LPCTSTR)escapedMsg,
        (LPCTSTR)m_appName, (LPCTSTR)m_hostname);

    if (!m_operation.IsEmpty())
    {
        CString opField;
        opField.Format(_T(",\"Operation\":\"%s\""), (LPCTSTR)escapedOp);
        json += opField;
    }

    if (step >= 0 && total > 0)
    {
        CString stepField;
        stepField.Format(_T(",\"Step\":%d,\"TotalSteps\":%d"), step, total);
        json += stepField;
    }

    json += _T("}\n");

    // Write as UTF-8
    FILE* f = nullptr;
    _tfopen_s(&f, m_logFilePath, _T("a, ccs=UTF-8"));
    if (f)
    {
        _fputts(json, f);
        fclose(f);
    }
}

CString CLogUtils::JsonEscape(LPCTSTR input)
{
    if (!input)
        return CString();

    CString result;
    for (int i = 0; input[i] != _T('\0'); i++)
    {
        switch (input[i])
        {
        case _T('"'):   result += _T("\\\""); break;
        case _T('\\'):  result += _T("\\\\"); break;
        case _T('\n'):  result += _T("\\n");  break;
        case _T('\r'):  result += _T("\\r");  break;
        case _T('\t'):  result += _T("\\t");  break;
        default:        result += input[i];    break;
        }
    }
    return result;
}

CString CLogUtils::GetISOTimestamp()
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    CString ts;
    ts.Format(_T("%04d-%02d-%02dT%02d:%02d:%02d.%03d"),
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return ts;
}
