/*********************************************************************/
/* qres - Windows 95 quickres requestor.                             */
/*        This program provides an automated interface to Microsofts */
/*        QuickRes screen mode switch applet.                        */
/*                                                                   */
/* Author: Berend Engelbrecht                                        */
/*                                                                   */
/* Revision History:                                                 */
/*-------------------------------------------------------------------*/
/*YYMMDD| Author         | Reason                        | Version   */
/*------+----------------+-------------------------------+-----------*/
/*970102| B.Engelbrecht  | Created                       | 0.1.0.0   */
/*970110| B.Engelbrecht  | Added /R parameter            | 0.2.0.0   */
/*970308| B.Engelbrecht  | First NT beta                 | 1.0.5.0   */
/*970315| B.Engelbrecht  | Support display frequency     | 1.0.6.0   */
/*970405| B.Engelbrecht  | Extra switches                | 1.0.7.0   */
/*971212| B.Engelbrecht  | Win98 stuff                   | 1.0.8.2   */
/*050109| B.Engelbrecht  | Modifications for Borland C++ | 1.0.9.7   */
/*      |                |                               |           */
/*                                                                   */
/* Copyright 1997-2005 Berend Engelbrecht. All rights reserved.      */
/*********************************************************************/
#if defined(__BORLANDC__)
#pragma warn - 8080 // suppress warnings for _str functions in BCPP 6.0
#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qreslib.h"

#ifndef DM_POSITION
#define DM_POSITION 0x00000020L
#endif

