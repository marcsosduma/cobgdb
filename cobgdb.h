#if defined(_WIN32)
#include <windows.h>
#define VK_DOWN 40
#define VK_UP 38
#define VK_PGDOWN 34
#define VK_PGUP 33
#define VK_ENTER 13
#ifndef VK_ESCAPE
#define VK_ESCAPE 27
#endif
#define VK_DEL 3
#define VK_LEFT 37
#define VK_RIGHT 39
#elif defined(__linux__)
#define CTRL_KEY(k) ((k) & 0x1f)
#define VK_BACKSPACE 127
#define VK_DOWN 40
#define VK_UP 38
#define VK_PGDOWN 34
#define VK_PGUP 33
#define VK_RIGHT 39
#define VK_LEFT 37
#define VK_ENTER 13
#define VK_ESCAPE 27
#define VK_DEL 3
#endif // Windows/Linux
#ifndef boolean 
#define boolean int
#endif
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
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
#include <wchar.h>
#include <time.h>
#define MAX_MATCH_LENGTH 512

struct st_highlt {
    int color;
    wchar_t * token;
    int size;
    int type;
    struct st_highlt * next;
};


struct st_lines{
	char * line;
	int file_line;
	char breakpoint;
    struct st_highlt * high;
	struct st_lines * line_after;
	struct st_lines * line_before;
    struct st_lines * last;
};

typedef struct st_lines Lines;

// LINES TO DEBUG
struct ST_Line {
    char * fileCobol;
    char * fileC;
    struct st_show_var * show_var;
    int lineCobol;
    int lineC;
    int endPerformLine;
    int isCall;
    int isEntry;
    int lineProgramExit;
    struct ST_Line * next;
    struct ST_Line * before;
    struct ST_Line * last;
};

typedef struct ST_Line ST_Line;

struct st_cobgdb {
	char name_file[256];
    char file_cobol[512];
    char title[512];
    char cfile[1048];
    char cwd[512];
    char first_file[512];
    int num_dig;
    int debug_line;
    int running;
    int showFile;
    int waitAnswer;
    int changeLine;
	Lines * lines;
	int qtd_lines;
    int ctlVar;    
    double isStepOver;
    int mouse;
    int mouseX;
    int mouseY;
    int mouseButton;	
    int line_pos;
    int input_character;
    int showVariables;
};

// ATTRIBUTTES -- START
struct ST_Attribute {
    char * key;
    char * cName;
    char * type;
    int digits;
    int scale;
    char * flagStr;
    unsigned int flags;
    struct ST_Attribute * next;
    struct ST_Attribute * before;
    struct ST_Attribute * last;
};

typedef struct ST_Attribute ST_Attribute;

enum CobFlag {
    HAVE_SIGN,
    SIGN_SEPARATE,
    SIGN_LEADING,
    BLANK_ZERO,
    JUSTIFIED,
    BINARY_SWAP,
    REAL_BINARY,
    IS_POINTER,
    NO_SIGN_NIBBLE,
    IS_FP,
    REAL_SIGN,
    BINARY_TRUNC,
    CONSTANT
};

enum MOUSE_READ {
    MOUSE_OFF,
    MOUSE_NORMAL,
    MOUSE_EXT
};

struct st_parse {
    char * token;
    int size;
    int type;
    struct st_parse * next;
};

struct st_bkpoint {
    char * name;
    int line;
    struct st_bkpoint * next;
};

typedef struct st_bkpoint ST_bk;

// ATTRIBUTTES - END
// DebuggerVariable - START
struct ST_DebuggerVariable {
        char * cobolName;
        char * cName;
        char * functionName;
        struct ST_Attribute * attribute;
        int size;
        char * value;
        char * dataSotorage;
        char * variablesByC;
        char * variablesByCobol;
        char show;
        int  ctlVar;
        struct ST_DebuggerVariable * parent;
        struct ST_DebuggerVariable * first_children;
        struct ST_DebuggerVariable * brother;
        struct ST_DebuggerVariable * next;
        struct ST_DebuggerVariable * before;
        struct ST_DebuggerVariable * last;
    };


typedef struct ST_DebuggerVariable ST_DebuggerVariable;


struct st_show_var {
    ST_DebuggerVariable * var;
    struct st_show_var * next;
};


struct ST_Watch{
    ST_DebuggerVariable * var;
    ST_Line * line;
    int status;
    int posx;
    int posy;
    int posx1;
    int posy1;
    int size;
    int new;
    time_t start_time;    
    struct ST_Watch * next; 
};
typedef struct ST_Watch ST_Watch;


// structure MI

struct ST_TableValues {
    char * key;
    char * value;
    struct ST_TableValues * array;
    struct ST_TableValues * next_array;
    struct ST_TableValues * next;
};

struct ST_OutOfBandRecord {
    int isStream;
    char * type;
    char * asyncClass;
    struct ST_TableValues * output;
    char * content;
    struct ST_OutOfBandRecord * next;
};

struct ST_ResultRecords{
    char * resultClass;
    struct ST_TableValues * results; 

};

typedef struct ST_ResultRecords ST_ResultRecords;
typedef struct ST_OutOfBandRecord ST_OutOfBandRecord;
typedef struct ST_TableValues ST_TableValues;

struct ST_MIInfo {
    int token;
    ST_OutOfBandRecord * outOfBandRecord;
    ST_ResultRecords * resultRecords;
};

typedef struct ST_MIInfo ST_MIInfo;

enum types{
    TP_SPACES,
    TP_TOKEN,
    TP_SYMBOL,
    TP_NUMBER,
    TP_COMMENT,
    TP_STRING1,
    TP_STRING2,
    TP_ALPHA,
    TP_RESERVED,
    TP_VAR_RESERVED,
    TP_OTHER
};

