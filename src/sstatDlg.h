//******************************************************************************
// sstat server monitoring tool Copyright (C) 2004 coder_1024
// (Email: coder_1024@hotmail.com)
// 
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option) any
// later version.
// 
// This library is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License along
// with this library; if not, write to the
// 
// Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330,
// Boston, MA 02111-1307 USA
//******************************************************************************

// sstatDlg.h : header file
//

#pragma once
#include "afxcmn.h"

#include <string>
#include <list>
#include <vector>
using namespace std;

class mutex
{
public:
    mutex()
    {
        InitializeCriticalSection(&critSec);
    }

    ~mutex()
    {
        unlock();
    }

    void lock()
    {
        EnterCriticalSection(&critSec);
    }

    void unlock()
    {
        LeaveCriticalSection(&critSec);
    }

protected:
    CRITICAL_SECTION critSec;
};

class mutexguard
{
public:
    mutexguard(mutex & m)
        : m(m)
    {
        m.lock();
    }

    ~mutexguard()
    {
        m.unlock();
    }

protected:
    mutex & m;
};

typedef enum ServerStatus
{
    SS_UNKNOWN = 0,
    SS_DOWN    = 1,
    SS_UP      = 2
};

#define LEN_SERVERSTATUS 3

class ServerInfo
{
public:
    ServerInfo()
    {
        port = -1;
        status = SS_UNKNOWN;
        lastUpdate = currentUpdate = 0;
        itemIdx = -1;
        thread = NULL;
        socket = INVALID_SOCKET;
    }

    string displayName;
    string host;
    int port;

    ServerStatus status;
    long lastUpdate, currentUpdate;
    int itemIdx;

    HANDLE thread;
    SOCKET socket;
    mutex m;

};

// CsstatDlg dialog
class CsstatDlg : public CDialog
{
// Construction
public:
    CsstatDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    enum { IDD = IDD_SSTAT_DIALOG };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


// Implementation
protected:
    int sldx, sldy;
    list<ServerInfo*> servers;
    ServerInfo * selectedServer;
    long updatePeriod, timeout;
    long lastUpdate;
	static bool enableSounds;
	static string initialSound, downSound, upSound;
	bool enableLogging;
	string logFile;
    string configFile;
    HANDLE monitorThread;
    
    string instanceTitle;
    NOTIFYICONDATA nid;

    CBitmap m_bmpUnknown, m_bmpDown, m_bmpUp;
    CBitmap m_bmpUnknownSel, m_bmpDownSel, m_bmpUpSel;
    CImageList m_imgList;

    HICON trayIcons[LEN_SERVERSTATUS];

    bool loadConfig(CString & errText);
    vector<string> tokenizeString(char * str, char * sep);

    static DWORD WINAPI MonitorThread(LPVOID lpParm);
    static DWORD WINAPI ServerStatusThread(LPVOID lpParm);

    void LogServerStates();

    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    CListCtrl m_statusList;
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    afx_msg void OnNMDblclkStatuslist(NMHDR *pNMHDR, LRESULT *pResult);
protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};