// GetWinVer returns one of the WVER_... constants to
// indicate the detected Windows version
int GetWinVer(void)
{
    OSVERSIONINFO tVerInfo;
    char szVer[80];

    tVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&tVerInfo) ||
        (tVerInfo.dwMajorVersion < 4))
        return WVER_UNSUPPORTED;

    if (tVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        return WVER_NT;

    if ((tVerInfo.dwMajorVersion == 4) &&
        (tVerInfo.dwMinorVersion == 0) &&
        (tVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
        return (tVerInfo.szCSDVersion[1] == 'B') ? WVER_OSR2 : WVER_95;

    if ((tVerInfo.dwMajorVersion == 4) &&
        (tVerInfo.dwMinorVersion == 10) &&
        (tVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
        return WVER_98;

    if ((tVerInfo.dwMajorVersion == 4) &&
        (tVerInfo.dwMinorVersion == 90) &&
        (tVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
        return WVER_ME;

    sprintf(szVer, "%ld.%ld %ld", tVerInfo.dwMajorVersion,
            tVerInfo.dwMinorVersion, tVerInfo.dwPlatformId);
    MessageBox(NULL, szVer, "qres", 0);
    return WVER_UNKNOWN;
}

void SetRegValue(HKEY hKey, LPCTSTR lpSubKey,
                 LPCTSTR lpValueName, LPCTSTR lpValue)
{
    if (RegOpenKeyEx(hKey, lpSubKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        if (lpValue)
        {
            RegSetValueEx(hKey, lpValueName, 0, REG_SZ, (const BYTE *)lpValue, strlen(lpValue) + 1);
        }
        else
        {
            RegDeleteValue(hKey, lpValueName);
        }
        RegCloseKey(hKey);
    }
}

// IsQResDlgOpen returns TRUE if the specified dialog window
// is open and belongs to the same process as the quickres
// application.
//
BOOL IsQResDlgOpen(QUICKRES *ptQRes, char *szDlgName)
{
    HWND hwndDlg;
    DWORD dwDlgProcess;

    hwndDlg = FindWindowEx(NULL, NULL, MAKEINTRESOURCE(32770U),
                           szDlgName);
    if (!hwndDlg)
        return FALSE;

    // Retrieve and compare the dlg process id, return TRUE if
    // it is equal to QuickRes process id
    GetWindowThreadProcessId(hwndDlg, &dwDlgProcess);

    return (ptQRes->dwProcess == dwDlgProcess);
}

// GetCurQResMode returns the current QuickRes settings for the
// screen in *ptMode.
//
void GetCurQResMode(QRMODE *ptMode)
{
    HWND hwndScreen = GetDesktopWindow();
    HDC hdcScreen = GetDC(hwndScreen);

    ptMode->dwXRes = (DWORD)GetDeviceCaps(hdcScreen, HORZRES);
    ptMode->dwYRes = (DWORD)GetDeviceCaps(hdcScreen, VERTRES);
    ptMode->wBitsPixel = (UINT)GetDeviceCaps(hdcScreen, BITSPIXEL);
    if (GetWinVer() == WVER_NT)
        ptMode->wFreq = (UINT)GetDeviceCaps(hdcScreen, VREFRESH);

    ReleaseDC(hwndScreen, hdcScreen);
}

// CompleteQResPars fills out parameters not specified by the user
// based on the current screen mode. It also initialises ptParsRet->mOld
// and sets the QF_NOSWITCH flag if no mode switch is required.
void CompleteQResPars(QRES_PARS *ptParsRet)
{
    GetCurQResMode(&ptParsRet->mOld);
    if (!ptParsRet->mNew.dwXRes && !ptParsRet->mNew.dwYRes)
    {
        ptParsRet->mNew.dwXRes = ptParsRet->mOld.dwXRes;
        ptParsRet->mNew.dwYRes = ptParsRet->mOld.dwYRes;
    }
    else if (ptParsRet->mNew.dwXRes == ptParsRet->mOld.dwXRes)
    {
        if (!ptParsRet->mNew.dwYRes)
            ptParsRet->mNew.dwYRes = ptParsRet->mOld.dwYRes;
    }
    else if (ptParsRet->mNew.dwYRes == ptParsRet->mOld.dwYRes)
    {
        if (!ptParsRet->mNew.dwXRes)
            ptParsRet->mNew.dwXRes = ptParsRet->mOld.dwXRes;
    }
    if (!ptParsRet->mNew.wBitsPixel)
        ptParsRet->mNew.wBitsPixel = ptParsRet->mOld.wBitsPixel;

    // Compare the requested mode with the current one and raise
    // QF_NOSWITCH if they are identical
    if (!memcmp(&ptParsRet->mNew, &ptParsRet->mOld, sizeof(QRMODE)))
        ptParsRet->dwFlags |= QF_NOSWITCH;
    ptParsRet->mNew.dwFlags = ptParsRet->dwFlags; // New 20000522
    ptParsRet->mOld.dwFlags = ptParsRet->dwFlags; // New 20030907
}

// FindQuickResApp looks for the Microsoft QuickRes applet
// and returns its main window and other required data in
// *ptQRes. If the applet is found the return value is TRUE,
// otherwise FALSE.
//
BOOL FindQuickResApp(QUICKRES *ptQRes)
{
    int iWinVer = GetWinVer();
    memset(ptQRes, 0, sizeof(QUICKRES));

    if (iWinVer == WVER_UNSUPPORTED)
        return FALSE;

    // If we are running on NT 4.0 we don't need QuickRes
    if (iWinVer == WVER_NT)
        ptQRes->fDirect = TRUE;

    if (ptQRes->fDirect)
        return TRUE;

    // Attempt to start QuickRes, in case the user didn't
    // Note: RunDLL doesn't work for Win98 beta. Why?
    // [HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run]
    // "Taskbar Display Controls"="RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY"
    switch (iWinVer)
    {
    case WVER_95:
        WinExec("quickres", SW_NORMAL);
        break;
    case WVER_OSR2:
    case WVER_98:
    case WVER_ME:
        SetRegValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    "Taskbar Display Controls", "RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY");
        WinExec("RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY", SW_NORMAL);
        break;
    }

    // Wait briefly to let other app run its course
    Sleep(50);

    // This finds QuickRes on Win95 OSR2:
    ptQRes->hwnd = FindWindowEx(NULL, NULL, "SysQuickRes", "QuickRes");
    if (ptQRes->hwnd)
    {
        ptQRes->fOSR2 = TRUE;
        ptQRes->wFirstMenuID = 350;
    }
    else
    {
        ptQRes->hwnd = FindWindowEx(NULL, NULL, "QuickRes", "QuickRes");
        if (!ptQRes->hwnd)
            return FALSE;
        ptQRes->wFirstMenuID = 1000;
    }
    ptQRes->dwThread = GetWindowThreadProcessId(ptQRes->hwnd,
                                                &ptQRes->dwProcess);
    return TRUE;
}

// FindReqQResMode returns >= 0 if the requested mode is supported
// by the installed graphics driver, -1 if the mode is not found.
// The return value must be added to the menu base to get the
// quickres menu option ID for the requested mode.
//
static int FindReqQResMode(QRMODE *ptReqMode, BOOL fOSR2)
{
    DEVMODE tMode;
    DWORD dwMode = 0;
    int iMenu = 0;
    BOOL fFound = FALSE;

    while (EnumDisplaySettings(NULL, dwMode, &tMode))
    {
        if (!fOSR2 || (tMode.dmBitsPerPel >= 8))
        {
            if (tMode.dmBitsPerPel == (DWORD)ptReqMode->wBitsPixel)
            {
                if ((!ptReqMode->dwXRes ||
                     (ptReqMode->dwXRes == tMode.dmPelsWidth)) &&
                    (!ptReqMode->dwYRes ||
                     (ptReqMode->dwYRes == tMode.dmPelsHeight)))
                {
                    fFound = TRUE;
                    break;
                }
            }
            iMenu++;
        }
        dwMode++;
    }
    return fFound ? iMenu : -1;
}

// SwitchQResNT switches the mode without the help of QuickRes
// on Windows NT 4.x
BOOL SwitchQResNT(QRMODE *ptReqMode)
{
    DEVMODE tMode;
    DWORD dwMode = 0;
    DWORD dwFlags = 0; // 20000522

    while (EnumDisplaySettings(NULL, dwMode, &tMode))
    {
        if (tMode.dmBitsPerPel == (DWORD)ptReqMode->wBitsPixel)
        {
            if ((!ptReqMode->dwXRes ||
                 (ptReqMode->dwXRes == tMode.dmPelsWidth)) &&
                (!ptReqMode->dwYRes ||
                 (ptReqMode->dwYRes == tMode.dmPelsHeight)) &&
                (!ptReqMode->wFreq ||
                 ((DWORD)ptReqMode->wFreq == tMode.dmDisplayFrequency)))
            {
                // found matching mode
                tMode.dmSize = sizeof(DEVMODE);
                tMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;
                if (ptReqMode->wFreq)
                    tMode.dmFields |= DM_DISPLAYFREQUENCY;

                // 20000522 - new support for permanent mode switch on NT
                if (ptReqMode->dwFlags & (QF_SAVEUSER | QF_SAVEALL))
                {
                    dwFlags |= CDS_UPDATEREGISTRY;
                    if (ptReqMode->dwFlags & QF_SAVEALL)
                    {
                        dwFlags |= CDS_GLOBAL;
                    }
                }
                //MessageBox(NULL, "NT2", "qres", 0);
                dwFlags |= CDS_SET_PRIMARY;
                return (ChangeDisplaySettings(&tMode, dwFlags) == DISP_CHANGE_SUCCESSFUL);
            }
        }
        dwMode++;
    }
    return FALSE;
}

// SwitchQResMode calls FindReqQResMode to find requested mode,
// then requests mode switch from QuickRes
BOOL SwitchQResMode(QRMODE *ptReqMode, QUICKRES *ptQRes)
{
    int iMode;

    if (ptQRes->fDirect)
        return SwitchQResNT(ptReqMode);

    if (!ptQRes->hwnd)
        return FALSE;

    // Find QuickRes menu index for requested screen mode
    iMode = FindReqQResMode(ptReqMode, ptQRes->fOSR2);
    if (iMode < 0)
        return FALSE;
    iMode += ptQRes->wFirstMenuID;

    // Do the mode switch and briefly wait for the result
    PostMessage(ptQRes->hwnd, WM_COMMAND, (WPARAM)iMode, 0L);

    // Briefly wait for the result of the resolution switch.
    // If the 'compatibility warning' box (OSR2) is open, wait for
    // it to close.
    do
    {
        Sleep(900);
    } while (IsQResDlgOpen(ptQRes, ptQRes->fOSR2 ? "Compatibility Warning" : "QuickRes"));
    Sleep(100);
    return TRUE;
}

// GetQResPars parses the command line passed to qres.
// Return value is the number of distinct parameters found.
// The function returns -1 and sets ptParsRet->cErrPar if an
// unknown parameter is encountered.
//
int GetQResPars(LPSTR lpszCmdLine,    // Command line string
                QRES_PARS *ptParsRet) // Returns parameters
{
    int iTokens, iOfs;
    DWORD dwValue;
    char *szToken;
    static char szApp[512];

    memset(ptParsRet, 0, sizeof(QRES_PARS));
    strncpy(szApp, lpszCmdLine, sizeof(szApp));
    szApp[sizeof(szApp) - 1] = 0;
    for (szToken = strtok(lpszCmdLine, " "), iTokens = 0;
         szToken;
         szToken = strtok(NULL, " "), iTokens++)
    {
        // /X=nnn is alternate notation for X=nnn
        if ((strlen(szToken) > 2) && (szToken[0] == '/') &&
            (szToken[2] == '='))
            szToken++;

        // All numeric options have format A=value, no spaces allowed
        if ((strlen(szToken) > 1) && (szToken[1] == '='))
        {
            dwValue = strtoul(szToken + 2, NULL, 10);
            switch (toupper(*szToken))
            {
            case 'X':
                ptParsRet->mNew.dwXRes = dwValue;
                break;
            case 'Y':
                ptParsRet->mNew.dwYRes = dwValue;
                break;
            case 'C':
                // It is allowed to specify '256 colors' instead of
                // '8 bits color'.
                if (dwValue < 256)
                    ptParsRet->mNew.wBitsPixel = (UINT)dwValue;
                else if (dwValue == 256)
                    ptParsRet->mNew.wBitsPixel = 8;
                break;
            case 'F': // 970315: frequency, NT only
                ptParsRet->mNew.wFreq = (UINT)dwValue;
                break;
            case 'Q': // 970405: QRes delay in 1/10 sec
                ptParsRet->wQRDelay = (UINT)dwValue;
                break;
            case 'A': // 970405: App delay in 1/10 sec
                ptParsRet->wAppDelay = (UINT)dwValue;
                break;
            case 'S':
                switch ((UINT)dwValue)
                {
                case 1:
                    ptParsRet->dwFlags |= QF_SAVEUSER;
                    break;
                case 2:
                    ptParsRet->dwFlags |= QF_SAVEALL;
                    break;
                default:
                    sprintf(ptParsRet->szErr, "%s - option value not allowed\n\n", szToken);
                    return -1;
                }
                break;
            default:
                // unknown option
                sprintf(ptParsRet->szErr, "Unknown option: '%c'\n\n",
                        *szToken);
                return -1;
            }
        }
        else if ((strlen(szToken) > 1) && (szToken[0] == '/'))
        {
            // Boolean flags list starts with '/'
            while (*(++szToken))
            {
                switch (toupper(*szToken))
                {
                case 'R':
                    ptParsRet->dwFlags |= QF_RESTORE;
                    break;
                case 'D': // 970405: Direct switch (no QuickRes)
                    ptParsRet->dwFlags |= QF_DIRECT;
                    break;
                case 'H': // 020330: hide quickres after use
                    ptParsRet->dwFlags |= QF_HIDEQR;
                    break;
                case '$': // 970913: Check QuickRes
                    ptParsRet->dwFlags |= QF_CHECK;
                    break;
                default:
                    // unknown flag
                    sprintf(ptParsRet->szErr, "Unknown option: '%c'\n\n",
                            *szToken);
                    return -1;
                }
            }
        }
        else
        {
            // It is assumed that the file name of the application to
            // run directly follows the last option token.
            // Long file names with embedded spaces are OK, quotes not
            // necessary but allowed.
            iOfs = szToken - lpszCmdLine;
            if (iOfs > 0)
            {
                memmove(ptParsRet->szApp, szApp + iOfs,
                        sizeof(szApp) - iOfs);
                strcpy(szApp, ptParsRet->szApp);
                strlwr(szApp);
                szToken = strstr(szApp, ".scr");

                // .scr must be at end of string or followed by quote or
                // space.
                if (szToken && (!*(szToken + 4) || strchr("\"' ", *(szToken + 4))))
                    ptParsRet->dwFlags |= QF_SCRSAVER;
            }
            break; // stop parsing when file name is found
        }
    }

    return iTokens;
}

VOID CALLBACK QResMessageBoxCallback(LPHELPINFO lpHelpInfo)
{
    WinHelp(NULL, "qres.hlp", HELP_CONTEXT, lpHelpInfo->dwContextId);
}

// CheckQuickRes checks and reports on QuickRes installation
// Return value is TRUE if everything is OK.
// If bShowIfOK is FALSE no messagebox is shown if everything
// is OK.
BOOL CheckQuickRes(BOOL bShowIfOK)
{
    int iWinVer = GetWinVer();
    char *szVer = "Unknown Windows version\n"
                  "QRes may not work on this PC.";
    char *szApp = "";
    char szMsg[400];
    int iTopic = 10000; // QuickRes help topic
    UINT uStyle = MB_OK;
    BOOL bOk = TRUE;
    MSGBOXPARAMS tParams;

    // detect Windows version
    switch (iWinVer)
    {
    case WVER_UNSUPPORTED:
        szVer = "This version of Windows is not supported.\r\n"
                "QRes can not be used on your PC.";
        bOk = FALSE;
        break;
    case WVER_95:
        szVer = "Detected Windows 95 (old version).";
        uStyle |= MB_HELP;
        break;
    case WVER_OSR2:
        szVer = "Detected Windows 95 OSR2 with built-in QuickRes.";
        break;
    case WVER_98:
    case WVER_ME:
        szVer = "Detected Windows 98 or higher with built-in QuickRes.";
        break;
    case WVER_NT:
        szVer = "Detected Windows NT 4.0 or higher\r\n\r\n"
                "Microsoft QuickRes is not needed on your PC.";
        break;
    }

    if ((iWinVer != WVER_UNSUPPORTED) && (iWinVer != WVER_NT))
    {
        QUICKRES tApp;
        if (FindQuickResApp(&tApp))
        {
            szApp = "QuickRes is correctly installed on your PC.";
        }
        else if ((iWinVer == WVER_OSR2) || (iWinVer == WVER_98))
        {
            szApp = "Microsoft QuickRes is not enabled\r\n"
                    "on your PC. (see Help)";
            bOk = FALSE;
            iTopic = 10003; // Memphis help topic
            uStyle |= MB_HELP;
        }
        else
        {
            szApp = "Microsoft QuickRes is not or not correctly installed\r\n"
                    "on your PC.\r\n"
                    "Please download and install QuickRes (see Help)";
            bOk = FALSE;
            iTopic = 10001; // Powertoy help topic
            uStyle |= MB_HELP;
        }
    }

    if (bShowIfOK || !bOk)
    {
        if (iWinVer == WVER_UNSUPPORTED)
            uStyle |= MB_ICONERROR;
        else
            uStyle |= (bOk ? MB_ICONINFORMATION : MB_ICONWARNING);

        sprintf(szMsg, "%s\r\n\r\n%s", szVer, szApp);
        memset(&tParams, 0, sizeof(tParams));
        tParams.cbSize = sizeof(tParams);
        tParams.lpszText = szMsg;
        tParams.lpszCaption = "QuickRes Installation Check";
        tParams.dwStyle = uStyle;
        tParams.dwContextHelpId = iTopic;
        tParams.lpfnMsgBoxCallback = QResMessageBoxCallback;
        MessageBoxIndirect(&tParams);
    }

    return bOk;
}
