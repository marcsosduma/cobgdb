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
#include "cobgdb.h"

extern ST_DebuggerVariable * DebuggerVariable;
extern int ctlVar;
extern int color_frame;
int showOne = FALSE;
int expand = FALSE;
wchar_t wcBuffer[512];

int show_opt_var(){
    char * opt = " COBGDB - (R)return (ENTER)expand/contract";
    char aux[100];
    sprintf(aux,"%-80s\r", opt);
    gotoxy(1,1);
    printBK(aux, color_white, color_frame);
    gotoxy(1,24);
    sprintf(aux,"%80s\r"," ");
    printBK(aux, color_frame, color_frame);
}

int print_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *)){
    int bkg;
    (*notShow)--;
    if(*notShow<0 && lin<22){
        gotoxy(1,lin+2);
        if(level>0 && ctlVar!=var->ctlVar){
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
        bkg=(line_pos==lin)?color_gray:color_black;
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
    if(var->first_children!=NULL && var->show=='-'){
        lin=print_variable(++level, notShow, line_pos, start_lin, 
                   end_lin, lin, start_linex_x,
                   var->first_children,sendCommandGdb);
    }
    if(var->brother!=NULL){
        lin = print_variable(++level, notShow, line_pos, start_lin, 
                   end_lin, lin, start_linex_x,
                   var->brother,sendCommandGdb);
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

int show_variables(int (*sendCommandGdb)(char *)){
    gotoxy(1,1);
    int lin=0;    
    int line_pos=0;
    int start_window_line = 0;
    int qtd_window_line = 23;
    int start_linex_x = 0;      
    expand = FALSE;
    char aux[100];
    int bkg;

    char input_character= ' ';
    ST_DebuggerVariable * var = firstVar(var);
    int old_lin=lin;
    int notShow;
    while(input_character!='R' && input_character!='r'){
        notShow=start_window_line;
        if(var!=NULL){
            gotoxy(1,1);
            show_opt_var();
            gotoxy(1,2);
            showOne = FALSE;
            while(var!=NULL){
                if(var->parent==NULL){
                    lin=print_variable(0, &notShow, line_pos, start_window_line, 
                        start_window_line+20, lin, start_linex_x,
                        var,sendCommandGdb);
                }
                //lin=(lin==old_lin)?lin+1:lin;
                var=var->next;
            }
            while(var==NULL && lin<22){
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
                if(line_pos<=20){
                    line_pos++;
                }else{
                    if(showOne) start_window_line++; 
                }
                var=firstVar(var);
                lin=0;
                break;
            case VK_PGUP:
                if(line_pos>0){
                    line_pos-=22;
                    line_pos=(line_pos<0)?0:line_pos;
                }else{
                    start_window_line-=22;
                    start_window_line=(start_window_line<0)?0:start_window_line;
                }
                var=firstVar(var);
                lin=0;
                break;
            case VK_PGDOWN: 
                if(line_pos<21){
                    line_pos=21;
                }else if(showOne){
                    line_pos=21;
                    start_window_line=start_window_line+22;
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
            case 'r':
            case 'R':
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
}

int hover_variable(int level, int * notShow, int line_pos, int start_lin, 
                   int end_lin, int lin, int start_linex_x,
                   ST_DebuggerVariable * var,int (*sendCommandGdb)(char *), int bkg){
   
    if(lin<22){
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
        print_colorBK(color_yellow, bkg);
        printf("%ls",L" ");  
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
             printf("%*ls",linW+1,L" ");
        }else{
            printf("%ls",L" ");  
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
    int qtd_window_line = 23;
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
    while(input_character!='R' && input_character!='r'){
        while(h!=NULL){
            if(h->type==TP_ALPHA){
                wcsncpy(wcBuffer, &h->token[st], h->size);
                wcBuffer[h->size]='\0';   
                int len = wcstombs(NULL, wcBuffer, 0);
                char *chval = (char *)malloc(len + 1);
                wcstombs(chval, wcBuffer, len + 1);
                ST_DebuggerVariable * var =  findVariableByCobol(functionName, chval);
                if(var!=NULL){
                    int notShow=FALSE;
                    qtd++;
                    if(qtd==1){
                        gotoxy(10,lin+2);
                        print_colorBK(color_white, color_blue);
                        printf("%-61ls",L" Show Line Variables");
                        printf("%ls",L"-");
                        lin++;
                    }
                    lin=hover_variable(0, &notShow, line_pos, start_window_line, 
                        22, lin, start_linex_x,
                        var,sendCommandGdb, bkg);
                }
                if(qtd>0){
                    gotoxy(10,lin+2);
                    print_colorBK(color_light_blue, bkg);
                    printf("%62ls",L" ");
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
                if(line_pos<=20){
                    line_pos++;
                }else{
                    if(showOne) start_window_line++; 
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_PGUP:
                if(line_pos>0){
                    line_pos-=22;
                    line_pos=(line_pos<0)?0:line_pos;
                }else{
                    start_window_line-=22;
                    start_window_line=(start_window_line<0)?0:start_window_line;
                }
                h = high;
                qtd = 0; st=0; lin=line_start;
                break;
            case VK_PGDOWN: 
                if(line_pos<21){
                    line_pos=21;
                }else if(showOne){
                    line_pos=21;
                    start_window_line=start_window_line+22;
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
