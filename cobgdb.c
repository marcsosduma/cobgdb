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
#define VIEW_LINES 24
#define VIEW_COLS  80
struct program_file program_file;
char file_cobol[512];
char first_file[512];
int start_window_line = 0;
int qtd_window_line = VIEW_LINES-2;
int start_line_x = 0;
int debug_line = -1;
int running = TRUE;
int waitAnswer = FALSE;
char * gdbOutput = NULL;
int showFile=TRUE;
int ctlVar=0;
char * ttyName=NULL;
int WIDTH=VIEW_COLS;
int HEIGHT=VIEW_LINES;
int changeLine=FALSE;
int color_frame=color_light_blue;
char decimal_separator = '.';
clock_t start_time  = -1;

ST_Line * LineDebug=NULL;
ST_Attribute * Attributes=NULL;
ST_DebuggerVariable * DebuggerVariable=NULL;
ST_bk * BPList=NULL;
ST_Watch * Watching=NULL;
Lines * lines = NULL;
int start_gdb(char * name, char * cwd);

void free_memory()
{
    Lines * lines = program_file.lines;
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
    program_file.lines = NULL;
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

int cobc_compile(char file[][512], char values[10][256], int arg_count){
    char *param[20]; 

    // Initialize the first 10 elements with the initial values.
    char *initial_params[] = {
        "-g ",
        "-fsource-location ",
        "-ftraceall ",
        "-Q ",
        "--coverage ",
        "-A ",
        "--coverage ",
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
    if(debug_line>=0){
        // exec_line before window
        if(debug_line<start_window_line || (debug_line-start_window_line-*line_pos)>10){
            start_window_line = debug_line - (qtd_window_line*30)/100;
            if(start_window_line<0) start_window_line=0;
            if(qtd_window_line>program_file.qtd_lines) qtd_window_line=program_file.qtd_lines;
            *line_pos = debug_line - 1 - start_window_line;
        }else{
            int new_pos = debug_line - 1 - start_window_line;
            if(new_pos<(VIEW_LINES-4)){
                *line_pos=new_pos;
            }else{
                start_window_line = debug_line - VIEW_LINES + 4;
                *line_pos=(VIEW_LINES-5);
                //debug_line--;
            }
        }
    }
    Lines * line = program_file.lines;
    while(line->file_line<=start_window_line){
        if(line->line_after!=NULL) line=line->line_after;
    }
    return line;
}

int show_opt(){
    char * opt = " COBGDB - (R)run (N/S)step (B)breakpoint (G)go (C)cursor (V/H)var (Q)quit";
    char aux[100];
    sprintf(aux,"%-80s\r", opt);
    gotoxy(1,1);
    printBK(aux, color_white, color_frame);
    gotoxy(1,VIEW_LINES-1);
    sprintf(aux,"%-80s\r",file_cobol);
    printBK(aux, color_light_gray, color_frame);
}

int show_info(){
    gotoxy(1,VIEW_LINES);
    if(debug_line>0 && !running){
        print_colorBK(color_green, color_black);
        printf("%s\r", "Debugging             ");
    }else if(debug_line>0 && running){
        print_colorBK(color_red, color_black);
        printf("%s\r", "Running               ");
    }else{
        print_colorBK(color_yellow, color_black);
        if(waitAnswer){
            printf("%s\r", "Waiting               ");
        }else{
            printf("%s\r", "                      ");
        }
    }
    gotoxy(1,VIEW_LINES);
    if(showFile==FALSE) fflush(stdout);
}

int show_file(Lines * lines, int line_pos, struct st_highlt ** exe_line){
    Lines * show_line=lines;
    char vbreak = ' ';
    char pline[252];
    char aux[100];
    
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
            if(debug_line==show_line->file_line && !running){
                print(">", color_green);
                if(show_line->high!=NULL) 
                    *exe_line=show_line->high;
            }else{
                chExec = (debug_line==show_line->file_line)?'!': ' ';
                print_color(color_red);
                printf("%c",chExec);
            }     
            print_color(color_gray);
            printf("%-4d ", show_line->file_line);
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
                int  qtdMove = (72 < nm)? 72 :len - start_line_x;
                if(qtdMove>0){
                    printf("%.*ls",qtdMove, &wcharString[start_line_x]);
                    nm=72-qtdMove;
                    if(nm>0) printf("%*s", nm, " ");
                }else{
                    printf("%72s", " ");
                }
                free(wcharString);
            }else{
                bkgColor=(line_pos==i)?color_gray:-1;
                printHighlight(show_line->high, bkgColor, start_line_x, 72);
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



int debug(int line_pos, int (*sendCommandGdb)(char *)){
    int width=0, height=0;
    int qtd_page = 0;
    char input_character;
    char command[256];
    int dblAux = -1;
    int check_size=0;
    struct st_highlt * exe_line;

    if(qtd_window_line>program_file.qtd_lines) qtd_window_line=program_file.qtd_lines;
    lines = program_file.lines;
    Lines * lb = NULL;
    int line_file=0;
    int bstop = 0;
    //(void)setvbuf(stdout, NULL, _IONBF, 16384);
    cursorOFF();
    clearScreen();
    while(program_file.lines!=NULL && !bstop){
        if(showFile){
            show_opt();
            exe_line=NULL;
            line_file = lines->file_line; 
            line_pos=show_file(lines, line_pos, &exe_line);
            int aux1=debug_line;
            int aux2=running;
            var_watching(exe_line, sendCommandGdb, waitAnswer);
            debug_line=aux1;
            running=aux2;
            fflush(stdout);
            print_color_reset();
            showFile=FALSE;
        }
        if(waitAnswer && start_time>0){
            clock_t end_time = clock();
            double elapsed_time = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
            if(elapsed_time>3){
                if(check_size<3) showFile=win_size_verify(showFile, &check_size);
                input_character =  key_press();
            }else{
                input_character = -1;
            }
        }else{
            if(check_size<3) showFile=win_size_verify(showFile, &check_size);
            input_character =  key_press();
            start_time=-1;
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
                showFile=TRUE;            
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
                showFile=TRUE;
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
                showFile=TRUE;
                break;
            case VK_PGDOWN: 
                qtd_page = 0;
                if(line_pos<(VIEW_LINES-4)){
                    line_pos=VIEW_LINES-4;
                }else{
                    if((program_file.qtd_lines-start_window_line)>(VIEW_LINES-3)){
                        while((qtd_page++)<(VIEW_LINES-2) && lines->line_after!=NULL){
                            start_window_line++;
                            lines=lines->line_after;
                        }
                        line_pos=VIEW_LINES-4;
                    }
                }
                while((lines->file_line+line_pos)>program_file.qtd_lines) line_pos--;
                showFile=TRUE;
                break;
            case VK_LEFT:
                if(start_line_x>0){
                    start_line_x--;
                    showFile=TRUE;
                }
                break;
            case VK_RIGHT:
                if(start_line_x<250){
                    start_line_x++;
                    showFile=TRUE;
                }
                break;
            case 'b':    
            case 'B':    
                if(!waitAnswer){
                    lb = lines;     
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        lb->breakpoint=(lb->breakpoint=='S')?'N':'S';
                        if(lb->breakpoint=='S'){
                            MI2addBreakPoint(sendCommandGdb, program_file.name_file, lb->file_line);
                        }else{
                            MI2removeBreakPoint(sendCommandGdb, lines, program_file.name_file, lb->file_line);
                        }  
                        showFile=TRUE;
                        MI2getStack(sendCommandGdb,1);
                    } 
                }
                break;
            case 'd':
                if(!waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    lb->breakpoint='N';
                    MI2removeBreakPoint(sendCommandGdb, lines, program_file.name_file, lb->file_line);
                }
                break;
            case 'q':
            case 'Q':
                sendCommandGdb("gdb-exit\n");
                bstop = 1;
                showFile=TRUE;
                break;
            case 'r':
            case 'R':
                MI2start(sendCommandGdb);
                break;
            case 's':
            case 'S':
                dblAux= debug_line;
                if(!waitAnswer){
                    MI2stepInto(sendCommandGdb);
                    start_time = clock();
                }
                debug_line = dblAux;
                break;
            case 'n':
            case 'N':
                if(!waitAnswer){
                    MI2stepOver(sendCommandGdb);
                    start_time = clock();
                }
                break;
            case 'g':
            case 'G':
                if(!waitAnswer) MI2stepOut(sendCommandGdb);
                break;
            case 'c':
            case 'C':
                if(!waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        MI2goToCursor(sendCommandGdb, program_file.name_file, lb->file_line);
                    } 
                    showFile=TRUE;
                }
                break;
            case 'h':
            case 'H':
                if(!waitAnswer){ 
                    lb = lines;           
                    for(int a=0;a<line_pos;a++) lb=lb->line_after;
                    int check=hasCobolLine(lb->file_line);
                    if(check>0){
                        ctlVar=(ctlVar>10000)?1:ctlVar+1;
                        waitAnswer = TRUE;
                        int aux1=debug_line;
                        int aux2=running;
                        MI2hoverVariable(sendCommandGdb, lb);
                        debug_line=aux1;
                        running=aux2;      
                        showFile=TRUE;
                    } 
                }
                break;
            case 'v':
            case 'V':
                if(!waitAnswer){
                    ctlVar=(ctlVar>10000)?1:ctlVar+1;
                    waitAnswer = TRUE;
                    int aux1=debug_line;
                    int aux2=running;
                    MI2variablesRequest(sendCommandGdb);
                    show_variables(sendCommandGdb);
                    debug_line=aux1;
                    running=aux2;      
                    showFile=TRUE;
                }
                break;
            case 'f':
            case 'F':
                if(!waitAnswer){
                   show_sources(sendCommandGdb);
                   showFile=TRUE;
                   MI2getStack(sendCommandGdb,1);
                }
                break;
            case 'l':
            case 'L':
                printf(" ");
                break;
            default: 
                if(waitAnswer){
                    MI2verifyGdb(sendCommandGdb);
                    if(showFile && changeLine){
                        lines = set_window_pos(&line_pos);
                        changeLine=FALSE;
                        start_time=-1;
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
    debug_line = -1;

    strcpy(program_file.name_file, nameCobFile);
    readCodFile(&program_file);
    freeWatchingList();
    lines = program_file.lines;
    if(BPList!=NULL){
        Lines * line = program_file.lines;
        line->breakpoint='N';
        while(line!=NULL){
            ST_bk * search = BPList;
            while(search!=NULL){
                if(search->line==line->file_line && strcasecmp(search->name, file_cobol)==0){
                    line->breakpoint='S';
                }
                search=search->next;
            }            
            line=line->line_after;
        }
    }
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
    strcpy(file_cobol,"");
    if (locale_info != NULL) {
        decimal_separator = locale_info->decimal_point[0];
    }
    initTerminal();
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
                    strcpy(program_file.name_file,argv[i]);
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
        //cobc_compile(program_file.name_file, values, arg_count);
        cobc_compile(fileCobGroup, values, arg_count);
        printf("Parser starting...\n");
		SourceMap(fileCGroup);
        printf("Parser end...\n");
        strcpy(file_cobol,program_file.name_file);
        char cwd[512];
        getPathName(cwd, file_cobol);
        strcpy(first_file, file_cobol);
        loadfile(file_cobol);
        if(withHigh) highlightParse(); 
        //printf("The current locale is %s \n",setlocale(LC_ALL,""));
        //while(key_press()<=0);
        #if defined(__linux__)
        ttyName=openOuput(program_file.name_file);
        if(ttyName==NULL){
            printf("\n\n%s\n","Error!!! Install 'xterm' for output or use the 'sleep' command.\n");
            freeFile();
            return 0;
        } 
        #endif
        start_gdb(nameExecFile,cwd);
        freeBKList();
        freeFile();
        freeWatchingList();
        //TODO: 
        //freeVariables();
        if(ttyName!=NULL) free(ttyName);
        gotoxy(1,(VIEW_LINES-1));
    }
    print_color_reset();
    cursorON();
    printf("The end of the COBGBD execution.\n");
    fflush(stdout);
    return 0;
}
