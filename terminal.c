/*
 * COBGDB GnuCOBOL Debugger:
 *
 * Colorization functions:
 * The functions related to colorization were obtained from:
 * https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
 *
 * Raw mode on Linux:
 * The information on how to use the rawmode function on Linux was obtained from:
 * https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
 *
 * Windows Console interface:
 * Much of the information about the Windows Console interface was obtained from:
 * https://learn.microsoft.com/en-us/windows/console/
 *
 * License:
 * This program is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 * 
 * Author: Marcos Martins Duma
 */

#if defined(_WIN32)
#include <Windows.h>
#include <stdio.h>
#include <WinCon.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
/* this define is only available in recent Windows SDKs,
   so we add it, if missing; note that the feature itself needs
   Windows 10 with a build number 10586 or greater */
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#include <conio.h>
#include <stdlib.h>
#include <wchar.h>
#define maxViewWidth 200
#define maxViewHeight 100
void SetConsoleSize(HANDLE hStdout, int cols, int rows );
#define VKEY_BACKSPACE 8
#elif defined(__linux__)
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <wchar.h>
#if __has_include(<X11/Xlib.h>)
#ifndef HAVE_X11
#define HAVE_X11
#endif
#include <X11/Xlib.h>
#endif
#endif // Windows/Linux
#include <time.h>
#include "cobgdb.h"

#define STRCURSOR_OFF  "\e[?25l"
#define STRCURSOR_ON   "\e[?25h"
#define ANSI_UNDERLINE "\033[4m"
#define ANSI_RESET     "\033[0m"

char color[200];
extern struct st_cobgdb cob;

void cursorON();
void cursorOFF();
void clearScreen();
int mouseCobAction(int col, int line, int type);
int mouseCobRigthAction(int col, int line);
void mouseCobHover(int col, int line);

extern int VIEW_LINES;
extern int VIEW_COLS;


int TERM_WIDTH = 132;
int TERM_HEIGHT= 48;

#if defined(__linux__)
#define TM_ESCAPE '\x1b'
#define TM_CSI '['
#define TM_MOUSE 'M'

struct editorConfig {
	  int cx, cy;
	  int screenrows;
	  int screencols;
	  struct termios orig_termios;
};

struct editorConfig E;
void print_colorBK(const int textcolor, const int backgroundcolor);

void die(const char *s) {    
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	exit(1);
}

void disableRawMode() {
	  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
          die("tcsetattr");
      tcflush(STDIN_FILENO, TCIFLUSH);
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}


int readKeyLinux(int type) {
    unsigned char buf[32];
    ssize_t nread;

    if ((nread = read(STDIN_FILENO, buf, sizeof(buf))) <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }

    if (buf[0] == TM_ESCAPE && buf[1] == TM_CSI) {
        if (nread >= 3) {
            switch (buf[2]) {
                case '3':
                    return VKEY_DEL;
                case '5':
                    return VKEY_PGUP;
                case '6':
                    return VKEY_PGDOWN;
                case 'A':
                    return VKEY_UP;
                case 'B':
                    return VKEY_DOWN;
                case 'C':
                    return VKEY_RIGHT;
                case 'D':
                    return VKEY_LEFT;
            }
        }
        if (buf[2] == TM_MOUSE) {
            // Mouse
            struct {
                unsigned char button;
                unsigned char x;
                unsigned char y;
            } mouseEvent;

            if (type > 0 && nread >= (ssize_t) sizeof(mouseEvent)) {
                memcpy(&mouseEvent, buf + 3, sizeof(mouseEvent)); 
                if (mouseEvent.button == 96) {
                    return VKEY_UP;
                }
                if (mouseEvent.button == 97) {
                    return VKEY_DOWN;
                }
                mouseCobHover(mouseEvent.x - 33, mouseEvent.y - 33);
                cob.mouseX = mouseEvent.x - 33;
                cob.mouseY = mouseEvent.y - 33;
                if (mouseEvent.button == 32) {
                    return mouseCobAction(mouseEvent.x - 33, mouseEvent.y - 33, type);
                }
                if (mouseEvent.button == 34) {
                    return mouseCobRigthAction(mouseEvent.x - 33, mouseEvent.y - 33);
                }
            }
            return 0;
        }
    } else {
        // Ctrl+F = ASCII 6
        if (buf[0] == 6) {
            return VKEY_CTRLF;
        }
        wchar_t wc;
        if (mbtowc(&wc, (const char *)buf, nread) <= 0) {
            return buf[0];
        }
        return (int)wc;
    }
    return 0;
}
#endif

