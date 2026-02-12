#include "pch.h"
#include "LogUtils.h"
#include <fstream>

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

void CLogUtils::Log(LPCTSTR message)
{
    CString text;
    text.Format(_T("%s\r\n"), message);
    AppendToEdit(text);
    AppendToFile(text);
}

void CLogUtils::LogStep(int step, int total, LPCTSTR message)
{
    CString text;
    text.Format(_T("[%d/%d] %s\r\n"), step, total, message);
    AppendToEdit(text);
    AppendToFile(text);
}

void CLogUtils::LogSuccess(LPCTSTR message)
{
    CString text;
    text.Format(_T("  OK: %s\r\n"), message);
    AppendToEdit(text);
    AppendToFile(text);
}

void CLogUtils::LogWarning(LPCTSTR message)
{
    CString text;
    text.Format(_T("  WARNING: %s\r\n"), message);
    AppendToEdit(text);
    AppendToFile(text);
}

void CLogUtils::LogError(LPCTSTR message)
{
    CString text;
    text.Format(_T("  ERROR: %s\r\n"), message);
    AppendToEdit(text);
    AppendToFile(text);
}

void CLogUtils::LogInfo(LPCTSTR message)
{
    CString text;
    text.Format(_T("  %s\r\n"), message);
    AppendToEdit(text);
    AppendToFile(text);
}

void CLogUtils::LogSeparator()
{
    CString text(_T("======================================================\r\n"));
    AppendToEdit(text);
    AppendToFile(text);
}

void CLogUtils::Clear()
{
    if (m_pEdit && ::IsWindow(m_pEdit->GetSafeHwnd()))
    {
        m_pEdit->SetWindowText(_T(""));
    }
}

void CLogUtils::EnableFileLog(LPCTSTR filePath)
{
    m_logFilePath = filePath;
    m_fileLogEnabled = true;
}

void CLogUtils::DisableFileLog()
{
    m_fileLogEnabled = false;
    m_logFilePath.Empty();
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

void CLogUtils::AppendToFile(LPCTSTR text)
{
    if (!m_fileLogEnabled || m_logFilePath.IsEmpty())
        return;

    // Ensure directory exists
    CString dir = m_logFilePath.Left(m_logFilePath.ReverseFind(_T('\\')));
    ::CreateDirectory(dir, nullptr);

    FILE* f = nullptr;
    _tfopen_s(&f, m_logFilePath, _T("a"));
    if (f)
    {
        _fputts(text, f);
        fclose(f);
    }
}
