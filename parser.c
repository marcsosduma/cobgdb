/* 
   This code is based on the GnuCOBOL Debugger extension available at: 
   https://github.com/OlegKunitsyn/gnucobol-debug
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include "cobgdb.h"
#ifndef EOVERFLOW
#define EOVERFLOW 132
#endif

//#define debug 0

char* toUpper(char* str);
int fileNameCompare(char * fileOne, char * fileTwo);
// external variables
extern ST_Line * LineDebug;
extern ST_Attribute * Attributes;
extern ST_DebuggerVariable * DebuggerVariable;
extern struct st_cobgdb cob;
//
ST_Line * LineAtu=NULL;
ST_Attribute * AtualAttrib;
ST_DebuggerVariable * AtuDebuggerVariable;

static char cwd[512]="";
static char cleanedFile[256];
static char version[100]="";

// File C structure
struct st_clines{
	char * line;
	int cline;
	int cobolLine;
	int breakpoint;
	struct st_clines * line_after;
	struct st_clines * line_before;
};

typedef struct st_clines Clines;

struct ST_CFILE {
	char name_cfile[256];
	Clines * clines;
	int qtd_clines;	
};
// FILE C

// LINES TO DEBUG
ST_Attribute * GetAttributes(char * key){
    ST_Attribute * searchAttribute = Attributes;
    while(searchAttribute!=NULL){
        if(strcmp(searchAttribute->key, key)==0) break;
        searchAttribute = searchAttribute->next;
    }
    return searchAttribute;
}

int64_t my_getline(char **restrict line, size_t *restrict len, FILE *restrict fp);

int strpos(char *str1, char *str2)
{
    int b=strlen(str2);
    for(int a=0;a<strlen(str1);a++){
        if(strncmp(&str1[a], str2, b)==0){
            return a;
        }
    }
    return -1;
}

int searchflagMatchers(char * flagStr){
    // 0xNNN1 0xNNN2 0xNNN4 0xNNN8 0xNN1N 0xNN2N 0xNN4N 0xNN8N 0xN1NN 0xN2NN 0xN4NN 0xN8NN 0x1NNN   
    if(flagStr==NULL || strlen(flagStr)<6) return -1;
    char dig[10];
    int s = 0;
    int ret=-1;
    for(int x=5;x>1;x--){
        subString(flagStr, x, 1, dig );
        int qtd=strpos("1248", dig);
        if(qtd>=0){
            ret = qtd+s;
            break;
        }
        s+=4;
    }
    return ret;
}

void buildFlags(ST_Attribute * attribute){  
        if (attribute->flagStr==NULL) {
            attribute->flags[0]=-1;
            return;
        }
        char m[10][512];
        int a=0;
        int qtd = searchflagMatchers(attribute->flagStr);
        if(qtd>=0){
            attribute->flags[a++]=qtd;
        }
        attribute->flags[a++]=-1;
}

int PushAttribute(char * key, char * cName, char * type, int digits, int scale, char * flagStr){
    ST_Attribute* new_attrib = (ST_Attribute*) malloc(sizeof(ST_Attribute));
    new_attrib->next=NULL;
    new_attrib->before=NULL;
    new_attrib->key = (char *) malloc(strlen(key)+1);
    strcpy(new_attrib->key, key);
    new_attrib->cName = (char *) malloc(strlen(cName)+1);
    strcpy(new_attrib->cName, cName);
    new_attrib->type = (char *) malloc(strlen(type)+1);
    strcpy(new_attrib->type, type);
    new_attrib->digits = digits;
    new_attrib->scale = scale;
    new_attrib->flagStr = (char *) malloc(strlen(flagStr)+1);
    strcpy(new_attrib->flagStr, flagStr);
    buildFlags(new_attrib);
    if(Attributes==NULL){
         Attributes=new_attrib;
         Attributes->next = NULL;
         Attributes->before = NULL;
         Attributes->last = NULL;
         Attributes->last = new_attrib;
         AtualAttrib = Attributes;
    }else{
         AtualAttrib = Attributes->last;
         AtualAttrib->next=new_attrib;
         new_attrib->before=AtualAttrib;
         new_attrib->next = NULL;
         AtualAttrib = new_attrib;
         Attributes->last = new_attrib;
    }
}

int PopAttribute(){
    if(Attributes!=NULL){
        ST_Attribute * toRemove = Attributes->last;
        AtualAttrib = Attributes->last;
        AtualAttrib = AtualAttrib->before;
        if(AtualAttrib==NULL)
            Attributes = NULL;
        else  
            AtualAttrib->next=NULL;
        if(toRemove->key!=NULL) free(toRemove->key);
        if(toRemove->cName!=NULL) free(toRemove->cName);
        if(toRemove->type!=NULL) free(toRemove->type);
        if(toRemove->flagStr!=NULL) free(toRemove->flagStr);
        free(toRemove);
    }
}

char * VariableType(char * type_org){
    char *org[]={"0x00","0x01","0x02","0x10","0x11","0x12","0x13","0x14","0x15","0x16","0x17",
                "0x18","0x19","0x1A","0x1B","0x24","0x20","0x21","0x22","0x23","0x40","0x41",
                "int" ,"cob_u8_t",""};
    char *ret[]={"unknown","group","boolean","numeric","numeric binary","numeric packed","numeric float","numeric double",
                "numeric l double","numeric FP DEC64","numeric FP DEC128","numeric FP BIN32","numeric FP BIN64","numeric FP BIN128","numeric COMP5","numeric edited","alphanumeric","alphanumeric","alphanumeric","alphanumeric edited",
                "national","national edited","integer","group",""};
    int idx=0;  
    while(strlen(org[idx])>0){
        if(strcmp(org[idx],type_org)==0) 
            return ret[idx];
        idx++;
    }
    return "";
}

int PushDebuggerVariable(char * cobolName, char * cName, char * functionName, ST_Attribute * attribute , int size){
    ST_DebuggerVariable * debVar = (ST_DebuggerVariable*) malloc(sizeof(ST_DebuggerVariable));
    debVar->cobolName = (char *) malloc(strlen(cobolName)+1);
    strcpy(debVar->cobolName, cobolName);
    debVar->cName = (char *) malloc(strlen(cName)+1);
    strcpy(debVar->cName, cName);
    debVar->functionName = (char *) malloc(strlen(functionName)+1);
    strcpy(debVar->functionName, functionName);
    debVar->attribute=attribute;   
    debVar->size = size;
    debVar->value=strdup("");
    debVar->dataSotorage=NULL;
    debVar->variablesByC=NULL;
    debVar->variablesByCobol=NULL;
    debVar->parent=NULL;
    debVar->first_children=NULL;
    debVar->brother=NULL;
    debVar->next=NULL;
    debVar->before=NULL;
    debVar->show=' ';
    debVar->ctlVar=-1;
    if(DebuggerVariable==NULL){
         DebuggerVariable=debVar;
         DebuggerVariable->next = NULL;
         DebuggerVariable->before = NULL;
         DebuggerVariable->last = DebuggerVariable;
         AtuDebuggerVariable = DebuggerVariable;
    }else{
         AtuDebuggerVariable= DebuggerVariable->last;
         AtuDebuggerVariable->next=debVar;
         debVar->before=AtuDebuggerVariable;
         debVar->next = NULL;
         AtuDebuggerVariable = debVar;
         DebuggerVariable->last = AtuDebuggerVariable;
    }
}

void AddChildDebuggerVariable(ST_DebuggerVariable * father, ST_DebuggerVariable * child){
    if(father->first_children==NULL){
        father->first_children=child;
        father->show='+';
        child->parent=father;
    }else{
        ST_DebuggerVariable * the_brother=father->first_children;
        while(the_brother->brother!=NULL){
            the_brother=the_brother->brother;
        }
        the_brother->brother=child;
        child->parent=father;
    }
}

char *  DebuggerVariable_Set(char * classe, char * value ){
    char tmp1[256];
    sprintf(tmp1,"%s.%s", classe, value );
    int size_var = strlen(tmp1);
    char * toSet = malloc(size_var+1);
    strcpy(toSet, tmp1);
    return toSet;
}

char *  DebuggerVariable_ChildSet(char * classe, char * father, char * child ){
    char tmp1[256];
    char tmp2[256];
    char tmp3[256];
    strcpy(tmp2, father);
    strcpy(tmp3, child);
    sprintf(tmp1,"%s.%s.%s", classe, toUpper(tmp2), toUpper(tmp3) );
    int size_var = strlen(tmp1);
    char * toSet = malloc(size_var+1);
    strcpy(toSet, tmp1);
    return toSet;
}


ST_DebuggerVariable * GetDataSotorage(char * key){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){
        if(search->dataSotorage!=NULL && strcmp(search->dataSotorage, key)==0) break;
        search = search->next;
    }
    return search;
}

ST_DebuggerVariable * GetVariableByCobol(char * key){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){
        if(search->variablesByCobol!=NULL && strcmp(search->variablesByCobol, key)==0) break;
        search = search->next;
    }
    return search;
}

ST_DebuggerVariable * GetVariableByC(char * key){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){
        if(search->variablesByC!=NULL && strcmp(search->variablesByC, key)==0) break;
        search = search->next;
    }
    return search;
}
// DebuggerVariable - END

int PushLine(char * filePathCobol, int lineCobol, char * filePathC, int lineC, int lineProgramExit, int isCall){
    ST_Line* new_line = (ST_Line*) malloc(sizeof(ST_Line));
    new_line->next=NULL;
    new_line->before=NULL;
    new_line->fileCobol = (char *) malloc(strlen(filePathCobol)+1);
    strcpy(new_line->fileCobol, filePathCobol);
    new_line->lineCobol = lineCobol;
    new_line->fileC = (char *) malloc(strlen(filePathC)+1);
    strcpy(new_line->fileC, filePathC);
    new_line->lineC = lineC;
    new_line->endPerformLine = -1;
    new_line->isCall = isCall;
    new_line->lineProgramExit=lineProgramExit;
    if(LineDebug==NULL){
         LineDebug=new_line;
         LineDebug->next = NULL;
         LineDebug->before = NULL;
         LineDebug->last=new_line;
         LineAtu = LineDebug;
    }else{
         LineAtu=LineDebug->last;
         LineAtu->next=new_line;
         new_line->before=LineAtu;
         new_line->next = NULL;
         LineDebug->last=new_line;
         LineAtu = new_line;
    }
}

int PopLine(){
    if(LineAtu!=NULL){
        ST_Line * toRemove = LineAtu;
        LineAtu = LineAtu->before;
        if(LineAtu==NULL)
            LineDebug = NULL;
        else{  
            LineAtu->next=NULL;
            if(LineDebug->last!=NULL) LineDebug->last=LineAtu;
        }
        if(toRemove->fileCobol!=NULL) free(toRemove->fileCobol);
        if(toRemove->fileC!=NULL) free(toRemove->fileC);
        free(toRemove);
    }
}

int readCFile(struct ST_CFILE * program) {
    FILE *fp = fopen(program->name_cfile, "r");
    if(fp == NULL) {
        perror("Unable to open file!");
        exit(1);
    }
    // Read lines from a text file using our own a portable getline implementation
    char *line = NULL;
    size_t len = 0;
    program->clines = NULL;

    Clines * line_before = NULL;
    int qtd = 0;
    while(my_getline(&line, &len, fp) != -1) {
        Clines* new_line = (Clines*) malloc(sizeof(Clines));
        new_line->line_after=NULL;
        new_line->line_before=NULL;
        if(program->clines == NULL){
             program->clines = new_line;             
        }else{
            new_line->line_before = line_before;
            line_before->line_after = new_line;            
        }
        line_before = new_line;
        new_line->line = line;
        new_line->cline=qtd+1;
        qtd++;
        line = NULL;
    }
    fclose(fp);
    program->qtd_clines=qtd;
}

void freeST_CFILE(struct ST_CFILE * program_cfile){
    Clines * lines = program_cfile->clines;
    for(int idx=0;idx<program_cfile->qtd_clines;idx++){
        Clines * remove= lines;
        lines=lines->line_after;
        free(remove->line);
        free(remove);
    }
    program_cfile->qtd_clines=0;
}


void cleanedCFileName(char *dst, char *filename) {
    int len = strlen(filename);
    memcpy(dst, filename, len-2);
    dst[len - 2] = 0;
}

boolean fileCobolRegex(struct st_parse line_parsed[100], int qtt_tk, char * fileName){
    int pos=0;
    boolean ret = FALSE;
    struct st_parse * m;
    struct st_parse * m1;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=2 || strncmp(m->token,"/*", 2)!=0){ pos++; qtt++; continue;}
        m=tk_val(line_parsed, qtt_tk, pos+1);      
        if(m->size!=9 || strncasecmp(m->token,"Generated", 9)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        if(m->size!=4 || strncasecmp(m->token,"from", 4)!=0){ break;}
        m1=tk_val(line_parsed, qtt_tk, pos+3);        
        if(m1->type!=TP_ALPHA) { break;}
        m=tk_val(line_parsed, qtt_tk, pos+4);        
        if(m->size!=2 || strncmp(m->token,"*/", 2)!=0){ break;}
        strncpy(fileName, m1->token, m1->size);
        fileName[m1->size]='\0';
        ret = TRUE;
        break;
    }
    return ret;
}

