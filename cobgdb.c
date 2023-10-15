#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#if defined(__linux__)
#include <unistd.h>
#else
#include <windows.h>
#endif
#include "cobgdb.h"
#define __WITH_TESTS_ 
struct program_file program_file;
int start_window_line = 0;
int qtd_window_line = 23;
int start_line_x = 0;
int debug_line = -1;
int running = TRUE;
int waitAnswer = FALSE;
char * gdbOutput = NULL;
int showFile=TRUE;
int ctlVar=0;
char * ttyName=NULL;
int WIDTH=80;
int HEIGHT=25;
int changeLine=FALSE;
int color_frame=color_light_blue;
char decimal_separator = '.';

ST_Line * LineDebug=NULL;
ST_Attribute * Attributes=NULL;
ST_DebuggerVariable * DebuggerVariable=NULL;

int start_gdb(char * name);

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
}

int cobc_compile(char * file){
    char * param[] = {
                    "-g ",
                    "-fsource-location ",
                    "-ftraceall ",
                    "-Q ",
                    "--coverage ",
                    "-A ",
                    "--coverage ",
                    "-v ",
                    "-O0 ",
                    "-x "};

    char compiler[256] = "cobc ";
    int tot = (int)( sizeof(param) / sizeof(param[0]));
    for(int a=0;a< tot;a++){
        strcat(compiler,param[a]);
    }
    strcat(compiler,file);
    printf("%s\n",compiler);
    FILE *pfd = popen( compiler, "r" );
    if ( pfd == 0 ) {
        printf( "failed to open pipe\n" );
        return 1;
    }
    while ( !feof( pfd ) ) {
        char buf[1024] = {0};
        fgets( buf, sizeof(buf) - 1, pfd );
        printf( "%s", buf );
    }
    pclose( pfd );
    return 0;
}

