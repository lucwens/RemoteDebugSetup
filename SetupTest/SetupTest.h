#pragma once

#ifndef __AFXWIN_H__
    #error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

// CSetupTestApp:
// See SetupTest.cpp for the implementation of this class
//

class CSetupTestApp : public CWinApp
{
public:
    CSetupTestApp();

// Overrides
public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

extern CSetupTestApp theApp;