void mouseCobHover(int col, int line){
    cob.mouse = 0;
    if(col==VIEW_COLS-1  && line<VIEW_LINES/2-1) cob.mouse=1;
    if(col==VIEW_COLS-1  && line>=VIEW_LINES/2-1) cob.mouse=2;
    if(col<cob.num_dig+2 && line>0 && line<VIEW_LINES-2) cob.mouse=3;
    if(col==VIEW_COLS-17 && line==0) cob.mouse=10;
    if(col==VIEW_COLS-15 && line==0) cob.mouse=20;
    if(col==VIEW_COLS-13  && line==0) cob.mouse=30;
    if(col==VIEW_COLS-11  && line==0) cob.mouse=40;
    if(col==VIEW_COLS-9  && line==0) cob.mouse=50;    
    if(col==VIEW_COLS-7  && line==0) cob.mouse=60;    
    if(col==VIEW_COLS-5  && line==0) cob.mouse=70;    
    if(col==VIEW_COLS-3  && line==0) cob.mouse=80;    
}

int mouseCobAction(int col, int line, int type){
    int action=-1;
    cob.mouseButton = 1;
    switch (cob.mouse){
            case 1:
                action = VKEY_PGUP;
                break;
            case 2:
                action = VKEY_PGDOWN;
                break;
            case 10:
                action = 'R';
                break;
            case 20:
                action = 'N';
                break;
            case 30:
                action = 'S';
                break;
            case 40:
                action = 'G';
                break;
            case 50:
                action = 'Q';
                break;
            case 60:
                action = 'O';
                break;
            case 70:
                action = 'D';
                break;
            case 80:
                action = '?';
                break;
            default:
                action = -1;
                break;
    }
    if(action == -1 && line>0 && line<VIEW_LINES-2){
        if(type>1){
            cob.line_pos = line-1;
            cob.showFile = TRUE;
        }
        if(col<cob.num_dig+2){
            action = 'B';
        }else{
            action=VKEY_ENTER;
        }
    }
    return action;
}

int mouseCobRigthAction(int col, int line){
    int action= 'H';
    cob.mouseButton=2;
    if(line>0 && line<VIEW_LINES-2){
        cob.line_pos = line-1;
        cob.showFile = TRUE;
        if(col>1 && col<VIEW_COLS){
            action = 'H';
        }
    }
    return action;
}

void disableEcho() {
#if defined(__linux__)
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
}

void enableEcho() {
#if defined(__linux__)
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
}

void freeInputBuffer(){
#if defined(__linux__)
    tcflush(STDIN_FILENO, TCIFLUSH);
#endif
}


