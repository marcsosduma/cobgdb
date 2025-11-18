/*
 * COBGDB - GnuCOBOL Debugger
 *
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
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h> 
#include "cobgdb.h"

#ifndef VKEY_BACKSPACE
#define VKEY_BACKSPACE 8
#endif

extern struct st_cobgdb cob;
extern ST_DebuggerVariable * DebuggerVariable;

extern int VIEW_LINES;
extern int VIEW_COLS;
extern int color_frame;
extern ST_Watch * Watching;
int showOne = FALSE;
int expand = FALSE;
wchar_t wcBuffer[512];
ST_DebuggerVariable * currentVar;

int show_opt_var(){
    char * opt = " CobGDB - (R)return (ENTER)expand/contract (E)edit var";
    char aux[150];
    sprintf(aux,"%-*s\r",VIEW_COLS, opt);
    gotoxy(1,1);
    printBK(aux, color_white, color_frame);
    gotoxy(1,VIEW_LINES-1);
    sprintf(aux,"%*s\r",VIEW_COLS," ");
    printBK(aux, color_frame, color_frame);
    return TRUE;
}

int print_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *),
                   char * functionName){
    int bkg;
    (*notShow)--;
    if(*notShow<0 && strcmp(var->functionName,functionName)==0 && lin<VIEW_LINES-3){
        gotoxy(1,lin+2);
        if((level>0 || var->dataSotorage==NULL) && cob.ctlVar!=var->ctlVar){
            MI2evalVariable(sendCommandGdb,var,0,0);
            var->ctlVar=cob.ctlVar;
        }
        char varcobol[100];
        int linW=VIEW_COLS-2;
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
            wchar_t *wcharString = to_wide(var->value);
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

int edit_variable(int (*sendCommandGdb)(char *),int lin){
    char aux[500];
    int bkg= color_dark_red;

    while((lin+3)>=(VIEW_LINES-2)) lin--;
    gotoxy(10,lin+2);
    print_colorBK(color_white, bkg);
    draw_box_first(10,lin+2,61,"Edit Variable");
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
    //gotoxy(12+strlen(aux),lin+2);
    char * new_value=malloc(1024);
    new_value[0]='\0';
    if(currentVar->value!=NULL){
        //new_value = malloc(currentVar->size+3);
        if (currentVar->value[0] == L'"'){
            memset(new_value,' ',currentVar->size);
            new_value[currentVar->size]='\0';
            int q = strlen(currentVar->value)-2;
            strncpy(new_value, &currentVar->value[1], q);
            new_value[q]='\0';
        }else{
            sprintf(new_value,"%*s", currentVar->size, currentVar->value);
        }
        //new_value = strdup(currentVar->value);
    }
    if(updateStr(new_value,60-strlen(aux), 12+strlen(aux),lin+2)==TRUE){
        MI2editVariable(sendCommandGdb, currentVar, new_value);
    }
    free(new_value);
    MI2variablesRequest(sendCommandGdb);
    cob.ctlVar=(cob.ctlVar>10000)?1:cob.ctlVar+1;
    return TRUE;
}

int show_variables(int (*sendCommandGdb)(char *)){
    gotoxy(1,1);
    int lin=0;    
    int line_pos=0;
    int start_window_line = 0;
    int start_linex_x = 0;      
    expand = FALSE;
    int bkg;

    currentVar= NULL;
    char input_character= ' ';
    ST_DebuggerVariable * var = NULL;
    var = firstVar(var);
    int notShow;
    char * functionName = MI2getCurrentFunctionName(sendCommandGdb);
    if(functionName==NULL) return 0;
    while(input_character!='r'){
        disableEcho();
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
                printf("%*s",VIEW_COLS-2," ");
                print_colorBK(color_black, color_frame); printf(" ");
                lin++;
            }
            fflush(stdout);
        }
        gotoxy(1,1);
        enableEcho();
        expand = FALSE;
        var = NULL;
        input_character =  key_press(MOUSE_NORMAL);
        switch (input_character)
        {
            case VKEY_UP:
                if(line_pos>0){
                    line_pos--;
                }else{
                    start_window_line=(start_window_line>0)?start_window_line-1:0; 
                }
                var=firstVar(var);
                lin=0;
                break;
            case VKEY_DOWN: 
                if(line_pos<=VIEW_LINES-5){
                    line_pos++;
                }else{
                    if(showOne) start_window_line++; 
                }
                var=firstVar(var);
                lin=0;
                break;
            case VKEY_PGUP:
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
            case VKEY_PGDOWN: 
                if(line_pos<VIEW_LINES-4){
                    line_pos=VIEW_LINES-4;
                }else if(showOne){
                    line_pos=VIEW_LINES-4;
                    start_window_line=start_window_line+VIEW_LINES-3;
                }
                var=firstVar(var);
                lin=0;
                break;
            case VKEY_LEFT:
                start_linex_x=(start_linex_x>0)?start_linex_x-1:start_linex_x;
                var=firstVar(var);
                lin=0;
                break;
            case VKEY_RIGHT:
                start_linex_x=(start_linex_x<600)?start_linex_x+1:start_linex_x;
                var=firstVar(var);
                lin=0;
                break;
            case VKEY_ENTER:
                expand=TRUE;
                var=firstVar(var);
                lin=0;
                break;
            case 'e':
            case 'E':
                if(currentVar!=NULL){
                    edit_variable(sendCommandGdb, line_pos);
                    var=firstVar(var);
                    lin=0;
                }
                break;
            case VKEY_ESCAPE:
            case 'r':
            case 'R':
            case 'q':
            case 'Q':
                input_character='r';
                break;
            default: 
                break;
        }
        //show_info();
        //if(input_character>0) printf("%d\n", input_character);
    }
    if(functionName!=NULL) free(functionName);
    return TRUE;
}

int hover_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *), int bkg){
   
    int var_color = color_yellow;
    int value_color = color_green;
    int bcolor = bkg;

    if(lin<VIEW_LINES-3){
        gotoxy(10,lin+2);
        if(cob.ctlVar!=var->ctlVar){
            MI2evalVariable(sendCommandGdb,var,0,0);
            var->ctlVar=cob.ctlVar;
        }
        var->ctlVar=cob.ctlVar;
        char varcobol[100];
        int linW=60;
        showOne=TRUE;
        sprintf(varcobol,"%.*s%c%s: ",level*2," ",'-',var->cobolName);
        bcolor=bkg;
        if(line_pos==lin){
            bcolor = color_gray;
            currentVar = var;
        }
        print_colorBK(color_white, bkg);
        draw_box_border(10,lin+2);
        print_colorBK(var_color, bcolor);
        int nm = strlen(varcobol)-start_linex_x;
        int start_linex_x2=0;
        if(nm>0){
            printf("%-.*s", nm, &varcobol[start_linex_x]);
            linW-=nm;
        }else{
            start_linex_x2=-1 * nm;
        }
        if(var->value!=NULL){
            wchar_t *wcharString = to_wide(var->value);
            int lenVar = wcslen(wcharString);
            if(start_linex_x2<lenVar){
                nm = lenVar-start_linex_x2;
                if(nm>linW) nm=linW;
                print_colorBK(value_color, bcolor);
                wcsncpy(wcBuffer, &wcharString[start_linex_x2], nm);
                wcBuffer[nm]='\0';
                printf("%ls",wcBuffer);
                linW-=nm;
            }
            free(wcharString);
        }
        print_colorBK(value_color, bcolor);
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

int show_help_var(int show){
    #if defined(_WIN32)
    int len = VIEW_COLS-1;
    #else
    int len=VIEW_COLS;
    #endif
    gotoxy(1,VIEW_LINES);
    print_colorBK(color_green, color_black);
    if(show){
        printf("%-*s\r",len, "E - Edit variable");
    }else{
        printf("%-*s\r",len, " ");
    }
    gotoxy(1,VIEW_LINES);
    return TRUE;
}

int show_line_var(struct st_highlt * high, char * functionName, int (*sendCommandGdb)(char *)){
    int line_pos=0;
    int start_window_line = 0;
    int start_linex_x = 0;  
    int line_start = 7;    
    expand = FALSE;
    struct st_highlt * h = high;

    gotoxy(1,1);
    char input_character= ' ';
    wchar_t *wcBuffer = (wchar_t *)malloc(512);
    int bkg= color_dark_red;
    int qtd = 0;
    int st=0;
    int lin=line_start;
    disableEcho();
    fflush(stdin);
    while(input_character!='R' && input_character!='r' ){
        disableEcho();
        while(h!=NULL){
            show_help_var(TRUE);
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
                    lin=hover_variable(0, &notShow, line_pos + line_start + 1, start_window_line, 
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
        enableEcho();
        if(qtd==0) break;
        input_character =  key_press(MOUSE_NORMAL);
        if(cob.mouseButton==2) input_character='e';
        switch (input_character)
        {
            case VKEY_UP:
                if(line_pos>0){
                    line_pos--;
                }else{
                    start_window_line=(start_window_line>0)?start_window_line-1:0; 
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VKEY_DOWN: 
                if(line_pos<=VIEW_LINES-5 && (line_pos+line_start+2)<lin){
                    line_pos++;
                }else{
                    if(showOne) start_window_line++; 
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VKEY_PGUP:
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
            case VKEY_PGDOWN: 
                if(line_pos<VIEW_LINES-4){
                    line_pos=VIEW_LINES-4;
                }else if(showOne){
                    line_pos=VIEW_LINES-4;
                    start_window_line=start_window_line+VIEW_LINES-3;
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VKEY_LEFT:
                start_linex_x=(start_linex_x>0)?start_linex_x-1:start_linex_x;
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VKEY_RIGHT:
                start_linex_x=(start_linex_x<600)?start_linex_x+1:start_linex_x;
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VKEY_ENTER:
                expand=TRUE;
                qtd = 0; st=0; lin=line_start;
                if(input_character>0) input_character='r';
                break;
            case 'e':
            case 'E':
                if(currentVar!=NULL){
                    show_help_var(FALSE);
                    edit_variable(sendCommandGdb, line_start);
                    h = high;
                    qtd = 0; st=0; lin=line_start;
                }
                break;
            case 'R': 
            case 'r':
            case 'Q':
            case 'q':
            case VKEY_ESCAPE:
                if(input_character>0) input_character='r';
                break;
        }
        //show_info();
        //if(input_character>0) printf("%d\n", input_character);
    }
    free(wcBuffer);
    return TRUE;
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

boolean isCollide(ST_Watch * wnew, int debug_line, int * size){
    boolean result=FALSE;
    ST_Watch * wt = Watching;
    if(wnew->posy<=(debug_line+2) && wnew->posy1>=(debug_line+2)){
        *size = wnew->size;
        return TRUE;
    }
    while(wt!=NULL){
        if(wnew!=wt){
            if(((wnew->posx>=wt->posx-1 && wnew->posx<=wt->posx1+1)||
                (wnew->posx1>=wt->posx-1 && wnew->posx1<=wt->posx1+1)) &&
                ((wnew->posy>=wt->posy && wnew->posy<=wt->posy1)||
                (wnew->posy1>=wt->posy && wnew->posy1<=wt->posy1))){
                    *size = wt->size;
                    result=TRUE;
                    break;
            }
        }
        wt = wt->next;        
    }
    return result;
}

void findNewVarPos(ST_Watch * wnew, int debug_line){
    int phase=0;
    if(wnew->posy<0){
        wnew->posy =VIEW_LINES-4;
        wnew->posy1=VIEW_LINES-2;
        while(wnew->posy<=debug_line && wnew->posy1>=debug_line){
            wnew->posy--;
            wnew->posy1--;
        }
    }
    int size=0;
    while(phase<2){        
        if(phase==0){
            while(wnew->posy>1){
                if(!isCollide(wnew, debug_line, &size)){
                    return;
                }
                wnew->posy  -= 3;
                wnew->posy1 -= 3;
            }
            phase++;
            wnew->posy =VIEW_LINES-4;
            wnew->posy1=VIEW_LINES-2;
        }else{
            while(wnew->posy>1){
                if(!isCollide(wnew, debug_line, &size)){
                    return;
                }
                int aux = wnew->posx;
                int aux1= wnew->posx1;
                wnew->posx  -= size - 2;
                wnew->posx1 -= size - 2;
                while(wnew->posx>=10){
                    if(!isCollide(wnew, debug_line, &size)){
                       return;
                    }
                    wnew->posx  -= size - 2;
                    wnew->posx1 -= size - 2;
                }
                wnew->posx  = aux;
                wnew->posx1 = aux1;
                wnew->posy  -= 3;
                wnew->posy1 -= 3;
            }
            phase++;
            wnew->posy =VIEW_LINES-4;
            wnew->posy1=VIEW_LINES-2;
            break;
        }
    }
}

ST_Watch * new_watching(ST_Line * debug, ST_DebuggerVariable * var){
    ST_Watch * wnew=malloc(sizeof(ST_Watch));
    wnew->var= var;
    wnew->line= debug;
    wnew->start_time= -1;
    wnew->status=0;
    wnew->next=NULL;
    wnew->posy=-1;
    wnew->posx=-1;
    wnew->size=-1;
    wnew->new=TRUE;
    return wnew;
}

void var_watching(Lines * exe_line, int (*sendCommandGdb)(char *), int waitAnser , int debug_line){
    expand = FALSE;
    int bkg= color_light_gray; //color_dark_red;
    int fnew = color_blue;
    int fgc= color_dark_blue;
    int pos_y = VIEW_LINES - 4;

    ST_Line * debug = getLineC(cob.file_cobol, exe_line->file_line);
    struct st_show_var* show_var = debug->show_var;
    if(show_var!=NULL && strstr(show_var->var->cName,"b_")!=NULL){
        if(show_var->var->first_children!=NULL && strcmp(show_var->var->cobolName,show_var->var->first_children->cobolName)==0){
            show_var->var = show_var->var->first_children;
        }
    }
    ST_Watch * wt = Watching;
    ST_Watch * wt_before = NULL;
    boolean insert=TRUE;
    while(wt!=NULL){
        // remove or updates variable from the previously checked line
        if(wt->line==debug){
            insert = FALSE;
        }else{
            if(wt->start_time<0 && !waitAnser){
                wt->start_time= time(NULL);
                wt->status=2;
                wt->new = FALSE;
            }else if(wt->status==2 && wt->start_time>0){
                time_t end_time = time(NULL);
                double elapsed_time = difftime(end_time, wt->start_time);
                if(elapsed_time>0){
                    if(wt_before!=NULL){
                        wt_before->next=wt->next;
                    }else{
                        Watching=wt->next;
                    }
                    ST_Watch * to_free = wt;
                    wt=wt->next;
                    free(to_free);
                    continue;
                }
            }
        }
        wt_before=wt;
        pos_y = wt->posy-3;
        wt = wt->next;
    }
    // The line has not been processed yet; we need to check the variables
    if(insert){
        while(show_var!=NULL){
            wt = Watching;
            while(wt!=NULL){
                if(strcmp(wt->var->cobolName,show_var->var->cobolName)==0) break;
                wt=wt->next;
            }
            if(wt!=NULL){
                wt->line= debug;
                wt->start_time= -1;
                wt->status=0;
            }else{
                if(strstr( show_var->var->cobolName, "Implicit")==NULL){
                    ST_Watch * wnew = new_watching(debug, show_var->var);                        
                    wnew->posy = pos_y;
                    wnew->posy1 = pos_y+2;
                    if(Watching==NULL){
                        Watching = wnew;
                    }else{
                        wt = Watching;
                        while(wt->next!=NULL) wt=wt->next;
                        wt->next=wnew;
                    }
                }
            }
            show_var=show_var->next;
        }
    }
    wt = Watching;
    wchar_t * wcharString = (wchar_t*)malloc((500 + 1) * sizeof(wchar_t));
    int wideCharSize = 500;
    while(wt!=NULL){
        if(wt->status==0 || wt->status==2)
            if(!waitAnser){
                MI2evalVariable(sendCommandGdb,wt->var,0,0);
                wt->var->value=EspecialTrim(wt->var->value);
            }
        int len = strlen(wt->var->cobolName)+2;
        if(wt->var->parent!=NULL && strcmp(wt->var->parent->cobolName, wt->var->cobolName)!=0) len += strlen(wt->var->parent->cobolName)+1;
        if(wt->var->value!=NULL){
            #if defined(_WIN32)
            int new_size = MultiByteToWideChar(CP_UTF8, 0, wt->var->value, -1, NULL, 0);
            if(new_size>wideCharSize){
                wideCharSize = new_size;
                free(wcharString);
                wcharString = (wchar_t*)malloc((wideCharSize + 1) * sizeof(wchar_t));
            }
            MultiByteToWideChar(CP_UTF8, 0, wt->var->value, -1, wcharString,(strlen(wt->var->value) + 1) * sizeof(wchar_t) / sizeof(wcharString[0]));
            #else
            size_t new_size = mbstowcs(NULL, wt->var->value, 0);
            if(new_size>(size_t)wideCharSize){
                wideCharSize = new_size;
                free(wcharString);
                wcharString = (wchar_t*)malloc((wideCharSize + 1) * sizeof(wchar_t));
            }
            mbstowcs(wcharString, wt->var->value, strlen(wt->var->value) + 1);
            #endif
            int lenVar = wcslen(wcharString);
            if(lenVar>len) len=lenVar;
            if(len>60) len=60;
            if(len>wt->size){
                wt->posx= VIEW_COLS - len - 4;
                wt->posx1=VIEW_COLS - 4;
                wt->size=len;
            }
            findNewVarPos(wt, debug_line);
            wt->size= len;
        }else{
            if(len>wt->size){
                wt->posx= VIEW_COLS - len - 4;
                wt->posx1=VIEW_COLS - 4;
                wt->size=len;
            }
            wcscpy(wcharString, L"");
            findNewVarPos(wt, debug_line);
        }
        int fcollor = (wt->new)?fnew:fgc;
        print_colorBK(fcollor, bkg);
        int posy=wt->posy;
        if(wt->var->parent!=NULL && strcmp(wt->var->parent->cobolName, wt->var->cobolName)!=0){
            char name[500];
            sprintf(name,"%s%c%s",wt->var->parent->cobolName, '.', wt->var->cobolName);
            draw_box_first(wt->posx,posy++,wt->size,name);
        }else{
            draw_box_first(wt->posx,posy++,wt->size,wt->var->cobolName);
        }
        draw_box_border(wt->posx, posy);
        int to_move=wcslen(wcharString);
        if(to_move>wt->size) to_move=wt->size;
        #if defined(_WIN32)
        wcsncpy(wcBuffer, wcharString, to_move);
        wcBuffer[to_move]='\0';
        printf("%*ls",wt->size,wcBuffer);
        #else
        swprintf(wcBuffer, sizeof(wcBuffer) / sizeof(wcBuffer[0]), L"%*lc", wt->size, L' ');
        wcsncpy(wcBuffer, wcharString, to_move);
        wcBuffer[wt->size]='\0';
        printf("%*ls",wt->size,wcBuffer);
        #endif
        draw_box_border(wt->posx+wt->size+1, posy++);
        draw_box_last(wt->posx, posy, wt->size);
        wt=wt->next;
    }
    if(wcharString!=NULL) free(wcharString);
}

#define MAX_FILES 100
void show_sources(int (*sendCommandGdb)(char *), int mustParse){
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
    if(mustParse){
        char baseName[256];
        char nameCFile[1024];
        char fileCGroup[10][512];
        int nfile=0;
        for(int a=0;a<qtd_files;a++){
            fileNameWithoutExtension(files[a], &baseName[0]);
            normalizePath(baseName);
            strcpy(baseName,getFileNameFromPath(baseName));
            // C File
            snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
            if(!file_exists(nameCFile)){
                showCobMessage("File C not found.",1);
                return;
            }
            strcpy(fileCGroup[nfile],nameCFile);
            nfile++;
        }
        strcpy(fileCGroup[nfile], "");
        printf("Parser starting...\n");
		SourceMap(fileCGroup);
        printf("Parser end...\n");
        clearScreen();
        show_opt();
        show_info();
    }
    while(input_character!=-100){
        disableEcho();
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
                        printf("...%-57s",&files[f][start]);
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
        enableEcho();
        input_character =  key_press(MOUSE_NORMAL);
        switch (input_character)
        {
            case VKEY_UP:
                if(line_pos>0){
                    line_pos--;
                }else{
                    start_file=(start_file>0)?start_file-1:0; 
                }
                show = TRUE;
                break;
            case VKEY_DOWN: 
                if(line_pos<9){
                    line_pos++;
                }else{
                    start_file=(start_file<qtd_files)?start_file+1:qtd_files; 
                }
                show = TRUE;
                break;
            case VKEY_PGUP:
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
            case VKEY_PGDOWN: 
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
            case VKEY_ENTER:       
                if(file_sel<0) break;
                freeFile();
                strcpy(cob.file_cobol, files[file_sel]);
                strcpy(cob.first_file, cob.file_cobol);
                if(!file_exists(cob.file_cobol)){
                    clearScreen();
                    showCobMessage("File COB not found.",2);
                    fflush(stdout);
                    printf("File COB not found.");
                    return;
                }            
                loadfile(files[file_sel]);
                highlightParse();
                input_character=-100;
                break;
            case VKEY_ESCAPE:
                input_character=-100;
                break;
            default: 
                break;
        }
        //gotoxy(1,23); if(input_character>0) printf("%d\n", input_character);
    }
}

void load_file(){
    char input_character=-1;
    int bkgr = color_dark_red;
    int bkg;
    int frg = color_white;
    int csel = color_light_gray;
    int line_pos=0;
    char files[MAX_FILES][512];
    char cwd[512];
    int qtd_files=0;
    int start_file=0;
    int file_sel = -1;
    boolean show = TRUE;
    boolean must_out = FALSE;
    struct dirent *dir;
    char baseName[256];
    char nameCFile[1024];
    char str[512];
    char str1[512];
    int i=0;
    DIR *d;
    
    str[0] = '\0';
    str1[0] = '\0';
    getPathName(cwd, cob.first_file);
    while(!must_out){
        d = opendir(cwd);
        if (d) {
            qtd_files = 0;
            while ((dir = readdir(d)) != NULL) {
                if(strlen(str)>0 && istrstr(dir->d_name, str)==NULL) continue;
                if(istrstr(dir->d_name,".cbl")!=NULL || istrstr(dir->d_name,".cob")!=NULL){
                    sprintf(files[qtd_files],"%s/%s",cwd,dir->d_name);
                    normalizePath(files[qtd_files]);

                    fileNameWithoutExtension(files[qtd_files], &baseName[0]);
                    strcpy(baseName,getFileNameFromPath(baseName));
                    // C File
                    snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
                    if(!file_exists(nameCFile)){
                        continue;
                    }
                    qtd_files++;
                }
            }
            closedir(d);
        }
        show_info();
        while(input_character!=-100){
            disableEcho();
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
                            printf("...%-57s",&files[f][start]);
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
            strcpy(str1,str);
            gotoxy(1,VIEW_LINES); 
            print_colorBK(color_gray, color_black);
            if(strlen(str)>0)
                printf("Search: %-60s\r", str);
            else
                printf("<ESC> - Exit   <ENTER> - Select file\r");
            print_color_reset();
            fflush(stdout);
            gotoxy(1,1);
            enableEcho();
            input_character =  key_press(MOUSE_NORMAL);
            switch (input_character)
            {
                case VKEY_UP:
                    if(line_pos>0){
                        line_pos--;
                    }else{
                        start_file=(start_file>0)?start_file-1:0; 
                    }
                    show = TRUE;
                    break;
                case VKEY_DOWN: 
                    if(line_pos<9){
                        line_pos++;
                    }else{
                        start_file=(start_file<qtd_files)?start_file+1:qtd_files; 
                    }
                    show = TRUE;
                    break;
                case VKEY_PGUP:
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
                case VKEY_PGDOWN: 
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
                case VKEY_ENTER:       
                    if(file_sel<0) break;
                    freeFile();
                    strcpy(cob.file_cobol, files[file_sel]);
                    fileNameWithoutExtension(files[file_sel], &baseName[0]);
                    normalizePath(baseName);
                    strcpy(baseName,getFileNameFromPath(baseName));
                    // C File
                    snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
                    if(file_exists(nameCFile)){
                        cob.status_bar = 1;
                        parser(nameCFile, 0);
                    }
                    cob.status_bar = 0;         
                    loadfile(files[file_sel]);
                    highlightParse();
                    input_character=-100;
                    must_out = TRUE;
                    break;
                case VKEY_ESCAPE:
                    must_out = TRUE;
                    input_character=-100;
                    break;
                default: 
                    if ((input_character == VKEY_BACKSPACE || input_character==37) && i > 0) {
                        i--;
                        str[i] = '\0';
                    }else if(input_character==39 && strlen(str) < (size_t) size){
                        str[i++]=' ';
                        str[i]='\0';
                    } else if (strlen(str) < (size_t) size && input_character >= 32 && input_character!=127) {
                        str[i] = input_character;
                        i++;
                        str[i] = '\0';
                    }
                    break;
            }
            if(strcmp(str,str1)!=0) break;
        }
    }
}

void show_help_popup(char *text[], int ctext[], int qtt_lines){
    char input_character=-1;
    int bkgr = color_dark_red;
    int bkg;
    int frg = color_white;
    int csel = color_light_gray;
    int line_pos=0;
    int start_file=0;
    int lmax=VIEW_LINES/2+3;
    //int file_sel = -1;
    boolean show = TRUE;

    disableEcho();
    while(input_character!=-100){
        gotoxy(1,1);
        int lin = 4;
        int col = (VIEW_COLS-70)/2;
        int size = 70;
        if(show){
            gotoxy(col,lin);
            print_colorBK(frg, bkgr);
            draw_box_first(col,lin++,size,"CobGDB HELP");
            int f = start_file;
            int pos=0;
            while(pos<lmax){
                print_colorBK(frg, bkgr);
                draw_box_border(col, lin);
                if(f<qtt_lines){
                    bkg=(line_pos==pos)?csel:bkgr;
                    print_colorBK(ctext[f], bkg);
                    printf("%-70s",text[f]);
                }else{
                    print_colorBK(color_white, bkgr);
                    printf("%-70s"," ");
                }
                print_colorBK(frg, bkgr);
                draw_box_border(col+ size+1, lin);
                f++; lin++; pos++;
            }
            print_colorBK(frg, bkgr);
            draw_box_last(col, lin, size);
            show=FALSE;
        }
        print_color_reset();
        fflush(stdout);
        gotoxy(1,1);
        input_character =  key_press(MOUSE_NORMAL);
        switch (input_character)
        {
            case VKEY_UP:
                if(line_pos>0){
                    line_pos--;
                }else{
                    start_file=(start_file>0)?start_file-1:0; 
                }
                show = TRUE;
                break;
            case VKEY_DOWN: 
                if(line_pos<(lmax-1)){
                    line_pos++;
                }else{
                    start_file=(start_file<qtt_lines)?start_file+1:qtt_lines; 
                }
                show = TRUE;
                break;
            case VKEY_PGUP:
                if(line_pos>0){
                    line_pos= 0;
                }else{
                    line_pos-= (lmax-1);
                    line_pos=(line_pos<0)?0:line_pos;
                    start_file-=lmax;
                    start_file = (start_file>0)?start_file:0;
                }
                show = TRUE;
                break;
            case VKEY_PGDOWN: 
                if(line_pos<(lmax-1)){
                    line_pos=(lmax-1);
                }else{
                    line_pos+=(lmax-1);
                    if(line_pos>(lmax-1)) line_pos=(lmax-1);
                    start_file+=lmax;
                    start_file = (start_file>qtt_lines)?qtt_lines:start_file;
                }
                show = TRUE;
                break;
            case 'R':
            case 'r':
            case 'Q':
            case 'q':
            case VKEY_ENTER:
            case VKEY_ESCAPE:
                input_character=-100;
                break;
            default: 
                break;
        }
        //gotoxy(1,23); if(input_character>0) printf("%d\n", input_character);
    }
    enableEcho();
}

void show_help(int is_popup){
    char *text[] = {
        "                     CobGDB - Commands",
        " ",
        "B - Breakpoint: toggles the breakpoint at the current selected line",
        "    (can also be done with the mouse).",
        "R - Run: runs the program from the first statement until a breakpoint",
        "    is encountered.",
        "C - Cursor or Continue: runs the program until it reaches the",
        "    selected line.",
        "J - Jump: runs the program until it reaches the specified line.",
        "N - Next: runs the program until the next line but does not enter",
        "    a subroutine executed by CALL or PERFORM.",
        "S - Step: runs the program until the next line.",
        "G - GO: continues the program execution until it encounters a ",
        "    stopping point: breakpoint, end of the program, or the ",
        "    return from a subroutine - PERFORM/CALL.",
        "V - Variables: displays the set of variables for the running program.",
        "H - Show: shows the values of variables for the selected line",
        "    (right-click also functions).",
        "D - Display of variables: set the automatic display during ",
        "    debugging/animation ON or OFF (default is OFF).",
        "F - File: allows selecting the source file for debugging.",
        "W - Window Size: switches between window sizes: 80x24 and 132x34.",
        "O - Changes the focus to the window where the debugging code is being",
        "    displayed (Windows or X11 only).",
        "CTRL-F - Find: search for text in the source code.",
        "CTRL-L - Go to: Go to Line.",
        "CTRL-S - Save breakpoints.",
        "CTRL-B - Load saved breakpoints.",
        "Q - Quit: quits the program.",
        "  ",
        "CobGDB takes one or more programs with COB/CBL extension as parameters",
        "and runs the GnuCOBOL compiler with the following format:",
        "cobc -g -fsource-location -ftraceall -v -O0 -x prog.cob prog2.cob ...",
        "  ",
        "Example:",
        "cobgdb prog.cob subprog1.cob subprog2.cob",
        "  ",
        "You can run GDB/GDBSERVER remotely using the 'A' key. COBGDD will",
        "prompt you to provide the server and port in the format",
        "server:port  or the PID of the application.",
        "  ",
        "Example:",
        "localhost:5555",
        "or",
        "9112",
        "  ",
        "Command line:",
        "cobgdb --connect localhost:5555 prog.cob",
        "  ",
        "                 CobGDB Debugging a Pre-compiled File",
        "  ",
        "You can also use CobGDB to debug a previously generated executable ",
        "file. Compile the program with these options:",
        "cobc -g -fsource-location -ftraceall -v -O0 -x prog.cob prog2.cob ...",
        "  ",
        "To start debugging, run cobgdb using the --exe directive as follows:",
        "cobgdb --exe prog",
        "  "
    };

    int ctext[] = {
        color_green,
        color_white, color_white, color_white, color_white, color_white, color_white,
        color_white, color_white, color_white, color_white, color_white, color_white,
        color_white, color_white, color_white, color_white, color_white, color_white, 
        color_white, color_white, color_white, color_white, color_white, color_white,
        color_white, color_white, color_white, color_white, color_white, color_white, 
        color_white,
        color_yellow, color_white, color_white, color_yellow,
        color_white, color_white, color_white, color_white, color_white, color_white, 
        color_yellow, color_white, color_yellow,color_white,color_white, color_yellow,
        color_white, color_green, 
        color_white, color_white, color_white, 
        color_yellow,color_white, color_white, color_yellow,color_white
    };

    int qtt_lines=sizeof(text) / sizeof(text[0]);
    if(is_popup){
        show_help_popup(text, ctext, qtt_lines);
    }else{
        for(int a=0;a<qtt_lines;a++){
            //print_colorBK(ctext[a], color_black);
           printf("%-70s\n",text[a]);
           fflush(stdout);
        }
        //print_color_reset();
    }
}
