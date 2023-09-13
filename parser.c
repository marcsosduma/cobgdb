#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "cobgdb.h"
#include "regex_gdb.h"
#ifndef EOVERFLOW
#define EOVERFLOW 132
#endif

char* toUpper(char* str);
int fileNameCompare(char * fileOne, char * fileTwo);
//#define debug 0

//
extern ST_Line * LineDebug;
extern ST_Attribute * Attributes;
extern ST_DebuggerVariable * DebuggerVariable;
extern struct program_file program_file;
//
ST_Line * LineAtu=NULL;
ST_Attribute * AtualAttrib;
ST_DebuggerVariable * AtuDebuggerVariable;

static char cwd[512]="";
static char cleanedFile[256];
static char version[100];


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


char * flagMatchers[]={"0x[0-9][0-9][0-9]1","0x[0-9][0-9][0-9]2","0x[0-9][0-9][0-9]4","0x[0-9][0-9][0-9]8","0x[0-9][0-9]1[0-9]","0x[0-9][0-9]2[0-9]",
                      "0x[0-9][0-9]4[0-9]","0x[0-9][0-9]8[0-9]","0x[0-9]1[0-9][0-9]","0x[0-9]2[0-9][0-9]","0x[0-9]4[0-9][0-9]","0x[0-9]8[0-9][0-9]",
                      "0x1[0-9][0-9][0-9]"};

int64_t my_getline(char **restrict line, size_t *restrict len, FILE *restrict fp);

