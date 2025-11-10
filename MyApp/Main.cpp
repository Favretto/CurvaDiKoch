//
// Controlla comportamento finestra tramite BOOLs StartMinimized e MinimizeOnTNR
// 
// 
//
// ------------------------
// Curva di Koch (fiocco di neve) tramite ricorsione e tracciamento vettoriale - Versione 1.0
// Alessandro Favretto
// 10/11/2025
// ------------------------
//

#include <windows.h>
#include <shellapi.h>   // Aggiunto: necessario per la tray icon (Shell_NotifyIcon)
#include "resource.h"
#include <stdio.h>   // o <wchar.h> per swprintf_s e concatenazione stringhe

#include <cmath>
#define PI 3.14159265358979323846

#define WM_TRAYICON (WM_USER + 1)   // Aggiunto: messaggio personalizzato per la tray icon
NOTIFYICONDATA nid = { 0 };         // Aggiunto: struttura per l’icona di notifica
#define TRAY_ICON_ID 1              // Per icona sulla TNR

#define BTN_SAMPLE_BTN 1

#define IDM_FILE_EXIT  101
#define IDM_WINVER 102
#define IDM_HELP_ABOUT 103
#define IDM_SYSINFO 107
#define IDM_ALWAYS_ON_TOP 109
#define IDM_TEMPDIR 110
#define IDM_USRDIR 111
#define IDM_WINDIR 112

#define IDM_OPEN_GUI 200

// 1.Timer ogni 500 ms
#define IDT_CLOCK_TIMER 5001

WCHAR szTempPath[MAX_PATH];

HWND hLabel = NULL;
HWND hLabelDateTime = NULL;
HWND hwnd = NULL;

BOOL is64Bit();
void SetAlwaysOnTop(HWND hwnd, BOOL enable);

BOOL isTopMost = FALSE;
BOOL isAppRunningAsAdmin = FALSE;
BOOL StartMinimized = FALSE;
BOOL MinimizeOnTNR = FALSE;

HANDLE hMutex = NULL;

// Fattore di ingrandimento ...
double scale = 1.5;

// Dati e costanti
double D[12], R[12], Cc[7], Ss[7];
double X = 60, Y = 70;
int A = 1, L = 1;