//char procedureRegex[] = "/\\*\\sLine:\\s([0-9]+)(\\s+:\\sEntry\\s)?";
boolean procedureRegex(struct st_parse line_parsed[100], int qtt_tk, char * numberLine, boolean * isPerform){
    int pos=0;
    boolean ret = FALSE;
    struct st_parse * m;
    struct st_parse * m1;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=2 || strncmp(m->token,"/*", 2)!=0){ pos++; qtt++; continue;}
        m=tk_val(line_parsed, qtt_tk, pos+1);        
        if(m->size!=4 || strncasecmp(m->token,"Line", 4)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);      
        if(m->size!=1 || strncmp(m->token,":", 1)!=0){ break;}
        m1=tk_val(line_parsed, qtt_tk, pos+3);        
        if(m1->type!=TP_NUMBER){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+4);        
        if(m->size!=1 || strncmp(m->token,":", 1)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+5);        
        if(m->size==5 && strncasecmp(m->token,"Entry", 5)==0){ break;}
        strncpy(numberLine, m1->token, m1->size);
        numberLine[m1->size]='\0';
        ret = TRUE;
        int qtt = pos+4;
        *isPerform = FALSE;
        while(qtt<qtt_tk){
            m=tk_val(line_parsed, qtt_tk, qtt);        
            if(m->size!=7 || strncasecmp(m->token,"Perform", 7)!=0){ qtt++; continue;}
            *isPerform = TRUE;
            break;
        }
        break;
    }
    return ret;
}