int key_press(int type){
#if defined(_WIN32)
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD inp;
    DWORD numEvents;
    DWORD mode = ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    MOUSE_EVENT_RECORD mer;
    int virtualKeyCode = 0;
    cob.mouseButton=0;

    SetConsoleMode(hIn, mode);
    if (PeekConsoleInput(hIn, &inp, 1, &numEvents) && numEvents > 0) {
        if(ReadConsoleInputW(hIn, &inp, 1, &numEvents)) {
            if (type>0 && inp.EventType == MOUSE_EVENT) {
                mer = inp.Event.MouseEvent;
                cob.mouseX = mer.dwMousePosition.X;
                cob.mouseY = mer.dwMousePosition.Y;
                mouseCobHover(cob.mouseX, cob.mouseY);
                if(mer.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED){
                    return mouseCobAction(cob.mouseX, cob.mouseY, type);
                }
                if(mer.dwButtonState == RIGHTMOST_BUTTON_PRESSED && type){
                    return mouseCobRigthAction(cob.mouseX, cob.mouseY);                        
                }
                if (inp.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) {
                    int delta = GET_WHEEL_DELTA_WPARAM(inp.Event.MouseEvent.dwButtonState);
                    if (delta > 0) 
                        return VKEY_UP;
                    else
                        return VKEY_DOWN;
                }                
                //printf("Mouse X: %d, Y: %d\n", mouseX, mouseY);
            }
            DWORD ctrlState = inp.Event.KeyEvent.dwControlKeyState;
            virtualKeyCode = inp.Event.KeyEvent.wVirtualKeyCode;
            if ((ctrlState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) && inp.Event.KeyEvent.bKeyDown ){
                     virtualKeyCode = inp.Event.KeyEvent.wVirtualKeyCode;
                     if(virtualKeyCode == 'F'){
                        FlushConsoleInputBuffer(hIn);                        
                        return VKEY_CTRLF;
                     }
            } else if(inp.Event.KeyEvent.bKeyDown && inp.Event.KeyEvent.uChar.UnicodeChar != 0) {
                    WCHAR key = inp.Event.KeyEvent.uChar.UnicodeChar;
                    FlushConsoleInputBuffer(hIn);
                    return (int)key;
            } else if (inp.Event.KeyEvent.bKeyDown) { 
                virtualKeyCode = inp.Event.KeyEvent.wVirtualKeyCode;
                if(virtualKeyCode==VKEY_PGUP || virtualKeyCode==VKEY_PGDOWN || virtualKeyCode==VKEY_UP ||
                   virtualKeyCode==VKEY_DOWN || virtualKeyCode==VKEY_RIGHT || virtualKeyCode==VKEY_LEFT){
                    return virtualKeyCode;
                }
                if (virtualKeyCode == VK_DELETE) {
                    return VKEY_DEL;
                }
                FlushConsoleInputBuffer(hIn);
                return 0;
            }
        }
    }
    
    return 0;      
#elif defined(__linux__)
  enableRawMode();
  cob.mouseButton=0;
  int ch_lin= readKeyLinux(type);
  disableRawMode();
  return ch_lin;
#endif // Windows/Linux
}

int readchar(char * str, int size) {
    char c =' ';
    int i = 0;
    str[0] = '\0';
    cursorON();
    while (1) {
        do{
            c = key_press(MOUSE_OFF);
            fflush(stdout);
        }while(c<=0);
        if (c == 13) {
            break;
        }
        if ((c == VKEY_BACKSPACE || c==37) && i > 0) {
            putchar(8);
            putchar(' ');
            putchar(8);
            i--;
            str[i] = '\0';
        }else if(c==39 && strlen(str) < (size_t) size){
            putchar(' ');
            str[i++]=' ';
            str[i]='\0';
        } else if (strlen(str) < (size_t) size && c >= 32) {
            str[i] = c;
            putchar(c);
            i++;
            str[i] = '\0';
        }
    }
    cursorOFF();
    return 0;
}

int updateStr(char *value, int size, int x, int y) {
    int len = strlen(value)+1;
    int len_char = len;
    wchar_t *str = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
    wchar_t *wcBuffer = NULL;
    wchar_t c = L' ';
    int i = 0;
    int result = TRUE;

    #if defined(_WIN32)
    MultiByteToWideChar(CP_UTF8, 0, value, -1, str,(len_char + 1) * sizeof(wchar_t) / sizeof(str[0]));
    #else
    mbstowcs(str, value, len_char + 1);
    #endif

    len = wcslen(str);
    if (str[0] == L'"') {
        wcBuffer = wcsdup(str);
        wcscpy(str, wcBuffer + 1);
        free(wcBuffer);
        len = wcslen(str);
        if (str[len - 1] == L'"') {
            str[len - 1] = L'\0';
            len--;
        }
    }
    wcBuffer = malloc(256);

    int lt = (len > size) ? size : len;
    int startChar = 0;
    int isPrint = TRUE;
    print_colorBK(color_green, color_red);
    cursorON();
    i = 0;
    while (1) {
        if (isPrint) {
            wcsncpy(wcBuffer, &str[startChar], lt);
            wcBuffer[lt]=L'\0';
            gotoxy(x, y);
            printf("%*ls\r", lt, wcBuffer);
            fflush(stdout);
            isPrint = FALSE;
        }
        gotoxy(x + i, y);
        do {
            c = key_press(MOUSE_OFF);
            fflush(stdout);
        } while (c == 0);
        //gotoxy(1, 1); printf("Key = %d    \r", c); fflush(stdout);
        if (c == VKEY_ENTER) {
            break;
        }
        if (c == VKEY_ESCAPE) {
            result = FALSE;
            break;
        }
        if (c == VKEY_DEL) c = L' ';
        if (c == VKEY_LEFT && i >= 0) {
            i--;
            if (i < 0) {
                startChar = (startChar > 0) ? startChar - 1 : startChar;
                isPrint = TRUE;
                i = 0;
            }
        } else if (c == VKEY_BACKSPACE && i >= 0) {
            i--;
            isPrint = TRUE;
            if (i < 0) {
                startChar = (startChar > 0) ? startChar - 1 : startChar;
                i = 0;
            } else {
                str[startChar + i] = L' ';
            }
        } else if (c == VKEY_RIGHT) {
            i++;
            if (i >= lt) {
                startChar = (str[i + startChar] != L'\0') ? startChar + 1 : startChar;
                isPrint = TRUE;
                i--;
            }
        } else if (i < lt) {
            if (c == 37 && i == 0) continue;
            str[startChar + i] = c;
            i++;
            if (i >= lt) {
                startChar = (str[i + startChar] != L'\0') ? startChar + 1 : startChar;
                i--;
            }
            isPrint = TRUE;
        }
    }
    cursorOFF();
    #if defined(_WIN32)
    WideCharToMultiByte(CP_UTF8, 0, str, -1, value, len_char+1, NULL, NULL);
    #else
    wcsrtombs(value, (const wchar_t **) &str, len_char+1, NULL);
    #endif
    free(wcBuffer);
    return result;
}

