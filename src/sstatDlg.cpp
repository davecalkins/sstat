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

// sstatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "sstat.h"
#include "sstatDlg.h"
#include ".\sstatdlg.h"

#include <fstream>
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define IMG_WIDTH 16
#define IMG_HEIGHT 16

#define CFG_MAXLINE_LEN       4096
#define CFG_SEP               ","
#define CFG_TITLE_STR         "TITLE"
#define CFG_WIDTH_STR         "WIDTH"
#define CFG_HEIGHT_STR        "HEIGHT"
#define CFG_SERVER_STR        "SERVER"
#define CFG_UPDATE_PERIOD_STR "UPDATEPERIOD"
#define CFG_TIMEOUT_STR       "TIMEOUT"
#define CFG_ENABLELOGGING_STR "ENABLELOGGING"
#define CFG_LOGFILE_STR       "LOGFILE"
#define CFG_ENABLESOUNDS_STR  "ENABLESOUNDS"
#define CFG_DOWNSOUND_STR     "DOWNSOUND"
#define CFG_UPSOUND_STR       "UPSOUND"

#define MIN_WIDTH 50
#define MIN_HEIGHT 50

#define MIN_UPDATE_PERIOD 5000
#define MIN_TIMEOUT       1000

#define MONITOR_THREAD_PERIOD 250

#define ICON_UID         101
#define ICON_MSG         WM_APP + ICON_UID
#define ICON_TIP_LOADING "initializing"

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CsstatDlg dialog


bool CsstatDlg::enableSounds = false;
string CsstatDlg::initialSound;
string CsstatDlg::downSound;
string CsstatDlg::upSound;

CsstatDlg::CsstatDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CsstatDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    lastUpdate = 0;
	enableLogging = false;
    selectedServer = NULL;
}

void CsstatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATUSLIST, m_statusList);
}

BEGIN_MESSAGE_MAP(CsstatDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_NOTIFY(NM_DBLCLK, IDC_STATUSLIST, OnNMDblclkStatuslist)
END_MESSAGE_MAP()

class mycmdline : public CCommandLineInfo
{
public:
    mycmdline()
    {
        numArgs = 0;
    }

    virtual void ParseParam(const char * pszParam, BOOL bFlag, BOOL bLast)
    {
        numArgs++;
        string arg = pszParam;
        args.push_back(arg);
    }

    int numArgs;
    vector<string> args;
};

// CsstatDlg message handlers

BOOL CsstatDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add extra initialization here
    WSAData wd;
    WSAStartup(MAKEWORD(2,0),&wd);

    CRect wr,lr;
    GetWindowRect(wr);
    m_statusList.GetWindowRect(lr);
    sldx = wr.Width() - lr.Width();
    sldy = wr.Height() - lr.Height();

    m_bmpUnknown.LoadBitmap(IDB_UNKNOWN);
    m_bmpDown.LoadBitmap(IDB_DOWN);
    m_bmpUp.LoadBitmap(IDB_UP);

    m_bmpUnknownSel.LoadBitmap(IDB_UNKNOWN_SEL);
    m_bmpDownSel.LoadBitmap(IDB_DOWN_SEL);
    m_bmpUpSel.LoadBitmap(IDB_UP_SEL);

    COLORREF clr = RGB(0,0,0);
    m_imgList.Create(IMG_WIDTH,IMG_HEIGHT,0,1,1);
    m_imgList.Add(&m_bmpUnknown,clr);
    m_imgList.Add(&m_bmpDown,clr);
    m_imgList.Add(&m_bmpUp,clr);
    m_imgList.Add(&m_bmpUnknownSel,clr);
    m_imgList.Add(&m_bmpDownSel,clr);
    m_imgList.Add(&m_bmpUpSel,clr);

    m_statusList.SetImageList(&m_imgList,LVSIL_NORMAL);

    configFile = "sstat.cfg";
    mycmdline mcl;
    AfxGetApp()->ParseCommandLine(mcl);
    if (mcl.numArgs > 0)
    {
        configFile = mcl.args[0];
    }

    trayIcons[0] = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_ICON_UNK));
    trayIcons[1] = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_ICON_DOWN));
    trayIcons[2] = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_ICON_UP));

	ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
	nid.cbSize				= sizeof(NOTIFYICONDATA);
	nid.hWnd				= GetSafeHwnd();
	nid.uID					= ICON_UID;
	nid.uCallbackMessage	= ICON_MSG;
	nid.hIcon				= trayIcons[0];
	nid.uFlags				= NIF_ICON | NIF_MESSAGE | NIF_TIP;
	strcpy(nid.szTip,ICON_TIP_LOADING);
	Shell_NotifyIcon(NIM_ADD,&nid);

    CString errText;
    if (!loadConfig(errText))
    {
        CString s;
        s.Format("sstat: Initialization Error\n=> %s", errText);
        AfxMessageBox(s);
        PostMessage(WM_QUIT,0,0);
    }

	if (enableSounds)
	{
		sndPlaySound(0,0);
	}

    CString sectionName;
    sectionName.Format("settings-%s", instanceTitle.c_str());

    CString ps;
    ps = AfxGetApp()->GetProfileString(sectionName,"selectedServer","");
    selectedServer = NULL;
    if (ps.GetLength() > 0)
    {
        list<ServerInfo*>::iterator i;
        for (i = servers.begin(); i != servers.end(); ++i)
        {
            if (ps == ((*i)->displayName).c_str())
            {
                selectedServer = *i;
                break;
            }
        }

        if (selectedServer == NULL)
        {
            ps = "";
        }
    }

    if (ps.GetLength() == 0)
    {
        if (servers.size() > 0)
        {
            selectedServer = *(servers.begin());
            ps = selectedServer->displayName.c_str();
        }
    }

    if (selectedServer != NULL)
    {
        mutexguard m(selectedServer->m);
        selectedServer->currentUpdate++;
    }

    AfxGetApp()->WriteProfileString(sectionName,"selectedServer",ps);

    DWORD threadID;
    monitorThread = CreateThread(0,0,MonitorThread,(LPVOID)this,0,&threadID);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CsstatDlg::OnDestroy()
{
    TerminateThread(monitorThread,0);
    CloseHandle(monitorThread);
    
	ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
	nid.cbSize				= sizeof(NOTIFYICONDATA);
	nid.hWnd				= GetSafeHwnd();
	nid.uID					= ICON_UID;
	Shell_NotifyIcon(NIM_DELETE,&nid);

    list<ServerInfo*>::iterator i;
    for (i = servers.begin(); i != servers.end(); ++i)
    {
        delete *i;
    }

    CDialog::OnDestroy();
}

void CsstatDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CsstatDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CsstatDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CsstatDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if ((NULL != GetSafeHwnd()) &&
        (NULL != m_statusList.GetSafeHwnd()))
    {
        CRect wr;
        GetWindowRect(wr);

        int sx = wr.Width()-sldx;
        int sy = wr.Height()-sldy;

        if (sx < 0) sx = 0;
        if (sy < 0) sy = 0;

        m_statusList.SetWindowPos(0,0,0,sx,sy,SWP_NOZORDER|SWP_NOMOVE);

        if (nType == SIZE_MINIMIZED)
        {
            ShowWindow(SW_HIDE);
        }
    }
}

bool CsstatDlg::loadConfig(CString & errText)
{
    bool result = true;

    map<string,int> validToks;
    validToks[CFG_TITLE_STR] = 2;
    validToks[CFG_WIDTH_STR] = 2;
    validToks[CFG_HEIGHT_STR] = 2;
    validToks[CFG_SERVER_STR] = 4;
    validToks[CFG_UPDATE_PERIOD_STR] = 2;
    validToks[CFG_TIMEOUT_STR] = 2;
    validToks[CFG_ENABLELOGGING_STR] = 2;
    validToks[CFG_LOGFILE_STR] = 2;
    validToks[CFG_ENABLESOUNDS_STR] = 2;
    validToks[CFG_DOWNSOUND_STR] = 2;
    validToks[CFG_UPSOUND_STR] = 2;

    try
    {
        ifstream f(configFile.c_str());
        if (f.good())
        {
            char buf[CFG_MAXLINE_LEN+1];

            int line = 0;
            int itemIdx = 0;
            int width,height;
            CRect wr;
            GetWindowRect(wr);
            width = wr.Width();
            height = wr.Height();

            while (!f.eof() && result)
            {
                memset(buf,0,CFG_MAXLINE_LEN+1);
                f.getline(buf,CFG_MAXLINE_LEN);
                line++;

                if ((strlen(buf) > 0) &&
                    (buf[0] != '#'))
                {
                    vector<string> toks = tokenizeString(buf,CFG_SEP);
                    if (toks.size() > 0)
                    {
                        map<string,int>::iterator i;
                        i = validToks.find(toks[0]);
                        if (i != validToks.end())
                        {
                            if (toks.size() == i->second)
                            {
                                // title line
                                if (strncmp(buf,CFG_TITLE_STR,strlen(CFG_TITLE_STR)) == 0)
                                {
                                    SetWindowText(toks[1].c_str());
                                    instanceTitle = toks[1];
                                }
                                // width line
                                else if (strncmp(buf,CFG_WIDTH_STR,strlen(CFG_WIDTH_STR)) == 0)
                                {
                                    int w = atoi(toks[1].c_str());
                                    if (w >= MIN_WIDTH)
                                    {
                                        width = w;
                                    }
                                    else
                                    {
                                        result = false;
                                        errText.Format("(%s,line %d): width must be at least %d",
                                            configFile,line,MIN_WIDTH);
                                    }
                                }
                                // height line
                                else if (strncmp(buf,CFG_HEIGHT_STR,strlen(CFG_HEIGHT_STR)) == 0)
                                {
                                    int h = atoi(toks[1].c_str());
                                    if (h >= MIN_HEIGHT)
                                    {
                                        height = h;
                                    }
                                    else
                                    {
                                        result = false;
                                        errText.Format("(%s,line %d): height must be at least %d",
                                            configFile,line,MIN_HEIGHT);
                                    }
                                }
                                // update period line
                                else if (strncmp(buf,CFG_UPDATE_PERIOD_STR,strlen(CFG_UPDATE_PERIOD_STR)) == 0)
                                {
                                    updatePeriod = atoi(toks[1].c_str());
                                    if (updatePeriod < MIN_UPDATE_PERIOD)
                                    {
                                        result = false;
                                        errText.Format("(%s,line %d): update period must be at least %d",
                                            configFile,line,MIN_UPDATE_PERIOD);
                                    }
                                }
                                // timeout line
                                else if (strncmp(buf,CFG_TIMEOUT_STR,strlen(CFG_TIMEOUT_STR)) == 0)
                                {
                                    timeout = atoi(toks[1].c_str());
                                    if (timeout < MIN_TIMEOUT)
                                    {
                                        result = false;
                                        errText.Format("(%s,line %d): timeout must be at least %d",
                                            configFile,line,MIN_TIMEOUT);
                                    }
                                }
                                // enable logging line
                                else if (strncmp(buf,CFG_ENABLELOGGING_STR,strlen(CFG_ENABLELOGGING_STR)) == 0)
                                {
									enableLogging = (toks[1] == "1");
                                }
                                // logfile line
                                else if (strncmp(buf,CFG_LOGFILE_STR,strlen(CFG_LOGFILE_STR)) == 0)
                                {
									logFile = toks[1];
                                }
                                // enable sounds line
                                else if (strncmp(buf,CFG_ENABLESOUNDS_STR,strlen(CFG_ENABLESOUNDS_STR)) == 0)
                                {
									enableSounds = (toks[1] == "1");
                                }
                                // down sound line
                                else if (strncmp(buf,CFG_DOWNSOUND_STR,strlen(CFG_DOWNSOUND_STR)) == 0)
                                {
									downSound = toks[1];
                                }
                                // up sound line
                                else if (strncmp(buf,CFG_UPSOUND_STR,strlen(CFG_UPSOUND_STR)) == 0)
                                {
									upSound = toks[1];
                                }
                                // server line
                                else if (strncmp(buf,CFG_SERVER_STR,strlen(CFG_SERVER_STR)) == 0)
                                {
                                    int port = atoi(toks[3].c_str());
                                    if (port > 0)
                                    {
                                        ServerInfo * s = new ServerInfo;
                                        s->displayName = toks[1];
                                        s->host = toks[2];
                                        s->port = port;
                                        s->status = SS_UNKNOWN;

                                        s->itemIdx = m_statusList.InsertItem(itemIdx++,s->displayName.c_str(),(int)SS_UNKNOWN);

                                        servers.push_back(s);
                                    }
                                    else
                                    {
                                        result = false;
                                        errText.Format("(%s,line %d): invalid port specified",
                                            configFile,line);
                                    }
                                }
                            }
                            else
                            {
                                result = false;
                                errText.Format("(%s,line %d): incorrect # values",
                                    configFile,line);
                            }
                        }
                        else
                        {
                            result = false;
                            errText.Format("(%s,line %d): unrecognized line type",
                                configFile,line);
                        }
                    }
                    else
                    {
                        result = false;
                        errText.Format("(%s,line %d): badly formatted line",
                            configFile,line);
                    }
                }
            }

            if ((width != wr.Width()) ||
                (height != wr.Height()))
            {
                SetWindowPos(0,0,0,width,height,SWP_NOZORDER|SWP_NOMOVE);
            }

            f.close();
        }
        else
        {
            result = false;
            errText.Format("unable to open config file: %s",configFile);
        }
    }

    catch (...)
    {
        result = false;
        errText = "error while reading file";
    }

    return result;
}

