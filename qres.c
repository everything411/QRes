#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qres.h"

static char szMsg[800];

// Windows main. Note that we do not have a message loop.
// All error messages etc. are shown in message boxes and the
// program does not require a GUI. This keeps it nice and small
// and guarantees good performance on all systems.
//
int WINAPI WinMain(
    HINSTANCE hInstance,     // Instance handle
    HINSTANCE hPrevInstance, // Previous instance handle
    LPSTR lpszCmdLine,       // Command line string
    int cmdShow)             // ShowWindow flag
{
    QUICKRES tQRes;                // QuickRes application data
    QRES_PARS tCmdPars, tKeepPars; // User-specified parameters
    int iResult;
    MSG tMsg;

    // Get rid of the process feedback cursor (this is a bit dirty).
    // Windows 95 and NT normally display the feedback cursor until
    // a program starts its message loop. Since qres doesn't have one
    // this would mean it would be displayed for the maximum period of
    // 7 seconds when qres is started, confusing.
    if (PeekMessage(&tMsg, (HWND)0, 0, 0, PM_NOREMOVE))
        GetMessage(&tMsg, (HWND)0, 0, 0);

    memset(&tQRes, 0, sizeof(tQRes));
    memset(&tCmdPars, 0, sizeof(tCmdPars));

    // Get command line parameters
    iResult = GetQResPars(lpszCmdLine, &tCmdPars);
    if (iResult <= 0)
    {
        strcpy(szMsg, tCmdPars.szErr);
        strcat(szMsg, "QResNT version 0.0.0.1\n"
                      "Copyright 1998-2000 Berend Engelbrecht - einhard@wxs.nl\n"
                      "Modyfied by everything411 - everything411@163.com\n"
                      "syntax: qres options [program name]\n"
                      "where options is one or more of the following:\n"
                      " X=n - set horizontal pixels\n"
                      " Y=n - set vertical pixels\n"
                      " C=n - set bits/pixel or number of colors\n"
                      " /R  - restore mode after program finishes\n"
                      " /D  - Direct screen mode switch. (always used)\n"
                      " f=n - Switch to n Hertz display refresh rate.\n");
        MessageBox(NULL, szMsg, "QResNT: screen mode switch", MB_OK);
        return 0;
    }

    tKeepPars = tCmdPars;

    do // while (tCmdPars.dwFlags & QF_SCRSAVER)
    {
        tCmdPars = tKeepPars;

        // Complete missing parameters
        CompleteQResPars(&tCmdPars);

        // 970405: New delay before looking for QuickRes
        if (tCmdPars.wQRDelay)
            Sleep(100 * tCmdPars.wQRDelay);
        else
            Sleep(100); // minimum delay

        if (!(tCmdPars.dwFlags & QF_NOSWITCH))
        {
            if (!SwitchQResMode(&tCmdPars.mNew, &tQRes))
            {
                sprintf(szMsg, "Screen mode not found: x=%ld y=%ld c=%d",
                        tCmdPars.mNew.dwXRes, tCmdPars.mNew.dwYRes,
                        tCmdPars.mNew.wBitsPixel);
                MessageBox(NULL, szMsg, "qres", 0);
                return 0;
            }
        }

        // Finally run the app if one was specified
        if (tCmdPars.szApp[0])
        {
            STARTUPINFO tStartup;
            PROCESS_INFORMATION tProcess;
            DWORD dwExitCode;

            memset(&tStartup, 0, sizeof(STARTUPINFO));
            tStartup.cb = sizeof(STARTUPINFO);
            tStartup.dwFlags = STARTF_USESHOWWINDOW;
            tStartup.wShowWindow = (WORD)cmdShow;

            if (tCmdPars.wAppDelay)
                Sleep(100 * tCmdPars.wAppDelay);

            if (CreateProcess(NULL,
                              tCmdPars.szApp,           // pointer to command line string
                              NULL,                     // pointer to process security attributes
                              NULL,                     // pointer to thread security attributes
                              FALSE,                    // handle inheritance flag
                              CREATE_NEW_PROCESS_GROUP, // creation flags
                              NULL,                     // pointer to new environment block
                              NULL,                     // pointer to current directory name
                              &tStartup,                // pointer to STARTUPINFO
                              &tProcess                 // pointer to PROCESS_INFORMATION
                              ))
            {
                if (tCmdPars.dwFlags & QF_RESTORE)
                {
                    // Wait for app to quit
                    while (GetExitCodeProcess(tProcess.hProcess, &dwExitCode))
                    {
                        if (dwExitCode != STILL_ACTIVE)
                            break;
                        Sleep(1000);
                    }
                }

                // Detach child process
                CloseHandle(tProcess.hThread);
                CloseHandle(tProcess.hProcess);
            }
            else
            {
                sprintf(szMsg, "Cannot run %s", tCmdPars.szApp);
                MessageBox(NULL, szMsg, "QResNT", 0);
            }
            // Switch back to old mode
            if (tCmdPars.dwFlags & QF_RESTORE)
            {
                SwitchQResMode(&tCmdPars.mOld, &tQRes);
            }
        }
    } while (tCmdPars.dwFlags & QF_SCRSAVER);
    return 0;
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

// GetCurQResMode returns the current QuickRes settings for the
// screen in *ptMode.
//
void GetCurQResMode(QRMODE *ptMode)
{
    HWND hwndScreen = GetDesktopWindow();
    HDC hdcScreen = GetDC(hwndScreen);

    ptMode->dwXRes = (DWORD)GetDeviceCaps(hdcScreen, DESKTOPHORZRES);
    ptMode->dwYRes = (DWORD)GetDeviceCaps(hdcScreen, DESKTOPVERTRES);
    ptMode->wBitsPixel = (UINT)GetDeviceCaps(hdcScreen, BITSPIXEL);
    ptMode->wFreq = (UINT)GetDeviceCaps(hdcScreen, VREFRESH);

    ReleaseDC(hwndScreen, hdcScreen);
}

// SwitchQResMode calls SwitchQResNT to switch to the requested mode
BOOL SwitchQResMode(QRMODE *ptReqMode, QUICKRES *ptQRes)
{
    return SwitchQResNT(ptReqMode);
}

// OSVERSIONINFO GetWindowsVersion()
// {
//     OSVERSIONINFO ovi;
//     ovi.dwOSVersionInfoSize = sizeof(ovi);
//     GetVersionEx(&ovi);
//     return ovi;
// }