void get_terminal_size(int *width, int *height) {
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *width = (int)(csbi.srWindow.Right-csbi.srWindow.Left+1);
    *height = (int)(csbi.srWindow.Bottom-csbi.srWindow.Top+1);
#elif defined(__linux__)
    struct winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    *width = (int)(ws.ws_col);
    *height = (int)(ws.ws_row);
    system("echo -ne \"\\033]0;CobGDB Debug\\007\"");
    printf("\033]0;%s\007", "CobGDB Debug");
    fflush(stdout);
#endif // Windows/Linux
}

#if defined(_WIN32)
int win_size_verify(int showFile, int *check_size){
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL isMaximized = IsZoomed(hConsole);
    *check_size=FALSE;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    //GetLargestConsoleWindowSize(hConsole);
    if (isMaximized==TRUE || csbi.dwSize.X != TERM_WIDTH || csbi.dwSize.Y != TERM_HEIGHT) {
        if (isMaximized) {
            ShowWindow(hConsole, SW_RESTORE); 
        }
        SetConsoleSize(hConsole, TERM_WIDTH, TERM_HEIGHT);
        SMALL_RECT sr = {0,0,TERM_WIDTH-1,TERM_HEIGHT-1};
        SetConsoleWindowInfo(hConsole, TRUE, &sr);
        cursorOFF();
        clearScreen();
        SetForegroundWindow(GetConsoleWindow());
        Sleep(100);
        return TRUE;
    }
    return showFile;
}
#else
boolean show_message=TRUE;

int win_size_verify(int showFile, int *check_size){
    struct winsize w;
    int ret=showFile;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if(w.ws_row != TERM_HEIGHT || w.ws_col != TERM_WIDTH) {
        w.ws_row = TERM_HEIGHT;
        w.ws_col = TERM_WIDTH;
        ioctl(STDOUT_FILENO, TIOCSWINSZ, &w);
        printf("\033[8;%d;%dt", TERM_HEIGHT, TERM_WIDTH);
        if(show_message){
            print_colorBK(color_dark_red, color_black);
            printf("%s\r", "Resizing the screen.");
            show_message=FALSE;
        }
        fflush(stdout);
        *check_size=*check_size+1;
        usleep(1500);
        ret=1;
    }else{
        *check_size = 0;
    }
    return ret;
}
#endif


#if defined(_WIN32)
void DisableMaxWindow(){
    HWND hwnd = GetConsoleWindow();
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_FRAMECHANGED);
}
#endif

