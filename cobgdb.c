#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#if defined(__linux__)
#include <unistd.h>
#else
#include <windows.h>
#endif
#include "cobgdb.h"
#define __WITH_TESTS_ 

struct st_cobgdb cob ={
    .debug_line = -1,
    .ctlVar = -1,
    .running = TRUE,
    .waitAnswer = FALSE,
    .changeLine = FALSE,
    .showFile = TRUE,
    .isStepOver = -1
};

int start_window_line = 0;
int qtd_window_line = VIEW_LINES-2;
int start_line_x = 0;
int NUM_DIG=4;

char * gdbOutput = NULL;
int color_frame=color_light_blue;

ST_Line * LineDebug=NULL;
ST_Attribute * Attributes=NULL;
ST_DebuggerVariable * DebuggerVariable=NULL;
ST_bk * BPList=NULL;
ST_Watch * Watching=NULL;
Lines * lines = NULL;
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
        }
        if (current_line->high != NULL) {
            freeHighlight(current_line->high);
        }
        free(current_line);
    }
    cob.lines = NULL;
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

    // Initialize the first 10 elements with the initial values.
    char *initial_params[] = {
        "-g ",
        "-fsource-location ",
        "-ftraceall ",
        //"-Q ",
        //"--coverage ",
        //"-A ",
        //"--coverage ",
        "-v ",
        "-free ",
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
        if(a>=initFree) free(param[a]);  // Free the dynamically allocated memory for the values.
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

    pclose(pfd);
    return 0;
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
                //debug_line--;
            }
        }
    }
    Lines * line = cob.lines;
    while(line->file_line<=start_window_line){
        if(line->line_after!=NULL) line=line->line_after;
    }
    return line;
}

int show_opt(){
    char * opt = " COBGDB                  GnuCOBOL GDB Interpreter                       (?)Help";
    char aux[100];
    snprintf(aux,82,"%-80.80s\r", opt);
    gotoxy(1,1);
    printBK(aux, color_white, color_frame);
    gotoxy(1,VIEW_LINES-1);
    snprintf(aux,82,"%-80.80s\r",cob.file_cobol);
    printBK(aux, color_light_gray, color_frame);
}

int show_info(){
    gotoxy(1,VIEW_LINES);
    if(cob.debug_line>0 && !cob.running){
        print_colorBK(color_green, color_black);
        printf("%s\r", "Debugging             ");
    }else if(cob.debug_line>0 && cob.running){
        print_colorBK(color_red, color_black);
        printf("%s\r", "Running               ");
    }else{
        print_colorBK(color_yellow, color_black);
        if(cob.waitAnswer){
            printf("%s\r", "Waiting               ");
        }else{
            #if defined(_WIN32)
            printf("%79s\r", " ");
            #else
            printf("%80s\r", " ");
            #endif
        }
    }
    gotoxy(1,VIEW_LINES);
    if(cob.showFile==FALSE) fflush(stdout);
}

int show_file(Lines * lines, int line_pos, struct st_highlt ** exe_line){
    Lines * show_line=lines;
    char vbreak = ' ';
    char pline[252];
    char aux[100];
    int NUM_TXT=76-NUM_DIG;
    
    gotoxy(1,1);
    show_opt();
    size_t  bkgColor=color_gray;
    char chExec = ' ';
    for(size_t i=0;i<(VIEW_LINES-3);i++){
        if(show_line != NULL){
            gotoxy(1,i+2);
            if(show_line->breakpoint=='S'){
                printBK("B",color_yellow, color_frame);
            }else{
                printBK(" ",color_yellow, color_frame);
            }
            if(cob.debug_line==show_line->file_line && !cob.running){
                print(">", color_green); //print_color(color_green); draw_utf8_text("\u25BA");
                if(show_line->high!=NULL) 
                    *exe_line=show_line->high;
            }else{
                chExec = (cob.debug_line==show_line->file_line)?'!': ' ';
                print_color(color_red);
                printf("%c",chExec);
            }     
            print_color(color_gray);
            printf("%-*d ", NUM_DIG, show_line->file_line);
            if(show_line->high==NULL){
                if(show_line->line !=NULL) show_line->line[strcspn(show_line->line,"\n")]='\0';
                size_t  len=strlen(show_line->line);
                wchar_t *wcharString = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
                #if defined(_WIN32)
                MultiByteToWideChar(CP_UTF8, 0, show_line->line, -1, wcharString,(len + 1) * sizeof(wchar_t) / sizeof(wcharString[0]));
                #else
                mbstowcs(wcharString, show_line->line, strlen(show_line->line) + 1);
                #endif
                len = wcslen(wcharString);
                //if(len>0) show_line->line[strcspn(show_line->line,"\n")]='\0';
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
                    printf("%72s", " ");
                }
                free(wcharString);
            }else{
                bkgColor=(line_pos==i)?color_gray:-1;
                printHighlight(show_line->high, bkgColor, start_line_x, NUM_TXT);
            }
            show_line = show_line->line_after;
            if(show_line == NULL && line_pos>i){
                line_pos=i-1;
            }
            print_no_resetBK(" ",color_white, color_frame);
        }else{
            gotoxy(1,i+2);
            print_no_resetBK(" ",color_white, color_frame);
            print_colorBK(color_white,color_black);
            printf("%78s"," ");
            print_no_resetBK(" \r",color_white, color_frame);
        }        
    }
    return line_pos;
}