vector<string> CsstatDlg::tokenizeString(char * str, char * sep)
{
    vector<string> toks;

    if ((str != NULL) && (sep != NULL))
    {
        char * tok;
        tok = strtok(str,sep);
        while (tok != NULL)
        {
            toks.push_back(tok);
            tok = strtok(NULL,sep);
        }
    }

    return toks;
}

DWORD WINAPI CsstatDlg::MonitorThread(LPVOID lpParm)
{
    CsstatDlg * self = (CsstatDlg*)lpParm;

    while(true)
    {
        list<ServerInfo*>::iterator i;

        for (i = self->servers.begin(); i != self->servers.end(); ++i)
        {
            bool killThread = false;
            HANDLE hThread = NULL;
            SOCKET socket = INVALID_SOCKET;

            {
                mutexguard m((*i)->m);

                if ((*i)->thread != NULL)
                {
                    if ((GetTickCount() - self->lastUpdate) > self->timeout)
                    {
                        killThread = true;
                        hThread = (*i)->thread;
                        (*i)->thread = NULL;
                        socket = (*i)->socket;
                        (*i)->socket = INVALID_SOCKET;
                    }
                }
            }

            if (killThread)
            {
                if (hThread != NULL)
                {
                    TerminateThread(hThread,0);                    
                    CloseHandle(hThread);
                }

                if (socket != INVALID_SOCKET)
                {
                    shutdown(socket,SD_BOTH);
                    closesocket(socket);
                    CloseHandle((HANDLE)socket);
                }

				ServerStatus prevStatus;

                {
                    mutexguard m((*i)->m);

					prevStatus = (*i)->status;
                    (*i)->status = SS_DOWN;
                    (*i)->currentUpdate++;
                }

				if (enableSounds)
				{
					if (prevStatus != SS_DOWN)
					{
						sndPlaySound(0,0);
						sndPlaySound(downSound.c_str(),SND_ASYNC | SND_NODEFAULT);
					}
				}
            }
        }

		int numUpdates = 0;

        for (i = self->servers.begin(); i != self->servers.end(); ++i)
        {
            bool doUpdate = false;
            ServerInfo * info;

            {
                mutexguard m((*i)->m);

                if ((*i)->currentUpdate > (*i)->lastUpdate)
                {
                    doUpdate = true;
					numUpdates++;
                    info = *i;
                    (*i)->lastUpdate = (*i)->currentUpdate;
                }
            }

            if (doUpdate)
            {
                LVITEM i;
                memset(&i,0,sizeof(LVITEM));
                i.mask = LVIF_IMAGE;
                i.iItem = info->itemIdx;
                i.iSubItem = 0;
                i.iImage = (int)info->status;
                if (self->selectedServer == info)
                {
                    i.iImage += 3;

		            ZeroMemory(&self->nid,sizeof(NOTIFYICONDATA));
		            self->nid.cbSize				= sizeof(NOTIFYICONDATA);
		            self->nid.hWnd			     = self->GetSafeHwnd();
		            self->nid.uID					= ICON_UID;
		            self->nid.hIcon				= self->trayIcons[(int)info->status];
		            self->nid.uFlags				= NIF_ICON | NIF_TIP;
                    strcpy(self->nid.szTip,info->displayName.c_str());
                    Shell_NotifyIcon(NIM_MODIFY,&self->nid);
                }
                self->m_statusList.SetItem(&i);
            }
        }

		if (numUpdates > 0)
		{
			self->LogServerStates();
		}

        if ((GetTickCount() - self->lastUpdate) > self->updatePeriod)
        {
            for (i = self->servers.begin(); i != self->servers.end(); ++i)
            {
                DWORD threadID;
                HANDLE hThread = CreateThread(0,0,ServerStatusThread,(LPVOID)*i,CREATE_SUSPENDED,&threadID);
                if (hThread == NULL)
                {
                    DWORD err = GetLastError();
                    TRACE1("CreateThread() failed, GetLastError() == %d\n", err);
                }

                {
                    mutexguard m((*i)->m);
                    (*i)->thread = hThread;
                }
                
                ResumeThread(hThread);
                CloseHandle(hThread);
            }

            self->lastUpdate = GetTickCount();
        }

        Sleep(MONITOR_THREAD_PERIOD);
    }

    return 0;
}

