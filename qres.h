// dwFlags constants
#define QF_NOSWITCH 0x00000001 // Set if no mode switch required
#define QF_RESTORE 0x00000002  // Set if old mode must be restored
#define QF_DIRECT 0x00000004   // Set to switch without QuickRes
#define QF_CHECK 0x00000008    // Just check QuickRes setup
#define QF_SCRSAVER 0x00000010 // Set if app is a screen saver
#define QF_FINDEXE 0x00000020  // 981024: use FindExecutable
#define QF_SAVEUSER 0x00000040 // 000522: save new resolution for
//         current user
#define QF_SAVEALL 0x00000080 // 000522: save new resolution for
//         all users
#define QF_HIDEQR 0x00000100 // 020330: hide quickres after use

// QuickRes screen mode data
typedef struct
{
    DWORD dwXRes, dwYRes; // screen dimensions in pixels
    UINT wBitsPixel;      // color depth in bits per pixel
    UINT wFreq;           // refresh rate in hertz
    DWORD dwFlags;        // flags, see QF_* constants
} QRMODE;

// Structure holding parsed command line parameters
typedef struct
{
    QRMODE mNew;     // requested screen mode
    QRMODE mOld;     // old screen mode
    DWORD dwFlags;   // flags, see QF_* constants
    UINT wQRDelay;   // QuickRes delay in 1/10 sec
    UINT wAppDelay;  // Application delay in 1/10 sec
    char szApp[512]; // requested application
    char szErr[80];  // set if unknown parameter encountered
} QRES_PARS;

// Structure holding QuickRes app info
typedef struct
{
    HWND hwnd;                 // Main app window
    DWORD dwProcess, dwThread; // Process and main thread id
    UINT wFirstMenuID;         // First menu option ID
    BOOL fOSR2;                // TRUE for OSR2 built-in version
    BOOL fDirect;              // TRUE for direct switch
} QUICKRES;


BOOL SwitchQResMode(QRMODE *ptReqMode, QUICKRES *ptQRes);
void GetCurQResMode(QRMODE *ptMode);
void CompleteQResPars(QRES_PARS *ptParsRet);
int GetQResPars(LPSTR lpszCmdLine,    // Command line string
                QRES_PARS *ptParsRet); // Returns parameters
OSVERSIONINFO GetWindowsVersion();
