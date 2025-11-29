/*
 * COBGDB GnuCOBOL Debugger:
 * This code is based on the GnuCOBOL Debugger extension available at:
 * https://github.com/OlegKunitsyn/gnucobol-debug
 *
 * License:
 * This code is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 * 
 * Author: Marcos Martins Duma
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#if defined(__linux__)
#include <unistd.h>
#include <stdint.h>
#else
#include <windows.h>
#include <io.h>
#define access _access
#define F_OK 0
#endif
#include "cobgdb.h"
#define __WITH_TESTS_
#define COBGDB_VERSION "2.2.0"

struct st_cobgdb cob ={
    .debug_line = -1,
    .ctlVar = -1,
    .running = TRUE,
    .waitAnswer = FALSE,
    .changeLine = FALSE,
    .showFile = TRUE,
    .isStepOver = -1,
    .mouse = 0,
    .num_dig = 4,
    .showVariables = FALSE,
    .status_bar = 0,
};

int VIEW_COLS=  80;
int VIEW_LINES= 24;

int start_window_line = 0;
int qtd_window_line = 22;
int start_line_x = 0;
int WAIT_GDB=100;
volatile int CHECKING_SCR_SIZE = 0;
volatile int CHECKING_HOVER = 0;
int mousex=-1;
int mousey=-1;
double check_hover_start=0;
char * gdbOutput = NULL;
int color_frame=color_light_blue;

ST_Line * LineDebug=NULL;
ST_Attribute * Attributes=NULL;
ST_DebuggerVariable * DebuggerVariable=NULL;
ST_bk * BPList=NULL;
ST_Watch * Watching=NULL;
Lines * lines = NULL;
struct st_cfiles * parsed_cfiles = NULL;
int start_gdb(char * name, char * cwd);

void free_memory()
{
    Lines * lines = cob.lines;
    Lines *current_line;

    while (lines != NULL) {
        current_line = lines;
        lines = lines->line_after;
        if (current_line->line != NULL) {
            free(current_line->line);
            current_line->line= NULL;
        }
        if (current_line->high != NULL) {
            freeHighlight(current_line->high);
            current_line->high= NULL;
        }
        free(current_line);
    }
    cob.lines = NULL;
}

/**
 * Free c files parsed.
 */
void free_cfiles(void) {
    struct st_cfiles * curr = parsed_cfiles;
    while (curr) {
        struct st_cfiles *next = (struct st_cfiles *) curr->next;
        free(curr->file);
        free(curr);
        curr = next;
    }
    parsed_cfiles = NULL;
}


void freeWatchingList()
{
    ST_Watch * search = Watching;
    ST_Watch *current;

    while (search != NULL) {
        current = search;
        search = search->next;
        free(current);
    }
    Watching = NULL;
}

void freeBKList()
{
    ST_bk * search = BPList;
    ST_bk *current;

    while (search != NULL) {
        current = search;
        search = search->next;
        free(current->name);
        free(current);
    }
    BPList = NULL;
}

void show_version(){
    printf("CobGDB - GnuCobol GDB Interpreter - version %s\n", COBGDB_VERSION);
    printf("Copyright (C) 2013 Free Software Foundation, Inc.\n");
    printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
    printf("This is free software: you are free to change and redistribute it.\n");
    #ifdef __MINGW32__
    printf("This CobGDB was configured as \"MinGW32\".\n");
    #elif defined(__GNUC__)
    printf("This CobGDB was compiled with GCC.\n");
    #else
    printf("Compiler information not available.\n");
    #endif
    printf("For bug reporting instructions, please see:\n");
    printf("<https://github.com/marcsosduma/cobgdb>.\n");
}

#if defined(__linux__)
#if !defined(CLOCK_MONOTONIC)
#define CLOCK_MONOTONIC		1
#endif
#endif