//char procedureFixRegex[] = "#line\\s([0-9]+)\\s\".*\\.c\"";
boolean procedureFixRegex(struct st_parse line_parsed[100], int qtt_tk, char * numberLine){
    int pos=0;
    boolean ret = FALSE;
    struct st_parse * m;
    struct st_parse * m1;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=5 || strncasecmp(m->token,"#line", 5)!=0){ break;}
        m1=tk_val(line_parsed, qtt_tk, pos+1);        
        if(m1->type!=TP_NUMBER){ break;}
        int p1= pos+2;
        while(p1<qtt_tk){
            m=tk_val(line_parsed, qtt_tk, p1);
            if(m->size>3){
                char * cmp = (m->token + m->size - 3);
                if(strncasecmp(cmp,".c\"",3)==0){
                    ret = TRUE;
                    strncpy(numberLine, m1->token, m1->size);
                    numberLine[m1->size]='\0';
                    break;
                }
            }
            p1++;
        }
        break;
    }
    return ret;
}

boolean fixOlderFormat(char * value){
    if(value==NULL || istrstr(value,"cob_trace_stmt")!=NULL) return TRUE;
    return FALSE;
}

//char frame_ptrFindRegex[] = "frame\\_ptr--;";
boolean frame_ptrFindRegex(char * value){
    if(value==NULL || istrstr(value,"frame_ptr--;")!=NULL) return TRUE;
    return FALSE;
}

