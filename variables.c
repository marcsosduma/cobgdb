/* 
   This code is based on the GnuCOBOL Debugger extension available at: 
   https://github.com/OlegKunitsyn/gnucobol-debug
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <time.h>
#include "cobgdb.h"
#define VIEW_LINES 24
#define VIEW_COLS  80

extern ST_DebuggerVariable * DebuggerVariable;
extern int ctlVar;
extern int color_frame;
extern ST_Watch * Watching;
extern char file_cobol[512];
int showOne = FALSE;
int expand = FALSE;
wchar_t wcBuffer[512];
ST_DebuggerVariable * currentVar;

int show_opt_var(){
    char * opt = " COBGDB - (R)return (ENTER)expand/contract (C)change var";
    char aux[100];
    sprintf(aux,"%-80s\r", opt);
    gotoxy(1,1);
    printBK(aux, color_white, color_frame);
    gotoxy(1,VIEW_LINES-1);
    sprintf(aux,"%80s\r"," ");
    printBK(aux, color_frame, color_frame);
}

int print_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *),
                   char * functionName){
    int bkg;
    (*notShow)--;
    if(*notShow<0 && strcmp(var->functionName,functionName)==0 && lin<VIEW_LINES-3){
        gotoxy(1,lin+2);
        if((level>0 || var->dataSotorage==NULL) && ctlVar!=var->ctlVar){
            MI2evalVariable(sendCommandGdb,var,0,0);
            var->ctlVar=ctlVar;
        }
        char varcobol[100];
        int linW=78;
        showOne=TRUE;
        if(expand && line_pos==lin){
            if(var->show=='+')
                var->show='-';
            else if(var->show=='-')
                var->show='+';
        }
        sprintf(varcobol,"%.*s%c%s: ",level*2," ",var->show,var->cobolName);
        printBK(" ", color_white, color_frame);  
        bkg=color_black;
        if(line_pos==lin){
            bkg = color_gray;
            currentVar = var;
        }
        print_colorBK(color_pink, bkg);
        int nm = strlen(varcobol)-start_linex_x;
        int start_linex_x2=0;
        if(nm>0){
            printf("%-.*s", nm, &varcobol[start_linex_x]);
            linW-=nm;
        }else{
            bkg=(line_pos==lin)?color_gray:color_black;
            start_linex_x2=-1 * nm;
        }
        if(var->value!=NULL){
            wchar_t *wcharString = (wchar_t *)malloc((strlen(var->value) + 1) * sizeof(wchar_t));           
            #if defined(_WIN32)
            MultiByteToWideChar(CP_UTF8, 0, var->value, -1, wcharString,(strlen(var->value) + 1) * sizeof(wchar_t) / sizeof(wcharString[0]));
            #else
            mbstowcs(wcharString, var->value, strlen(var->value) + 1);
            #endif
            int lenVar = wcslen(wcharString);
            if(start_linex_x2<lenVar){
                nm = lenVar-start_linex_x2;
                if(nm>linW) nm=linW;
                print_colorBK(color_green, bkg);
                wcsncpy(wcBuffer, &wcharString[start_linex_x2], nm);
                wcBuffer[nm]='\0';
                printf("%ls",wcBuffer);
                linW-=nm;
            }
            free(wcharString);
        }
        print_colorBK(color_green, bkg);
        if(linW>0){
             printf("%*ls",linW,L" ");
        }
        printBK(" ", color_white, color_frame);  
        lin++;
    }
    if(strcmp(var->functionName,functionName)==0){
        if(var->first_children!=NULL && var->show=='-'){
            lin=print_variable(++level, notShow, line_pos, start_lin, 
                    end_lin, lin, start_linex_x,
                    var->first_children,sendCommandGdb, functionName);
        }
        if(var->brother!=NULL){
            lin = print_variable(++level, notShow, line_pos, start_lin, 
                    end_lin, lin, start_linex_x,
                    var->brother,sendCommandGdb, functionName);
        }
    }
    return lin;
}

ST_DebuggerVariable * firstVar(ST_DebuggerVariable * var){
    var = DebuggerVariable;
    while(TRUE){
        if(var->variablesByCobol!=NULL && var->parent==NULL){
            break;
        }
        var=var->next;
    }
    return var;
}

int change_var(int (*sendCommandGdb)(char *),int lin){
    char aux[500];
    int bkg= color_dark_red;

    while((lin+3)>=(VIEW_LINES-2)) lin--;
    gotoxy(10,lin+2);
    print_colorBK(color_white, bkg);
    draw_box_first(10,lin+2,61,"Change Variable");
    draw_box_border(10,lin+3);
    draw_box_border(72,lin+3);
    print_colorBK(color_yellow, bkg);
    gotoxy(11,lin+3);
    sprintf(aux,"%s:",currentVar->cobolName);
    printf("%-61s",aux);
    lin++;
    print_colorBK(color_white, bkg);
    draw_box_last(10,lin+3,61);
    print_colorBK(color_green, bkg);
    fflush(stdout);
    gotoxy(12+strlen(aux),lin+2);
    readchar(aux,50);    
    MI2changeVariable(sendCommandGdb, currentVar, aux);
    MI2variablesRequest(sendCommandGdb);
    ctlVar=(ctlVar>10000)?1:ctlVar+1;
}

int show_variables(int (*sendCommandGdb)(char *)){
    gotoxy(1,1);
    int lin=0;    
    int line_pos=0;
    int start_window_line = 0;
    int qtd_window_line = VIEW_LINES-2;
    int start_linex_x = 0;      
    expand = FALSE;
    char aux[100];
    int bkg;

    currentVar= NULL;
    char input_character= ' ';
    ST_DebuggerVariable * var = firstVar(var);
    int old_lin=lin;
    int notShow;
    char * functionName = MI2getCurrentFunctionName(sendCommandGdb);
    if(functionName==NULL) return 0;
    while(input_character!='r'){
        notShow=start_window_line;
        if(var!=NULL){
            gotoxy(1,1);
            show_opt_var();
            gotoxy(1,2);
            showOne = FALSE;
            currentVar = NULL;
            while(var!=NULL){
                if(var->parent==NULL){
                    lin=print_variable(0, &notShow, line_pos, start_window_line, 
                        start_window_line+VIEW_LINES-5, lin, start_linex_x,
                        var,sendCommandGdb, functionName);
                }
                //lin=(lin==old_lin)?lin+1:lin;
                var=var->next;
            }
            while(var==NULL && lin<VIEW_LINES-3){
                gotoxy(1,lin+2);
                print_colorBK(color_black, color_frame); printf(" ");
                bkg=(line_pos==lin)?color_gray:color_black;
                print_colorBK(color_black, bkg);
                printf("%78s"," ");
                print_colorBK(color_black, color_frame); printf(" ");
                lin++;
            }
            fflush(stdout);
        }
        gotoxy(1,1);
        expand = FALSE;
        var = NULL;
        input_character =  key_press();
        switch (input_character)
        {
            case VK_UP:
                if(line_pos>0){
                    line_pos--;
                }else{
                    start_window_line=(start_window_line>0)?start_window_line-1:0; 
                }
                var=firstVar(var);
                lin=0;
                break;
            case VK_DOWN: 
                if(line_pos<=VIEW_LINES-5){
                    line_pos++;
                }else{
                    if(showOne) start_window_line++; 
                }
                var=firstVar(var);
                lin=0;
                break;
            case VK_PGUP:
                if(line_pos>0){
                    line_pos-= (VIEW_LINES-3);
                    line_pos=(line_pos<0)?0:line_pos;
                }else{
                    start_window_line-= (VIEW_LINES-3);
                    start_window_line=(start_window_line<0)?0:start_window_line;
                }
                var=firstVar(var);
                lin=0;
                break;
            case VK_PGDOWN: 
                if(line_pos<VIEW_LINES-4){
                    line_pos=VIEW_LINES-4;
                }else if(showOne){
                    line_pos=VIEW_LINES-4;
                    start_window_line=start_window_line+VIEW_LINES-3;
                }
                var=firstVar(var);
                lin=0;
                break;
            case VK_LEFT:
                start_linex_x=(start_linex_x>0)?start_linex_x-1:start_linex_x;
                var=firstVar(var);
                lin=0;
                break;
            case VK_RIGHT:
                start_linex_x=(start_linex_x<600)?start_linex_x+1:start_linex_x;
                var=firstVar(var);
                lin=0;
                break;
            case VK_ENTER:
                expand=TRUE;
                var=firstVar(var);
                lin=0;
                break;
            case 'c':
            case 'C':
                if(currentVar!=NULL){
                    change_var(sendCommandGdb, line_pos);
                    var=firstVar(var);
                    lin=0;
                }
                break;
            case VK_ESCAPE:
            case 'r':
            case 'R':
                input_character='r';
                break;
            case 'b':
            case 'B':
                printf("stop\n");
                break;
            default: 
                break;
        }
        //show_info();
        //if(input_character>0) printf("%d\n", input_character);
    }
    if(functionName!=NULL) free(functionName);
}

int hover_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *), int bkg){
   
    int var_color = color_yellow;
    int value_color = color_green;

    if(lin<VIEW_LINES-3){
        gotoxy(10,lin+2);
        if(ctlVar!=var->ctlVar){
            MI2evalVariable(sendCommandGdb,var,0,0);
            var->ctlVar=ctlVar;
        }
        var->ctlVar=ctlVar;
        char varcobol[100];
        int linW=60;
        showOne=TRUE;
        sprintf(varcobol,"%.*s%c%s: ",level*2," ",'-',var->cobolName);
        print_colorBK(color_white, bkg);
        draw_box_border(10,lin+2);
        print_colorBK(var_color, bkg);
        int nm = strlen(varcobol)-start_linex_x;
        int start_linex_x2=0;
        if(nm>0){
            printf("%-.*s", nm, &varcobol[start_linex_x]);
            linW-=nm;
        }else{
            start_linex_x2=-1 * nm;
        }
        if(var->value!=NULL){
            wchar_t *wcharString = (wchar_t *)malloc((strlen(var->value) + 1) * sizeof(wchar_t));           
            #if defined(_WIN32)
            MultiByteToWideChar(CP_UTF8, 0, var->value, -1, wcharString,(strlen(var->value) + 1) * sizeof(wchar_t) / sizeof(wcharString[0]));
            #else
            mbstowcs(wcharString, var->value, strlen(var->value) + 1);
            #endif
            int lenVar = wcslen(wcharString);
            if(start_linex_x2<lenVar){
                nm = lenVar-start_linex_x2;
                if(nm>linW) nm=linW;
                print_colorBK(value_color, bkg);
                wcsncpy(wcBuffer, &wcharString[start_linex_x2], nm);
                wcBuffer[nm]='\0';
                printf("%ls",wcBuffer);
                linW-=nm;
            }
            free(wcharString);
        }
        print_colorBK(value_color, bkg);
        if(linW>0){
            printf("%*ls",linW+1,L" ");
            print_colorBK(color_white, bkg);
            draw_box_border(72,lin+2);
        }else{
            printf("%ls",L" ");  
            print_colorBK(color_white, bkg);
            draw_box_border(72,lin+2);
        }
        lin++;
    }
    if(var->first_children!=NULL){
        lin=hover_variable(++level, notShow, line_pos, start_lin, 
                   end_lin, lin, start_linex_x,
                   var->first_children,sendCommandGdb, bkg);
    }
    if(var->brother!=NULL){
        lin = hover_variable(++level, notShow, line_pos, start_lin, 
                   end_lin, lin, start_linex_x,
                   var->brother,sendCommandGdb, bkg);
    }
    return lin;
}

int show_line_var(struct st_highlt * high, char * functionName, int (*sendCommandGdb)(char *)){
    char command[256];
    int line_pos=0;
    int start_window_line = 0;
    int qtd_window_line = VIEW_LINES-2;
    int start_linex_x = 0;  
    int line_start = 7;    
    expand = FALSE;
    char aux[100];
    struct st_highlt * h = high;

    gotoxy(1,1);
    char input_character= ' ';
    wchar_t vl[100];
    wchar_t *wcBuffer = (wchar_t *)malloc(512);
    int bkg= color_dark_red;
    int qtd = 0;
    int st=0;
    int lin=line_start;
    while(input_character!='R' && input_character!='r' ){
        while(h!=NULL){
            if(h->type==TP_ALPHA){
                wcsncpy(wcBuffer, &h->token[st], h->size);
                wcBuffer[h->size]='\0';   
                int len = wcstombs(NULL, wcBuffer, 0);
                char *chval = (char *)malloc(len + 1);
                wcstombs(chval, wcBuffer, len + 1);
                ST_DebuggerVariable * var =  findVariableByCobol(functionName, toUpper(chval));
                if(var!=NULL){
                    int notShow=FALSE;
                    qtd++;
                    if(qtd==1){
                        //gotoxy(10,boxy);
                        print_colorBK(color_white, bkg);
                        draw_box_first(10,lin+2,61,"Show Line Variables");
                        lin++;
                    }
                    lin=hover_variable(0, &notShow, line_pos, start_window_line, 
                        VIEW_LINES-3, lin, start_linex_x,
                        var,sendCommandGdb, bkg);
                }
                if(qtd>0){
                    print_colorBK(color_white, bkg);
                    draw_box_last(10,lin+2,61);
                }
                free(chval);
                //printf("%*ls",qtdMove,wcBuffer);
            }
            h=h->next;
        }
        fflush(stdout);
        if(qtd==0) break;
        input_character =  key_press();
        switch (input_character)
        {
            case VK_UP:
                if(line_pos>0){
                    line_pos--;
                }else{
                    start_window_line=(start_window_line>0)?start_window_line-1:0; 
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_DOWN: 
                if(line_pos<=VIEW_LINES-5){
                    line_pos++;
                }else{
                    if(showOne) start_window_line++; 
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_PGUP:
                if(line_pos>0){
                    line_pos-=(VIEW_LINES-3);
                    line_pos=(line_pos<0)?0:line_pos;
                }else{
                    start_window_line-=(VIEW_LINES-3);
                    start_window_line=(start_window_line<0)?0:start_window_line;
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_PGDOWN: 
                if(line_pos<VIEW_LINES-4){
                    line_pos=VIEW_LINES-4;
                }else if(showOne){
                    line_pos=VIEW_LINES-4;
                    start_window_line=start_window_line+VIEW_LINES-3;
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_LEFT:
                start_linex_x=(start_linex_x>0)?start_linex_x-1:start_linex_x;
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_RIGHT:
                start_linex_x=(start_linex_x<600)?start_linex_x+1:start_linex_x;
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_ENTER:
                expand=TRUE;
                qtd = 0; st=0; lin=line_start;
            default: 
                if(input_character>0) input_character='r';
                break;
        }
        //show_info();
        //if(input_character>0) printf("%d\n", input_character);
    }
    free(wcBuffer);
}



char * EspecialTrim(char *s) {
    int end, len;
    char *buf;
    int j = 0;
    if(s==NULL) return NULL;
    len = strlen(s);
    if(len<1) return NULL;
    buf = (char *)malloc(len + 1);  // +3 to include the null terminator and two quotes
    if(s[0]=='"' || s[0]=='\'') buf[j++]=s[0];
    for (end = len - 1; end >= 0 && (isspace(s[end]) || s[end] == '"'); end--);
    if(end>=0){ 
        int to_move=end-j+1;
        memcpy(&buf[j],&s[j],to_move);
        j=j+to_move;
    }
    if(s[len-1]=='"' || s[len-1]=='\'') 
        buf[j++]=s[len-1];
    buf[j]='\0';
    strcpy(s, buf);
    free(buf);
    return s;
}

ST_Watch * new_watching(struct st_highlt * exe_line, ST_DebuggerVariable * var, int posx, int posy){
    ST_Watch * wnew=malloc(sizeof(ST_Watch));
    wnew->var= var;
    wnew->line= exe_line;
    wnew->posy= posy;
    wnew->posx = posx;
    wnew->start_time= -1;
    wnew->status=0;
    wnew->next=NULL;
}

void var_watching(struct st_highlt * exe_line, int (*sendCommandGdb)(char *), int waitAnser){
    char command[256];
    int line_pos=0;
    int start_window_line = 0;
    int qtd_window_line = VIEW_LINES-2;
    int start_linex_x = 0;  
    int line_start = 7;    
    expand = FALSE;
    char aux[100];
    wchar_t wcharString[512];
    struct st_highlt * h = exe_line;
    int bkg= color_light_gray; //color_dark_red;
    int fkg= color_dark_blue;

    ST_Watch * wt = Watching;
    ST_Watch * wt_before = NULL;
    boolean insert=TRUE;
    int posy_b= 1000; // VIEW_LINES-5;
    int posy_a = VIEW_LINES-5;
    while(wt!=NULL){
        if(posy_a>=wt->posy) posy_a=wt->posy-3;
        if(wt->line==exe_line){
            int status=1;
            insert = FALSE;
        }else{
            if(wt->start_time<0 && !waitAnser){
                wt->start_time= time(NULL);
                wt->status=2;
            }else if(wt->status==2 && wt->start_time>0){
                time_t end_time = time(NULL);
                double elapsed_time = difftime(end_time, wt->start_time);
                if(elapsed_time>0){
                    if(wt_before!=NULL){
                        wt_before->next=wt->next;
                    }else{
                        Watching=wt->next;
                    }
                    if(posy_b>wt->posy) posy_b=wt->posy;
                    ST_Watch * to_free = wt;
                    wt=wt->next;
                    free(to_free);
                    continue;
                }
            }
        }
        wt_before=wt;
        wt = wt->next;
    }
    if(insert){
        int st=0;
        char chval[500];
        char * functionName = NULL;
        while(h!=NULL){
            if(h->type==TP_ALPHA){
                wcsncpy(wcBuffer, &h->token[st], h->size);
                wcBuffer[h->size]='\0';   
                int len = wcstombs(NULL, wcBuffer, 0);
                wcstombs(chval, wcBuffer, len + 1);
                if(functionName==NULL && !waitAnser) 
                    functionName = MI2getCurrentFunctionName(sendCommandGdb);
                ST_DebuggerVariable * var =  findVariableByCobol(functionName, toUpper(chval));
                if(var!=NULL){
                    if(posy_a<3) posy_a = posy_b;
                    if(posy_a<3 || posy_a>(VIEW_LINES-5)) posy_a = VIEW_LINES-5;
                    wt = Watching;
                    wt_before = NULL;
                    while(wt!=NULL){
                        if(strcmp(wt->var->cobolName,var->cobolName)==0) break;
                        wt_before=wt;
                        wt=wt->next;
                    }
                    if(wt!=NULL){
                        wt->line= exe_line;
                        wt->start_time= -1;
                        wt->status=0;
                    }else{
                        ST_Watch * wnew = new_watching(exe_line, var, -1, posy_a);
                        if(wt_before==NULL){
                            Watching=wnew;
                        }else{
                            wt_before->next=wnew;
                        }
                        wt_before=wnew;
                    }
                    posy_a-=3;
                }
            }
            h=h->next;
        }
    }
    wt = Watching;
    while(wt!=NULL){
        if(wt->status==0 || wt->status==2)
            if(!waitAnser){
                MI2evalVariable(sendCommandGdb,wt->var,0,0);
                wt->var->value=EspecialTrim(wt->var->value);
            }
        int len = strlen(wt->var->cobolName)+2;
        if(wt->var->value!=NULL){
            #if defined(_WIN32)
            MultiByteToWideChar(CP_UTF8, 0, wt->var->value, -1, wcharString,(strlen(wt->var->value) + 1) * sizeof(wchar_t) / sizeof(wcharString[0]));
            #else
            mbstowcs(wcharString, wt->var->value, strlen(wt->var->value) + 1);
            #endif
            int lenVar = wcslen(wcharString);
            if(lenVar>len) len=lenVar;
            if(len>60) len=60;
            wt->posx= VIEW_COLS - len - 6;
            wt->size= len+2;
        }else{
            wt->size= len;
            wt->posx= VIEW_COLS - len - 6;
            wcscpy(wcharString,L"");
        }
        print_colorBK(fkg, bkg);
        int posy=wt->posy;
        draw_box_first(wt->posx,posy++,wt->size,wt->var->cobolName);
        draw_box_border(wt->posx, posy);
        int to_move=wcslen(wcharString);
        if(to_move>wt->size) to_move=wt->size;
        wcsncpy(wcBuffer, wcharString, to_move);
        wcBuffer[to_move]='\0';
        printf("%-*ls",wt->size,wcBuffer);
        draw_box_border(wt->posx+wt->size+1, posy++);
        draw_box_last(wt->posx, posy, wt->size);
        wt=wt->next;
    }
}

#define MAX_FILES 100

void show_sources(int (*sendCommandGdb)(char *)){
    char input_character=-1;
    int bkgr = color_dark_red;
    int bkg;
    int frg = color_white;
    int csel = color_light_gray;
    int line_pos=0;
    char files[MAX_FILES][512];
    int qtd_files=0;
    int start_file=0;
    int file_sel = -1;
    boolean show = TRUE;

    qtd_files = MI2sourceFiles(sendCommandGdb,files);
    while(input_character!=-100){
        gotoxy(1,1);
        int lin = 7;
        int col = 9;
        int size = 60;
        if(show){
            gotoxy(col,lin);
            print_colorBK(frg, bkgr);
            draw_box_first(col,lin++,size,"Source Files");
            int f = start_file;
            int pos=0;
            file_sel=-1;
            while(pos<10){
                print_colorBK(frg, bkgr);
                draw_box_border(col, lin);
                bkg=(line_pos==pos)?csel:bkgr;
                print_colorBK(frg, bkg);
                if(f<qtd_files){
                    int len = strlen(files[f]);
                    if(len>60){
                        int start=len-56;
                        printf("...%-60s",&files[f][start]);
                    }else{
                        printf("%-60s",files[f]);
                    }
                    if(line_pos==pos) file_sel=f;
                }else{
                    printf("%-60s"," ");
                }
                print_colorBK(frg, bkgr);
                draw_box_border(col+ size+1, lin);
                f++; lin++; pos++;
            }
            print_colorBK(frg, bkgr);
            draw_box_last(col, lin, size);
        }
        print_color_reset();
        fflush(stdout);
        gotoxy(1,1);
        input_character =  key_press();
        switch (input_character)
        {
            case VK_UP:
                if(line_pos>0){
                    line_pos--;
                }else{
                    start_file=(start_file>0)?start_file-1:0; 
                }
                show = TRUE;
                break;
            case VK_DOWN: 
                if(line_pos<9){
                    line_pos++;
                }else{
                    start_file=(start_file<qtd_files)?start_file+1:qtd_files; 
                }
                show = TRUE;
                break;
            case VK_PGUP:
                if(line_pos>0){
                    line_pos= 0;
                }else{
                    line_pos-= 9;
                    line_pos=(line_pos<0)?0:line_pos;
                    start_file-=10;
                    start_file = (start_file>0)?start_file:0;
                }
                show = TRUE;
                break;
            case VK_PGDOWN: 
                if(line_pos<9){
                    line_pos=9;
                }else{
                    line_pos+=9;
                    if(line_pos>9) line_pos=9;
                    start_file+=10;
                    start_file = (start_file>qtd_files)?qtd_files:start_file;
                }
                show = TRUE;
                break;
            case VK_ENTER:       
                if(file_sel<0) break;
                freeFile();
                strcpy(file_cobol, files[file_sel]);
                loadfile(files[file_sel]);
                highlightParse();
                input_character=-100;
                break;
            case 'R':
            case 'r':
            case 'Q':
            case 'q':
            case VK_ESCAPE:
                printf("stop\n");
                input_character=-100;
                break;
            default: 
                break;
        }
        //gotoxy(1,23); if(input_character>0) printf("%d\n", input_character);
    }
}