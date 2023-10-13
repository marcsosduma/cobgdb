/* 
   This code is based on the GnuCOBOL Debugger extension available at: 
   https://github.com/OlegKunitsyn/gnucobol-debug
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cobgdb.h"
#include "regex_gdb.h"
//#define DEBUG 0

extern ST_Line * LineDebug;
extern char * gdbOutput;
extern char m[10][512];
extern int debug_line;
extern int running;
extern int showFile;
extern int waitAnswer;
extern int changeLine;
char lastComand[200];
int subroutine=-1;

char nonOutput[] = "(([0-9]*|undefined)[\\*\\+\\-\\=\\~\\@\\&\\^])([^\\*\\+\\-\\=\\~\\@\\&\\^]{1,})";
char gdbRegex[]  = "([0-9]*|undefined)\\(gdb\\)";
char numRegex[]  = "[0-9]+";
char gcovRegex[] = "\"([0-9a-z_\\-\\/\\s\\:\\\\]+\\.o)\"";

void MI2log(char * log){
    #ifdef DEBUG
    printf("%s\n", log);
    #endif
}

int couldBeOutput(char * line) {
    int qtd=regex(nonOutput, line, m);
    if(qtd==0) return 1;
    return 0;
}

ST_Line * hasLineCobol(ST_MIInfo * parsed){
    boolean find=FALSE;
    ST_Line * hasLine = NULL;
    if(parsed->outOfBandRecord->output->next!=NULL){
        ST_TableValues * search1=parseMIvalueOf(parsed->outOfBandRecord->output->next, "frame.fullname", NULL, &find);
        if(search1==NULL) return NULL;
        find=FALSE;
        ST_TableValues * search2=parseMIvalueOf(parsed->outOfBandRecord->output->next, "frame.line", NULL, &find);
        if(search2==NULL || search2->value==NULL) return NULL;
        int lineC = atoi(search2->value);
        hasLine =  getLineCobol( search1->value, lineC);
        if(hasLine!=NULL) 
            subroutine = hasLine->endPerformLine;

    }
    return hasLine;
}

ST_MIInfo * MI2onOuput(int (*sendCommandGdb)(char *), int tk){
   if(gdbOutput==NULL) return NULL;
   char * line = strtok(gdbOutput, "\n");
   int lineChange=FALSE;
   ST_MIInfo * parsedRet = NULL;
   // loop through the string to extract all lines
   while( line != NULL ) {
      #ifdef DEBUG
      printf( " %s\n", line ); //printing each token
      #endif
      if(couldBeOutput(line)){
            int qtd = regex(gdbRegex, line, m);
            if (qtd==0) {
                MI2log(line);
            }
      } else {
            if(line!=NULL){
                ST_MIInfo * parsed = parseMI(line);
                if (parsed->resultRecords!=NULL){
                    if(strcmp(parsed->resultRecords->resultClass,"error")==0 || strcmp(parsed->resultRecords->resultClass,"done")==0 ) {
                        waitAnswer=FALSE;
                        running=FALSE;
                        showFile=TRUE;
                        debug_line=-1;
                        if(parsed->token>0 && tk>0 && parsed->token==tk){
                            parsedRet=parsed;
                        }
                    }
                }
                if(parsed!=NULL && parsed->outOfBandRecord!=NULL){
                    if(parsed->outOfBandRecord->type!=NULL && strcmp(parsed->outOfBandRecord->type,"exec")==0){
                        if(parsed->outOfBandRecord->asyncClass!=NULL && strcmp(parsed->outOfBandRecord->asyncClass,"running")==0){
                            running=TRUE;
                            //showFile=TRUE;
                        }else if(parsed->outOfBandRecord->asyncClass!=NULL && strcmp(parsed->outOfBandRecord->asyncClass,"stopped")==0){
                            ST_TableValues * reason = parsed->outOfBandRecord->output;
                            if(reason!=NULL && reason->key!=NULL && strcmp(reason->key,"reason")==0){
                                if(strcmp(reason->value,"breakpoint-hit")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        sendCommandGdb("exec-next\n");
                                        lineChange=TRUE;
                                    }else{
                                        debug_line = hasLine->lineCobol;
                                        changeLine = TRUE;
                                        waitAnswer=FALSE;
                                        running=FALSE;
                                        showFile=TRUE;
                                    }
                                }else if(strcmp(reason->value,"end-stepping-range")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        sendCommandGdb("exec-next\n");
                                    }else{
                                        waitAnswer=FALSE;
                                        debug_line = hasLine->lineCobol;
                                        changeLine = TRUE;
                                        running=FALSE;
                                        showFile=TRUE;
                                    }
                                }else if(strcmp(reason->value,"location-reached")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        sendCommandGdb(lastComand);
                                    }else{
                                        debug_line = hasLine->lineCobol;
                                        changeLine = TRUE;
                                        running = FALSE;                                        
                                        showFile=TRUE;
                                        waitAnswer=FALSE;
                                    }
                                }else if(strcmp(reason->value,"function-finished")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        sendCommandGdb(lastComand);
                                    }else{
                                        debug_line = hasLine->lineCobol;
                                        changeLine = TRUE;
                                        running = FALSE;                                        
                                        showFile=TRUE;
                                        waitAnswer=FALSE;
                                    }
                                }else if(strcmp(reason->value,"signal-received")==0){
                                        showFile=TRUE;
                                        waitAnswer=FALSE;
                                        running=FALSE;
                                }else if(strcmp(reason->value,"exited-normally")==0){
                                        showFile=TRUE;
                                        waitAnswer=FALSE;
                                        running=FALSE;
                                        debug_line=-1;

                                }else if(strcmp(reason->value,"exited")==0){
                                        showFile=TRUE;
                                        waitAnswer=FALSE;
                                        running=FALSE;
                                        debug_line=-1;
                                }
                            }
                        }
                    }
                }
                if(parsed!=parsedRet) freeParsed(parsed);
            }
      }
      line = strtok(NULL, "\n");
   }
   strcpy(gdbOutput,"");
   if(lineChange){
        sendCommandGdb("");
        MI2onOuput(sendCommandGdb, -1);
   }
   return parsedRet;
}

int MI2stepOver(int (*sendCommandGdb)(char *)){
    strcpy(lastComand,"exec-next\n"); 
    char command[200];
    if(subroutine>0){
        sprintf(command,"exec-until %d\n", subroutine); 
        sendCommandGdb(command);
    }else{
        sendCommandGdb(lastComand);
    }
    waitAnswer = TRUE;
    showFile = TRUE;
    running = TRUE;
}

int MI2stepInto(int (*sendCommandGdb)(char *)){
    strcpy(lastComand,"exec-next\n"); 
    char command[200];
    if(subroutine>0){
        sprintf(command,"%s%d\n","break-insert -f ",subroutine);
        sendCommandGdb(command);
        MI2verifyGdb(sendCommandGdb);
    }
    strcpy(command,"exec-step\n"); 
    sendCommandGdb(command);
    waitAnswer = TRUE;
    showFile = TRUE;
    running=TRUE;
    return 0;
}

int MI2stepOut(int (*sendCommandGdb)(char *)){
    strcpy(lastComand,"exec-finish\n"); 
    char command[200];
    strcpy(command,"exec-finish\n"); 
    sendCommandGdb(command);
    waitAnswer = TRUE;
    showFile = TRUE;
    running=TRUE;
    return 0;
}

int MI2start(int (*sendCommandGdb)(char *)){
    strcpy(lastComand,"exec-next\n"); 
    char command[]="exec-run\n";
    sendCommandGdb(command);
    waitAnswer = TRUE;
    running=TRUE;
    showFile = TRUE;
    return 0;
}

int MI2verifyGdb(int (*sendCommandGdb)(char *)){
    sendCommandGdb("");
    MI2onOuput(sendCommandGdb, -1);
    return 0;
}

void wait_gdb_answer(int (*sendCommandGdb)(char *)){
    if(gdbOutput==NULL) gdbOutput=malloc(strlen((""))+1);
    strcpy(gdbOutput,"");
    do{
        sendCommandGdb("");
    }while(strlen(gdbOutput)==0 && strcmp(gdbOutput,"\n")!=0);
}

int MI2addBreakPoint(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber ){
    char command[256];
    ST_Line * line = getLineC(fileCobol, lineNumber);
    if(line!=NULL){
        sprintf(command,"%s%s:%d\n","break-insert -f ",line->fileC,line->lineC);
        sendCommandGdb(command);
        waitAnswer = TRUE;
    }
}

int MI2goToCursor(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber ){
    char command[256];
    ST_Line * line = getLineC(fileCobol, lineNumber);
    if(line!=NULL){
        sprintf(command,"%s%s:%d\n","break-insert -t ",line->fileC,line->lineC);
        sendCommandGdb(command);
        wait_gdb_answer(sendCommandGdb);
        if(debug_line>0)
            strcpy(command,"exec-finish\n"); 
        else
            strcpy(command,"exec-run\n");
        sendCommandGdb(command);
        wait_gdb_answer(sendCommandGdb);
        waitAnswer = TRUE;
        showFile = TRUE;
        running=TRUE;
    }
}


int MI2removeBreakPoint (int (*sendCommandGdb)(char *), Lines * lines, char * fileCobol, int lineNumber ){
    ST_Line * line = getLineC(fileCobol, lineNumber);
    if(line!=NULL){
        char command[256];
        strcpy(command,"break-delete\n");
        sendCommandGdb(command);
        waitAnswer = TRUE;
        Lines * lb = lines;
        while(lb!=NULL){
            if(lb->breakpoint=='S'){
                MI2addBreakPoint(sendCommandGdb, fileCobol, lb->file_line );
            }
            lb = lb->line_after;
        }
    }
    return 0;
}

int MI2evalVariable(int (*sendCommandGdb)(char *), ST_DebuggerVariable * var, int thread, int frame){
    int hasCobGetFieldStringFunction = FALSE;

    char command[512];
    char st[100];
    strcpy(command,"data-evaluate-expression ");
        if (thread != 0) {
            sprintf(st,"--thread %d --frame %d ",thread, frame);
            strcat(command,st);
        }
        if (hasCobGetFieldStringFunction && strncmp(var->cName,"f_",2)==0) {
            sprintf(st,"\"(char *)cob_get_field_str_buffered(&%s})\"", var->cName);
            strcat(command,st);
        } else if (strncmp(var->cName,"f_",2)==0) {
            sprintf(st,"%s.data", var->cName);
            strcat(command,st);
        } else {
             strcat(command,var->cName);
        }
        strcat(command,"\n");
        int tk=sendCommandGdb(command);
        wait_gdb_answer(sendCommandGdb);
        ST_MIInfo * parsed=MI2onOuput(sendCommandGdb, tk);
        if(parsed!=NULL){
            int find=FALSE;
            ST_TableValues * search=parseMIvalueOf(parsed->resultRecords->results, "value", NULL, &find);
            if(search!=NULL && search->value!=NULL){
               var->value=debugParse(search->value, var->size, var->attribute->scale, var->attribute->type);
            }
            freeParsed(parsed);
        } 
}

char * MI2getCurrentFunctionName(int (*sendCommandGdb)(char *)){
    char * ret = NULL;
    int tk = sendCommandGdb("stack-info-frame\n");
    wait_gdb_answer(sendCommandGdb);
    ST_MIInfo * parsed=MI2onOuput(sendCommandGdb, tk);
    if(parsed!=NULL){    
        int find=FALSE;
        ST_TableValues * search=parseMIvalueOf(parsed->resultRecords->results, "frame.func", NULL, &find);
        if(search!=NULL && search->value!=NULL)
            ret = toLower(strdup(search->value));
        freeParsed(parsed);
    }
    return ret;
}

void tableVariableElements(ST_TableValues * start, char * functionName){
        ST_TableValues * sch=start;
        if(sch->next_array!=NULL){
           tableVariableElements(sch->next_array, functionName);
        }
        if(sch->array!=NULL){
           tableVariableElements(sch->array, functionName);
        }
        if(sch->next!=NULL){
           tableVariableElements(sch->next, functionName);
        }
        if(sch->key!=NULL && strcmp(sch->key,"name")==0){
            if(strncmp(sch->value,"b_",2)==0){
                char varNameC[256];
                sprintf(varNameC,"%s%c%s",functionName,'.',sch->value);
                ST_DebuggerVariable * cobolVariable =  getVariableByC(varNameC);
                if(cobolVariable!=NULL && sch->next!=NULL){
                    ST_TableValues * value = sch->next;
                    if(value->value!=NULL){
                        if(cobolVariable->value!=NULL) free(cobolVariable->value);
                        cobolVariable->value=strdup(value->value);
                    }
                }
            }
        }
}

void MI2getStackVariables(int (*sendCommandGdb)(char *), int thread,int  frame){
        char * functionName = MI2getCurrentFunctionName(sendCommandGdb);

        char command[256];
        sprintf(command,"stack-list-variables --thread %d --frame %d --all-values\n", thread, frame);
        int tk=sendCommandGdb(command);
        wait_gdb_answer(sendCommandGdb);
        ST_MIInfo * parsed=MI2onOuput(sendCommandGdb, tk);
        if(parsed!=NULL){
            int find=FALSE;
            ST_TableValues * search=parseMIvalueOf(parsed->resultRecords->results, "variables", NULL, &find);
            if(search!=NULL){
               tableVariableElements(search, functionName);
            }
            freeParsed(parsed);
        }
        if(functionName!=NULL) free(functionName);
}

int MI2variablesRequest(int (*sendCommandGdb)(char *)){
    MI2getStackVariables(sendCommandGdb,1,0);
    return 0;
}

int MI2hoverVariable(int (*sendCommandGdb)(char *), Lines * line ){
    char * functionName = MI2getCurrentFunctionName(sendCommandGdb);
    show_line_var(line->high, functionName, sendCommandGdb);
    if(functionName!=NULL) free(functionName);
}

char* cleanRawValue(const char* rawValue) {
    const char* containsQuotes = "\"";
    char* cleanedRawValue = strdup(rawValue);

    if (cleanedRawValue && cleanedRawValue[0] == '\"') {
        memmove(cleanedRawValue, cleanedRawValue + 1, strlen(cleanedRawValue));
    }
    size_t length = strlen(cleanedRawValue);
    if (length > 0 && cleanedRawValue[length - 1] == '\"') {
        cleanedRawValue[length - 1] = '\0';
    }
    char* currentPos = cleanedRawValue;
    char* result = NULL;
    while (1) {
        currentPos = strstr(currentPos, containsQuotes);
        if (!currentPos) {
            break;
        }
        memmove(currentPos + 1, currentPos, strlen(currentPos) + 1);
        *currentPos = '\\';
        currentPos += 2;
    }
    return cleanedRawValue;
}


int MI2changeVariable(int (*sendCommandGdb)(char *), ST_DebuggerVariable * var, char * rawValue){
    int hasCobGetFieldStringFunction = FALSE;
    char aux[256];
    char command[512];
    char * cleanedRawValue = cleanRawValue(rawValue);
    if (var->attribute!=NULL && strcmp(var->attribute->type,"integer")==0){
        sprintf(command,"gdb-set var %s=%s\n", var->variablesByC, cleanedRawValue);
        int tk=sendCommandGdb(command);
        wait_gdb_answer(sendCommandGdb);
    }else if (hasCobGetFieldStringFunction && strncmp(var->cName,"f_",2)==0) {
        sprintf(command,"gdb-set var %s=%s\n", var->variablesByC, cleanedRawValue);
        int tk=sendCommandGdb(command);
        wait_gdb_answer(sendCommandGdb);
    } else{
        strcpy(aux, var->cName);
        if(strncmp(var->cName,"f_",2)==0) strcat(aux, ".data");
        char * finalValue = NULL;
        if(var->attribute!=NULL){
            finalValue=formatValueVar(rawValue, var->size, var->attribute->scale, var->attribute->type);
            sprintf(command,"data-evaluate-expression \"(void)memcpy(%s,\\\"%s\\\",%d)\"\n", aux, finalValue, var->size);
            sendCommandGdb(command);
            wait_gdb_answer(sendCommandGdb);
        }
    }
}