double getCurrentTime() {
#ifdef _WIN32
    LARGE_INTEGER frequency, start;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    return (double)start.QuadPart / frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

// Function to check the availability of a command
int isCommandInstalled(const char *command) {
#ifdef _WIN32
    // Windows implementation
    char fullPath[MAX_PATH];
    DWORD result = SearchPath(NULL, command, ".exe", MAX_PATH, fullPath, NULL);
    return result != 0;
#else
    // Linux implementation
    char whichCommand[100];
    snprintf(whichCommand, sizeof(whichCommand) - 1, "which %s > /dev/null 2> /dev/null", command);
    return system(whichCommand) == 0;
#endif
}

int cobc_compile(char file[][512], char values[10][256], int arg_count){
    char *param[20]; 

    // Check if the cobc command is available
    if(!isCommandInstalled("cobc")){
        printf("The GnuCOBOL cobc command is not available!\n");
        fflush(stdout);
        while(key_press(MOUSE_OFF)<=0);
        return 0;
    }
    // Initialize the first elements with the initial values.
    char *initial_params[] = {
        "-g ",
        "-fsource-location ",
        "-ftraceall ",
        "-v ",
        "-O0 ",
        "-x "
    };
    int param_count = sizeof(initial_params) / sizeof(initial_params[0]);
    int initFree = param_count;
    for (int i = 0; i < param_count; i++) {
        param[i] = initial_params[i];
    }
    for (int i = 0; i < arg_count && param_count < 20; i++) {
        char *param_value = malloc(strlen(values[i]) + 8);
        if (param_value == NULL) {
            return 1;
        }
        snprintf(param_value, strlen(values[i]) + 8, "%s ", values[i]);
        param[param_count] = param_value;
        param_count++;
    }

    char compiler[512] = "cobc ";
    
    for (int a = 0; a < param_count; a++) {
        strcat(compiler, param[a]);
        if(a>=initFree) free(param[a]); 
    }
    int nfile=0;
    while(strlen(file[nfile])>1){
        strcat(compiler, file[nfile++]);
        strcat(compiler, " ");
    }
    printf("%s\n", compiler);

    FILE *pfd = popen(compiler, "r");
    if (pfd == NULL) {
        printf("failed to open pipe\n");
        return 1;
    }

    while (!feof(pfd)) {
        char buf[1024] = {0};
        fgets(buf, sizeof(buf) - 1, pfd);
        printf("%s", buf);
    }

    int ret = pclose(pfd);
    return ret;
}

Lines * set_window_pos(int * line_pos){
    if(cob.debug_line>=0){
        // exec_line before window
        if(cob.debug_line<start_window_line || (cob.debug_line-start_window_line-*line_pos)>10){
            start_window_line = cob.debug_line - (qtd_window_line*30)/100;
            if(start_window_line<0) start_window_line=0;
            if(qtd_window_line>cob.qtd_lines) qtd_window_line=cob.qtd_lines;
            *line_pos = cob.debug_line - 1 - start_window_line;
        }else{
            int new_pos = cob.debug_line - 1 - start_window_line;
            if(new_pos<(VIEW_LINES-4)){
                *line_pos=new_pos;
            }else{
                start_window_line = cob.debug_line - VIEW_LINES + 4;
                *line_pos=(VIEW_LINES-5);
            }
        }
    }
    Lines * line = cob.lines;
    if(line!=NULL){
        while(line->file_line<=start_window_line){
            if(line->line_after!=NULL) line=line->line_after;
        }
    }
    return line;
}

int show_esc_exit(){
    gotoxy(1,VIEW_LINES);
    print_colorBK(color_yellow, color_black);
    printf("%-*s\r",VIEW_COLS-1, "Press ESC to exit");
    fflush(stdout);
    return TRUE;
}

int search_text(Lines ** lines) {
    Lines *line = cob.lines;
    wchar_t buffer[500];
    int lin = VIEW_LINES-9;
    int col = (VIEW_COLS-60)/2;
    int bkg = color_dark_red;
    int found = 0;

    show_esc_exit();
    gotoxy(10, lin + 2);
    print_colorBK(color_white, bkg);
    draw_box_first(col, lin + 2, 61, "Search");
    draw_box_border(col, lin + 3);
    draw_box_border(col+62, lin + 3);
    print_colorBK(color_yellow, bkg);
    gotoxy(col+1, lin + 3);
    printf("%61s"," ");
    lin++;
    print_colorBK(color_white, bkg);
    draw_box_last(col, lin + 3, 61);
    print_colorBK(color_green, bkg);
    fflush(stdout);
    gotoxy(col+61, lin + 2);
    int a=strlen(cob.find_text);
    if(a<61){
        cob.find_text[a]=' ';
        cob.find_text[99]='\0';
    }
    if (updateStr(cob.find_text, 61, col+1, lin + 2) == FALSE)
        return 1;
    a=60; 
    while(a>=0){
        if(cob.find_text[a]!=' '){
            cob.find_text[a+1]='\0';
            break;
        }else if(a==0)
            cob.find_text[a]='\0';
        a--;
    }
    #if defined(_WIN32)
    MultiByteToWideChar(CP_UTF8, 0, cob.find_text, -1, buffer, 500);
    #else
    mbstowcs(buffer, cob.find_text, 499);
    #endif
    buffer[499] = L'\0';
    wchar_t *wcharString = malloc(501 * sizeof(wchar_t));
    if (!wcharString) return -1;
    int init_pos = start_window_line + cob.line_pos;
    int line_pos = 0;
    int loop = (cob.line_pos>0)?TRUE:FALSE;
    while (line != NULL) {
        if (!line->line) {
            line = line->line_after;
            continue;
        }
        if(line_pos++<init_pos){
            line = line->line_after;
            continue;
        }
        int lenLine = strlen(line->line);
        if (lenLine > 500) {
            free(wcharString);
            wcharString = malloc((lenLine + 1) * sizeof(wchar_t));
            if (!wcharString) return -1;
        }
        #if defined(_WIN32)
        MultiByteToWideChar(CP_UTF8, 0, line->line, -1, wcharString, lenLine + 1);
        #else
        mbstowcs(wcharString, line->line, lenLine + 1);
        #endif
        const wchar_t *pos = wcsistr(wcharString, buffer);
        if (pos) {
            start_window_line = line_pos;
            cob.line_pos = 0;
            *lines=line;
            int qtt=3;
            while((*lines)->line_before!=NULL && qtt-->0){
                *lines = (*lines)->line_before;
                 start_window_line--;
                 cob.line_pos++;
            }
            found = 2;
            break;
        }
        line = line->line_after;
        if(line==NULL && loop){
            loop=FALSE;
            line = cob.lines;
            init_pos = 0; line_pos=0;
        }
    }
    free(wcharString);
    return found;
}

int gotoLine(Lines **lines) {
    Lines *line = cob.lines;
    char buffer[500];
    int lin = VIEW_LINES-9;
    int col = (VIEW_COLS-22)/2;
    int bkg = color_dark_red;
    int found = 0;

    show_esc_exit();
    gotoxy(col, lin + 2);
    print_colorBK(color_white, bkg);
    draw_box_first(col, lin + 2, 21, "Line");
    draw_box_border(col, lin + 3);
    draw_box_border(col+22, lin + 3);
    print_colorBK(color_yellow, bkg);
    gotoxy(col+1, lin + 3);
    printf("%21s"," ");
    lin++;
    print_colorBK(color_white, bkg);
    draw_box_last(col, lin + 3, 21);
    print_colorBK(color_green, bkg);
    fflush(stdout);
    gotoxy(col+1, lin + 2);
    if(readnum(buffer,20)==FALSE)
        return 1;
    int new_line = atoi(buffer);
    if(new_line==0){
        return 0;
    }
    while (line != NULL) {
        if(new_line==line->file_line){
            start_window_line = new_line;
            cob.line_pos = 0;
            *lines=line;
            int qtt=3;
            while((*lines)->line_before!=NULL && qtt-->0){
                *lines = (*lines)->line_before;
                 start_window_line--;
                 cob.line_pos++;
            }
            found = 2;
            break;
        }
        line = line->line_after;
    }
    return found;
}


int show_button() {
    static const char *symbols[] = {
        "\u2592 ", "\u25BA ", "\u2192 ", "\u2193 ",
        "\u2191 ", "\u25A0 ", "\u2194 ", "D ", "? "
    };
    static const int mouse_ids[] = {
        0, 10, 20, 30, 40, 50, 60, 70, 80
    };
    print_colorBK(color_blue, color_cyan);
    gotoxy(VIEW_COLS - 18, 1);
    for (int i = 0; i < 9; i++) {
        int bg = (cob.mouse == mouse_ids[i] && i>0) ? color_red : color_blue;
        print_colorBK(bg, color_cyan);
        if (i >= 7)
            printf("%s", symbols[i]);
        else
            draw_utf8_text(symbols[i]);
    }
    print_color_reset();
    return TRUE;
}

int show_opt(){
    char * opt = " CobGDB                  GnuCOBOL GDB Interpreter";
    char aux[200];
    snprintf(aux,VIEW_COLS+2,"%-*.*s\r",VIEW_COLS-19, VIEW_COLS-19, opt);
    gotoxy(1,1);
    printBK(aux, color_white, color_frame);
    if(cob.mouse>9) show_button();
    gotoxy(VIEW_COLS,1);
    print_colorBK(color_light_gray, color_frame);
    draw_utf8_text("\u2191");
    gotoxy(1,VIEW_LINES-1);
    snprintf(aux,VIEW_COLS+2,"%-*.*s\r",VIEW_COLS-1,VIEW_COLS-1,cob.file_cobol);
    printBK(aux, color_white, color_frame);
    gotoxy(VIEW_COLS,VIEW_LINES-1);
    print_colorBK(color_light_gray, color_frame);
    draw_utf8_text("\u2193");
    return TRUE;
}

int show_wait(){
    gotoxy(1,VIEW_LINES);
    print_colorBK(color_red, color_black);
    printf("%-*s\r",VIEW_COLS-1, "Running");
    fflush(stdout);
    return TRUE;
}

int show_info() {
    int len =
    #if defined(_WIN32)
        VIEW_COLS - 1;
    #else
        VIEW_COLS;
    #endif
    const char *msg = NULL;
    int bg = color_black;
    int fg = color_yellow;
    gotoxy(1, VIEW_LINES);
    if (cob.status_bar > 0) {
        fg = color_pink;
        switch (cob.status_bar) {
            case 1: msg = "Parsing file"; break;
            case 2: msg = "Loading file"; break;
        }
    }else if (cob.mouse != 0) {
        fg = color_green;
        switch (cob.mouse) {
            case 1:  msg = "page up"; break;
            case 2:  msg = "page down"; break;
            case 3:  msg = "breakpoint"; break;
            case 10: msg = "run"; break;
            case 20: msg = "next"; break;
            case 30: msg = "step"; break;
            case 40: msg = "go"; break;
            case 50: msg = "quit"; break;
            case 60: msg = "switch to the debug output"; break;
            case 70:
                msg = cob.showVariables ?
                        "display of variables: ON" :
                        "display of variables: OFF";
                break;
            case 80: msg = "help"; break;
        }
    }else {
        if (cob.debug_line > 0 && !cob.running) {
            fg = color_green;
            msg = "Debugging";
        }else if (cob.debug_line > 0 && cob.running) {
            fg = color_red;
            msg = "Running";
        }else {
            fg = color_yellow;
            msg = cob.waitAnswer ? "Waiting" : " ";
        }
    }
    if (msg != NULL) {
        print_colorBK(fg, bg);
        printf("%-*s\r", len, msg);
    }
    gotoxy(1, VIEW_LINES);
    show_button();
    gotoxy(1, VIEW_LINES);
    if (!cob.showFile)
        fflush(stdout);
    return TRUE;
}

int show_file(Lines * lines_file, int line_pos, Lines ** line_debug){
    Lines * show_line=lines_file;
    int NUM_TXT=VIEW_COLS-4-cob.num_dig;    

    gotoxy(1,1);
    show_opt();
    int  bar_color = color_dark_gray;
    int  bkgColor=color_gray;
    char chExec = ' ';
    for(int i=0;i<(VIEW_LINES-3);i++){
        gotoxy(1,i+2);
        if(!show_line){
            print_no_resetBK(" ",color_white, color_frame);
            print_colorBK(color_white,color_black);
            printf("%*s",VIEW_COLS-2," ");
            print_no_resetBK(" \r",color_white, color_frame);
            continue;
        }
        //\u25CF or B (Breakpoint)
        printBK(show_line->breakpoint == 'S' ? "B" : " ", color_yellow, color_frame);
        //print_colorBK(color_red, color_frame);
        //draw_utf8_text(show_line->breakpoint == 'S' ? "\u25CF" : " ");
        print_colorBK(color_black, color_black);
        if(cob.debug_line==show_line->file_line && !cob.running){
            print(">", color_green);
            if(WAIT_GDB>10)
                focusOnCobgdb();
            WAIT_GDB = 0;
            *line_debug=show_line;
        }else{
            chExec = (cob.debug_line==show_line->file_line)?'!': ' ';
            print_color(color_red);
            printf("%c",chExec);
        }     
        print_colorBK(color_gray, color_black);
        printf("%-*d ", cob.num_dig, show_line->file_line);
        if(show_line->high==NULL){
            if(show_line->line)
                show_line->line[strcspn(show_line->line,"\n")]='\0';
            int  len=strlen(show_line->line);
            wchar_t *wcharString = to_wide(show_line->line);
            len = wcslen(wcharString);
            if(line_pos==i)
                print_colorBK(color_green, bkgColor);
            else
                print_colorBK(color_white, color_black);
            int  nm = len - start_line_x;
            int  qtdMove = (NUM_TXT < nm)? NUM_TXT :len - start_line_x;
            if(qtdMove>0){
                printf("%.*ls",qtdMove, &wcharString[start_line_x]);
                nm=NUM_TXT-qtdMove;
                if(nm>0) printf("%*s", nm, " ");
            }else{
                printf("%*s", VIEW_COLS-7, " ");
            }
            free(wcharString);
        }else{
            bkgColor=(line_pos==i)?bar_color:-1;
            printHighlight(show_line->high, bkgColor, start_line_x, NUM_TXT);
        }
        show_line = show_line->line_after;
        if(!show_line && line_pos>i)
            line_pos=i-1;
        print_no_resetBK(" ",color_white, color_frame);
    }      
    return line_pos;
}

int initTerminal(){
    int width=0, height=0;

    set_terminal_size(VIEW_COLS, VIEW_LINES);
    #if defined(_WIN32)
    DisableMaxWindow();
    Sleep(200);
    #elif defined(__linux__)
    sleep(1);
    #endif
    get_terminal_size(&width, &height);
    printf("width= %d", width);
    printf(", height= %d\n", height);
    qtd_window_line = height-2;
    return TRUE;
}

int set_first_break(int (*sendCommandGdb)(char *)){
    ST_Line * debug = LineDebug;
    int ret = 1;
    
    while(debug!=NULL){
        if(debug->isEntry==TRUE && strcasecmp(debug->fileCobol,cob.file_cobol)==0) break;
        debug=debug->next;
    }
    if(debug!=NULL){
        MI2addBreakPoint(sendCommandGdb, cob.name_file, debug->lineCobol);
        Lines * line = cob.lines;
        while(line!=NULL){
            if(line->file_line==debug->lineCobol){
                line->breakpoint='S';
                ret= line->file_line;
                if(cob.qtd_lines>5){
                    start_window_line = line->file_line-4;
                    ret = 3;                
                }
                break;
            }
            line=line->line_after;
        }
    }
    return ret;
}

void td_check_screen_size(void *arg){
    cob.showFile=win_size_verify(cob.showFile);
    if(!cob.showFile) sleep_ms(1000);
    (void)arg;
    CHECKING_SCR_SIZE=FALSE;
}    

void td_check_hover_var(void *arg) {
    struct st_hoverer_var *hVar = arg;    
    if (hVar->isHoverVar && hVar->cobVar!=NULL) {
        int dx = cob.mouseX - hVar->hover_x;
        int dy = cob.mouseY - hVar->hover_y;
        if (dx < -1 || dx >= 10 || abs(dy) >= 2) {
            hVar->isHoverVar = FALSE;
            cob.showFile   = TRUE;
        }
        CHECKING_HOVER = FALSE;
        return;
    }
    if (mousex != cob.mouseX || mousey != cob.mouseY) {
        check_hover_start = getCurrentTime();
        check_hover_start = check_hover_start;
    }
    mousex = cob.mouseX;
    mousey = cob.mouseY;
    double elapsed_time = getCurrentTime() - check_hover_start;
    if (elapsed_time > 1 && cob.mouseY > 0 && cob.mouseY < VIEW_LINES - 2){
        if (check_hover(hVar, start_window_line, start_line_x,cob.mouseX, cob.mouseY)) {
            mousex = -9999;
            mousey = -9999;
        }
    }
    CHECKING_HOVER = FALSE;
}

int debug(int (*sendCommandGdb)(char *)){
    int qtd_page = 0;
    int tempValue = -1;
    char key=' ';
    thread_t t1;
    struct st_hoverer_var hVar ={
        .hover_var[0] = '\0',
        .isHoverVar = FALSE,
        .cobVar = NULL
    };

    initTerminal();
    cob.line_pos=0;
    Lines * lb = NULL;
    int bstop = FALSE;
    cursorOFF();
    print_colorBK(color_white, color_black);
    clearScreen();
    Lines * line_debug=NULL;
    if(lines==NULL){
        show_sources(sendCommandGdb, TRUE);
    }
    if(qtd_window_line>cob.qtd_lines) qtd_window_line=cob.qtd_lines;
    cob.line_pos=set_first_break(sendCommandGdb);
    lines = set_window_pos(&cob.line_pos);
    while(cob.lines!=NULL && !bstop){       
        if(cob.showFile){
            line_debug=NULL;
            disableEcho();
            show_opt();
            cob.line_pos=show_file(lines, cob.line_pos, &line_debug);
            int aux1=cob.debug_line;
            int aux2=cob.running;
            if(!cob.waitAnswer){
                if(line_debug!=NULL && cob.showVariables) var_watching(line_debug, sendCommandGdb, cob.waitAnswer, cob.line_pos);
                if(hVar.isHoverVar) show_hover_var(sendCommandGdb, &hVar);
            }
            cob.debug_line=aux1;
            cob.running=aux2;
            print_color_reset();
            fflush(stdout);
            cob.showFile=FALSE;
            show_info();
            enableEcho();
            continue;
        }
        cob.input_character = -1;
        if(cob.isStepOver<0){
            cob.input_character = key_press(MOUSE_EXT);
            if(!cob.waitAnswer){
                if(CHECKING_SCR_SIZE==FALSE){
                    CHECKING_SCR_SIZE=TRUE;
                    thread_create(&t1, td_check_screen_size, (void*)1);
                } 
                if(CHECKING_HOVER==FALSE && cob.input_character == 0){
                    CHECKING_HOVER=TRUE;
                    thread_create(&t1, td_check_hover_var, (void *) &hVar );
                }
            }
        }
        if(cob.connect[0]!='\0') cob.input_character='a';
        switch (cob.input_character)
        {
            case VKEY_UP:
                if(cob.line_pos==0 && start_window_line>0){
                    if(lines->line_before!= NULL){
                        start_window_line--;
                        lines = lines->line_before;
                    }
                }else if(cob.line_pos>0){
                    cob.line_pos--;
                }
                cob.showFile=TRUE;            
                break;
            case VKEY_DOWN: 
                if(lines->line_after!= NULL){
                    if(cob.line_pos>(VIEW_LINES-5)){
                        start_window_line++;                 
                        lines = lines->line_after;
                    }else if(cob.line_pos<=(VIEW_LINES-5)){
                        cob.line_pos++;
                    }
                    while((lines->file_line+cob.line_pos)>cob.qtd_lines) cob.line_pos--;
                }
                cob.showFile=TRUE;
                break;
            case VKEY_PGUP:
                if(cob.line_pos>0){
                    cob.line_pos=0;
                }else{
                    qtd_page = VIEW_LINES-3;
                    cob.line_pos=0;
                    while((qtd_page--)>0 && lines->line_before!=NULL){
                        start_window_line--;
                        lines = lines->line_before;
                    }
                }
                cob.showFile=TRUE;
                break;
            case VKEY_PGDOWN: 
                qtd_page = 0;
                if(cob.line_pos<(VIEW_LINES-4)){
                    cob.line_pos=VIEW_LINES-4;
                }else{
                    if((cob.qtd_lines-start_window_line)>(VIEW_LINES-3)){
                        while((qtd_page++)<(VIEW_LINES-3) && lines->line_after!=NULL){
                            start_window_line++;
                            lines=lines->line_after;
                        }
                        cob.line_pos=VIEW_LINES-4;
                    }
                }
                while((lines->file_line+cob.line_pos)>cob.qtd_lines) cob.line_pos--;
                cob.showFile=TRUE;
                break;
            case VKEY_LEFT:
                if(start_line_x>0){
                    start_line_x--;
                    cob.showFile=TRUE;
                }
                break;
            case VKEY_RIGHT:
                if(start_line_x<250){
                    start_line_x++;
                    cob.showFile=TRUE;
                }
                break;
            case 'b':    
            case 'B':    
                if(!cob.waitAnswer){
                    lb = lines;     
                    for(int a=0;a<cob.line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        lb->breakpoint=(lb->breakpoint=='S')?'N':'S';
                        if(lb->breakpoint=='S'){
                            MI2addBreakPoint(sendCommandGdb, cob.name_file, lb->file_line);
                        }else{
                            MI2removeBreakPoint(sendCommandGdb, cob.name_file, lb->file_line);
                        }  
                        cob.showFile=TRUE;
                        MI2getStack(sendCommandGdb,1);
                    }else{
                        showCobMessage("Not a debugeable line",1);
                    } 
                }
                break;
            case 'k':
            case 'K':
                if(!cob.waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<cob.line_pos;a++) lb=lb->line_after;
                    lb->breakpoint='N';
                    MI2removeBreakPoint(sendCommandGdb, cob.name_file, lb->file_line);
                }
                break;
            case 'q':
            case 'Q':
                key = showCobMessage("Would you like to exit the program?", 3);
                if(key=='Y' || key=='y'){
                    sendCommandGdb("gdb-exit\n");
                    bstop = 1;
                }
                cob.showFile=TRUE; 
                break;
            case 'r':
            case 'R':
                if(!cob.waitAnswer){
                    if(cob.debug_line>0){
                        char key = showCobMessage("Would you like to \"Run\" the program again ? (= Restart)", 3);
                        if(key!='Y' && key!='y'){
                            cob.showFile=TRUE;
                            break;
                        }
                    }
                    show_wait();
                    disableEcho();
                    MI2start(sendCommandGdb);
                    WAIT_GDB=100;
                    lines = set_window_pos(&cob.line_pos);
                    cob.status_bar = 0;
                    enableEcho();
                }
                break;
            case 's':
            case 'S':
                hVar.isHoverVar = FALSE;
                tempValue= cob.debug_line;
                if(!cob.waitAnswer){
                    MI2stepInto(sendCommandGdb);
                    cob.status_bar = 0;
                }
                cob.debug_line = tempValue;
                break;
            case 'n':
            case 'N':
                hVar.isHoverVar = FALSE;
                tempValue= cob.debug_line;
                if(!cob.waitAnswer){
                    MI2stepOver(sendCommandGdb);
                    cob.status_bar = 0;
                }
                cob.debug_line = tempValue;
                break;
            case 'g':
            case 'G':
                if(!cob.waitAnswer) MI2stepOut(sendCommandGdb);
                break;
            case 'c':
            case 'C':
                hVar.isHoverVar = FALSE;
                if(!cob.waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<cob.line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        show_wait();
                        disableEcho();
                        MI2goToCursor(sendCommandGdb, cob.name_file, lb->file_line);
                        enableEcho();
                        WAIT_GDB=100;
                        lines = set_window_pos(&cob.line_pos);
                        cob.status_bar = 0;
                    }else{
                        showCobMessage("Not a debugeable line",1);
                    } 
                    cob.showFile=TRUE;
                }
                break;
            case 'j':
            case 'J':
                hVar.isHoverVar = FALSE;
                if(!cob.waitAnswer){ 
                    show_wait();
                    if(MI2lineToJump(sendCommandGdb)==0){
                        line_debug=NULL;
                        show_file(lines, cob.line_pos, &line_debug);
                        showCobMessage("Not a debuggable line.",2);
                    }else{
                        WAIT_GDB=100;
                    }
                    cob.showFile=TRUE;
                    cob.status_bar = 0;
                }
                break;
            case 'h':
            case 'H':
                if(!cob.waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<cob.line_pos;a++) lb=lb->line_after;
                    cob.ctlVar=(cob.ctlVar>10000)?1:cob.ctlVar+1;
                    cob.waitAnswer = TRUE;
                    int aux1=cob.debug_line;
                    int aux2=cob.running;
                    MI2hoverVariable(sendCommandGdb, lb);
                    cob.debug_line=aux1;
                    cob.running=aux2;      
                    cob.showFile=TRUE;
                }
                break;
            case 'v':
            case 'V':
                if(!cob.waitAnswer){
                    cob.ctlVar=(cob.ctlVar>10000)?1:cob.ctlVar+1;
                    cob.waitAnswer = TRUE;
                    int aux1=cob.debug_line;
                    int aux2=cob.running;
                    MI2variablesRequest(sendCommandGdb);
                    show_variables(sendCommandGdb);
                    cob.debug_line=aux1;
                    cob.running=aux2;      
                    cob.showFile=TRUE;
                }
                break;
            case 'f':
            case 'F':
                hVar.isHoverVar = FALSE;
                if(!cob.waitAnswer){
                   load_file();
                   //show_sources(sendCommandGdb, FALSE);
                   cob.showFile=TRUE;
                   MI2getStack(sendCommandGdb,1);
                }
                break;
            case '?':
                hVar.isHoverVar = FALSE;
                if(!cob.waitAnswer){
                    show_help(TRUE);
                    cob.showFile=TRUE;
                }
                break;
            case 'a':
            case 'A':
                hVar.isHoverVar = FALSE;
                if(!cob.waitAnswer){
                    cob.input_character=' ';
                    gotoxy(1,VIEW_LINES);
                    disableEcho();
                    printf("%s\r", "Connecting            ");
                    fflush(stdout);
                    MI2attach(sendCommandGdb);
                    cob.showFile=TRUE;
                    lines = set_window_pos(&cob.line_pos);
                }
                break;
            case 'W':
            case 'w':
                if(VIEW_COLS==80){
                    VIEW_COLS=132;
                    VIEW_LINES=34;
                }else{
                    VIEW_COLS=80;
                    VIEW_LINES=24;
                }
                freeWatchingList();
                clearScreen();
                set_terminal_size(VIEW_COLS, VIEW_LINES);
                cob.showFile = TRUE;
                lines = set_window_pos(&cob.line_pos);
                break;
            case 'D':
            case 'd':
                // tooble show variables
                cob.showVariables ^= 1;
                if(cob.mouse!=60){
                    if(cob.showVariables)
                        showCobMessage("display of variables: ON",1);
                    else
                        showCobMessage("display of variables: OFF",1);
                }
                cob.showFile = TRUE;
                break;
            case 'O':
            case 'o':
                focus_window_by_title(cob.title);
                break;
            case VKEY_CTRLF:            
                tempValue=0;
                do{
                    tempValue=search_text(&lines);
                    if(tempValue==0){
                        showCobMessage("Text not found.",1);
                        cob.line_pos=show_file(lines, cob.line_pos, &line_debug);
                    }else if(tempValue==2){
                        cob.line_pos=show_file(lines, cob.line_pos, &line_debug);
                    }
                }while(tempValue!=1);
                cob.showFile = TRUE;
                cob.status_bar = 0;
                break;
            case VKEY_CTRLG:
                if(gotoLine(&lines)==0){
                    cob.line_pos=show_file(lines, cob.line_pos, &line_debug);
                    showCobMessage("Line not found.",1);
                }
                cob.showFile = TRUE;
                cob.status_bar = 0;
                break;
            case VKEY_CTRLS:
                if(save_breakpoints(BPList, cob.first_file)){
                    showCobMessage("Breakpoints saved",1);
                }
                cob.showFile = TRUE;
                cob.status_bar = 0;
                break;                
            case VKEY_CTRLB:
                tempValue= cob.debug_line;
                BPList = load_breakpoints(sendCommandGdb, BPList, cob.first_file);
                cob.showFile = TRUE;
                cob.status_bar = 0;
                cob.debug_line = tempValue;
                break;                
            default: 
                if(cob.waitAnswer){
                    WAIT_GDB++;
                    MI2verifyGdb(sendCommandGdb);
                    if(cob.showFile && cob.changeLine){
                        lines = set_window_pos(&cob.line_pos);
                        cob.changeLine=FALSE;
                        cob.isStepOver=-1;
                    }
                    if(!cob.waitAnswer && cob.debug_line==-1 && strcmp(cob.connect,"attaching")==0)
                        cob.waitAnswer=TRUE;
                }
                break;
        }
        show_info();
        //if(cob.input_character!=0) printf("Key = %d", cob.input_character);
    }
    return TRUE;
}