//char call_FindRegex[] = "call";
boolean call_FindRegex(char * value){
    if(value==NULL || istrstr(value,"call")!=NULL) return TRUE;
    return FALSE;
}

//char attributeRegex[] = "static\\sconst\\scob_field_attr\\s(a_[0-9]+).*\\{(0x[0-9]*),\\s*([0-9-]*),\\s*([0-9-]*),\\s*(0x[0-9]{4}),.*";
boolean attributeRegex(struct st_parse line_parsed[100], int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    int idx=0;
    boolean ret = FALSE;
    struct st_parse * m;
    struct st_parse * m1;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=6 || strncasecmp(m->token,"static", 6)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);        
        if(m->size!=5 || strncasecmp(m->token,"const", 5)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        if(m->size!=14 || strncasecmp(m->token,"cob_field_attr", 14)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+3);        
        if(m->size==0){ break;}
        strncpy(mm[idx],m->token,m->size);
        mm[idx++][m->size]='\0';
        m=tk_val(line_parsed, qtt_tk, pos+4);    
        if(m->size!=1 || strncmp(m->token,"=", 1)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+5);    
        if(m->size!=1 || strncmp(m->token,"{", 1)!=0){ break;}
        int p1= pos+6;
        while(p1<qtt_tk){
            m=tk_val(line_parsed, qtt_tk, p1);
            if(strncmp(m->token,",", m->size)==0) {p1++; continue;}
            if(strncmp(m->token,"}", m->size)==0) {break;}
            if(m->size>0){
                strncpy(mm[idx],m->token,m->size);
                mm[idx++][m->size]='\0';
                ret = TRUE;
            }
            p1++;
        }
        break;
    }
    return ret;
}