// Prototipi
void DrawKoch(HDC hdc);
void SubRec(HDC hdc);
void SubDL(HDC hdc);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE retSE = NULL;

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Colore penna: simile a INK 5
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 255, 255));
        SelectObject(hdc, pen);
        SetBkColor(hdc, RGB(0, 0, 128));
        SetBkMode(hdc, OPAQUE);

        DrawKoch(hdc);

        DeleteObject(pen);
        EndPaint(hwnd, &ps);
    }
    
    break;

    case WM_TRAYICON:
        // Aggiunto: gestisce i click sull’icona della tray
        if (lParam == WM_LBUTTONDBLCLK)
        {
            Shell_NotifyIcon(NIM_DELETE, &nid); // rimuove l’icona, spostato in WM_DESTROY ...
            ShowWindow(hwnd, SW_SHOW);          // ripristina la finestra
            SetForegroundWindow(hwnd);
        }
        else if (lParam == WM_RBUTTONUP) // AGGIUNTO: click destro
        {
            HMENU hTrayMenu = CreatePopupMenu();
            AppendMenu(hTrayMenu, MF_STRING, IDM_OPEN_GUI, L"▶  Apri GUI");
            AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);

            HMENU hHelpMenu = CreatePopupMenu();
            AppendMenu(hHelpMenu, MF_STRING, IDM_WINVER, L"Versione OS");
            AppendMenu(hHelpMenu, MF_STRING, IDM_SYSINFO, L"Informazioni di Sistema");
            AppendMenu(hHelpMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"Circa");
            AppendMenu(hTrayMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"&?");
            AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hTrayMenu, MF_STRING, IDM_FILE_EXIT, L"Esci da Fract");
            
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);

            DestroyMenu(hHelpMenu);
            DestroyMenu(hTrayMenu);
        }
        break;

    case WM_SYSCOMMAND:
        // Aggiunto: intercetta la minimizzazione per nascondere la finestra e mettere l’icona nella tray
        if (((wParam & 0xFFF0) == SC_MINIMIZE) && MinimizeOnTNR)
        {
            ShowWindow(hwnd, SW_HIDE); // nasconde la finestra
            Shell_NotifyIcon(NIM_ADD, &nid); // aggiunge l’icona di notifica
            return 0;
        }

        // Dopo aver gestito la parte che ti interessa, devi lasciare che Windows completi l’elaborazione predefinita richiamando DefWindowProc
        return DefWindowProc(hwnd, msg, wParam, lParam);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case BTN_SAMPLE_BTN:
            MessageBox(hwnd, L"Intercettato evento CLICK!", L"Bottone premuto ...", MB_OK | MB_ICONINFORMATION);
            break;

        case IDM_FILE_EXIT:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDM_TEMPDIR:
            GetEnvironmentVariableW(L"TEMP", szTempPath, MAX_PATH);

            retSE = ShellExecute(hwnd, L"open", szTempPath, NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"%TEMP% not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_USRDIR:
            GetEnvironmentVariableW(L"USERPROFILE", szTempPath, MAX_PATH);

            retSE = ShellExecute(hwnd, L"open", szTempPath, NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"%TEMP% not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_WINDIR:
            GetEnvironmentVariableW(L"WINDIR", szTempPath, MAX_PATH);

            retSE = ShellExecute(hwnd, L"open", szTempPath, NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"%TEMP% not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_SYSINFO:
            retSE = ShellExecute(hwnd, L"open", L"msinfo32.exe", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"msinfo32.exe not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_WINVER:
            retSE = ShellExecute(hwnd, L"open", L"winver.exe", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"winver.exe not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_ALWAYS_ON_TOP:

            SetAlwaysOnTop(hwnd, !isTopMost);
            isTopMost = !isTopMost;

            MENUITEMINFO mii;
            ZeroMemory(&mii, sizeof(MENUITEMINFO)); // azzera la struttura
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STATE;

            GetMenuItemInfo(GetMenu(hwnd), IDM_ALWAYS_ON_TOP, FALSE, &mii);

            if (mii.fState & MFS_CHECKED)
                CheckMenuItem(GetMenu(hwnd), IDM_ALWAYS_ON_TOP, MF_BYCOMMAND | MF_UNCHECKED);
            else
                CheckMenuItem(GetMenu(hwnd), IDM_ALWAYS_ON_TOP, MF_BYCOMMAND | MF_CHECKED);

            break;
        case IDM_HELP_ABOUT:
            if(is64Bit())
                MessageBox(hwnd, L"Fract v1.0 (x64) per Windows 10 e Windows 11\n\nRealizzato da Alessandro Favretto.\n\n\nApplicazione standalone realizzata in VC++ e WinAPI.",
                    L"Informazioni su Fract\n",
                    MB_OK | MB_ICONINFORMATION);
            else
                MessageBox(hwnd, L"Fract v1.0 (x86) per Windows 10 e Windows 11\n\nRealizzato da Alessandro Favretto.\n\n\nApplicazione standalone realizzata in VC++ e WinAPI.",
                    L"Informazioni su Fract\n",
                    MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_OPEN_GUI:
            ShowWindow(hwnd, SW_SHOW);          // ripristina la finestra
            SetForegroundWindow(hwnd);
            break;
        }

        break;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndCtrl = (HWND)lParam;

        if (hwndCtrl == hLabel)  { // Gestione Label Header
            SetTextColor(hdcStatic, RGB(192, 125, 0));      // COLORE FOREGROUND
            SetBkMode(hdcStatic, RGB(0, 0, 0));             // COLORE BACKGROUND
            return (INT_PTR)GetStockObject(BLACK_BRUSH);
        }
        
        break;
    }

    case WM_SETCURSOR:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        HWND hCwfp = ChildWindowFromPoint(hwnd, pt);

        if (hCwfp == GetDlgItem(hwnd, BTN_SAMPLE_BTN))// ||
            //hCwfp == GetDlgItem(hwnd, BTN_SAMPLE_BT2) ||
            //hCwfp == GetDlgItem(hwnd, BTN_SAMPLE_BT3) ||
            //hCwfp == GetDlgItem(hwnd, ...)
            //)
        {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE; // messaggio gestito
        }
        else {
            SetCursor(LoadCursor(NULL, IDC_ARROW)); // freccia se fuori bottone
            return TRUE;
        }

        break;
    }
    case WM_TIMER:
        if (wParam == IDT_CLOCK_TIMER) {

            SYSTEMTIME st;
            GetLocalTime(&st);

            wchar_t szDateTime[64];
            swprintf_s(szDateTime, L"%02d/%02d/%04d %02d:%02d:%02d",
                st.wDay, st.wMonth, st.wYear,
                st.wHour, st.wMinute, st.wSecond);

            SetWindowText(hLabelDateTime, szDateTime);
        }
        break;
    case WM_CLOSE:
    {
        int result = MessageBox(hwnd, L"Vuoi davvero uscire dall'applicazione?", L"Conferma uscita",
            MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);

        if (result == IDYES) {
            DestroyWindow(hwnd);
        }
        // se NO, ignora la chiusura
        return 0;
    }
    case WM_DESTROY:{
        NOTIFYICONDATA nid = { 0 };
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = TRAY_ICON_ID;
        Shell_NotifyIcon(NIM_DELETE, &nid); // Aggiunto: rimuove l’icona di notifica se presente
        
        // 3.Timer ogni 700 ms
        KillTimer(hwnd, IDT_CLOCK_TIMER);

        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void DrawKoch(HDC hdc)
{
    D[2] = 110;
    R[2] = 4;
    X = 230 / scale; // POSIZIONE ORIZZONATALE
    Y = 190 / scale; // POSIZIONE VERTICALE
    A = 1;
    L = 1;

    // precalcolo sin e cos ...
    for (int i = 0; i <= 5; ++i) {
        Cc[i + 1] = cos(i * PI / 3);
        Ss[i + 1] = sin(i * PI / 3);
    }

    SubRec(hdc);
    A -= 2;
    SubRec(hdc);
    A -= 2;
    SubRec(hdc);
}

// --- Subroutine ---
void SubRec(HDC hdc)
{
    int R1 = 0;
    L++;
    if (R[(int)L] == 1) R1 = 1;
    if (R1 == 1)
    {
        L--;
        SubDL(hdc);
        return;
    }

    R[(int)L + 1] = R[(int)L] - 1;
    D[(int)L + 1] = D[(int)L] / 3.0;

    SubRec(hdc);
    A++;
    SubRec(hdc);
    A -= 2;
    SubRec(hdc);
    A++;
    SubRec(hdc);
    L--;
}

// --- Subroutine ---
void SubDL(HDC hdc)
{
    double XL = X;
    double YL = Y;

    A += 6;
    while (A >= 6)
        A -= 6;

    X = X + D[(int)L + 1] * Cc[A + 1];
    Y = Y - D[(int)L + 1] * Ss[A + 1];  // invertito asse Y per GDI

    MoveToEx(hdc, (int)(XL * scale), (int)(YL * scale), NULL);
    LineTo(hdc, (int)(X * scale), (int)(Y * scale));
}

BOOL is64Bit() {
#if defined(_WIN64)
    return TRUE;  // Processo a 64 bit
#else
    return FALSE; // Processo a 32 bit
#endif
}

void SetAlwaysOnTop(HWND hwnd, BOOL enable)
{
    SetWindowPos(
        hwnd,
        enable ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE
    );
}

bool IsRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;

    // Crea un SID per il gruppo Administrators
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup))
    {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 1.Impedisce l'esecuzione multipla ...    
    hMutex = CreateMutexW(NULL, TRUE, L"Fract_Mutex_SingleInstance");

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, L"Processo già in esecuzione.", L"Istanze multiple non permesse ...", MB_OK | MB_ICONEXCLAMATION);
        if(hMutex!=NULL) CloseHandle(hMutex);
        return 0;
    }

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"KochCurve";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 128));

    // --- Aggiunta icona ---
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));    // icona grande (taskbar, Alt+Tab)
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));  // icona piccola (tray, titlebar)

    RegisterClassExW(&wc);

    // CENTRATURA DELLA FINESTRA SULLO SCHERMO ...
    // 
    // Dimensioni dello schermo
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calcolo posizione centrata
    int x = (screenWidth - 640) / 2;
    int y = (screenHeight - 480) / 2;

    isAppRunningAsAdmin = IsRunningAsAdmin();

    if (is64Bit()) {
        if (isAppRunningAsAdmin)
            hwnd = CreateWindowEx(
                0,
                L"KochCurve",
                L"Fract v1.0 (x64) - {Administrator}",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
        else
            hwnd = CreateWindowEx(
                0,
                L"KochCurve",
                L"Fract v1.0 (x64)",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
    } else {
        if (isAppRunningAsAdmin)
            hwnd = CreateWindowEx(
                0,
                L"KochCurve",
                L"Fract v1.0 (x86) - {Administrator}",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
        else
            hwnd = CreateWindowEx(
                0,
                L"KochCurve",
                L"Fract v1.0 (x86)",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
    }

    // Aggiunto: inizializzazione struttura tray icon
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID; // 1; Icona definita sopra
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    //nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // icona

    lstrcpy(nid.szTip, L"Fract - Doppio-click per ripristinare");

    // Label Exmple
    hLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"CURVA DI HELGE VON KOCH",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 10, 600, 20,       // posizione e dimensione
        hwnd, NULL, hInstance, NULL
    );

    // Label DATETIME aggiornato da TIMER ...
    hLabelDateTime = CreateWindowEx(
        0,
        L"STATIC",
        L"DATA-ORA",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        50, 375, 500, 20,       // posizione e dimensione
        hwnd, NULL, hInstance, NULL
    );
    
    // Pulsanti
    CreateWindowEx(0, L"BUTTON", L"CLICK ME!",
        WS_CHILD | WS_VISIBLE,
        175, 330, 250, 30,
        hwnd, (HMENU)BTN_SAMPLE_BTN, hInstance, NULL);

    //HFONT hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    HFONT hFont = CreateFontW(
        15,                 // altezza del font in pixel
        0,                  // larghezza carattere (0 = proporzionale)
        0, 0,               // angolo escapement e orientamento
        FW_NORMAL,          // spessore (FW_BOLD per grassetto)
        TRUE, FALSE, FALSE,// italic, underline, strikeout
        ANSI_CHARSET,       // charset
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, // stile font
        L"Courier New"         // nome font nativo Windows (alternative: "Segoe UI", "Courier New", "Arial")
    );

    // BEGIN: SWAP TEXTBOX MULTILINE ...
    // 
    HWND hSwapInfo = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_READONLY,
        175, 250, 250, 70,
        hwnd,
        (HMENU)1001,
        hInstance,
        NULL);

    SendMessageW(hSwapInfo, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetWindowTextW(hSwapInfo, L"Esempio di frattale:\r\nLo scopo originale era dimostrare l’esistenza di curve continue ma non derivabili in alcun punto.\r\nLa curva di Koch (fiocco di neve) fu ideata dal matematico svedese Helge von Koch nel 1904\r\nIl termine frattale è stato coniato nel 1975 da Benoît Mandelbrot.");

    // END

    // BEGIN: GESTIONE MENU A TENDINA ...
    // 
    HMENU hMenuBar = CreateMenu();      // menu principale (barra)

    HMENU hFileMenu = CreateMenu();     // menu File
    HMENU hWindowMenu = CreateMenu();   // menu Finestra
    HMENU hHelpMenu = CreateMenu();     // menu Help

    HMENU hGoTo = CreateMenu();         // Navigazione Cartelle Notevoli
    

    // Navigazione Cartelle Notevoli: Aggiunge voci al sotto-sottomenu ...
    AppendMenu(hGoTo, MF_STRING, IDM_TEMPDIR, L"Cartella TEMP");
    AppendMenu(hGoTo, MF_STRING, IDM_USRDIR, L"Cartella USER");
    AppendMenu(hGoTo, MF_STRING, IDM_WINDIR, L"Cartella OS");
    
    // Aggiunge i sottomenu al menu File
    AppendMenu(hFileMenu, MF_POPUP, (UINT_PTR)hGoTo, L"Vai a ...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"Esci");
    
    AppendMenu(hWindowMenu, MF_STRING, IDM_ALWAYS_ON_TOP, L"Sempre in primo piano");

    AppendMenu(hHelpMenu, MF_STRING, IDM_WINVER, L"Versione OS");
    AppendMenu(hHelpMenu, MF_STRING, IDM_SYSINFO, L"Informazioni di Sistema");
    AppendMenu(hHelpMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"Circa");
 
    // Aggiunge i sottomenu alla barra principale ...
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hFileMenu, L"&File");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hWindowMenu, L"Fi&nestra");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hHelpMenu, L"&?");

    // Imposta la barra menu sulla finestra ...
    SetMenu(hwnd, hMenuBar);

    // END

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 2.Timer ogni 700 ms
    SetTimer(hwnd, IDT_CLOCK_TIMER, 500, NULL);

    // Riduco la finestra sulla TNR postando rispettivo messaggio ...
    if (StartMinimized) {
        if (MinimizeOnTNR)
            Shell_NotifyIcon(NIM_ADD, &nid); // aggiunge l'icona nella tray

        PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //2.Impedisce l'esecuzione multipla ...
    if (hMutex != NULL) CloseHandle(hMutex);

    return (int)msg.wParam;
}