void set_terminal_size(int width, int height){
    TERM_WIDTH = width;
    TERM_HEIGHT = height;
#if defined(_WIN32)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL isMaximized = IsZoomed(hConsole);
    if (hConsole != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
            if (isMaximized) {
                ShowWindow(hConsole, SW_RESTORE); 
            }
            SetConsoleSize(hConsole, TERM_WIDTH, TERM_HEIGHT);
            COORD bufferSize = { width, height };
            if (SetConsoleScreenBufferSize(hConsole, bufferSize)) {
                SMALL_RECT sr;
                sr.Left = 0;
                sr.Top = 0;
                sr.Right = width - 1;
                sr.Bottom = height - 1;
                SetConsoleWindowInfo(hConsole, TRUE, &sr);
                GetConsoleScreenBufferInfo(hConsole, &csbi);
                COORD bufferSize = { csbi.dwSize.X, csbi.srWindow.Bottom + 1 };
                SetConsoleScreenBufferSize(hConsole, bufferSize);
            }
        }
    }
#elif defined(__linux__)
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return;
    }
    if (w.ws_col != width || w.ws_row != height) {
        w.ws_col = width;
        w.ws_row = height;
        ioctl(STDOUT_FILENO, TIOCSWINSZ, &w);
        printf("\033[8;%d;%dt", height, width);
        printf("\033[?47l");
        printf("\033[0;0r");
        fflush(stdout);
        tcflush(STDIN_FILENO, TCIFLUSH);
    }
#endif // Windows/Linux
}

#if  defined(__linux__)
#ifdef HAVE_X11
int is_x11_environment() {
    const char *display = getenv("DISPLAY");
    return display != NULL && display[0] != '\0';
}

Window find_window_by_title(Display *display, Window root, const char *title) {
    Window root_return, parent_return;
    Window *children;
    unsigned int nchildren;

    if (!XQueryTree(display, root, &root_return, &parent_return, &children, &nchildren)) {
        return 0;
    }

    for (unsigned int i = 0; i < nchildren; i++) {
        char *window_name = NULL;
        if (XFetchName(display, children[i], &window_name)) {
            if (window_name && strstr(window_name, title)) {
                Window found = children[i];
                XFree(window_name);
                return found;
            }
            XFree(window_name);
        }

        // Recurse
        Window child = find_window_by_title(display, children[i], title);
        if (child) return child;
    }

    if (children) XFree(children);
    return 0;
}

void focus_window_by_title_linux(const char *title_to_find) {
    if (!is_x11_environment()) {
        return;
    }

    Display *display = XOpenDisplay(NULL);
    if (!display) {
        return;
    }

    Window root = DefaultRootWindow(display);
    Window win = find_window_by_title(display, root, title_to_find);

    if (win != 0) {
        //printf("Janela encontrada: 0x%lx\n", win);
        XRaiseWindow(display, win); // Traz para frente
        XSetInputFocus(display, win, RevertToParent, CurrentTime); // Foca
    }

    XCloseDisplay(display);
}
#else
void focus_window_by_title_linux(const char *title_to_find) {
    (void)title_to_find; 
}
#endif
#endif  

void focusOnCobgdb() {
    #if defined(_WIN32)
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole == NULL) return;
    // Simulate ALT key pressed to allow SetForegroundWindow
    INPUT input[2] = {};
    // Press Alt
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_MENU;
    // Release Alt
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_MENU;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    // Send the simulated inputs
    SendInput(2, input, sizeof(INPUT));
    // Bring to front and set focus
    ShowWindow(hwndConsole, SW_RESTORE);
    SetForegroundWindow(hwndConsole);
    SetFocus(hwndConsole);
    SetActiveWindow(hwndConsole);
    #else
    #ifdef HAVE_X11
    focus_window_by_title_linux("CobGDB Debug");
    #endif
    #endif
}


void focus_window_by_title(const char *window_title) {
    #if defined(_WIN32)
    HWND hwnd = FindWindowA(NULL, window_title);
    if (hwnd != NULL) {
        ShowWindow(hwnd, SW_SHOW); 
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        SetActiveWindow(hwnd);
    }
    #else
    (void)window_title; 
    #ifdef HAVE_X11
    char linux_title[300];
    snprintf(linux_title, sizeof(linux_title) - 1, "GnuCOBOL Debug - %s", cob.name_file);
    if (!is_x11_environment()) {
        showCobMessage("Works only in X11 environments",1);
    }else{
        focus_window_by_title_linux(linux_title);
    }
    #endif
    #endif
}