//char dataStorageRegex[] = "static\\s+(.*)\\s+(b\\_[0-9]+)(\\;|\\[[0-9]+\\]).*/\\*\\s+([0-9a-z\\_\\-]+).*\\s+\\*/.*";
boolean dataStorageRegex(struct st_parse line_parsed[100], int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    int idx=0;
    boolean ret = FALSE;
    struct st_parse * m;
    struct st_parse * m1;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=6 || strncasecmp(m->token,"static", 6)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);        
        strcmp(mm[idx],"");
        if(m->size>0) strncpy(mm[idx],m->token,m->size);
        mm[idx][m->size]='\0';
        idx++;
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        if(m->size<2 || strncasecmp(m->token,"b_", 2)!=0){ break;}
        strncpy(mm[idx],m->token,m->size);
        mm[idx++][m->size]='\0';
        int p1= pos+3;
        m=tk_val(line_parsed, qtt_tk, pos+3);
        if(m->size==1 && strncmp(m->token,"[", 1)==0){
             strcpy(mm[idx],"");
            while(p1<qtt_tk){
                m=tk_val(line_parsed, qtt_tk, p1++);
                int len = strlen(mm[idx]);
                strncat(mm[idx], m->token,m->size);
                mm[idx][(len+m->size)]='\0';
                if(m->size==1 && strncmp(m->token,"]", 1)==0) {idx++;break;}
            }
        }else{
            strcmp(mm[idx++],"");
        }
        while(p1<qtt_tk){
            m=tk_val(line_parsed, qtt_tk, p1);
            if(m->size!=2 || strncmp(m->token,"/*", 2)!=0) {p1++; continue;}
            p1++;
            strcpy(mm[idx],"");
            int words=0;
            while(p1<qtt_tk){
                m=tk_val(line_parsed, qtt_tk, p1++);
                if(m->size==2 && strncmp(m->token,"*/", 2)==0) {idx++; break;}
                if(words++>0) strcat(mm[idx]," ");
                int len = strlen(mm[idx]);
                strncat(mm[idx], m->token,m->size);
                mm[idx][(len+m->size)]='\0';
                ret = TRUE;
            }            
        }
        break;
    }
    return ret;
}

//char fieldRegex[] = "static\\s+cob_field\\s+([0-9a-z\\_]+)\\s+\\=\\s+\\{([0-9]+)\\,\\s+([0-9a-z\\_]+).+\\&(a\\_[0-9]+).*/\\*\\s+([0-9a-z\\_\\-]+)\\s+\\*/";
boolean fieldRegex(struct st_parse line_parsed[100], int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    int idx=0;
    boolean ret = FALSE;
    struct st_parse * m;
    struct st_parse * m1;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=6 || strncasecmp(m->token,"static", 6)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);        
        if(m->size!=9 || strncasecmp(m->token,"cob_field", 9)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        strcmp(mm[idx],"");
        if(m->size>0) strncpy(mm[idx],m->token,m->size);
        mm[idx++][m->size]='\0';
        m=tk_val(line_parsed, qtt_tk, pos+3);        
        if(m->size==0 || strncmp(m->token,"=", 1)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+4);
        if(m->size<0 || strncmp(m->token,"{", 1)!=0) { break;}
        int p1 = pos+5;
        while(p1<qtt_tk){
            m=tk_val(line_parsed, qtt_tk, p1);
            if(m->size>0 && strncmp(m->token,",", 1)==0) { p1++;continue;}
            if(m->size>0 && strncmp(m->token,"&", 1)==0) { p1++;continue;}
            if(m->size<1 || strncmp(m->token,"}", 1)==0) { break;}
            strncpy(mm[idx],m->token,m->size);
            mm[idx++][m->size]='\0';
            while(p1<qtt_tk && m->size>0 && strncmp(m->token,"}", 1)!=0
                    && strncmp(m->token,",", 1)!=0){
                p1++;
                m=tk_val(line_parsed, qtt_tk, p1);
             }
        }
        p1++;
        while(p1<qtt_tk){
            m=tk_val(line_parsed, qtt_tk, p1);
            if(m->size<2 || strncmp(m->token,"/*", 2)!=0) {p1++; continue;}
            p1++;
            strcpy(mm[idx],"");
            int words=0;
            while(p1<qtt_tk){
                m=tk_val(line_parsed, qtt_tk, p1++);
                if(m->size>1 && strncmp(m->token,"*/", 2)==0) {idx++; break;}
                if(words++>0) strcat(mm[idx]," ");
                int len = strlen(mm[idx]);
                strncat(mm[idx], m->token,m->size);
                mm[idx][(len+m->size)]='\0';
                ret = TRUE;
            }            
        }
        break;
    }
    return ret;
}

//char functionRegex[] = "/\\*\\sProgram\\slocal\\svariables\\sfor\\s'(.*)'\\s\\*/";
boolean functionRegex(struct st_parse line_parsed[100], int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    int idx=0;
    boolean ret = FALSE;
    struct st_parse * m;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=2 || strncmp(m->token,"/*", 2)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);        
        if(m->size!=7 || strncasecmp(m->token,"Program",7)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        if(m->size!=5 || strncasecmp(m->token,"local", 5)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+3);        
        if(m->size!=9 || strncasecmp(m->token,"variables", 9)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+4);        
        if(m->size!=3 || strncasecmp(m->token,"for", 3)!=0){ break;}
        int p1 = pos+5;
        strcpy(mm[idx],"");
        while(p1<qtt_tk){
            m=tk_val(line_parsed, qtt_tk, p1++);
            if(m->size==2 && strncmp(m->token,"*/", 2)==0) { break;}
            int len = strlen(mm[idx]);
            strncat(mm[idx], m->token,m->size);
            mm[idx][(len+m->size)]='\0';
            ret = TRUE;
        }            
        break;
    }
    return ret;
}

