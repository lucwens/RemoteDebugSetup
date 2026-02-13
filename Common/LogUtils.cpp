#include "pch.h"
#include "LogUtils.h"
#include <ShlObj.h>
#include <Richedit.h>

CLogUtils::CLogUtils()
    : m_pEdit(nullptr)
    , m_fileLogEnabled(false)
{
}

CLogUtils::~CLogUtils()
{
}

void CLogUtils::SetLogControl(CRichEditCtrl* pEdit)
{
    m_pEdit = pEdit;
    if (m_pEdit && ::IsWindow(m_pEdit->GetSafeHwnd()))
    {
        m_pEdit->SetBackgroundColor(FALSE, RGB(255, 255, 255));

        // Inherit the dialog font so the rich-edit looks identical to a CEdit
        CWnd* pParent = m_pEdit->GetParent();
        if (pParent)
        {
            CFont* pFont = pParent->GetFont();
            if (pFont)
            {
                LOGFONT lf = {};
                pFont->GetLogFont(&lf);

                HDC hDC = ::GetDC(nullptr);
                int pts = MulDiv(abs(lf.lfHeight), 72, GetDeviceCaps(hDC, LOGPIXELSY));
                ::ReleaseDC(nullptr, hDC);

                CHARFORMAT2 cf = {};
                cf.cbSize = sizeof(cf);
                cf.dwMask = CFM_FACE | CFM_SIZE;
                cf.yHeight = pts * 20;  // twips
                _tcscpy_s(cf.szFaceName, lf.lfFaceName);
                m_pEdit->SetDefaultCharFormat(cf);
            }
        }
    }
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
    AppendToEdit(text, RGB(0, 100, 200));  // blue
    WriteJsonLine(_T("STEP"), message, step, total);
}

void CLogUtils::LogSuccess(LPCTSTR message)
{
    CString text;
    text.Format(_T("  OK: %s\r\n"), message);
    AppendToEdit(text, RGB(0, 140, 0));  // green
    WriteJsonLine(_T("SUCCESS"), message);
}

void CLogUtils::LogWarning(LPCTSTR message)
{
    CString text;
    text.Format(_T("  WARNING: %s\r\n"), message);
    AppendToEdit(text, RGB(220, 120, 0));  // orange
    WriteJsonLine(_T("WARNING"), message);
}

void CLogUtils::LogError(LPCTSTR message)
{
    CString text;
    text.Format(_T("  ERROR: %s\r\n"), message);
    AppendToEdit(text, RGB(200, 0, 0));  // red
    WriteJsonLine(_T("ERROR"), message);
}

void CLogUtils::LogInfo(LPCTSTR message)
{
    CString text;
    text.Format(_T("  %s\r\n"), message);
    AppendToEdit(text, RGB(128, 128, 128));  // grey

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
    AppendToEdit(text, RGB(128, 128, 128));  // grey
    // Separators are visual-only, not written to JSON
}

void CLogUtils::Clear()
{
    if (m_pEdit && ::IsWindow(m_pEdit->GetSafeHwnd()))
    {
        m_pEdit->SetWindowText(_T(""));
    }
}

void CLogUtils::AppendToEdit(LPCTSTR text, COLORREF color)
{
    if (!m_pEdit || !::IsWindow(m_pEdit->GetSafeHwnd()))
        return;

    // Move caret to end
    long len = m_pEdit->GetTextLength();
    m_pEdit->SetSel(len, len);

    // Set text color for the insertion
    CHARFORMAT2 cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    cf.dwEffects = 0;  // clear CFE_AUTOCOLOR
    m_pEdit->SetSelectionCharFormat(cf);

    // Insert the text
    m_pEdit->ReplaceSel(text);

    // Auto-scroll to bottom
    m_pEdit->SendMessage(WM_VSCROLL, SB_BOTTOM, 0);

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