DWORD WINAPI CsstatDlg::ServerStatusThread(LPVOID lpParm)
{
    ServerInfo * server = (ServerInfo*)lpParm;

    string host;
    int port;
	ServerStatus prevStatus,newStatus;
    HANDLE hThread = NULL;

    {
        mutexguard m(server->m);

        host = server->host;
        port = server->port;
		prevStatus = server->status;
        hThread = server->thread;
    }

    bool serverAvailable = false;
    hostent * he = gethostbyname(host.c_str());
    if (he != 0)
    {
        SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
        if (sock != INVALID_SOCKET)
        {
            {
                mutexguard m(server->m);
                server->socket = sock;
            }

            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons((u_short)port);
            memcpy(&addr.sin_addr,he->h_addr_list[0],sizeof(in_addr));

            if (connect(sock,(struct sockaddr *)&addr,sizeof(sockaddr)) != SOCKET_ERROR)
            {
                serverAvailable = true;
            }

            {
                mutexguard m(server->m);
                shutdown(sock,SD_BOTH);
                closesocket(sock);
                server->socket = INVALID_SOCKET;
            }
        }
    }

    {
        mutexguard m(server->m);

        if (serverAvailable)
        {
            server->status = SS_UP;
        }
        else
        {
            server->status = SS_DOWN;
        }

		newStatus = server->status;

		if (newStatus != prevStatus)
		{
			server->currentUpdate++;
		}

		server->thread = NULL;
    }

	if (enableSounds)
	{
		if (prevStatus != SS_UNKNOWN)
		{
			if (prevStatus != newStatus)
			{
				if (newStatus == SS_UP)
				{
					sndPlaySound(0,0);
					sndPlaySound(upSound.c_str(),SND_ASYNC | SND_NODEFAULT);
				}
				else if (newStatus == SS_DOWN)
				{
					sndPlaySound(0,0);
					sndPlaySound(downSound.c_str(),SND_ASYNC | SND_NODEFAULT);
				}
			}
		}
    }

    return 0;
}