//char versionRegex[] = "/\\*\\sGenerated by\\s+cobc\\s([0-9a-z\\-\\.]+)\\s+\\*/";
boolean versionRegex(struct st_parse line_parsed[100], int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    boolean ret = FALSE;
    struct st_parse * m;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=2 || strncmp(m->token,"/*", 2)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);      
        if(m->size!=9 || strncasecmp(m->token,"Generated", 9)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        if(m->size!=2 || strncasecmp(m->token,"by", 2)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+3);        
        if(m->size!=4 || strncasecmp(m->token,"cobc", 4)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+4);        
        strncpy(mm[0], m->token, m->size);
        mm[0][m->size]='\0';
        ret = TRUE;
        break;
    }
    return ret;
}

/* Program exit */
boolean programExit(struct st_parse line_parsed[100], int qtt_tk){
    int pos=0;
    boolean ret = FALSE;
    struct st_parse * m;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=2 || strncmp(m->token,"/*", 2)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);      
        if(m->size!=7 || strncasecmp(m->token,"Program", 7)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        if(m->size!=4 || strncasecmp(m->token,"exit", 4)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+3);        
       if(m->size!=2 || strncmp(m->token,"*/", 2)!=0){ break;}
        ret = TRUE;
        break;
    }
    return ret;
}

//char fileIncludeRegex[] = "\\#include\\s+\\\"([0-9a-z_\\-\\.\\s]+)\\\"";
boolean fileIncludeRegex(struct st_parse line_parsed[100], int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    boolean ret = FALSE;
    struct st_parse * m;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=8 || strncasecmp(m->token,"#include", 8)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);      
        if(m->size<1 || strncmp(m->token,"\"", 1)!=0){ break;}
        strncpy(mm[0], m->token, m->size);
        mm[0][m->size]='\0';
        ret = TRUE;
        break;
    }
    return ret;
}