int loadfile(char * nameCobFile) {
    start_window_line = 0;
    start_line_x = 0;
    cob.debug_line = -1;
    char baseName[500];
    char temp_name[1024];

    if(!file_exists(nameCobFile))
        return FALSE;
    strcpy(cob.name_file, nameCobFile);
    fileNameWithoutExtension(nameCobFile, &baseName[0]);
    normalizePath(baseName);
    strcpy(baseName,getFileNameFromPath(baseName));
    snprintf(temp_name, sizeof(temp_name), "%s/%s.c", cob.cwd, baseName);
    if(!file_exists(temp_name))
        return FALSE;
    strcpy(cob.cfile, temp_name);        
    readCodFile(&cob);
    freeWatchingList();
    lines = cob.lines;
    char aux[10];
    sprintf(aux,"%d",cob.qtd_lines);
    cob.num_dig=strlen(aux);
    if(BPList!=NULL){
        Lines * line = cob.lines;
        line->breakpoint='N';
        while(line!=NULL){
            ST_bk * search = BPList;
            while(search!=NULL){
                if(search->line==line->file_line && strcasecmp(search->name, cob.file_cobol)==0){
                    int check=hasCobolLine(line->file_line);
                    if(check>0)
                        line->breakpoint='S';
                }
                search=search->next;
            }            
            line=line->line_after;
        }
    }
    return TRUE;
}