char fileNameWithoutExtension(char * file, char * onlyName){
    int qtd=strlen(file);
    int a=0;
    if(strchr(file,'.')!=NULL){
        while(file[qtd]!='.') qtd--;
        onlyName[qtd--]='\0';
        while(qtd>=0){
            onlyName[qtd]=file[qtd];
            qtd--;
        }   
    }else{
        strcpy(onlyName, file);
    }
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
            if(new_pos<21){
                *line_pos=new_pos;
            }else{
                start_window_line = debug_line - 21;
                *line_pos=20;
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
    char * opt = " COBGDB - (R)run (N)next (S)step (C)continue (G)cursor (V/H)variables (Q)quit";
    char aux[100];
    sprintf(aux,"%-80s\r", opt);
    gotoxy(1,1);
    printBK(aux, color_white, color_frame);
    gotoxy(1,24);
    sprintf(aux,"%80s\r"," ");
    printBK(aux, color_frame, color_frame);
}

int show_info(){
    gotoxy(1,25);
    if(debug_line>0 && !running){
        print_colorBK(color_green, color_black);
        printf("%s\r", "Debugging             ");
    }else if(debug_line>0 && running){
        print_colorBK(color_red, color_black);
        printf("%s\r", "Running               ");
    }else{
        print_colorBK(color_black, color_black);
        printf("%s\r", "                      ");
    }
    gotoxy(1,25);
}

int show_file(Lines * lines, int line_pos){
    Lines * show_line=lines;
    char vbreak = ' ';
    char pline[252];
    char aux[100];
    
    gotoxy(1,1);
    show_opt();
    int bkgColor=color_gray;
    char chExec = ' ';
    for(int i=0;i<22;i++){
        if(show_line != NULL){
            gotoxy(1,i+2);
            if(show_line->breakpoint=='S'){
                printBK("B",color_yellow, color_frame);
            }else{
                printBK(" ",color_yellow, color_frame);
            }
            if(debug_line==show_line->file_line && !running){
                print(">", color_green);
            }else{
                chExec = (debug_line==show_line->file_line)?'!': ' ';
                print_color(color_red);
                printf("%c",(debug_line==show_line->file_line)?'!': ' ');
            }     
            print_color(color_gray);
            printf("%-4d ", show_line->file_line);
            if(show_line->high==NULL){
                show_line->line[strcspn(show_line->line,"\n")]='\0';
                int len=strlen(show_line->line);
                //if(len>0) show_line->line[strcspn(show_line->line,"\n")]='\0';
                if(line_pos==i)
                    print_colorBK(color_green, bkgColor);
                else
                    print_colorBK(color_white, color_black);
                int nm = len -start_line_x;
                if(nm>0){
                    printf("%s", &show_line->line[start_line_x]);
                    nm=(72-nm>0)?72-nm:72;
                    printf("%*s", nm, " ");
                }else{
                    printf("%72s", " ");
                }
            }else{
                bkgColor=(line_pos==i)?color_gray:-1;
                printHighlight(show_line->high, bkgColor, start_line_x, 72);
            }
            show_line = show_line->line_after;
            if(show_line == NULL){
                if(line_pos>i) line_pos=i-1;
            }
            //gotoxy(80,i+2);
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
    
    if(qtd_window_line>program_file.qtd_lines) qtd_window_line=program_file.qtd_lines;
    Lines * lines = program_file.lines;
    Lines * lb = NULL;
    int line_file=0;
    int bstop = 0;
    //startBuffer();

    //setvbuf(stdout, NULL, _IOFBF, 8192); // Habilita o buffering de saída com um buffer de 4 KB
    cursorOFF();
    clearScreen();
    while(program_file.lines!=NULL && !bstop){
        if(showFile){
            //setvbuf(stdout, NULL, _IONBF, 8192);
            show_opt();
            line_file = lines->file_line;
            line_pos=show_file(lines, line_pos);
            showFile=FALSE;
            print_color_reset();
            fflush(stdout);
        }
        input_character =  key_press();
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
                    if(line_pos>20){
                        start_window_line++;                 
                        lines = lines->line_after;
                    }else if(line_pos<=20){
                        line_pos++;
                    }
                }
                showFile=TRUE;
                break;
            case VK_PGUP:
                if(line_pos>0){
                    line_pos=0;
                }else{
                    qtd_page = 23;
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
                if(line_pos<21){
                    line_pos=21;
                }else{
                    if((program_file.qtd_lines-start_window_line)>22){
                        while((qtd_page++)<23 && lines->line_after!=NULL){
                            start_window_line++;
                            lines=lines->line_after;
                        }
                        line_pos=21;
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
                    } 
                    showFile=TRUE;
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
                if(!waitAnswer) MI2stepInto(sendCommandGdb);
                debug_line = dblAux;
                break;
            case 'n':
            case 'N':
                if(!waitAnswer) MI2stepOver(sendCommandGdb);
                break;
            case 'c':
            case 'C':
                if(!waitAnswer) MI2stepOut(sendCommandGdb);
                break;
            case 'g':
            case 'G':
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
                    }
                }
                break;
        }
        show_info();
        //printf("Tecla = %d", input_character);
    }
}

int loadfile(char * nameCobFile) {
    int width=0, height=0;
    int qtd_page = 0;
    int input_character;
    strcpy(program_file.name_file, nameCobFile);
    cobc_compile(program_file.name_file);
    set_terminal_size(80, 25);
    #if defined(_WIN32)
    Sleep(200);
    #elif defined(__linux__)
    sleep(1);
    #endif
    get_terminal_size(&width, &height);
    printf("width= %d", width);
    printf(", height= %d\n", height);
    readCodFile(&program_file);
}

int freeFile(){
    free_memory();
}

int main(int argc, char **argv) {
    char nameExecFile[256];
    char baseName[256];
    char nameCFile[256];
    struct lconv *locale_info = localeconv();
    setlocale(LC_CTYPE, "");
    setlocale(LC_ALL,"");
    #if defined(_WIN32)
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    #endif    

    if (locale_info != NULL) {
        decimal_separator = locale_info->decimal_point[0];
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
        strcpy(program_file.name_file,argv[1]);
        cobc_compile(program_file.name_file);
        fileNameWithoutExtension(program_file.name_file, &baseName[0]);
        strcpy(nameExecFile, baseName);
        #if defined(_WIN32)
        strcat(nameExecFile,".exe");
        #endif
        printf("Name: %s",nameExecFile);
        loadfile(program_file.name_file);
        strcpy(nameCFile,baseName);
        strcat(nameCFile,".c");
        printf("Parser starting...\n");
        normalizePath(nameCFile);
		char fileGroup[4][512];
		strcpy(fileGroup[0],nameCFile);
		strcpy(fileGroup[1], "");
		SourceMap(fileGroup);
        executeParse(); 
        printf("Parser end...\n");
        //printf("A localidade corrente agora é %s \n",setlocale(LC_ALL,""));
        //while(key_press()<=0);
        #if defined(__linux__)
        ttyName=openOuput(program_file.name_file);
        if(ttyName==NULL){
            printf("\n\n%s\n","Error!!! Install 'xterm' for output or use the 'sleep' command.\n");
            freeFile();
            return 0;
        } 
        #endif
        start_gdb(nameExecFile);
        freeFile();
        //TODO: 
        //freeVariables();
        if(ttyName!=NULL) free(ttyName);
        gotoxy(1,24);
    }
    print_color_reset();
    cursorON();
    printf("The end of the COBGBD execution.\n");
    fflush(stdout);
    return 0;
}