int parser(char * file_name, int fileN){
    char m[10][512];
    char fileCobol[1024];
    char functionName[512];
    char fileC[512];
    char basename[512];
    char buffer[512];
    int performLine = -1;
    struct ST_CFILE program_cfile;
    int qtd = 0;
    boolean isVersion2_2_or_3_1_1=FALSE;
    char *path = NULL;    
    char tmp[300];
    int lineProgramExit = 1;

    normalizePath(file_name);

    if(strlen(cwd)==0){
        path = realpath(file_name, cwd);
        normalizePath(path);
        strcpy(file_name, path);
        getPathName(cwd, path);
        strcpy(basename,getFileNameFromPath(file_name));
        cleanedCFileName(cleanedFile, basename);
    }else{
        strcpy(basename,getFileNameFromPath(file_name));
    }
    char sep = '/';

    sprintf(file_name,"%s%c%s",cwd,sep,basename);

    strcpy(fileC, file_name);
    strcpy(program_cfile.name_cfile, file_name);
    
    readCFile(&program_cfile);
    Clines * lines = program_cfile.clines;  
    boolean bfileCobolRegex=FALSE;
    struct st_parse line_parsed[100];
    int qtt_tk;
    //gotoxy(1,1); noColor();
    for(int lineNumber=0;lineNumber<program_cfile.qtd_clines;lineNumber++){
        #ifdef debug
        printf("%s", lines->line);
        #endif

        qtt_tk=0;
        lineParse(lines->line, line_parsed, &qtt_tk);
        if(!bfileCobolRegex){            
            bfileCobolRegex = fileCobolRegex(line_parsed, qtt_tk, &tmp[0]);
            if(bfileCobolRegex){
                char * test= realpath(tmp, buffer);
                if(!isAbsolutPath(tmp)){
                    char tmp1[512];
                    strcpy(tmp1,getFileNameFromPath(tmp));
                    sprintf(fileCobol,"%s%c%s",cwd,sep,tmp1);
                }else{
                    strcpy(fileCobol, tmp);
                }            
                if(fileN==0){
                    strcpy(cob.name_file, fileCobol);
                }
            }
        }
        
        //qtd=regex(functionRegex1, lines->line, m);
        boolean bfunctionRegex = functionRegex(line_parsed, qtt_tk, m);
        if(bfunctionRegex){
            if(m[0][0]=='\'' || m[0][0]=='"'){
                char dest[100];
                subString(m[0],1,strlen(m[0])-2, dest);
                strcpy(functionName,toLower(dest));
                strcat(functionName, "_");
            }else{
                strcpy(functionName,toLower(m[0]));
                strcat(functionName, "_");
            }
        }

        boolean isPerform=FALSE;
        boolean bprocedureRegex = procedureRegex(line_parsed, qtt_tk, &tmp[0], &isPerform);
        if(bprocedureRegex){
                int lineCobol = atoi(tmp);
                if(LineAtu!=NULL && fileNameCompare(LineAtu->fileCobol, fileCobol)==0 && performLine==-2 && LineAtu->lineCobol == lineCobol) {
                    PopLine();
                }
                if(isPerform)
                    performLine = -2;
                else
                    performLine = -1;
                boolean isCall= call_FindRegex(lines->line);
                PushLine(fileCobol, lineCobol, fileC, lineNumber + 2, lineProgramExit, isCall);
        }

        // fix new codegen - new
        //qtd=regex(procedureFixRegex, lines->line, m);
        boolean bprocedureFixRegex = procedureFixRegex(line_parsed, qtt_tk, &tmp[0]);
        if(bprocedureFixRegex>0 && LineAtu!=NULL && LineAtu->lineProgramExit>=lineProgramExit){
            int lineC = atoi(tmp);
            boolean isOldFormat = (lines->line_before!=NULL)?fixOlderFormat(lines->line_before->line):FALSE;
            if(fileNameCompare(LineAtu->fileCobol, fileCobol)==0 && (isVersion2_2_or_3_1_1 || !isOldFormat)){
                LineAtu->lineC = lineC;                    
            }
        }
        //Attributes
        //qtd=regex(attributeRegex, lines->line, m);
        boolean battributeRegex = attributeRegex(line_parsed, qtt_tk, m);
        if(battributeRegex){
            int digits=atoi(m[2]);
            int scale=atoi(m[3]);
            char key[256];
            strcpy(key, cleanedFile); 
            strcat(key,".");
            strcat(key,m[0]);
            PushAttribute(key, m[0],  VariableType(m[1]), digits, scale, m[4]);
        }
        //DebuggerVariable
        //qtd=regex(dataStorageRegex, lines->line, m);
        boolean bdataStorageRegex = dataStorageRegex(line_parsed, qtt_tk, m);
        if(bdataStorageRegex){
            int size=0;
            if(m[2][0]=='['){
                char dest[100];
                subString(m[2],1,strlen(m[2])-1, dest);
                size = atoi(dest);
            }
            ST_Attribute* new_attrib = (ST_Attribute*) malloc(sizeof(ST_Attribute));
            char * type =  VariableType(m[0]);
            new_attrib->type = (char *) malloc(strlen(type)+1);
            strcpy(new_attrib->type, type);
            new_attrib->digits=0; new_attrib->scale=0; new_attrib->cName=NULL;  new_attrib->key=NULL;
            new_attrib->flagStr=NULL; new_attrib->next=NULL; new_attrib->before=NULL; new_attrib->last=NULL;
            PushDebuggerVariable(m[3], m[1], functionName, new_attrib , size);
            AtuDebuggerVariable->dataSotorage=DebuggerVariable_Set(functionName, m[1] );
            AtuDebuggerVariable->variablesByC=DebuggerVariable_Set(functionName, m[1] );
            AtuDebuggerVariable->variablesByCobol=DebuggerVariable_Set(functionName, toUpper(m[3]) );
        }

        //qtd=regex(fieldRegex, lines->line, m);
        boolean bfieldRegex = fieldRegex(line_parsed, qtt_tk, m);
        if(bfieldRegex){
            char tmp1[256];
            //sprintf(tmp1,"%s.%s",cleanedFile,m[4]);
            strcpy(tmp1,cleanedFile);
            strcat(tmp1,".");
            strcat(tmp1,m[3]);
            ST_Attribute * attribute = GetAttributes(tmp1);
            //sprintf(tmp1,"%s.%s",functionName,m[3]);
            strcpy(tmp1,functionName);
            strcat(tmp1,".");
            strcat(tmp1,m[2]);
            ST_DebuggerVariable * dataStorage = GetDataSotorage(tmp1);
            int size = atoi(m[1]);
            PushDebuggerVariable(m[4], m[0], functionName, attribute, size);
            AtuDebuggerVariable->variablesByC = DebuggerVariable_Set(functionName, AtuDebuggerVariable->cName );
            if(dataStorage!=NULL){
                AddChildDebuggerVariable(dataStorage, AtuDebuggerVariable);
                AtuDebuggerVariable->variablesByCobol=DebuggerVariable_ChildSet(functionName,dataStorage->cobolName,AtuDebuggerVariable->cobolName);
            }else{
                strcpy(tmp1, AtuDebuggerVariable->cobolName);
                AtuDebuggerVariable->variablesByCobol=DebuggerVariable_Set(functionName, toUpper(tmp1) );
            }
        }

        //qtd=regex(fileIncludeRegex, lines->line, m);
        boolean bfileIncludeRegex = fileIncludeRegex(line_parsed, qtt_tk, m);
        if(bfileIncludeRegex){
            m[0][strcspn(m[0], "\n")] = '\0';
            if(m[0][0]=='\'' || m[0][0]=='"'){
                char dest[100];
                subString(m[0],1,strlen(m[0])-2, dest);
                parser(dest, fileN);
            }
        }
        //qtd=regex(versionRegex, lines->line, m);
        boolean bversionRegex = versionRegex(line_parsed, qtt_tk, m);
        if(bversionRegex){
            strcpy(version, m[0]);
            if(strncmp(version,"2.2",3)==0 || strncmp(version,"3.1.1",5)==0) isVersion2_2_or_3_1_1 = TRUE;
        }
        if(performLine==-2){
            //qtd=regex(frame_ptrFindRegex, lines->line, m);
            boolean bframe_ptrFindRegex= frame_ptrFindRegex(lines->line);
            if(bframe_ptrFindRegex){
                LineAtu->endPerformLine = lineNumber+1;
                performLine=-1;
            }
        }
        boolean bprogramExit=programExit(line_parsed, qtt_tk);
        if(bprogramExit){
            lineProgramExit=lineNumber;
        }
        if(lines->line_after!=NULL) lines=lines->line_after;
    }
    freeST_CFILE(&program_cfile);
}