int freeFile(){
    free_memory();
    return TRUE;
}

int freeAll(){
    freeBKList();
    freeFile();
    freeWatchingList();
    free_cfiles();
    return TRUE;
}

void setup_locale(void) {
    setlocale(LC_ALL, "");
#if defined(_WIN32)
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void exit_with_message(const char *msg, int code) {
    printf("%s\n", msg);
    fflush(stdout);
    exit(code);
}

int handle_connect_args(int argc, char **argv) {
    strcpy(cob.connect,"");
    if (argc < 3)
        return 0;

    if (strcmp(argv[1], "--connect") == 0) {
        if(argc<3){
            printf("Please provide the parameters.\n");
            return 0;
        }
        strcpy(cob.connect,argv[2]);
        return 1;
    }
    return 0;
}

int handle_special_args(int argc, char **argv, int arg_init) {
    if (argc < arg_init - 1 + 2)
        exit_with_message("Please provide the GnuCOBOL file.", 0);

    if (strcmp(argv[arg_init], "--test") == 0) {
#ifdef __WITH_TESTS_
        testParser();
        testMI2();
#else
        printf("Tests not included.\n");
#endif
        return 1;
    }
    if (strcmp(argv[arg_init], "--version") == 0) {
        show_version();
        return 1;
    }
    if (strcmp(argv[arg_init], "--help") == 0) {
        show_help(FALSE);
        return 1;
    }
    return 0;
}

void init_cob_context(const char *cwd, const char *nameExecFile) {
    strcpy(cob.cwd, cwd);
    normalizePath(cob.cwd);
#if defined(_WIN32)
    snprintf(cob.title, sizeof(cob.title), "%s\\%s", cwd, nameExecFile);
#else
    snprintf(cob.title, sizeof(cob.title), "%s/%s", cwd, nameExecFile);
#endif
    strcpy(cob.file_cobol,"");
    sprintf(cob.find_text, "%61s", "");
}

int handle_exe_mode(char *exePath) {
    if (access(exePath, F_OK) < 0)
        exit_with_message("Executable not found.", 0);

    char *cwd = getCurrentDirectory();
    char baseName[256];
    fileNameWithoutExtension(exePath, baseName);
    normalizePath(baseName);

#if defined(_WIN32)
    strcat(baseName, ".exe");
#endif

    gotoxy(1,1);
    print_no_resetBK("",color_white, color_black);
    clearScreen();
    disableEcho();
    printf("Name: %s\n",baseName);        
    draw_box_first(10,10,60,"Utilize the parameters below to compile your COBOL program:");
    draw_box_border(10,11);
    draw_box_border(71,11);
    print_colorBK(color_yellow, color_black);
    gotoxy(11,11);
    printf("%-*s\r",60,"cobc -g -fsource-location -ftraceall -v -O0 -x prog.cob");
    print_colorBK(color_white, color_black);
    draw_box_last(10,12,60);
    gotoxy(10,15);
    printf("Press a key to continue...");
    enableEcho();
    fflush(stdout);
    double check_start = getCurrentTime();
    while (key_press(MOUSE_OFF)<=0) {
        double end_time = getCurrentTime();
        double elapsed_time = end_time - check_start;
        if (elapsed_time > 4) {
            break;
        }
    }

    init_cob_context(cwd, baseName);
    start_gdb(baseName, cob.cwd);

    freeAll();
    free(cwd);
    gotoxy(1,(VIEW_LINES-1));

    return 0;
}

int handle_cob_files(int argc, char **argv, int arg_init) {
    char *cwd = getCurrentDirectory();
    char nameExecFile[256], baseName[256];
    char fileCGroup[10][512], fileCobGroup[10][512];
    char nameCFile[1024];
    char values[10][256];
    int arg_count = 0, nfile = 0;

    init_cob_context(cwd, "");
    free(cwd);

    for (int i = arg_init; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (arg_count < 10)
                strncpy(values[arg_count++], argv[i], sizeof(values[0]) - 1);
            continue;
        }
        char ext[16];
        fileExtension(argv[i], ext);
        if (strcasecmp(ext, ".cob")!=0 && strcasecmp(ext, ".cbl")!=0)
            continue;

        fileNameWithoutExtension(argv[i], &baseName[0]);
        normalizePath(baseName);
        strcpy(baseName,getFileNameFromPath(baseName));
        if (nfile == 0) {
            strcpy(cob.name_file, argv[i]);
            strcpy(nameExecFile, baseName);
#if defined(_WIN32)
            strcat(nameExecFile, ".exe");
#endif
        }
        // C File
        snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
        strcpy(fileCGroup[nfile],nameCFile);
        // Cobol File
        realpath(argv[i], fileCobGroup[nfile]);
        normalizePath(fileCobGroup[nfile]);
        nfile++;
    }
    
    if (nfile == 0)
        exit_with_message("Enter a file to debug.", 1);

    fileCobGroup[nfile][0] = '\0';
    fileCGroup[nfile][0] = '\0';

    int ret = cobc_compile(fileCobGroup, values, arg_count);
    if (ret) {
        printf("CobGDB: Issue in cobc, code %d\n", ret);
        return ret;
    }

    printf("Parser starting...\n");
    SourceMap(fileCGroup);
    printf("Parser end...\n");

    strcpy(cob.file_cobol, cob.name_file);
    strcpy(cob.first_file, cob.file_cobol);
    loadfile(cob.file_cobol);
    highlightParse();

    strcat(cob.title, nameExecFile);
    start_gdb(nameExecFile, cob.cwd);
    freeAll();
    gotoxy(1,(VIEW_LINES-1));
    return 0;
}

int main(int argc, char **argv) {
    int arg_init=1;
    setup_locale();
    init_terminal_colors();

    if(handle_connect_args(argc, argv)){
        arg_init=3;
    }
    if (handle_special_args(argc, argv, arg_init))
        return 0;
    if (!isCommandInstalled("gdb"))
        exit_with_message("GDB is not installed.", 0);
    if (argc >= 3 && strcmp(argv[arg_init], "--exe") == 0)
        handle_exe_mode(argv[arg_init+1]);
    else
        handle_cob_files(argc, argv, arg_init);
    print_color_reset();
    cursorON();
    printf("The end of the CobGDB execution.\n");
    fflush(stdout);
    return 0;
}