int initTerminal(){
    int width=0, height=0;

    set_terminal_size(VIEW_COLS, VIEW_LINES);
    #if defined(_WIN32)
    Sleep(200);
    #elif defined(__linux__)
    sleep(1);
    #endif
    get_terminal_size(&width, &height);
    printf("width= %d", width);
    printf(", height= %d\n", height);
}

int debug(int line_pos, int (*sendCommandGdb)(char *)){
    int width=0, height=0;
    int qtd_page = 0;
    char input_character;
    char command[256];
    int dblAux = -1;
    int check_size=0;
    struct st_highlt * exe_line;
    double check_start = getCurrentTime();

    initTerminal();
    //while(key_press()<=0);
    if(qtd_window_line>cob.qtd_lines) qtd_window_line=cob.qtd_lines;
    lines = cob.lines;
    Lines * lb = NULL;
    int line_file=0;
    int bstop = 0;
    //(void)setvbuf(stdout, NULL, _IONBF, 16384);
    cursorOFF();
    clearScreen();
    while(cob.lines!=NULL && !bstop){
        if(cob.showFile){
            show_opt();
            exe_line=NULL;
            line_file = lines->file_line; 
            line_pos=show_file(lines, line_pos, &exe_line);
            int aux1=cob.debug_line;
            int aux2=cob.running;
            var_watching(exe_line, sendCommandGdb, cob.waitAnswer, line_pos);
            cob.debug_line=aux1;
            cob.running=aux2;
            fflush(stdout);
            print_color_reset();
            cob.showFile=FALSE;
        }
        input_character = -1;
        if(cob.waitAnswer && cob.isStepOver<0){
            input_character = key_press();
        }else{
            if(check_size<3){
                double end_time = getCurrentTime();
                double elapsed_time = end_time - check_start;
                if(elapsed_time>1){
                    cob.showFile=win_size_verify(cob.showFile, &check_size);
                    check_start = getCurrentTime();
                }
            }
            if(cob.isStepOver<0) input_character = key_press();
        }
        switch (input_character)
        {
            case VK_UP:
                if(line_pos==0 && start_window_line>0){
                    if(lines->line_before!= NULL){
                        start_window_line--;
                        lines = lines->line_before;
                    }
                }else if(line_pos>0){
                    line_pos--;
                }
                cob.showFile=TRUE;            
                break;
            case VK_DOWN: 
                if(lines->line_after!= NULL){
                    if(line_pos>(VIEW_LINES-5)){
                        start_window_line++;                 
                        lines = lines->line_after;
                    }else if(line_pos<=(VIEW_LINES-5)){
                        line_pos++;
                    }
                }
                cob.showFile=TRUE;
                break;
            case VK_PGUP:
                if(line_pos>0){
                    line_pos=0;
                }else{
                    qtd_page = VIEW_LINES-2;
                    line_pos=0;
                    while((qtd_page--)>0 && lines->line_before!=NULL){
                        start_window_line--;
                        lines = lines->line_before;
                    }
                }
                cob.showFile=TRUE;
                break;
            case VK_PGDOWN: 
                qtd_page = 0;
                if(line_pos<(VIEW_LINES-4)){
                    line_pos=VIEW_LINES-4;
                }else{
                    if((cob.qtd_lines-start_window_line)>(VIEW_LINES-3)){
                        while((qtd_page++)<(VIEW_LINES-2) && lines->line_after!=NULL){
                            start_window_line++;
                            lines=lines->line_after;
                        }
                        line_pos=VIEW_LINES-4;
                    }
                }
                while((lines->file_line+line_pos)>cob.qtd_lines) line_pos--;
                cob.showFile=TRUE;
                break;
            case VK_LEFT:
                if(start_line_x>0){
                    start_line_x--;
                    cob.showFile=TRUE;
                }
                break;
            case VK_RIGHT:
                if(start_line_x<250){
                    start_line_x++;
                    cob.showFile=TRUE;
                }
                break;
            case 'b':    
            case 'B':    
                if(!cob.waitAnswer){
                    lb = lines;     
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        lb->breakpoint=(lb->breakpoint=='S')?'N':'S';
                        if(lb->breakpoint=='S'){
                            MI2addBreakPoint(sendCommandGdb, cob.name_file, lb->file_line);
                        }else{
                            MI2removeBreakPoint(sendCommandGdb, lines, cob.name_file, lb->file_line);
                        }  
                        cob.showFile=TRUE;
                        MI2getStack(sendCommandGdb,1);
                    } 
                }
                break;
            case 'd':
                if(!cob.waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    lb->breakpoint='N';
                    MI2removeBreakPoint(sendCommandGdb, lines, cob.name_file, lb->file_line);
                }
                break;
            case 'q':
            case 'Q':
                sendCommandGdb("gdb-exit\n");
                bstop = 1;
                cob.showFile=TRUE;
                break;
            case 'r':
            case 'R':
                if(!cob.waitAnswer){
                    MI2start(sendCommandGdb);
                }
                break;
            case 's':
            case 'S':
                dblAux= cob.debug_line;
                if(!cob.waitAnswer){
                    MI2stepInto(sendCommandGdb);
                }
                cob.debug_line = dblAux;
                break;
            case 'n':
            case 'N':
                if(!cob.waitAnswer){
                    MI2stepOver(sendCommandGdb);
                    //cob.start_time = getCurrentTime();
                }
                break;
            case 'g':
            case 'G':
                if(!cob.waitAnswer) MI2stepOut(sendCommandGdb);
                break;
            case 'c':
            case 'C':
                if(!cob.waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        MI2goToCursor(sendCommandGdb, cob.name_file, lb->file_line);
                    } 
                    cob.showFile=TRUE;
                }
                break;
            case 'h':
            case 'H':
                if(!cob.waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        cob.ctlVar=(cob.ctlVar>10000)?1:cob.ctlVar+1;
                        cob.waitAnswer = TRUE;
                        int aux1=cob.debug_line;
                        int aux2=cob.running;
                        MI2hoverVariable(sendCommandGdb, lb);
                        cob.debug_line=aux1;
                        cob.running=aux2;      
                        cob.showFile=TRUE;
                    } 
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
                if(!cob.waitAnswer){
                   show_sources(sendCommandGdb);
                   cob.showFile=TRUE;
                   MI2getStack(sendCommandGdb,1);
                }
                break;
            case '?':
                if(!cob.waitAnswer){
                    show_help(sendCommandGdb);
                    cob.showFile=TRUE;
                }
                break;
            case 'a':
            case 'A':
                if(!cob.waitAnswer){
                    gotoxy(1,VIEW_LINES);
                    printf("%s\r", "Connecting            ");
                    fflush(stdout);
                    MI2attach(sendCommandGdb);
                    cob.showFile=TRUE;
                    MI2getStack(sendCommandGdb,1);
                }
                break;
            default: 
                if(cob.waitAnswer){
                    MI2verifyGdb(sendCommandGdb);
                    if(cob.showFile && cob.changeLine){
                        lines = set_window_pos(&line_pos);
                        cob.changeLine=FALSE;
                        cob.isStepOver=-1;
                    }
                }
                break;
        }
        show_info();
        //printf("Key = %d", input_character);
    }
}