void buildFlags(ST_Attribute * attribute){  
        if (attribute->flagStr==NULL) {
            attribute->flags[0]=-1;
            return;
        }
        char m[10][512];
        int a=0;
        for(int idx=0;idx<(sizeof(flagMatchers)/sizeof(flagMatchers[0]));idx++){
            int qtd=regex(flagMatchers[idx], attribute->flagStr, m);
            if(qtd){
                attribute->flags[a++]=idx;
            }
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
        if(strcmp(org[idx],type_org)==0) return ret[idx];
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

int PushLine(char * filePathCobol, int lineCobol, char * filePathC, int lineC){
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
        else  
            LineAtu->next=NULL;
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

int parser(char * file_name, int fileN){
    char procedureRegex[] = "/\\*\\sLine:\\s([0-9]+)(\\s+:\\sEntry\\s)?";
    //const fileCobolRegex = /\/\*\sGenerated from\s+([0-9a-z_\-\/\.\s\\:]+)\s+\*\//i;
    char fileCobolRegex[] = "/\\*\\sGenerated from\\s+([\\.0-9/a-z\\-]+\\w)"; // \\s+([0-9a-z\\/\\_\\-\\.\\s:]+)\\s+\\*/";
    char versionRegex[] = "/\\*\\sGenerated by\\s+cobc\\s([0-9a-z\\-\\.]+)\\s+\\*/";
    char subroutineRegex[] = "\\sPerform\\s";
    char procedureFixRegex[] = "#line\\s([0-9]+)\\s\".*\\.c\"";
    char fixOlderFormat[] = "cob\\_trace\\_stmt";
    char attributeRegex[] = "static\\sconst\\scob_field_attr\\s(a_[0-9]+).*\\{(0x[0-9]*),\\s*([0-9-]*),\\s*([0-9-]*),\\s*(0x[0-9]{4}),.*";
    char dataStorageRegex[] = "static\\s+(.*)\\s+(b\\_[0-9]+)(\\;|\\[[0-9]+\\]).*/\\*\\s+([0-9a-z\\_\\-]+).*\\s+\\*/.*";
    char fieldRegex[] = "static\\s+cob_field\\s+([0-9a-z\\_]+)\\s+\\=\\s+\\{([0-9]+)\\,\\s+([0-9a-z\\_]+).+\\&(a\\_[0-9]+).*/\\*\\s+([0-9a-z\\_\\-]+)\\s+\\*/";
    char fileIncludeRegex[] = "\\#include\\s+\\\"([0-9a-z_\\-\\.\\s]+)\\\"";
    char functionRegex[] = "/\\*\\sProgram\\slocal\\svariables\\sfor\\s'(.*)'\\s\\*/";
    char frame_ptrFindRegex[] = "frame\\_ptr--;";
    char m[10][512];
    char fileCobol[1024];
    char functionName[512];
    char fileC[512];
    char basename[512];
    char buffer[512];
    int performLine = -1;
    struct ST_CFILE program_cfile;
    int qtd = 0;
    int isVersion2_2_or_3_1_1=0;
    char *path = NULL;

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
    //gotoxy(1,1); noColor();
    for(int lineNumber=0;lineNumber<program_cfile.qtd_clines;lineNumber++){
        #ifdef debug
        printf("%s", lines->line);
        #endif
        qtd=regex(fileCobolRegex, lines->line, m);
        if(qtd>0){
            //char * test= realpath(m[1], buffer);
            //char sep = '/';
            if(!isAbsolutPath(m[1])){
                char tmp1[512];
                strcpy(tmp1,getFileNameFromPath(m[1]));
                sprintf(fileCobol,"%s%c%s",cwd,sep,tmp1);
            }else{
                strcpy(fileCobol, m[1]);
            }            
            if(fileN==0){
                strcpy(program_file.name_file, fileCobol);
            }
        }
        qtd=regex(functionRegex, lines->line, m);
        if(qtd>0){
            strcpy(functionName,toLower(m[1]));
            strcat(functionName, "_");
        }
        qtd=regex(procedureRegex, lines->line, m);
        if(qtd==2){
                int lineCobol = atoi(m[1]);
                if(LineAtu!=NULL && fileNameCompare(LineAtu->fileCobol, fileCobol) && LineAtu->lineCobol == lineCobol) {
                    PopLine();
                }
                int qt=regex(subroutineRegex, lines->line, m);
                if(qt>0)
                    performLine = -2;
                else
                    performLine = -1;
                PushLine(fileCobol, lineCobol, fileC, lineNumber + 2);
        }
        // fix new codegen - new
        qtd=regex(procedureFixRegex, lines->line, m);
        if(qtd>0 && LineAtu!=NULL){
            int lineC = atoi(m[1]);
            int isOldFormat = (lines->line_before!=NULL)?regex(fixOlderFormat, lines->line_before->line, m):0;
            if(isVersion2_2_or_3_1_1 || !isOldFormat){
                LineAtu->lineC = lineC;                    
            }
        }
        //Attributes
        qtd=regex(attributeRegex, lines->line, m);
        if(qtd>0){
            int digits=atoi(m[3]);
            int scale=atoi(m[4]);
            char key[256];
            strcpy(key, cleanedFile); 
            strcat(key,".");
            strcat(key,m[1]);
            PushAttribute(key, m[1],  VariableType(m[2]), digits, scale, m[5]);
        }
        //DebuggerVariable
        qtd=regex(dataStorageRegex, lines->line, m);
        if(qtd>0){
            int size=0;
            if(m[3][0]=='['){
                char dest[100];
                subString(m[3],1,strlen(m[3])-1, dest);
                size = atoi(dest);
            }
            ST_Attribute* new_attrib = (ST_Attribute*) malloc(sizeof(ST_Attribute));
            char * type =  VariableType(m[1]);
            new_attrib->type = (char *) malloc(strlen(type)+1);
            strcpy(new_attrib->type, type);
            new_attrib->digits=0;
            new_attrib->scale=0;
            PushDebuggerVariable(m[4], m[2], functionName, new_attrib , size);
            AtuDebuggerVariable->dataSotorage=DebuggerVariable_Set(functionName, m[2] );
            AtuDebuggerVariable->variablesByC=DebuggerVariable_Set(functionName, m[2] );
            AtuDebuggerVariable->variablesByCobol=DebuggerVariable_Set(functionName, toUpper(m[4]) );
        }

        qtd=regex(fieldRegex, lines->line, m);
        if(qtd>0){
            char tmp1[256];
            //sprintf(tmp1,"%s.%s",cleanedFile,m[4]);
            strcpy(tmp1,cleanedFile);
            strcat(tmp1,".");
            strcat(tmp1,m[4]);
            ST_Attribute * attribute = GetAttributes(tmp1);
            //sprintf(tmp1,"%s.%s",functionName,m[3]);
            strcpy(tmp1,functionName);
            strcat(tmp1,".");
            strcat(tmp1,m[3]);
            ST_DebuggerVariable * dataStorage = GetDataSotorage(tmp1);
            int size = atoi(m[2]);
            PushDebuggerVariable(m[5], m[1], functionName, attribute, size);
            AtuDebuggerVariable->variablesByC = DebuggerVariable_Set(functionName, AtuDebuggerVariable->cName );
            if(dataStorage!=NULL){
                AddChildDebuggerVariable(dataStorage, AtuDebuggerVariable);
                AtuDebuggerVariable->variablesByCobol=DebuggerVariable_ChildSet(functionName,dataStorage->cobolName,AtuDebuggerVariable->cobolName);
            }else{
                strcpy(tmp1, AtuDebuggerVariable->cobolName);
                AtuDebuggerVariable->variablesByCobol=DebuggerVariable_Set(functionName, toUpper(tmp1) );
            }
        }

        qtd=regex(fileIncludeRegex, lines->line, m);
        if(qtd>0){
            m[1][strcspn(m[1], "\n")] = '\0';
            parser(m[1], fileN);
        }
        qtd=regex(versionRegex, lines->line, m);
        if(qtd>0){
            strcpy(version, m[1]);
            if(strncmp(version,"2.2",3)==0 || strncmp(version,"3.1.1",5)==0) isVersion2_2_or_3_1_1 = 1;
        }
        if(performLine==-2){
            qtd=regex(frame_ptrFindRegex, lines->line, m);
            if(qtd>0){
                LineAtu->endPerformLine = lineNumber+1;
                performLine=-1;
            }
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



int fileNameCompare(char * fileOne, char * fileTwo){
#if defined(_WIN32)
    int result=0;
    char * f1=malloc(strlen(fileOne)+1);
    strcpy(f1, fileOne);
    char * f2=malloc(strlen(fileTwo)+1);
    strcpy(f2, fileTwo);
    result = strcmp(toUpper(f1),toUpper(f2));
    free(f1);
    free(f2);
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