#ifdef WIN32
    char *realpath( const char *name, char *resolved );
#endif
int show_info();
int isCommandInstalled(const char *command);
// Tests
int testMI2();
int testParser();
// cobgdb.c
int loadfile(char * nameCobFile);
int freeFile();
int isCommandInstalled(const char *command);
double getCurrentTime();
int show_opt();
// read_file.c
int readCodFile(struct st_cobgdb * program);
void GetFileParts(char *path, char *path_, char *base_, char *ext_);
char * getFileNameFromPath(char *path);
void   getPathName(char * path, char * org);
char * subString (const char* input, int offset, int len, char* dest);
void normalizePath(char * path);
char * Trim(char * s);
int isAbsolutPath(char * path);
char* toLower(char* str);
char* toUpper(char* str);
boolean isSpace(char c);
char *istrstr(const char *haystack, const char *needle);
void fileNameWithoutExtension(char * file, char * onlyName);
void fileExtension(char * file, char * onlyExtension);
char* getCurrentDirectory();
//terminal.c
void get_terminal_size(int *width, int *height);
void set_terminal_size(int width, int height);
int key_press(int type);
void gotoxy(int x, int y);
void clearScreen();
void cursorON();
void cursorOFF();
void print(char * s, int textcolor);
void printBK(char * s, int textcolor, int backgroundcolor);
void print_no_reset(char * s, int textcolor);
void print_no_resetBK(char * s, int textcolor, int backgroundcolor);
void print_color_reset();
void print_color(int textcolor);
void print_colorBK(const int textcolor, const int backgroundcolor);
int readchar(char * str, int size);
int updateStr(char * value, int size, int x, int y);
int win_size_verify(int showFile, int *check_size);
int draw_box_first(int posx, int posy, int width, char *text);
int draw_box_last(int posx, int posy, int width);
int draw_box_border(int posx, int posy);
void draw_utf8_text(const char* text);
int showCobMessage(char * message, int type);
void disableEcho();
void enableEcho();
void freeInputBuffer();
void focusOnCobgdb();
void focus_window_by_title(const char *window_title);
#if defined(_WIN32)
void DisableMaxWindow();
#endif
// parser.c
void SourceMap(char fileGroup[][512]);
int parser(char * file_name, int fileN);
int hasCobolLine(int lineCobol);
ST_Line * getLineC(char * fileCobol, int lineCobol);
ST_Line * getLineCobol(char * fileC, int lineC);
void getVersion(char vrs[]);
// parser_mi2.c
ST_MIInfo * parseMI(char * line);
int getLinesCount();
int getVariablesCount();
ST_TableValues * parseMIvalueOf(ST_TableValues * start, char * p, char * key, boolean * find);
ST_DebuggerVariable * getVariableByCobol(char * cobVar);
ST_DebuggerVariable * getVariableByC(char * cVar);
ST_DebuggerVariable * findVariableByCobol(char * functionName, char * cobVar);
ST_DebuggerVariable * findFieldVariableByCobol(char * functionName, char * cobVar, ST_DebuggerVariable * start);
ST_DebuggerVariable * getShowVariableByC(char * cVar);
ST_DebuggerVariable * getVariablesByCobol();
void freeParsed(ST_MIInfo * parsed);
// mi2.c
int couldBeOutput(char * line);
int MI2addBreakPoint(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber );
int MI2goToCursor(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber );
int MI2removeBreakPoint (int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber );
int MI2start(int (*sendCommandGdb)(char *));
int MI2stepOver(int (*sendCommandGdb)(char *));
int MI2stepInto(int (*sendCommandGdb)(char *));
ST_MIInfo * MI2onOuput(int (*sendCommandGdb)(char *), int token, int * status);
int MI2verifyGdb(int (*sendCommandGdb)(char *));
int MI2stepOut(int (*sendCommandGdb)(char *));
int MI2variablesRequest(int (*sendCommandGdb)(char *));
int MI2evalVariable(int (*sendCommandGdb)(char *), ST_DebuggerVariable * var, int thread, int frame);
int MI2hoverVariable(int (*sendCommandGdb)(char *), Lines * lines );
int MI2editVariable(int (*sendCommandGdb)(char *), ST_DebuggerVariable * var, char * rawValue);
char * MI2getCurrentFunctionName(int (*sendCommandGdb)(char *));
int MI2getStack(int (*sendCommandGdb)(char *), int thread);
int MI2sourceFiles(int (*sendCommandGdb)(char *), char files[][512]);
int MI2attach(int (*sendCommandGdb)(char *));
int MI2lineToJump(int (*sendCommandGdb)(char *));
//variables.c
int show_variables(int (*sendCommandGdb)(char *));
int hover_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *), int bkg);
int show_line_var(struct st_highlt * high, char * functionName, int (*sendCommandGdb)(char *));  
void var_watching(Lines * exe_line, int (*sendCommandGdb)(char *), int waitAnser, int debug_line);  
void show_sources(int (*sendCommandGdb)(char *), int mustParse);
void show_help();
//debugger.c
char* debugParseBuilIn(char* valueStr, int fieldSize, int scale, char* type, unsigned int flags, int digits);
char* formatValueVar(char* valueStr, int fieldSize, int scale, char* type, unsigned int flags);
char* parseUsage(char* valueStr);
// output.c
void openOuput(int (*sendCommandGdb)(char *), char *target);
//highlight.c
int highlightParse();
void freeHighlight(struct st_highlt * hight);
int printHighlight(struct st_highlt * hight, int bkg, int start, int tot);
//sting_parser.c
void lineParse(char * line_to_parse, struct st_parse h[100], int *qtt );
struct st_parse * tk_val(struct st_parse line_parsed[100], int qtt_tk, int pos);