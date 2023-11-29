/* 
   This code is based on the GnuCOBOL Debugger extension available at: 
   https://github.com/OlegKunitsyn/gnucobol-debug
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__linux__)
#include <unistd.h>
#endif
#include "cobgdb.h"
//#define DEBUG 0

extern struct st_cobgdb cob;
extern ST_Line * LineDebug;
extern char * gdbOutput;
extern char m[10][512];
extern ST_bk * BPList;
char lastComand[200];
int subroutine=-1;

enum GDB_STATUS {
    GDB_RUNNING,
    GDB_BREAKPOINT,
    GDB_STEP_END,
    GDB_LOCATION_REACHED,
    GDB_END_STEPPING_RANGE,
    GDB_FUNCTION_FINISHED,
    GDB_STEP_OUT_END,
    GDB_SIGNAL_STOP,
    GDB_STOPPED,
    GDB_CONNECTED
};

void MI2log(char * log){
    #ifdef DEBUG
    printf("%s\n", log);
    #endif
}

//char nonOutput[] = "(([0-9]*|undefined)[\\*\\+\\-\\=\\~\\@\\&\\^])([^\\*\\+\\-\\=\\~\\@\\&\\^]{1,})";
int fNonOutput(char * line, char mm[][MAX_MATCH_LENGTH]){
   if(line==NULL) return 0;
    int len=strlen(line);
    if(len<1) return 0;
    char ch = line[0];
    int qtt=0, ret = 0, pos=0, pos1=0, pos2=0, pos3=0, find=0;
    if(qtt<len){
        ch=line[qtt];
        if((ch>='0' && ch<='9')){
            while((ch>='0' && ch<='9') && qtt<len){
                mm[0][pos++]=ch;
                mm[1][pos1++]=ch;
                qtt++;
                ch=line[qtt];
            }
            mm[0][pos]='\0';
            mm[1][pos1]='\0';
            find++;
        }else if(ch=='u' && len>=9 && strncmp(line, "undefined", 9)==0){
            strcpy(mm[0],"undefined");
            strcpy(mm[1],"undefined");
            qtt+=9;
            find++;
        }
        if(qtt<len){
            ch=line[qtt];
            if(ch=='*' || ch=='+' || ch=='-' || ch=='=' || ch=='~' || ch=='@' || ch=='&' || ch=='^'){
                mm[0][pos++]=ch;
                mm[2][pos2++]=ch;
                mm[0][pos]='\0';
                mm[2][pos2]='\0';
                find++;
            }else{
                return 0;
            }
        }
        if(qtt<len){
            qtt++;
            ch=line[qtt];
            int fd = 0;
            while(ch != '*' && ch != '+' && ch != '-' && ch != '=' &&
                    ch != '~' && ch != '@' && ch != '&' && ch != '^' && ch != ',' 
                    && qtt<len) {
                mm[0][pos++]=ch;
                mm[3][pos3++]=ch;
                qtt++;
                ch=line[qtt];
                fd++;
            }
            if(fd) ret=find+1;
            mm[0][pos]='\0';
            mm[3][pos3]='\0';
        }
    }
    return ret;
}

int couldBeOutput(char * line) {
    int qtd = fNonOutput(line, m);
    if(qtd==0) 
        return 1;
    return 0;
}

void loadCobSourceFile(char * atualFile, char * newFile){
    if(strcasecmp(atualFile, newFile)!=0){
        freeFile();
        strcpy(cob.file_cobol, newFile);
        loadfile(cob.file_cobol);
        highlightParse();
    }
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
        if(hasLine!=NULL){
            subroutine = hasLine->endPerformLine;
            loadCobSourceFile(cob.file_cobol, hasLine->fileCobol);
        } 

    }
    return hasLine;
}

//char gdbRegex[]  = "([0-9]*|undefined)\\(gdb\\)";
int fGdbRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
   if(line==NULL) return 0;
   int len=strlen(line);
   char ch;
   ch = line[0];
   int pos=0;
   int ret=0;
   strcpy(mm[0],"");
   strcpy(mm[1],"");
   if((ch>='0' && ch<='9')){
        while(ch>='0' && ch<='9' && pos<len){
            mm[0][pos]=ch;
            mm[1][pos]=ch;
            ch=line[++pos];
        }
        mm[0][pos]='\0';
        mm[1][pos]='\0';
   }else if(len>=9 && strncmp(line, "undefined", 9)==0){
        strcpy(mm[0],"undefined");
        strcpy(mm[1],"undefined");
        pos+=9;
   }
   len = strlen(&line[pos]);
   if(len>=5 & strncmp(&line[pos],"(gdb)",5)){
    strcat(m[0],"(gdb)");
    strcat(m[1],"(gdb)");
    ret=1;
   }
   return ret;
}

ST_MIInfo * MI2onOuput(int (*sendCommandGdb)(char *), int tk, int * status){
   *status=GDB_RUNNING;
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
            //int qtd = regex(gdbRegex, line, m);
            int qtd=fGdbRegex(line, m);
            if (qtd==0) {
                MI2log(line);
            }
      } else {
            if(line!=NULL){
                ST_MIInfo * parsed = parseMI(line);
                if (parsed->resultRecords!=NULL){
                    if(strcmp(parsed->resultRecords->resultClass,"error")==0 || strcmp(parsed->resultRecords->resultClass,"done")==0 ) {
                        *status=GDB_STEP_END;
                        cob.waitAnswer=FALSE;
                        cob.running=FALSE;
                        cob.showFile=TRUE;
                        cob.debug_line=-1;
                        if(parsed->token>0 && tk>0 && parsed->token==tk){
                            parsedRet=parsed;
                        }
                    }else if(strcmp(parsed->resultRecords->resultClass,"connected")==0){
                        *status=GDB_CONNECTED;
                    }
                }
                if(parsed!=NULL && parsed->outOfBandRecord!=NULL){
                    if(parsed->outOfBandRecord->type!=NULL && strcmp(parsed->outOfBandRecord->type,"exec")==0){
                        if(parsed->outOfBandRecord->asyncClass!=NULL && strcmp(parsed->outOfBandRecord->asyncClass,"running")==0){
                            cob.running=TRUE;
                            *status=GDB_RUNNING;
                        }else if(parsed->outOfBandRecord->asyncClass!=NULL && strcmp(parsed->outOfBandRecord->asyncClass,"stopped")==0){
                            ST_TableValues * reason = parsed->outOfBandRecord->output;
                            if(reason!=NULL && reason->key!=NULL && strcmp(reason->key,"reason")==0){
                                if(strcmp(reason->value,"breakpoint-hit")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        *status = GDB_BREAKPOINT;
                                        sendCommandGdb("exec-next\n");
                                        lineChange=TRUE;
                                    }else{
                                        *status = GDB_STEP_END;
                                        cob.debug_line = hasLine->lineCobol;
                                        cob.changeLine = TRUE;
                                        cob.waitAnswer=FALSE;
                                        cob.running=FALSE;
                                        cob.showFile=TRUE;
                                    }
                                }else if(strcmp(reason->value,"end-stepping-range")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        *status = GDB_END_STEPPING_RANGE;
                                        sendCommandGdb(lastComand);
                                    }else{
                                        *status = GDB_STEP_END;
                                        cob.waitAnswer=FALSE;
                                        cob.debug_line = hasLine->lineCobol;
                                        cob.changeLine = TRUE;
                                        cob.running=FALSE;
                                        cob.showFile=TRUE;
                                    }
                                }else if(strcmp(reason->value,"location-reached")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        *status = GDB_LOCATION_REACHED;
                                        sendCommandGdb(lastComand);
                                    }else{
                                        *status = GDB_STEP_END;
                                        cob.debug_line = hasLine->lineCobol;
                                        cob.changeLine = TRUE;
                                        cob.running = FALSE;                                        
                                        cob.showFile=TRUE;
                                        cob.waitAnswer=FALSE;
                                    }
                                }else if(strcmp(reason->value,"function-finished")==0){
                                    ST_Line * hasLine=hasLineCobol(parsed);
                                    if(hasLine==NULL){
                                        *status = GDB_FUNCTION_FINISHED;
                                        sendCommandGdb(lastComand);
                                    }else{
                                        *status = GDB_STEP_OUT_END;
                                        cob.debug_line = hasLine->lineCobol;
                                        cob.changeLine = TRUE;
                                        cob.running = FALSE;                                        
                                        cob.showFile=TRUE;
                                        cob.waitAnswer=FALSE;
                                    }
                                }else if(strcmp(reason->value,"signal-received")==0){
                                        *status = GDB_SIGNAL_STOP;
                                        cob.showFile=TRUE;
                                        cob.waitAnswer=FALSE;
                                        cob.running=FALSE;
                                        loadCobSourceFile(cob.file_cobol, cob.first_file);
                                }else if(strcmp(reason->value,"exited-normally")==0){
                                        *status = GDB_STEP_OUT_END;
                                        cob.showFile=TRUE;
                                        cob.waitAnswer=FALSE;
                                        cob.running=FALSE;
                                        cob.debug_line=-1;
                                        loadCobSourceFile(cob.file_cobol, cob.first_file);
                                }else if(strcmp(reason->value,"exited")==0){
                                        *status = GDB_STOPPED;
                                        cob.showFile=TRUE;
                                        cob.waitAnswer=FALSE;
                                        cob.running=FALSE;
                                        cob.debug_line=-1;
                                        loadCobSourceFile(cob.file_cobol, cob.first_file);
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
        int status;
        MI2onOuput(sendCommandGdb, -1, &status);
   }
   return parsedRet;
}

void wait_gdb_answer(int (*sendCommandGdb)(char *)){
    if(gdbOutput!=NULL) strcpy(gdbOutput,"");
    do{
        sendCommandGdb("");
    }while(strlen(gdbOutput)==0 && strcmp(gdbOutput,"\n")!=0);
}

int MI2getStack(int (*sendCommandGdb)(char *), int thread){
    int status, tk;
    char command[200];
    strcpy(lastComand,"exec-step\n"); 
    sprintf(command,"%s %d 0 0\n","stack-list-frames --thread",thread);
    tk=sendCommandGdb(command);
    ST_MIInfo * parsed=NULL;
    do{
        sendCommandGdb("");
        //printf("%s\n", gdbOutput);
        parsed=MI2onOuput(sendCommandGdb, tk, &status);
    }while(status==GDB_RUNNING);
    cob.debug_line = -1;
    if(parsed!=NULL){
        if (parsed->resultRecords!=NULL && strcmp(parsed->resultRecords->resultClass,"done")==0 ) {
            boolean find=FALSE;
            ST_Line * hasLine = NULL;
            ST_TableValues * search1=parseMIvalueOf(parsed->resultRecords->results, "@frame.fullname", NULL, &find);
            if(search1!=NULL && search1->value!=NULL){
                normalizePath(search1->value);
                char * fone=strdup(search1->value);
                char * ftwo=strdup(cob.file_cobol);
                fileNameWithoutExtension(search1->value, fone);
                fileNameWithoutExtension(cob.file_cobol, ftwo);
                if(strcmp(fone, ftwo)==0){
                    find=FALSE;
                    ST_TableValues * search2=parseMIvalueOf(parsed->resultRecords->results, "@frame.line", NULL, &find);
                    if(search2!=NULL && search2->value!=NULL){
                        int lineC = atoi(search2->value);
                        hasLine =  getLineCobol( search1->value, lineC);
                        if(hasLine!=NULL)
                            cob.debug_line = hasLine->lineCobol;
                    }
                }
                free(fone);
                free(ftwo);
            }
        }
        freeParsed(parsed);
    } 
    return 0;
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
    cob.waitAnswer = TRUE;
    cob.showFile = TRUE;
    cob.running = TRUE;
}

int MI2stepInto(int (*sendCommandGdb)(char *)){
    int status;
    char command[200];
    strcpy(lastComand,"exec-step\n"); 
    if(subroutine>0){
        sprintf(command,"%s%d\n","break-insert -t ",subroutine);
        sendCommandGdb(command);
        do{
            sendCommandGdb("");
            MI2onOuput(sendCommandGdb, -1, &status);
        }while(status==GDB_RUNNING);
    }
    cob.waitAnswer = TRUE;
    cob.showFile = TRUE;
    cob.running=TRUE;
    strcpy(command,"exec-step\n"); 
    sendCommandGdb(command);
    do{
        sendCommandGdb("");
        MI2onOuput(sendCommandGdb, -1, &status);
    }while(status==GDB_RUNNING);
    return 0;
}

int MI2stepOut(int (*sendCommandGdb)(char *)){
    strcpy(lastComand,"exec-next\n"); 
    char command[200];
    strcpy(command,"exec-finish\n"); 
    sendCommandGdb(command);
    cob.waitAnswer = TRUE;
    cob.showFile = TRUE;
    cob.running=TRUE;
    return 0;
}

int MI2start(int (*sendCommandGdb)(char *)){
    strcpy(lastComand,"exec-next\n"); 
    char command[]="exec-run\n";
    sendCommandGdb(command);
    cob.waitAnswer = TRUE;
    cob.running=TRUE;
    cob.showFile = TRUE;
    return 0;
}

int MI2verifyGdb(int (*sendCommandGdb)(char *)){
    int status;
    sendCommandGdb("");
    MI2onOuput(sendCommandGdb, -1, &status);
    return 0;
}

int MI2addBreakPoint(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber ){
    char command[256];
    int status=0;
    ST_Line * line = getLineC(fileCobol, lineNumber);
    if(line!=NULL){
        sprintf(command,"%s%s:%d\n","break-insert -f ",line->fileC,line->lineC);
        sendCommandGdb(command);
        do{
            sendCommandGdb("");
            MI2onOuput(sendCommandGdb, -1, &status);
        }while(status==GDB_RUNNING);
        cob.waitAnswer = FALSE;
        ST_bk * search = BPList;
        ST_bk * before = NULL;
        while(search!=NULL){
            if(search->line==lineNumber && strcasecmp(search->name, line->fileCobol)==0) return 0;
            before=search;
            search=search->next;
        }
        ST_bk * newbk = malloc(sizeof(ST_bk));
        newbk->name = strdup(fileCobol);
        newbk->next=NULL;
        newbk->line=lineNumber;
        if(BPList==NULL)
            BPList=newbk;
        else    
            before->next=newbk;
    }
}

int MI2removeBreakPoint (int (*sendCommandGdb)(char *), Lines * lines, char * fileCobol, int lineNumber ){
    ST_Line * line = getLineC(fileCobol, lineNumber);
    int status=0;
    if(line!=NULL){
        char command[256];
        strcpy(command,"break-delete\n");
        sendCommandGdb(command);
        do{
            sendCommandGdb("");
            MI2onOuput(sendCommandGdb, -1, &status);
        }while(status==GDB_RUNNING);
        ST_bk * search = BPList;
        ST_bk * before = NULL;
        ST_bk * remove = NULL;
        while(search!=NULL){
            if(search->line==lineNumber && strcasecmp(search->name, line->fileCobol)==0){
                if(before==NULL){
                    BPList=search->next;
                }else{
                    before->next=search->next;
                }
                remove = search;
            }else{
                remove= NULL;
                MI2addBreakPoint(sendCommandGdb, search->name, search->line );
            }
            before=search;
            search=search->next;
            if(remove!=NULL){
                free(remove->name);
                free(remove);
            }
        }
        cob.waitAnswer = FALSE;
    }
    return 0;
}

int MI2goToCursor(int (*sendCommandGdb)(char *), char * fileCobol, int lineNumber ){
    char command[256];
    int status=0;
    int isRunning = (cob.debug_line>0);
    ST_Line * line = getLineC(fileCobol, lineNumber);
    if(line!=NULL){
        sprintf(command,"%s%s:%d\n","break-insert -t ",line->fileC,line->lineC);
        sendCommandGdb(command);
        do{
            sendCommandGdb("");
            MI2onOuput(sendCommandGdb, -1, &status);
        }while(status==GDB_RUNNING);
        if(isRunning)
            strcpy(command,"exec-continue\n"); 
        else
            strcpy(command,"exec-run\n");
        sendCommandGdb(command);
        cob.waitAnswer = TRUE;
        cob.showFile = TRUE;
        cob.running=TRUE;
    }
}

int MI2evalVariable(int (*sendCommandGdb)(char *), ST_DebuggerVariable * var, int thread, int frame){
    int hasCobGetFieldStringFunction = FALSE;
    int status;
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
    ST_MIInfo * parsed=NULL;
    do{
        sendCommandGdb("");
        parsed=MI2onOuput(sendCommandGdb, tk, &status);
    }while(status==GDB_RUNNING);
    //ST_MIInfo * parsed=MI2onOuput(sendCommandGdb, tk, &status);
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
    ST_MIInfo * parsed= NULL;
    int status;
    int tk = sendCommandGdb("stack-info-frame\n");
    do{
        sendCommandGdb("");
        parsed=MI2onOuput(sendCommandGdb, tk, &status);
    }while(status==GDB_RUNNING);
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
        int status;
        char command[256];
        
        ST_MIInfo * parsed=NULL;
        char * functionName = MI2getCurrentFunctionName(sendCommandGdb);
        sprintf(command,"stack-list-variables --thread %d --frame %d --all-values\n", thread, frame);
        int tk=sendCommandGdb(command);
        do{
           sendCommandGdb("");
           parsed=MI2onOuput(sendCommandGdb, tk, &status);
        }while(status==GDB_RUNNING);
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
    int status, tk;
    char aux[256];
    char command[512];
    char * cleanedRawValue = cleanRawValue(rawValue);
    if (var->attribute!=NULL && strcmp(var->attribute->type,"integer")==0){
        sprintf(command,"gdb-set var %s=%s\n", var->variablesByC, cleanedRawValue);
        sendCommandGdb(command);
        do{
           sendCommandGdb("");
           MI2onOuput(sendCommandGdb, tk, &status);
        }while(status==GDB_RUNNING);
    }else if (hasCobGetFieldStringFunction && strncmp(var->cName,"f_",2)==0) {
        // TODO
        sprintf(command,"gdb-set var %s=%s\n", var->variablesByC, cleanedRawValue);
        sendCommandGdb(command);
        do{
           sendCommandGdb("");
           MI2onOuput(sendCommandGdb, tk, &status);
        }while(status==GDB_RUNNING);
    } else{
        strcpy(aux, var->cName);
        if(strncmp(var->cName,"f_",2)==0) strcat(aux, ".data");
        char * finalValue = NULL;
        if(var->attribute!=NULL){
            finalValue=formatValueVar(cleanedRawValue, var->size, var->attribute->scale, var->attribute->type);
            //sprintf(command,"interpreter-exec console \"set %s = \\\"%s\\\"\"\n", aux, finalValue);
            //sprintf(command,"data-evaluate-expression \"(void)memcpy(%s,\\\"%s\\\",%d)\"\n", aux, finalValue, var->size);
            int qtt=strlen(finalValue);
            if(qtt>var->size) qtt=var->size;
            sprintf(command,"data-evaluate-expression \"(void)strncpy(%s,\\\"%s\\\",%d)\"\n", aux, finalValue, qtt);
            sendCommandGdb(command);
            do{
                sendCommandGdb("");
                MI2onOuput(sendCommandGdb, tk, &status);
            }while(status==GDB_RUNNING);
            free(finalValue);
        }
    }
    free(cleanedRawValue);
}

#define MAX_FILES 100

int MI2sourceFiles(int (*sendCommandGdb)(char *), char files[][512]){
    int status;
    char* token;
    int fileCount = 0;
    char fileload[512];

    if(gdbOutput!=NULL){
        free(gdbOutput);
        gdbOutput=NULL;
    }
    sendCommandGdb("interpreter-exec console \"info sources\"\n");
    do{
        #if defined(_WIN32)
        Sleep(500);
        #else
        usleep(10500);
        #endif
        sendCommandGdb("");
    }while(strstr(gdbOutput,"^done")==NULL);
    //printf("%s\n", gdbOutput);
    char* saveptr1; // Initialize our save pointers to save the context of tokens
    char* saveptr2; 
    char * tk = strtok_r(gdbOutput, "~", &saveptr1);
    while (tk != NULL && fileCount < MAX_FILES) {
        token = strtok_r(tk, ",", &saveptr2);
        while (token != NULL && fileCount < MAX_FILES) {
            int len = strlen(token);
            if(len>4){
                char ext[10];
                memcpy ( ext, &token[len-4], 4 );
                ext[4]='\0';
                if(istrstr(token,".cbl")!=NULL || istrstr(token,".cob")!=NULL){
                    int start=0;
                    while(token[start]==' ' || token[start]=='"') start++;
                    int end=0;
                    strcpy(fileload,"");
                    while(istrstr(fileload,".cbl")==NULL && istrstr(fileload,".cob")==NULL){
                    fileload[end++]=token[start++];
                    fileload[end]='\0';
                    }
                    normalizePath(fileload);
                    boolean insert=TRUE;
                    for(int idx=0; idx<fileCount; idx++ ){
                        if(strcmp(files[idx],fileload)==0) { insert=FALSE; break; }
                    }
                    if(insert){
                        strcpy(files[fileCount],fileload);
                        fileCount++;
                    }
                }
            }
            token = strtok_r(NULL, ",", &saveptr2);
        }
        tk = strtok_r(NULL, "~", &saveptr1);
    }
    free(gdbOutput);
    gdbOutput=NULL;
    return fileCount;
}


int MI2attach(int (*sendCommandGdb)(char *)){
    int status, tk;
    int lin=10;
    char aux[500];
    int bkg= color_dark_red;
    gotoxy(10,lin+2);
    print_colorBK(color_white, bkg);
    draw_box_first(10,lin+2,61,"Attach GDBServer");
    draw_box_border(10,lin+3);
    draw_box_border(72,lin+3);
    print_colorBK(color_yellow, bkg);
    gotoxy(11,lin+3);
    sprintf(aux,"%s:","Enter (server:port)");
    printf("%-61s",aux);
    lin++;
    print_colorBK(color_white, bkg);
    draw_box_last(10,lin+3,61);
    print_colorBK(color_green, bkg);
    fflush(stdout);
    gotoxy(12+strlen(aux),lin+2);
    readchar(aux,50);  
    strcpy(lastComand,"exec-continue\n"); 
    char command[500];
    sprintf(command, "target-select remote %s\n", aux);
    sendCommandGdb(command);
    do{
        sendCommandGdb("");
        MI2onOuput(sendCommandGdb, tk, &status);
    }while(status!=GDB_CONNECTED && status==0); 
    if(gdbOutput!=NULL){
        free(gdbOutput);
        gdbOutput=NULL;
    }
    if(status==GDB_CONNECTED){
        strcpy(lastComand,"exec-next\n"); 
        strcpy(command,"exec-continue\n");
        sendCommandGdb(command);
        do{
            sendCommandGdb("");
            MI2onOuput(sendCommandGdb, tk, &status);
        }while(status==GDB_RUNNING); 
    }
    return 0;
}