void gotoxy(int x, int y){
    #if defined(_WIN32)
    COORD pos = {x-1, y-1};
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(output, pos);
    #elif defined(__linux__)
    printf("\033[%d;%dH", (y), (x));
    #endif
}

void clearScreen() {
#ifdef _WIN32
    HANDLE hStdOut;
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdOut, &mode);
    DWORD originalMode = mode;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdOut, mode);
    DWORD written = 0;
    PCWSTR sequence = L"\x1b[2J";
    WriteConsoleW(hStdOut, sequence, (DWORD)wcslen(sequence), &written, NULL);
    SetConsoleMode(hStdOut, originalMode);
    return;
#else
    //printf("\033[H\033[J");
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
#endif
}

void cursorOFF(){
#ifdef _WIN32
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
#else
    printf(STRCURSOR_OFF);
    const char seq[] = "\033[?1003h";
    write(STDOUT_FILENO, seq, sizeof(seq) - 1);
#endif
}

void cursorON(){
#ifdef _WIN32
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    cursorInfo.dwSize = 100;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
#else
    printf(STRCURSOR_ON);
    const char seq[] = "\033[?1003l"; 
    write(STDOUT_FILENO, seq, sizeof(seq) - 1); 
    //printf("\e[?9h");
#endif
}

#if defined(_WIN32)
static void checksize(int height, int width)
{
  if ( height > maxViewHeight )
  {
    printf("\n\n\nFatal error: the window (%d) is too high (max %d rows)!\n", height, maxViewHeight);
    _exit(0);
  }
  if ( width > maxViewWidth )
  {
    printf("\n\n\nFatal error: the window (%d) is too wide (max %d columns)!\n", width, maxViewWidth);
    _exit(0);
  }
}

int getRows(HANDLE hStdout){
  CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
  GetConsoleScreenBufferInfo( hStdout, &csbiInfo );
  return csbiInfo.dwSize.Y;
}

int getCols(HANDLE hStdout){
  CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
  GetConsoleScreenBufferInfo( hStdout, &csbiInfo);
  return csbiInfo.dwSize.X;
}

void SetConsoleSize(HANDLE hStdout, int cols, int rows )
{
  int oldr = getRows(hStdout);
  int oldc = getCols(hStdout);
  if ( cols == 0 ) cols = oldc;
  if ( rows == 0 ) rows = oldr;
  checksize(rows, cols);
  COORD newSize = { cols, rows };
  SMALL_RECT rect = { 0, 0, cols-1, rows-1 };
  CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
 
  if ( oldr <= rows )
  {
    if ( oldc <= cols )
    {                           // increasing both dimensions
 BUFWIN:
      if (!SetConsoleScreenBufferSize(hStdout, newSize)) {
            return;
      }
      if (!SetConsoleWindowInfo(hStdout, TRUE, &rect)) {
            return;
      }
    }else{                           // cols--, rows+
      SMALL_RECT tmp = { 0, 0, cols-1, oldr-1 };
      if (!SetConsoleWindowInfo(hStdout, TRUE, &tmp)) {
        return;
      }
      goto BUFWIN;
    }
  }else {
    if ( oldc <= cols ) {                           // cols+, rows--
      SMALL_RECT tmp = { 0, 0, oldc-1, rows-1 };
      if (!SetConsoleWindowInfo(hStdout, TRUE, &tmp)) {
        return;
      }
      goto BUFWIN;
    }
    else
    {                           // cols--, rows--
      if (!SetConsoleWindowInfo(hStdout, TRUE, &rect)) {
        return;
      }
      if (!SetConsoleScreenBufferSize(hStdout, newSize)) {
        return;
      }
    }
  }
  if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
        return;
  }
}
#endif