void CsstatDlg::LogServerStates()
{
	if (!enableLogging)
	{
		return;
	}

	try
	{
		bool fileExists = false;

		try
		{
	        ifstream tf(logFile.c_str());
		    if (tf.good())
			{
				fileExists = true;
				tf.close();
			}
		}
		
		catch (...)
		{
		}

		list<ServerInfo*>::iterator i;

		ofstream f(logFile.c_str(),ios::out|ios::app);
        if (f.good())
        {
			if (!fileExists)
			{
				f << "Time (# secs since epoch)";

				for (i = servers.begin(); i != servers.end(); ++i)
				{
					f << "," << (*i)->displayName;
				}

				f << endl;
			}

			f << (long)time(0);

			for (i = servers.begin(); i != servers.end(); ++i)
			{
				f << ",";

				if ((*i)->status == SS_DOWN)
				{
					f << "0";
				}
				else if ((*i)->status == SS_UP)
				{
					f << "1";
				}
				else if ((*i)->status == SS_UNKNOWN)
				{
					f << "?";
				}
			}

			f << endl << flush;
            f.close();
		}
	}

	catch (...)
	{
	}
}

void CsstatDlg::OnNMDblclkStatuslist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);

    list<ServerInfo*>::iterator i;
    for (i = servers.begin(); i != servers.end(); ++i)
    {
        if ((*i)->itemIdx == phdr->iItem)
        {
            if (selectedServer != (*i))
            {
                if (selectedServer != NULL)
                {
                    mutexguard m(selectedServer->m);

                    selectedServer->currentUpdate++;
                    selectedServer = (*i);
                }
                else
                {
                    selectedServer = (*i);
                }

                {
                    mutexguard m(selectedServer->m);

                    selectedServer->currentUpdate++;                    
                }

                CString sectionName;
                sectionName.Format("settings-%s", instanceTitle.c_str());
                AfxGetApp()->WriteProfileString(sectionName,"selectedServer",selectedServer->displayName.c_str());
            }

            break;
        }
    }

    *pResult = 0;
}

LRESULT CsstatDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if ((message == ICON_MSG) &&
        ((lParam == WM_LBUTTONUP) ||
         (lParam == WM_RBUTTONUP)))
    {
        ShowWindow(SW_SHOW);
        ShowWindow(SW_RESTORE);
    }

    return CDialog::WindowProc(message, wParam, lParam);
}
