/* 
   The functions related to colorization were obtained from the following site: 
   https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
   The information on how to use the rawmode function on Linux was obtained 
   from this website: https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
   Much of the information about the Windows Console interface was obtained from this
   website: https://learn.microsoft.com/en-us/windows/console/
   This program is provided without any warranty, express or implied.
   You may modify and distribute it at your own risk.
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
#define VK_BACKSPACE 8
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
#endif // Windows/Linux
#include <time.h>
#include "cobgdb.h"

#define STRCURSOR_OFF "\e[?25l"
#define STRCURSOR_ON  "\e[?25h"

char color[200];
extern struct st_cobgdb cob;

void cursorON();
void cursorOFF();
void clearScreen();
int mouseCobAction(int col, int line, int isPrincipal);
int mouseCobRigthAction(int col, int line);
void mouseCobHover(int col, int line);

int TERM_WIDTH = VIEW_COLS;
int TERM_HEIGHT= VIEW_LINES;

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

int readKeyLinux(int isPrincipal) {
	int nread;
	char c;
	if ((nread = read(STDIN_FILENO, &c, 1)) != 1)	{
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}
	if (c == TM_ESCAPE){
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return TM_ESCAPE;
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return TM_ESCAPE;

		if (seq[0] == TM_CSI){
			if (seq[1] >= '0' && seq[1] <= '9')	{
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return TM_ESCAPE;
				if (seq[2] == '~')	{
					switch (seq[1])
					{
					case '5':
						return VK_PGUP;
					case '6':
						return VK_PGDOWN;
					}
				}
			}
            else if( seq[1] == TM_MOUSE){
                struct {
                    unsigned char button;
                    unsigned char x;
                    unsigned char y;
                } mouseEvent;

                if(read(STDIN_FILENO, &mouseEvent, sizeof(mouseEvent)) == sizeof(mouseEvent)) {
                    if (mouseEvent.button == 96) {
                        return VK_UP;
                    }
                    if (mouseEvent.button == 97) {
                        return VK_DOWN;
                    }
                    mouseCobHover( mouseEvent.x - 33, mouseEvent.y - 33);
                    cob.mouseX = mouseEvent.x - 33; cob.mouseY = mouseEvent.y - 33;
                    int ret=-1;
                    if(mouseEvent.button == 32 ){
                        return mouseCobAction(mouseEvent.x - 33, mouseEvent.y - 33, isPrincipal);                        
                    }
                    if(mouseEvent.button == 34 && isPrincipal){
                        return mouseCobRigthAction(mouseEvent.x - 33, mouseEvent.y - 33);                        
                    }
                    //printf("Mouse X: %d, Y: %d, Button: %d\n", mouseEvent.x - 32, mouseEvent.y - 32, mouseEvent.button);
                }
                return -1;
            }else {
				switch (seq[1])
				{
                    case 'A':
                        return VK_UP;
                    case 'B':
                        return VK_DOWN;
                    case 'C':
                        return VK_RIGHT;
                    case 'D':
                        return VK_LEFT;
				}
			}
		}
		return TM_ESCAPE;
	}else{
		return c;
	}
}
#endif

void mouseCobHover(int col, int line){
    cob.mouse = 0;
    if(col==79 && line<11) cob.mouse=1;
    if(col==79 && line>=11) cob.mouse=2;
    if(col<cob.num_dig+2 && line>0 && line<22) cob.mouse=3;
    if(col==67 && line==0) cob.mouse=10;
    if(col==69 && line==0) cob.mouse=20;
    if(col==71 && line==0) cob.mouse=30;
    if(col==73 && line==0) cob.mouse=40;
    if(col==75 && line==0) cob.mouse=50;    
    if(col==77 && line==0) cob.mouse=60;    
}

int mouseCobAction(int col, int line, int isPrincipal){
    int action=-1;
    switch (cob.mouse){
            case 1:
                action = VK_PGUP;
                break;
            case 2:
                action = VK_PGDOWN;
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
                action = '?';
                break;
            default:
                action = -1;
                break;
    }
    if(action == -1 && line>0 && line<22){
        if(isPrincipal){
            cob.line_pos = line-1;
            cob.showFile = TRUE;
        }
        if(col<cob.num_dig+2){
            action = 'B';
        }else{
            action=VK_ENTER;
        }
    }
    return action;
}

int mouseCobRigthAction(int col, int line){
    int action= 'H';
    if(line>0 && line<22){
        cob.line_pos = line-1;
        cob.showFile = TRUE;
        if(col>1 && col<80){
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


int key_press(int isPrincipal){
#if defined(_WIN32)
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD inp;
    DWORD numEvents;
    DWORD mode = ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    MOUSE_EVENT_RECORD mer;

    SetConsoleMode(hIn, mode);

    if (PeekConsoleInput(hIn, &inp, 1, &numEvents) && numEvents > 0) {
        if (ReadConsoleInput(hIn, &inp, 1, &numEvents)) {
            if (inp.EventType == MOUSE_EVENT) {
                mer = inp.Event.MouseEvent;
                cob.mouseX = mer.dwMousePosition.X;
                cob.mouseY = mer.dwMousePosition.Y;
                mouseCobHover(cob.mouseX, cob.mouseY);
                if(mer.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED ){
                    return mouseCobAction(cob.mouseX, cob.mouseY, isPrincipal);
                }
                if(mer.dwButtonState == RIGHTMOST_BUTTON_PRESSED && isPrincipal){
                    return mouseCobRigthAction(cob.mouseX, cob.mouseY);                        
                }
                if (inp.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) {
                    int delta = GET_WHEEL_DELTA_WPARAM(inp.Event.MouseEvent.dwButtonState);
                    if (delta > 0) 
                        return VK_UP;
                    else
                        return VK_DOWN;
                }
                
                //printf("Mouse X: %d, Y: %d\n", mouseX, mouseY);
            }
            if (inp.EventType == KEY_EVENT){
                if (inp.Event.KeyEvent.bKeyDown && inp.Event.KeyEvent.uChar.AsciiChar != 0) {
                    char key = inp.Event.KeyEvent.uChar.AsciiChar;
                    if(key<0){
                          ReadConsoleInput( hIn, &inp, 1, &numEvents); 
                          key=0;
                    }
                    return (int)key;
                } else {
                    int virtualKeyCode = inp.Event.KeyEvent.wVirtualKeyCode;
                    if (virtualKeyCode==219) {
                        ReadConsoleInput( hIn, &inp, 1, &numEvents); 
                        return 0; // accents problem...
                    }
                    if(!inp.Event.KeyEvent.bKeyDown) return 0;
                    FlushConsoleInputBuffer(hIn);
                    return virtualKeyCode;
                }
            }
        }
    }
    return 0;
      
#elif defined(__linux__)
  enableRawMode();
  int ch_lin= readKeyLinux(isPrincipal);
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
            c = key_press(FALSE);
            fflush(stdout);
        }while(c<=0);
        if (c == 13) {
            break;
        }
        if ((c == VK_BACKSPACE | c==37) && i > 0) {
            putchar(8);
            putchar(' ');
            putchar(8);
            i--;
            str[i] = '\0';
        }else if(c==39 && strlen(str) < size){
            putchar(' ');
            str[i++]=' ';
            str[i]='\0';
        } else if (strlen(str) < size && c >= 32) {
            str[i] = c;
            putchar(c);
            i++;
            str[i] = '\0';
        }
    }
    cursorOFF();
    return 0;
}


int updateStr(char * value, int size, int x, int y) {
    char c =' ';
    int i = 0;
    char * str = value;
    int len = strlen(str);
    if (str[0] == '"') {
        str = &str[1];
        len = strlen(str);
        if (str[len - 1] == '"') {
            len--;
            str[len]='\0';
        }
    }
    int lt = (len>size)?size:len;
    int startChar = 0;
    int isPrint = TRUE;
    cursorON();
    i = 0;
    while (1) {
        if(isPrint){
            gotoxy(x, y);
            printf("%.*s\r",size,&str[startChar]);
            fflush(stdout);
            isPrint=FALSE;
        }
        gotoxy(x+i, y);
        do{
            c = key_press(FALSE);
            fflush(stdout);
        }while(c<=0);
        if (c == 13) {
            break;
        }
        if ((c == VK_BACKSPACE | c==37) && i >= 0) {
            i--;
            if(i<0){
                startChar = (startChar>0)?startChar-1: startChar;
                isPrint=TRUE;
                i=0;
            }
        }else if(c==39){
            i++;
            if(i>=lt){
                startChar = (str[i+startChar]!='\0')? startChar+1: startChar;
                isPrint=TRUE;
                i--;
            }
        } else if (i < lt && c >= 32) {
            if(c==37 && i==0) continue;
            str[startChar+i] = c;
            i++;
            if(i>=lt){
                startChar = (str[i+startChar]!='\0')? startChar+1: startChar;
                i--;
            }
            isPrint=TRUE;
        }
    }
    cursorOFF();
    return 0;
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
#endif // Windows/Linux
}

#if defined(_WIN32)
int win_size_verify(int showFile, int *check_size){
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD consoleSize;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL isMaximized = IsZoomed(hConsole);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    consoleSize = GetLargestConsoleWindowSize(hConsole);
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


void set_terminal_size(int width, int height){
    TERM_WIDTH = width;
    TERM_HEIGHT = height;
#if defined(_WIN32)
    int chesize=0;
    HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    SetForegroundWindow(hConsole);
    SetConsoleSize(hConsole, width, height);
    SMALL_RECT sr = {0,0,TERM_WIDTH,TERM_HEIGHT};
    SetConsoleWindowInfo(hConsole, TRUE, &sr);
    SetForegroundWindow(GetConsoleWindow());
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD bufferSize = { csbi.dwSize.X, csbi.srWindow.Bottom + 1 };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
#elif defined(__linux__)
    // Set the input buffer size to a larger value (e.g., 1024 bytes)
    //int bufferSize = 1024;
    //if (ioctl(STDIN_FILENO, FIONREAD, &bufferSize) == -1) {
    //    perror("ioctl");
    //    exit(EXIT_FAILURE);
    //}
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
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


void gotoxy(int x, int y){
    #if defined(_WIN32)
    COORD pos = {x-1, y-1};
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(output, pos);
    #elif defined(__linux__)
    printf("\033[%d;%dH", (y), (x));
    #endif
}

#ifdef _WIN32
    HANDLE hBuffer1;
    COORD bufferSize = {VIEW_COLS, VIEW_LINES};
    CHAR_INFO buffer[VIEW_COLS * VIEW_LINES];
    SMALL_RECT rect;
#endif


void clearScreen() {
#ifdef _WIN32
    HANDLE hStdOut;
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // Fetch existing console mode so we correctly add a flag and not turn off others
    DWORD mode = 0;
    GetConsoleMode(hStdOut, &mode);
    // Hold original mode to restore on exit to be cooperative with other command-line apps.
    DWORD originalMode = mode;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    // Try to set the mode.
    SetConsoleMode(hStdOut, mode);
    // Write the sequence for clearing the display.
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
    for (int i = 0; i < (width - wcslen(utext)); ++i) {
        strcat(line_h, horizontal);
    }
    strcat(line_h, topRightCorner);
    strcat(line_h, "\r");
    draw_utf8_text(line_h);
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
}

int draw_box_border(int posx, int posy) {
    char vertical[]   = "\u2502";   // UTF-8 character for vertical line
    gotoxy(posx, posy);
    draw_utf8_text(vertical);
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
    if(strlen(aux)>size) size=strlen(aux);
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
        printf(" Yes ");
        print_colorBK(color_white, bkg);
        printf("  ");
        print_colorBK(color_white, color_blue);
        printf(" No ");
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
    fflush(stdout);
    if(type==3){
        key = -1;
        while(key!='Y' && key!='y' && key!='N' && key!='n'){
            key = key_press(FALSE);
            if(key==VK_ENTER && cob.mouseY == posLine){
                if(cob.mouseX>=posYes && cob.mouseX<=posYes+4) key='Y';
                if(cob.mouseX>=posYes+7 && cob.mouseX<=posYes+10) key='N';
            }
        }
    }else{
        clock_t start_time = clock();
        while (key_press(FALSE) <= 0) {
            clock_t end_time = clock();
            double elapsed_time = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
            if (elapsed_time > 1) {
                break;
            }
        }
    }
    return key;
}