int hasCobolLine(int lineCobol){
    ST_Line * search = LineDebug;
    int line=-1;
    while(search!=NULL){
        if(search->lineCobol==lineCobol){
            line=search->lineC;
            break;
        }
        search = search->next;
    }
    return line;
}

ST_Line * getLineC(char * fileCobol, int lineCobol){
    ST_Line * search = LineDebug;
    while(search!=NULL){
        if(strlen(search->fileCobol)>0 && fileNameCompare(fileCobol,search->fileCobol)==0 && search->lineCobol==lineCobol) break;
        search = search->next;
    }
    return search;
}

ST_Line * getLineCobol(char * fileC, int lineC){
    ST_Line * search = LineDebug;
    if(fileC==NULL) return NULL;
    normalizePath(fileC);
    while(search!=NULL){
        if(strlen(search->fileC)>0 && fileNameCompare(fileC,search->fileC)==0 && search->lineC==lineC) break;
        search = search->next;
    }
    return search;
}

void getVersion(char vrs[]){
    strcpy(vrs, version);
}

int getLinesCount(){
    int tot = 0;
    ST_Line * search = LineDebug;
    while(search!=NULL){
        tot++;
        search = search->next;
    }
    return tot;
}

int getVariablesCount(){
        int tot = 0;
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){
        if(search->variablesByC!=NULL)
            tot++;
        search = search->next;
    }
    return tot;
}

ST_DebuggerVariable * getVariableByCobol(char * cobVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL && strcmp(search->variablesByCobol, cobVar)==0)
            break;
        search = search->next;
    }
    return search;
}

ST_DebuggerVariable * getVariablesByCobol(){
    ST_DebuggerVariable * search = DebuggerVariable;
    return search;
}


ST_DebuggerVariable * getVariableByC(char * cVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL && strcmp(search->variablesByC, cVar)==0)
            break;
        search = search->next;
    }
    return search;
}

ST_DebuggerVariable * findVariableByCobol(char * functionName, char * cobVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    char tmp1[256];
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL){
            sprintf(tmp1,"%s%c",functionName,'.');
            if(strstr(search->variablesByCobol,tmp1)!=NULL){
                sprintf(tmp1,"%c%s",'.',cobVar);
                if(strstr(search->variablesByCobol,tmp1)!=NULL){
                    break;
                }
            }
        }
        search = search->next;
    }
    return search;
}

ST_DebuggerVariable * findFieldVariableByCobol(char * functionName, char * cobVar, ST_DebuggerVariable * start){
    ST_DebuggerVariable * search = start;
    char tmp1[256];
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL){
            sprintf(tmp1,"%s%c",functionName,'.');
            if(strstr(search->variablesByCobol,tmp1)!=NULL){
                sprintf(tmp1,"%c%s",'.',cobVar);
                if(strstr(search->variablesByCobol,tmp1)!=NULL){
                    if(strncmp(search->cName,"f_",2)==0) break; 
                    ST_DebuggerVariable * search1=findFieldVariableByCobol(functionName, cobVar, search->next);
                    if(search1!=NULL) search=search1;
                    break;
                }
            }
        }
        search = search->next;
    }
    return search;
}


int fileNameCompare(char * fileOne, char * fileTwo){
#if defined(_WIN32)
    int result=0;
    result = strcasecmp(fileOne,fileTwo);
    return result;
#elif defined(__linux__)
    return strcmp(fileOne,fileTwo);
#endif // Windows/Linux 
}

void SourceMap(char fileGroup[][512]){
    int q = 0;
    LineDebug = NULL;
    Attributes = NULL;
    DebuggerVariable = NULL;

    while(strlen(fileGroup[q])>0){
        strcpy(cwd,"");
        parser(fileGroup[q], q);
        q++;
    }
}
