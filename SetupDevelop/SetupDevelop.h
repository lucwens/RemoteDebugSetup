#pragma once

#ifndef __AFXWIN_H__
    #error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

// CSetupDevelopApp:
// See SetupDevelop.cpp for the implementation of this class
//

class CSetupDevelopApp : public CWinApp
{
public:
    CSetupDevelopApp();

// Overrides
public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

extern CSetupDevelopApp theApp;
