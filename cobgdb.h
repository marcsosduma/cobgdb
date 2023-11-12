#if defined(_WIN32)
#include <windows.h>
#define VK_DOWN 40
#define VK_UP 38
#define VK_PGDOWN 34
#define VK_PGUP 33
#define VK_ENTER 13
#elif defined(__linux__)
#define VK_DOWN 40
#define VK_UP 38
#define VK_PGDOWN 34
#define VK_PGUP 33
#define VK_RIGHT 39
#define VK_LEFT 37
#define VK_ENTER 13
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
	int lineC;
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
    int lineCobol;
    int lineC;
    int endPerformLine;
    struct ST_Line * next;
    struct ST_Line * before;
    struct ST_Line * last;
};

typedef struct ST_Line ST_Line;

struct program_file {
	char name_file[256];
	Lines * lines;
	int qtd_lines;	
};

// ATTRIBUTTES -- START
struct ST_Attribute {
    char * key;
    char * cName;
    char * type;
    int digits;
    int scale;
    char * flagStr;
    int flags[10];
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

struct st_parse {
    char * token;
    int size;
    int type;
    struct st_parse * next;
};


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

#ifdef WIN32
    char *realpath( const char *name, char *resolved );
#endif

typedef struct ST_DebuggerVariable ST_DebuggerVariable;

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
    TP_OTHER
};

int show_info();
// Tests
int testMI2();
int testParser();
// cobgdb.c
int loadfile(char * nameCobFile);
int freeFile();
// read_file.c
int readCodFile(struct program_file * program);
void GetFileParts(char *path, char *path_, char *base_, char *ext_);
char * getFileNameFromPath(char *path);
void   getPathName(char * path, char * org);
char * subString (const char* input, int offset, int len, char* dest);
void normalizePath(char * path);
char * Trim(char * s);
int isAbsolutPath(char * path);
char* toLower(char* str);
char* toUpper(char* str);
//terminal.c
void get_terminal_size(int *width, int *height);
void set_terminal_size(int width, int height);
int key_press();
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
int win_size_verify(int showFile, int *check_size);
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
ST_DebuggerVariable * getVariablesByCobol();
void freeParsed(ST_MIInfo * parsed);
// mi2.c
int couldBeOutput(char * line);
int MI2addBreakPoint(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber );
int MI2goToCursor(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber );
int MI2removeBreakPoint (int (*sendCommandGdb)(char *), Lines * lines, char * fileCobol, int lineNumber );
int MI2start(int (*sendCommandGdb)(char *));
int MI2stepOver(int (*sendCommandGdb)(char *));
int MI2stepInto(int (*sendCommandGdb)(char *));
ST_MIInfo * MI2onOuput(int (*sendCommandGdb)(char *), int token, int * status);
int MI2verifyGdb(int (*sendCommandGdb)(char *));
int MI2stepOut(int (*sendCommandGdb)(char *));
int MI2variablesRequest(int (*sendCommandGdb)(char *));
int MI2evalVariable(int (*sendCommandGdb)(char *), ST_DebuggerVariable * var, int thread, int frame);
int MI2hoverVariable(int (*sendCommandGdb)(char *), Lines * lines );
int MI2changeVariable(int (*sendCommandGdb)(char *), ST_DebuggerVariable * var, char * rawValue);
//variables.c
int show_variables(int (*sendCommandGdb)(char *));
int hover_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *), int bkg);
int show_line_var(struct st_highlt * high, char * functionName, int (*sendCommandGdb)(char *));                   
//debugger.c
char* debugParse(char* valueStr, int fieldSize, int scale, char* type);
char* formatValueVar(char* valueStr, int fieldSize, int scale, char* type);
// output.c
char * openOuput(char *target);
//highlight.c
int executeParse();
void freeHighlight(struct st_highlt * hight);
int printHighlight(struct st_highlt * hight, int bkg, int start, int tot);
//sting_parser.c
void lineParse(char * line_to_parse, struct st_parse h[100], int *qtt );
struct st_parse * tk_val(struct st_parse line_parsed[100], int qtt_tk, int pos);