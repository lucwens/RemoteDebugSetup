#include "pch.h"
#include "SettingsUtils.h"
#include <ShlObj.h>

CString CSettingsUtils::GetSettingsDir()
{
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    CString dir(exePath);
    dir = dir.Left(dir.ReverseFind(_T('\\')));
    return dir + _T("\\Settings");
}

static CString JsonEscape(const CString& input)
{
    CString result;
    for (int i = 0; i < input.GetLength(); i++)
    {
        TCHAR ch = input[i];
        if (ch == _T('"'))       result += _T("\\\"");
        else if (ch == _T('\\')) result += _T("\\\\");
        else if (ch == _T('\n')) result += _T("\\n");
        else if (ch == _T('\r')) result += _T("\\r");
        else if (ch == _T('\t')) result += _T("\\t");
        else                     result += ch;
    }
    return result;
}

static CString JsonUnescape(const CString& input)
{
    CString result;
    for (int i = 0; i < input.GetLength(); i++)
    {
        if (input[i] == _T('\\') && i + 1 < input.GetLength())
        {
            TCHAR next = input[i + 1];
            if (next == _T('"'))       { result += _T('"');  i++; }
            else if (next == _T('\\')) { result += _T('\\'); i++; }
            else if (next == _T('n'))  { result += _T('\n'); i++; }
            else if (next == _T('r'))  { result += _T('\r'); i++; }
            else if (next == _T('t'))  { result += _T('\t'); i++; }
            else                       result += input[i];
        }
        else
        {
            result += input[i];
        }
    }
    return result;
}

// Find the closing quote, skipping escaped quotes. Returns index or -1.
static int FindClosingQuote(const CString& line, int startAfterQuote)
{
    for (int i = startAfterQuote; i < line.GetLength(); i++)
    {
        if (line[i] == _T('\\')) { i++; continue; }
        if (line[i] == _T('"'))  return i;
    }
    return -1;
}

bool CSettingsUtils::Save(LPCTSTR filePath, const std::map<CString, CString>& settings)
{
    // Ensure directory exists
    CString dir(filePath);
    dir = dir.Left(dir.ReverseFind(_T('\\')));
    SHCreateDirectoryEx(nullptr, dir, nullptr);

    CStdioFile file;
    if (!file.Open(filePath, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
        return false;

    file.WriteString(_T("{\n"));

    int index = 0;
    int total = (int)settings.size();
    for (auto it = settings.begin(); it != settings.end(); ++it, ++index)
    {
        CString line;
        line.Format(_T("  \"%s\": \"%s\"%s\n"),
                    (LPCTSTR)JsonEscape(it->first),
                    (LPCTSTR)JsonEscape(it->second),
                    (index < total - 1) ? _T(",") : _T(""));
        file.WriteString(line);
    }

    file.WriteString(_T("}\n"));
    file.Close();
    return true;
}

bool CSettingsUtils::Load(LPCTSTR filePath, std::map<CString, CString>& settings)
{
    CStdioFile file;
    if (!file.Open(filePath, CFile::modeRead | CFile::typeText))
        return false;

    settings.clear();
    CString line;
    while (file.ReadString(line))
    {
        line.Trim();
        if (line.IsEmpty() || line == _T("{") || line == _T("}"))
            continue;

        // Parse "key": "value" [,]
        int q1 = line.Find(_T('"'));
        if (q1 < 0) continue;
        int q2 = FindClosingQuote(line, q1 + 1);
        if (q2 < 0) continue;
        CString key = JsonUnescape(line.Mid(q1 + 1, q2 - q1 - 1));

        int colon = line.Find(_T(':'), q2);
        if (colon < 0) continue;
        int q3 = line.Find(_T('"'), colon);
        if (q3 < 0) continue;
        int q4 = FindClosingQuote(line, q3 + 1);
        if (q4 < 0) continue;
        CString value = JsonUnescape(line.Mid(q3 + 1, q4 - q3 - 1));

        settings[key] = value;
    }

    file.Close();
    return true;
}
