/* 
   The functions related to colorization were obtained from the following site: 
   https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#if defined(_WIN32)
#include <Windows.h>
#include <stdio.h>
#include <WinCon.h>
#include <conio.h>
#include <stdlib.h>
#define maxViewWidth 200
#define maxViewHeight 100
void SetConsoleSize(HANDLE hStdout, int cols, int rows );
#define VK_BACKSPACE 8
#elif defined(__linux__)
#include <sys/ioctl.h> // ioctl, TIOCGWINSZ
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#endif // Windows/Linux

#define color_black      0
#define color_dark_blue  1
#define color_dark_green 2
#define color_light_blue 3
#define color_dark_red   4
#define color_magenta    5
#define color_orange     6
#define color_light_gray 7
#define color_gray       8
#define color_blue       9
#define color_green     10
#define color_cyan      11
#define color_red       12
#define color_pink      13
#define color_yellow    14
#define color_white     15

char color[200];

void cursorON();
void cursorOFF();

#if defined(__linux__)
#define CTRL_KEY(k) ((k) & 0x1f)
#define VK_DOWN 40
#define VK_UP 38
#define VK_PGDOWN 34
#define VK_PGUP 33
#define VK_RIGHT 39
#define VK_LEFT 37
#define VK_ENTER 13
#define VK_BACKSPACE 127

struct editorConfig {
	  int cx, cy;
	  int screenrows;
	  int screencols;
	  struct termios orig_termios;
};

struct editorConfig E;

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}

void disableRawMode() {
	  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
		      die("tcsetattr");

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

int readKeyLinux() {
	int nread;
	char c;
	if ((nread = read(STDIN_FILENO, &c, 1)) != 1)	{
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}
	if (c == '\x1b'){
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if (seq[0] == '['){
			if (seq[1] >= '0' && seq[1] <= '9')	{
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
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
			else {
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
		return '\x1b';
	}else{
		return c;
	}
}

int getch(void) 
{ 
    struct termios oldattr, newattr; 
    int ch; 
    tcgetattr(STDIN_FILENO, &oldattr); 
    newattr = oldattr; 
    newattr.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr); 
    read(STDIN_FILENO, &ch, 1);
    //ch = getchar(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); 
    return ch; 
} 
/* reads from keypress, echoes */ 
int getche(void) 
{ 
    struct termios oldattr, newattr; 
    int ch; 
    tcgetattr(STDIN_FILENO, &oldattr); 
    newattr = oldattr; 
    newattr.c_lflag &= ~(ICANON); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr); 
    ch = getchar(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); 
    return ch; 
}
#endif

#if defined(_WIN32)
static int is_readable_console(HANDLE h)
{
    int ret = 0;
    DWORD n = 0;
    INPUT_RECORD ir;

    if (PeekConsoleInputA(h, &ir, 1, &n) && n > 0) {
        if (ir.EventType == KEY_EVENT) {
            ret = 1;
        }
        else {
           ReadConsoleInputA(h, &ir, 1, &n);
        }
    }
    return ret;
}
#endif

int key_press(){
#if defined(_WIN32)
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD inp;
    DWORD numEvents;

    if (is_readable_console(hIn)){ 
            ReadConsoleInput( hIn, &inp, 1, &numEvents);
            ReadConsoleInput( hIn, &inp, 1, &numEvents); 
            if (inp.Event.KeyEvent.uChar.AsciiChar != 0) {
                char character = inp.Event.KeyEvent.uChar.AsciiChar;
                return character;
            } else {
                switch (inp.EventType)
                {
                    case KEY_EVENT:
                    return inp.Event.KeyEvent.wVirtualKeyCode;
                    break;
                }
            }
    }
    /*
    if (PeekConsoleInput(hIn, &inp, 1, &numEvents) && numEvents > 0) {
        if (ReadConsoleInput(hIn, &inp, 1, &numEvents)) {
            if (inp.EventType == KEY_EVENT && inp.Event.KeyEvent.bKeyDown) {
                if (inp.Event.KeyEvent.uChar.AsciiChar != 0) {
                    return inp.Event.KeyEvent.uChar.AsciiChar;
                } else if (inp.Event.KeyEvent.uChar.UnicodeChar != 0) {
                    return inp.Event.KeyEvent.uChar.UnicodeChar;
                } else {
                    return inp.Event.KeyEvent.wVirtualKeyCode;
                }
            }
        }
    }
    */  
#elif defined(__linux__)
  enableRawMode();
  int ch_lin= readKeyLinux();
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
            c = key_press();
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

void set_terminal_size(int width, int height){
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo2;
    HANDLE hConsoleOut = GetStdHandle( STD_OUTPUT_HANDLE );
    SetConsoleSize(hConsoleOut, width, height);
#elif defined(__linux__)
   //signal(SIGWINCH, winsz_handler);
   struct winsize w;
   char buffer[80]="";
   sprintf(buffer, "\033[8;%d;%dt\n", height, width);
   ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
   printf("%s",buffer);
   fflush(stdout);
   tcdrain(STDOUT_FILENO);
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
    COORD bufferSize = {80, 25};
    CHAR_INFO buffer[80 * 25];
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
    printf("\033[H\033[J");
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
    printf("\e[?25l");
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
    printf("\e[?25h");
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
      SetConsoleScreenBufferSize( hStdout, newSize );
      SetConsoleWindowInfo( hStdout, TRUE, &rect );
    }else{                           // cols--, rows+
      SMALL_RECT tmp = { 0, 0, cols-1, oldr-1 };
      SetConsoleWindowInfo( hStdout, TRUE, &tmp );
      goto BUFWIN;
    }
  }else {
    if ( oldc <= cols ) {                           // cols+, rows--
      SMALL_RECT tmp = { 0, 0, oldc-1, rows-1 };
      SetConsoleWindowInfo( hStdout, TRUE, &tmp );
      goto BUFWIN;
    }
    else
    {                           // cols--, rows--
      SetConsoleWindowInfo( hStdout, TRUE, &rect );
      SetConsoleScreenBufferSize( hStdout, newSize );
    }
  }
  GetConsoleScreenBufferInfo( hStdout, &csbiInfo );
}

void cls_screen(HANDLE hConsole)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT scrollRect;
    COORD scrollTarget;
    CHAR_INFO fill;

    // Get the number of character cells in the current buffer.
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    // Scroll the rectangle of the entire buffer.
    scrollRect.Left = 0;
    scrollRect.Top = 0;
    scrollRect.Right = csbi.dwSize.X;
    scrollRect.Bottom = csbi.dwSize.Y;

    // Scroll it upwards off the top of the buffer with a magnitude of the entire height.
    scrollTarget.X = 0;
    scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

    // Fill with empty spaces with the buffer's default text attribute.
    fill.Char.UnicodeChar = TEXT(' ');
    fill.Attributes = csbi.wAttributes;

    // Do the scroll
    ScrollConsoleScreenBuffer(hConsole, &scrollRect, NULL, scrollTarget, &fill);

    // Move the cursor to the top left corner too.
    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = 0;

    SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
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