char * get_textcolor_code(const int textcolor) { // Linux only
    switch(textcolor) {
        case  0: return "30"; // color_black      0
        case  1: return "34"; // color_dark_blue  1
        case  2: return "32"; // color_dark_green 2
        case  3: return "36"; // color_light_blue 3
        case  4: return "31"; // color_dark_red   4
        case  5: return "35"; // color_magenta    5
        case  6: return "33"; // color_orange     6
        case  7: return "37"; // color_light_gray 7
        case  8: return "90"; // color_gray       8
        case  9: return "94"; // color_blue       9
        case 10: return "92"; // color_green     10
        case 11: return "96"; // color_cyan      11
        case 12: return "91"; // color_red       12
        case 13: return "95"; // color_pink      13
        case 14: return "93"; // color_yellow    14
        case 15: return "97"; // color_white     15
        default: return "37";
    }
}
char * get_backgroundcolor_code(const int backgroundcolor) { // Linux only
    switch(backgroundcolor) {
        case  0: return  "40"; // color_black      0
        case  1: return  "44"; // color_dark_blue  1
        case  2: return  "42"; // color_dark_green 2
        case  3: return  "46"; // color_light_blue 3
        case  4: return  "41"; // color_dark_red   4
        case  5: return  "45"; // color_magenta    5
        case  6: return  "43"; // color_orange     6
        case  7: return  "47"; // color_light_gray 7
        case  8: return "100"; // color_gray       8
        case  9: return "104"; // color_blue       9
        case 10: return "102"; // color_green     10
        case 11: return "106"; // color_cyan      11
        case 12: return "101"; // color_red       12
        case 13: return "105"; // color_pink      13
        case 14: return "103"; // color_yellow    14
        case 15: return "107"; // color_white     15
        default: return  "40";
    }
}
char * get_print_color(const int textcolor) { // Linux only
    sprintf(color,"\033[%sm",get_textcolor_code(textcolor) );
    return color;
}

char * get_print_colorBK(int textcolor, int backgroundcolor) { // Linux only
    sprintf(color,"\033[%s;%sm",get_textcolor_code(textcolor),  get_backgroundcolor_code(backgroundcolor));
    return color;
}

void print_color(int textcolor) {
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, textcolor);
#elif defined(__linux__)
    printf("%s",get_print_color(textcolor));
#endif // Windows/Linux
}
void print_colorBK(const int textcolor, const int backgroundcolor) {
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, backgroundcolor<<4|textcolor);
#elif defined(__linux__)
    printf("%s", get_print_colorBK(textcolor, backgroundcolor));
#endif // Windows/Linux
}
void print_color_reset() {
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, 7); // reset color
#elif defined(__linux__)
    printf("%s", "\033[0m"); // reset color
#endif // Windows/Linux
}

void print(char * s, int textcolor) {
    print_color(textcolor);
    printf("%s", s);
    print_color_reset();
}
void printBK(char * s, int textcolor, int backgroundcolor) {
    print_colorBK(textcolor, backgroundcolor);
    printf("%s", s);
    print_color_reset();
}
void print_no_reset(char * s, int textcolor) { // print with color, but don't reset color afterwards (faster)
    print_color(textcolor);
    printf("%s", s);
}
void print_no_resetBK(char * s, int textcolor, int backgroundcolor) { // print with color, but don't reset color afterwards (faster)
    print_colorBK(textcolor, backgroundcolor);
    printf("%s", s);
}

// BOX
void draw_utf8_text(const char* text) {
    #ifdef _WIN32
        HANDLE hStdOut;
        DWORD written = 0;
        wchar_t wcharString[500];

        hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wcharString, (strlen(text) + 1));
        WriteConsoleW(hStdOut, wcharString, (DWORD)wcslen(wcharString), &written, NULL);
    #else
        // Linux -> print the text directly
        printf("%s", text);
    #endif
}

int draw_box_first(int posx, int posy, int width, char *text) {
    char horizontal[] = "\u2500";   // UTF-8 character for horizontal line
    // Define the corners of the box using wide characters
    char topLeftCorner[] = "\u250C";      // UTF-8 character for top-left corner
    char topRightCorner[] = "\u2510";     // UTF-8 character for top-right corner
    char line_h[500];
    wchar_t utext[500];
    #if defined(_WIN32)
    MultiByteToWideChar(CP_UTF8, 0, text, -1, utext,(strlen(text) + 1) * sizeof(wchar_t) / sizeof(utext[0]));
    #else
    mbstowcs(utext, text, strlen(text) + 1);
    #endif
    // Line 1
    gotoxy(posx, posy);
    // Horizontal line
    snprintf(line_h, sizeof(line_h), "%s%s", topLeftCorner, text);
    for (int i = 0; i < (int) (width - wcslen(utext)); ++i) {
        strcat(line_h, horizontal);
    }
    strcat(line_h, topRightCorner);
    strcat(line_h, "\r");
    draw_utf8_text(line_h);
    return TRUE;
}