int loadfile(char * nameCobFile) {
    int input_character;

    start_window_line = 0;
    start_line_x = 0;
    cob.debug_line = -1;

    strcpy(cob.name_file, nameCobFile);
    readCodFile(&cob);
    freeWatchingList();
    lines = cob.lines;
    char aux[10];
    sprintf(aux,"%d",cob.qtd_lines);
    NUM_DIG=strlen(aux);
    if(BPList!=NULL){
        Lines * line = cob.lines;
        line->breakpoint='N';
        while(line!=NULL){
            ST_bk * search = BPList;
            while(search!=NULL){
                if(search->line==line->file_line && strcasecmp(search->name, cob.file_cobol)==0){
                    line->breakpoint='S';
                }
                search=search->next;
            }            
            line=line->line_after;
        }
    }
}

int freeFile(){
    free_memory();
}

int main(int argc, char **argv) {
    char nameExecFile[256];
    char baseName[256];
    char nameCFile[256];
    struct lconv *locale_info = localeconv();
    char values[10][256];   // Create an array of strings to store the arguments
    int arg_count = 0;      // Initialize a counter for storing arguments
    char fileCGroup[10][512];
    char fileCobGroup[10][512];

    setlocale(LC_CTYPE, "");
    setlocale(LC_ALL,"");
    #if defined(_WIN32)
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    #endif    
    strcpy(cob.file_cobol,"");
    //if (locale_info != NULL) {
    //    cob.decimal_separator = locale_info->decimal_point[0];
    //}
    if(!isCommandInstalled("cobc")){
        printf("The GnuCOBOL cobc command is not available!\n");
        while(key_press()<=0);
        return 0;
    }
 
    if(!isCommandInstalled("gdb")){
        printf("GDB is not installed.\n");
        while(key_press()<=0);
        return 0;
    }
    if(argc<2){
        printf("Please provide the GnuCOBOL file.\n");
        return 0;
    }
    if(argc>1 && strncmp(argv[1],"--test",6)==0){
        #ifdef __WITH_TESTS_
            testParser();
            testMI2();
        #else
            printf("Tests not included.\n");
        #endif
    }else{
        // Iterate through the arguments
        boolean withHigh=TRUE;
        int nfile=0;
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                // Check if the argument starts with "-"
                if (arg_count < 10) {
                    strncpy(values[arg_count], argv[i], sizeof(values[0]) - 1);
                    values[arg_count][sizeof(values[0]) - 1] = '\0';  // Ensure the string is terminated
                    if(strcmp(values[arg_count],"-nh")==0)
                        withHigh=FALSE;
                    else
                        arg_count++;
                }
            }else{
                fileNameWithoutExtension(argv[i], &baseName[0]);
                if(nfile==0){
                    strcpy(cob.name_file,argv[i]);
                    strcpy(nameExecFile, baseName);
                    #if defined(_WIN32)
                    strcat(nameExecFile,".exe");
                    #endif
                    printf("Name: %s\n",nameExecFile);
                }
                strcpy(nameCFile,baseName);
                strcat(nameCFile,".c");
                strcpy(fileCobGroup[nfile],argv[i]);
                normalizePath(fileCobGroup[nfile]);
                normalizePath(nameCFile);
                strcpy(fileCGroup[nfile],nameCFile);
                nfile++;
            }
        }
		strcpy(fileCobGroup[nfile], "");
		strcpy(fileCGroup[nfile], "");
        cobc_compile(fileCobGroup, values, arg_count);
        printf("Parser starting...\n");
		SourceMap(fileCGroup);
        printf("Parser end...\n");
        strcpy(cob.file_cobol,cob.name_file);
        char cwd[512];
        getPathName(cwd, cob.file_cobol);
        strcpy(cob.first_file, cob.file_cobol);
        loadfile(cob.file_cobol);
        if(withHigh) highlightParse(); 
        //printf("The current locale is %s \n",setlocale(LC_ALL,""));
        //while(key_press()<=0);        
        start_gdb(nameExecFile,cwd);
        freeBKList();
        freeFile();
        freeWatchingList();
        //TODO: 
        //freeVariables();
        gotoxy(1,(VIEW_LINES-1));
    }
    print_color_reset();
    cursorON();
    printf("The end of the COBGBD execution.\n");
    fflush(stdout);
    return 0;
}