int draw_box_last(int posx, int posy, int width) {
    char horizontal[] = "\u2500";         // UTF-8 character for horizontal line
    // Define the corners of the box using wide characters
    char bottomLeftCorner[] = "\u2514";   // UTF-8 character for bottom-left corner
    char bottomRightCorner[] = "\u2518";  // UTF-8 character for bottom-right corner
    char line_h[500];
    // Last line
    snprintf(line_h, sizeof(line_h), "%s", bottomLeftCorner);
    for (int i = 0; i < width; ++i) {
        strcat(line_h, horizontal);
    }
    strcat(line_h, bottomRightCorner);
    strcat(line_h, "\r");
    gotoxy(posx, posy);
    draw_utf8_text(line_h);
    return TRUE;
}

int draw_box_border(int posx, int posy) {
    char vertical[]   = "\u2502";   // UTF-8 character for vertical line
    gotoxy(posx, posy);
    draw_utf8_text(vertical);
    return TRUE;
}

void print_underlined(const char *text) {
    #ifdef _WIN32
       HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        printf(ANSI_UNDERLINE "%s" ANSI_RESET, text);
    #else
        printf(ANSI_UNDERLINE "%s" ANSI_RESET, text);
    #endif
}

int showCobMessage(char * message, int type){
    char aux[500];
    int bkg= color_dark_red;
    int lin=VIEW_LINES/2-2;
    int size = strlen(message);
    int col = (VIEW_COLS-size)/2;
    int key = -1;
    int posYes=-1;
    int posLine = -1;
    gotoxy(col,lin);
    print_colorBK(color_white, bkg);
    switch (type)
    {
        case 1:
            strcpy(aux,"Message");
            break;
        case 2:
            strcpy(aux,"Alert!");
            break;    
        case 3:
            strcpy(aux,"Message");
            break;
        default:
            break;
    }
    if(strlen(aux)> (size_t) size) size=strlen(aux);
    if(size<12) size=12;
    draw_box_first(col,lin,size+1,aux);
    draw_box_border(col,lin+1);
    draw_box_border(col+size+2,lin+1);
    print_colorBK(color_yellow, bkg);
    gotoxy(col+1,lin+1);
    printf("%-*s\r",size+1,message);
    if(type==3){
        lin++;
        posLine=lin;
        print_colorBK(color_white, bkg);
        draw_box_border(col,lin+1);
        int a = size/2-5;
        posYes=col + a;
        gotoxy(col+1,lin+1); printf("%-*s",a," ");
        print_colorBK(color_white, color_blue);
        printf(" "); print_underlined("Y"); print_colorBK(color_white, color_blue); printf("es ");
        print_colorBK(color_white, bkg);
        printf("  ");
        print_colorBK(color_white, color_blue);
        printf(" "); print_underlined("N"); print_colorBK(color_white, color_blue); printf("o ");
        print_colorBK(color_white, bkg);
        if(size>11)
            printf("%-*s\r",a+1," ");
        print_colorBK(color_white, bkg);
        draw_box_border(col+size+2,lin+1);
    }
    lin++;
    print_colorBK(color_white, bkg);
    draw_box_last(col,lin+1,size+1);
    print_colorBK(color_green, bkg);
    print_color_reset();
    fflush(stdout);
    if(type==3){
        key = -1;
        while(key!='Y' && key!='y' && key!='N' && key!='n'){
            key = key_press(MOUSE_NORMAL);
            if(key==VKEY_ENTER && cob.mouseY == posLine){
                if(cob.mouseX>=posYes && cob.mouseX<=posYes+4) key='Y';
                if(cob.mouseX>=posYes+7 && cob.mouseX<=posYes+10) key='N';
            }
        }
    }else{
        double check_start = getCurrentTime();
        while (key_press(MOUSE_NORMAL) <= 1) {
            double end_time = getCurrentTime();
            double elapsed_time = end_time - check_start;
            if (elapsed_time > type) {
                cob.showFile=TRUE;
                break;
            }
        }
    }
    return key;
}
