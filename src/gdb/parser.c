/*
 * COBGDB GnuCOBOL Debugger:
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

// ==========================================================================================
// Prototypes
// ==========================================================================================
int64_t my_getline(char **restrict line, size_t *restrict len, FILE *restrict fp);
char* toUpper(char* str);
int fileNameCompare(char * fileOne, char * fileTwo);

// =====================================================
// Global Variables
// =====================================================
extern ST_Line * LineDebug;
extern ST_Attribute * Attributes;
extern ST_DebuggerVariable * DebuggerVariable;
extern struct st_cobgdb cob;
extern struct st_cfiles * parsed_cfiles;

// =====================================================
// Local Variables
// =====================================================
ST_Line * LineAtu=NULL;
ST_Attribute * AtualAttrib;
ST_DebuggerVariable * AtuDebuggerVariable;

static char cwd[512]="";
static char cleanedFile[256];
static char version[100]="";
char functionName[512];

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


// ==========================================================================================
// Search for an attribute definition by its internal key.
//
// Performs a linear search through the global Attributes list.
//
// Returns a pointer to the matching attribute or NULL if not found.
// ==========================================================================================
ST_Attribute * GetAttributes(char * key){
    ST_Attribute * searchAttribute = Attributes;
    while(searchAttribute!=NULL){
        if(strcmp(searchAttribute->key, key)==0) break;
        searchAttribute = searchAttribute->next;
    }
    return searchAttribute;
}

// ==========================================================================================
// Locate the first occurrence of a substring inside another string.
//
// Similar to strstr(), but returns the zero-based position of the
// first match instead of a pointer.
//
// Returns the position of the match or -1 if not found.
// ==========================================================================================
int strpos(char *str1, char *str2)
{
    int b=strlen(str2);
    for(int a=0;a<(int)strlen(str1);a++){
        if(strncmp(&str1[a], str2, b)==0){
            return a;
        }
    }
    return -1;
}

// ==========================================================================================
// Convert a hexadecimal COBOL attribute flag string into a numeric mask.
//
// Example:
//
//   "0x0010" -> 0x0010
//
// If the input is invalid or missing, the flag mask is cleared.
// ==========================================================================================
void buildFlags(ST_Attribute * attribute, char * flagStr){  

        // 0xNNN1 0xNNN2 0xNNN4 0xNNN8 0xNN1N 0xNN2N 0xNN4N 0xNN8N 0xN1NN 0xN2NN 0xN4NN 0xN8NN 0x1NNN   
        if(flagStr==NULL || strlen(flagStr)<6){
            attribute->flags = 0;
        } else{
            sscanf(flagStr, "%x", &attribute->flags);
        }
}

// ==========================================================================================
// Create and register a new COBOL field attribute.
//
// Allocates a new ST_Attribute structure, fills all metadata fields,
// converts the flag string to a numeric mask and appends the object
// to the global attribute list.
//
// Returns TRUE on success.
// ==========================================================================================
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
    buildFlags(new_attrib, flagStr);
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
    return TRUE;
}

// ==========================================================================================
// Remove the last attribute from the global attribute list.
//
// Frees all dynamically allocated strings associated with the attribute.
//
// Returns TRUE.
// ==========================================================================================
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
    return TRUE;
}

// ==========================================================================================
// Convert a GnuCOBOL internal type code into a human-readable name.
//
// Example:
//
//   0x10 -> Numeric Display
//
// Returns a pointer to a static string.
// Do not free the returned value.
// ==========================================================================================
char * VariableType(char * type_org){
    char *org[]={"0x00","0x01","0x02","0x10","0x11","0x12","0x13","0x14","0x15","0x16","0x17",
                "0x18","0x19","0x1A","0x1B","0x24","0x20","0x21","0x22","0x23","0x40","0x41",
                "int" ,"cob_u8_t",""};
    char *ret[]={"unknown","group","boolean","numeric","numeric binary","numeric packed","numeric float","numeric double",
                "numeric l double","numeric FP DEC64","numeric FP DEC128","numeric FP BIN32",
                "numeric FP BIN64","numeric FP BIN128","numeric COMP5","numeric edited",
                "alphanumeric","alphanumeric","alphanumeric","alphanumeric edited",
                "national","national edited","integer","group",""};
    int idx=0;  
    while(strlen(org[idx])>0){
        if(strcasecmp(org[idx],type_org)==0) 
            return ret[idx];
        idx++;
    }
    return "";
}

// ==========================================================================================
// Add a debugger variable to the internal variable registry.
//
// Stores all metadata required by the debugger, including:
//
//   - COBOL variable name
//   - generated C variable name
//   - storage identifier
//   - variable type
//   - size information
//   - parent/child relationships
//
// The variable is appended to the internal linked list and becomes
// available for debugger lookups and watch operations.
//
// Returns a pointer to the newly created ST_DebuggerVariable structure.
// ==========================================================================================
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
    return TRUE;
}

// ==========================================================================================
// Attach a child debugger variable to a parent node.
//
// If the parent has no children, the specified variable becomes the
// first child. Otherwise, it is appended to the end of the sibling
// list.
//
// Updates parent-child relationships and marks the parent as
// expandable when its first child is added.
//
// Parameters:
//   father - Parent variable.
//   child  - Child variable to attach.
//
// Returns:
//   Nothing.
// ==========================================================================================
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

// ==========================================================================================
// Create a fully-qualified debugger variable identifier.
//
// Format:
//
//   <class>.<value>
//
// Example:
//
//   MAIN.CUSTOMER-NAME
//
// Returns a newly allocated string that must be freed by the caller.
// ==========================================================================================
char *  DebuggerVariable_Set(char * classe, char * value ){
    char tmp1[256];
    sprintf(tmp1,"%s.%s", classe, value );
    int size_var = strlen(tmp1);
    char * toSet = malloc(size_var+1);
    strcpy(toSet, tmp1);
    return toSet;
}

// ==========================================================================================
// Create a fully-qualified child variable identifier.
//
// Format:
//
//   <class>.<parent>.<child>
//
// Example:
//
//   MAIN.CUSTOMER.NAME
//
// Parent and child names are converted to uppercase.
//
// Returns a newly allocated string that must be freed by the caller.
// ==========================================================================================
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

// ==========================================================================================
// Search for a data storage variable using its internal identifier.
//
// Returns the matching debugger variable or NULL if not found.
// ==========================================================================================
ST_DebuggerVariable * GetDataSotorage(char * key){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){
        if(search->dataSotorage!=NULL && strcmp(search->dataSotorage, key)==0) break;
        search = search->next;
    }
    return search;
}

// ==========================================================================================
// Search for a debugger variable using its fully-qualified COBOL name.
//
// Returns the matching variable or NULL if not found.
// ==========================================================================================
ST_DebuggerVariable * GetVariableByCobol(char * key){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){
        if(search->variablesByCobol!=NULL && strcmp(search->variablesByCobol, key)==0) break;
        search = search->next;
    }
    return search;
}

// ==========================================================================================
// Search for a debugger variable using its generated C variable name.
//
// Returns the matching variable or NULL if not found.
// ==========================================================================================
ST_DebuggerVariable * GetVariableByC(char * key){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){
        if(search->variablesByC!=NULL && strcmp(search->variablesByC, key)==0) break;
        search = search->next;
    }
    return search;
}
// DebuggerVariable - END

// ==========================================================================================
// Add a COBOL-to-C source mapping entry.
//
// Stores the relationship between a COBOL source line and the
// corresponding generated C source line.
//
// Returns TRUE.
// ==========================================================================================
int PushLine(char * filePathCobol, int lineCobol, char * filePathC, int lineC, int lineProgramExit, int isCall, int * varEntry){
    ST_Line* new_line = (ST_Line*) malloc(sizeof(ST_Line));
    new_line->next=NULL;
    new_line->before=NULL;
    new_line->fileCobol = (char *) malloc(strlen(filePathCobol)+1);
    strcpy(new_line->fileCobol, filePathCobol);
    new_line->lineCobol = lineCobol;
    new_line->fileC = (char *) malloc(strlen(filePathC)+1);
    strcpy(new_line->fileC, filePathC);
    new_line->show_var = NULL;
    new_line->lineC = lineC;
    new_line->endPerformLine = -1;
    new_line->isCall = isCall;
    if(lineCobol>=*varEntry && *varEntry>=0){
        new_line->isEntry = TRUE;
        *varEntry=-1;
    }else{
        new_line->isEntry = FALSE;
    }
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
    return TRUE;
}

// ==========================================================================================
// Remove the most recently added source mapping entry.
//
// Releases all resources associated with the mapping.
//
// Returns TRUE.
// ==========================================================================================
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
    return TRUE;
}

// ==========================================================================================
// Associate debugger-visible variables with the current source line.
//
// The supplied variable names are resolved against the global debugger
// variable list and attached to the current line entry.
//
// Returns TRUE.
// ==========================================================================================
int PushVarLine(char * function, char var[][MAX_MATCH_LENGTH]){
    if(LineAtu!=NULL){
        int idx=0;
        struct st_show_var * var_first=NULL;
        struct st_show_var * var_before=NULL;
        while(strcmp(var[idx],"")!=0){
            struct st_show_var * new_show= malloc(sizeof(struct  st_show_var));
            ST_DebuggerVariable * search = DebuggerVariable;
            while(search!=NULL){
                if(strcmp(search->cName,var[idx])==0 && strcmp(search->functionName, function)==0){
                    new_show->var = search;
                    new_show->next=NULL;
                    if(var_first==NULL){
                        var_first = new_show;
                        var_before = new_show;
                    }else{
                        var_before->next = new_show;
                    }
                    var_before = new_show;
                    break;
                }
                search=search->next;
            }
            idx++;
        }
        if(var_first!=NULL){
            if(LineAtu->show_var==NULL){
                LineAtu->show_var=var_first;
            }else{
                struct st_show_var * show= LineAtu->show_var;
                while(show->next!=NULL) show=show->next;
                show->next=var_first;
            }        
        }
    }
    return TRUE;
}

// ==========================================================================================
// Load a generated C source file into memory.
//
// Reads every line and stores the content in a doubly linked list.
//
// Returns TRUE on success.
// ==========================================================================================
int readCFile(struct ST_CFILE * program) {
    FILE *fp = fopen(program->name_cfile, "r");
    if(fp == NULL) {
        char errorMsg[512];
        sprintf(errorMsg, "Unable to open file! %s", program->name_cfile);
        perror(errorMsg);
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
    return TRUE;
}

// ==========================================================================================
// Release all memory associated with a loaded C source file.
//
// Frees every stored source line and resets the file structure.
// ==========================================================================================
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

// ==========================================================================================
// Remove the ".c" extension from a generated C file name.
//
// Example:
//
//   sample.c -> sample
// ==========================================================================================
void cleanedCFileName(char *dst, char *filename) {
    int len = strlen(filename);
    memcpy(dst, filename, len-2);
    dst[len - 2] = 0;
}

// ==========================================================================================
// Detect and extract the original COBOL source file name from a
// generated C source line.
//
// Returns TRUE if a COBOL file reference is found.
// ==========================================================================================
boolean fileCobolRegex(struct st_parse * line_parsed, int qtt_tk, char * fileName){
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
        int p1 = pos+3;
        strcpy(fileName,"");
        while(p1<qtt_tk){
            ret=TRUE;
            m=tk_val(line_parsed, qtt_tk, p1++);
            if(m->size==2 && strncmp(m->token,"*/", 2)==0) {break;}
            int len = strlen(fileName);
            strncat(fileName, m->token,m->size);
            fileName[(len+m->size)]='\0';
        }
        break;
    }
    return ret;
}

//char procedureRegex[] = "/\\*\\sLine:\\s([0-9]+)(\\s+:\\sEntry\\s)?";
boolean procedureRegex(struct st_parse *line_parsed, int qtt_tk, char * numberLine, boolean * isPerform, int * varEntry){
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
        if(m->size==5 && strncasecmp(m->token,"Entry", 5)==0){
            if(*varEntry<0){
                char temp[50];
                strncpy(temp, m1->token, m1->size);
                temp[m1->size]='\0';
                *varEntry = atoi(temp);
            } 
            break;
        }
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
boolean procedureFixRegex(struct st_parse *line_parsed, int qtt_tk, char * numberLine){
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
boolean attributeRegex(struct st_parse *line_parsed, int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    int idx=0;
    boolean ret = FALSE;
    struct st_parse * m;
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
boolean dataStorageRegex(struct st_parse *line_parsed, int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    int idx=0;
    boolean ret = FALSE;
    struct st_parse * m;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=6 || strncasecmp(m->token,"static", 6)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);        
        strcpy(mm[idx],""); // TODO: test
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
            strcpy(mm[idx++],"");
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
boolean fieldRegex(struct st_parse *line_parsed, int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int pos=0;
    int idx=0;
    boolean ret = FALSE;
    struct st_parse * m;
    int qtt = 0;
    while(qtt<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size!=6 || strncasecmp(m->token,"static", 6)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+1);        
        if(m->size!=9 || strncasecmp(m->token,"cob_field", 9)!=0){ break;}
        m=tk_val(line_parsed, qtt_tk, pos+2);        
        strcpy(mm[idx],"");
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
boolean functionRegex(struct st_parse *line_parsed, int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
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
boolean versionRegex(struct st_parse *line_parsed, int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
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
boolean programExit(struct st_parse *line_parsed, int qtt_tk){
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
boolean fileIncludeRegex(struct st_parse *line_parsed, int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
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

// ==========================================================================================
// Extract debugger-visible variable references from a parsed C line.
//
// Recognizes generated variable names such as:
//
//   b_xxxx
//   f_xxxx
//
// Returns TRUE if at least one variable is found.
// ==========================================================================================
// variables -> b_xxxx or f_xxxx
boolean showVarRegex(struct st_parse *line_parsed, int qtt_tk, char mm[][MAX_MATCH_LENGTH]){
    int idx=0;
    int pos=0;
    boolean ret = FALSE;
    struct st_parse * m;
    while(pos<qtt_tk){
        m=tk_val(line_parsed, qtt_tk, pos);        
        if(m->size<2){ pos++; continue;}
        if(strncmp(m->token,"f_", 2)!=0 && strncmp(m->token,"b_", 2)!=0){pos++; continue;}
        strncpy(mm[idx],m->token, m->size);
        mm[idx++][m->size]='\0';
        pos++;
        ret=TRUE;
    }
    strcpy(mm[idx],"");
    return ret;
}

// ==========================================================================================
// Add a source file to the internal parsed-file registry.
//
// Prevents the same generated C source from being parsed multiple times.
//
// Returns:
//   1  - file added
//   0  - file already exists
//  -1  - error
// ==========================================================================================
int add_cfile(const char *file_name) {
    if (file_name == NULL || *file_name == '\0')
        return -1;
    struct st_cfiles *curr = parsed_cfiles;
    struct st_cfiles *prev = NULL;
    while (curr) {
        if (strcmp(curr->file, file_name) == 0)
            return 0;  // File already in the list
        prev = curr;
        curr = (struct st_cfiles *) curr->next;
    }
    struct st_cfiles *node = (struct st_cfiles *) malloc(sizeof(struct st_cfiles));
    if (!node)
        return -1;
    node->file = strdup(file_name);
    if (!node->file) {
        free(node);
        return -1;
    }
    node->next = NULL;
    // Insert node at the end of the list
    if (prev)
        prev->next = node;
    else
        parsed_cfiles = node;
    return 1;
}

// ==========================================================================================
// Parse a generated GnuCOBOL C source file.
//
// Extracts:
//
//   - source line mappings
//   - COBOL file references
//   - debugger variables
//   - attributes
//   - function information
//
// Builds the complete internal debugger metadata structure.
//
// Returns TRUE on success.
// ==========================================================================================
int parser(char * file_name, int fileN){
    char m[10][512];
    char fileCobol[1024];
    char fileC[512];
    char basename[512];
    int performLine = -1;
    struct ST_CFILE program_cfile;
    boolean isVersion2_2_or_3_1_1=FALSE;
    char *path = NULL;    
    char tmp[300];
    int lineProgramExit = 1;
    int varEntry=-1;

    //Check if the file has already been parsed
    if(add_cfile(file_name)==0)
        return TRUE;

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
        if(fileN == 0){
            cleanedCFileName(cleanedFile, basename);
        }
    }
    char sep = '/';

    sprintf(file_name,"%s%c%s",cwd,sep,basename);
    if(!file_exists(file_name))
        return 0;

    strcpy(fileC, file_name);
    strcpy(program_cfile.name_cfile, file_name);
    
    readCFile(&program_cfile);
    Clines * lines = program_cfile.clines;  
    boolean bfileCobolRegex=FALSE;
    int cap = 50;
    struct st_parse *line_parsed = malloc(cap * sizeof(struct st_parse));
    int qtt_tk;
    //gotoxy(1,1); noColor();
    for(int lineNumber=0;lineNumber<program_cfile.qtd_clines;lineNumber++){
        #ifdef debug
        printf("%s", lines->line);
        #endif

        qtt_tk=0;
        lineParse(lines->line, &line_parsed, &qtt_tk, &cap);
        if(!bfileCobolRegex){            
            bfileCobolRegex = fileCobolRegex(line_parsed, qtt_tk, &tmp[0]);
            if(bfileCobolRegex){
                if(isAbsolutePath(tmp)==0 || isAbsolutePath(tmp)==2){
                    char tmp1[512];
                    normalizePath(tmp);
                    strcpy(tmp1,getFileNameFromPath(tmp));
                    sprintf(fileCobol,"%s%c%s",cwd,sep,tmp1);
                }else{
                    strcpy(fileCobol, tmp);
                }            
                if(fileN==0 && strcmp(cob.name_file,"")==0){
                    strcpy(cob.name_file, fileCobol); // To assign the initial COBOL file
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

        if(LineAtu!=NULL){
            //qtd=regex(functionRegex1, lines->line, m);
            boolean bshowVarRegex = showVarRegex(line_parsed, qtt_tk, m);
            if(bshowVarRegex){
                PushVarLine(functionName, m);
            }
        }

        //char procedureRegex[] = "/\\*\\sLine:\\s([0-9]+)(\\s+:\\sEntry\\s)?";
        boolean isPerform=FALSE;
        boolean bprocedureRegex = procedureRegex(line_parsed, qtt_tk, &tmp[0], &isPerform, &varEntry);
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
                PushLine(fileCobol, lineCobol, fileC, lineNumber + 2, lineProgramExit, isCall, &varEntry);
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
                parser(dest, fileN+1);
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
    free(line_parsed);
    return TRUE;
}

// ==========================================================================================
// Check whether a COBOL source line exists in the mapping table.
//
// Returns the corresponding generated C line number,
// or -1 if the line is not mapped.
// ==========================================================================================
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

// ==========================================================================================
// Locate a source mapping entry using a COBOL file name and line number.
//
// Returns the matching mapping entry or NULL if not found.
// ==========================================================================================
ST_Line * getLineC(char * fileCobol, int lineCobol){
    ST_Line * search = LineDebug;
    while(search!=NULL){
        if(strlen(search->fileCobol)>0 && fileNameCompare(fileCobol,search->fileCobol)==0 && search->lineCobol==lineCobol) break;
        search = search->next;
    }
    return search;
}

// ==========================================================================================
// Locate the COBOL source line associated with a generated C line.
//
// Returns the matching source mapping entry or NULL if not found.
// ==========================================================================================
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

// ==========================================================================================
// Retrieve the detected GnuCOBOL version string.
//
// The version is copied into the supplied buffer.
// ==========================================================================================
void getVersion(char vrs[]){
    strcpy(vrs, version);
}

// ==========================================================================================
// Count all registered source mapping entries.
//
// Returns the total number of mapped source lines.
// ==========================================================================================
int getLinesCount(){
    int tot = 0;
    ST_Line * search = LineDebug;
    while(search!=NULL){
        tot++;
        search = search->next;
    }
    return tot;
}

// ==========================================================================================
// Count all registered debugger variables.
//
// Returns the total number of mapped variables.
// ==========================================================================================
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

// ==========================================================================================
// Search for a debugger variable using its COBOL identifier.
//
// Returns the matching variable or NULL if not found.
// ==========================================================================================
ST_DebuggerVariable * getVariableByCobol(char * cobVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL && strcmp(search->variablesByCobol, cobVar)==0)
            break;
        search = search->next;
    }
    return search;
}

// ==========================================================================================
// Return the head of the debugger variable list.
//
// Useful for iterating through all registered variables.
// ==========================================================================================
ST_DebuggerVariable * getVariablesByCobol(){
    ST_DebuggerVariable * search = DebuggerVariable;
    return search;
}

// ==========================================================================================
// Search for a debugger variable using its generated C name.
//
// Returns the matching variable or NULL if not found.
// ==========================================================================================
ST_DebuggerVariable * getVariableByC(char * cVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL && strcmp(search->variablesByC, cVar)==0)
            break;
        search = search->next;
    }
    return search;
}

// ==========================================================================================
// Search for a COBOL variable inside a specific program or function.
//
// Performs a case-insensitive lookup using the fully-qualified COBOL
// variable path.
//
// Returns the matching variable or NULL.
// ==========================================================================================
ST_DebuggerVariable * findVariableByCobol(char * functionName, char * cobVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    char tmp1[256];
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL){
            sprintf(tmp1,"%s%c",functionName,'.');
            if(my_strcasestr(search->variablesByCobol,tmp1)>=0){
                sprintf(tmp1,"%c%s",'.',cobVar);
                if(my_strcasestr(search->variablesByCobol,tmp1)>=0){
                    break;
                }
            }
        }
        search = search->next;
    }
    return search;
}

// ==========================================================================================
// Check whether a COBOL variable exists in the debugger metadata.
//
// Returns TRUE if found, otherwise FALSE.
// ==========================================================================================
int isVariableByCobol(char * cobVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    int ret=FALSE;
    char tmp1[256];
    sprintf(tmp1,".%s",cobVar);
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL){
            if(my_strcasestr(search->variablesByCobol,tmp1)>=0){
                ret=TRUE;
                break;
            }
        }
        search = search->next;
    }
    return ret;
}

// ==========================================================================================
// Search for a field within a COBOL group variable.
//
// The search starts at the supplied variable node and continues forward.
//
// Returns the matching field or NULL if not found.
// ==========================================================================================
ST_DebuggerVariable * findFieldVariableByCobol(char * functionName, char * cobVar, ST_DebuggerVariable * start){
    ST_DebuggerVariable * search = start;
    char tmp1[256];
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL){
            sprintf(tmp1,"%s%c",functionName,'.');
            if(my_strcasestr(search->variablesByCobol,tmp1)>=0){
                sprintf(tmp1,"%c%s",'.',cobVar);
                if(my_strcasestr(search->variablesByCobol,tmp1)>=0){
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

// ==========================================================================================
// Find a debugger variable by its generated C variable name.
// Searches the global debugger variable list and returns the first
// variable whose C identifier matches the specified name.
// This function is typically used when mapping a generated C variable
// back to its corresponding COBOL debugger variable.
// Example:
//   COBOL variable : CUSTOMER-NAME
//   C variable     : f_123_customer_name
// ST_DebuggerVariable *var =
//       getShowVariableByC("f_123_customer_name");
// Parameters:
//   cVar - Generated C variable name to search for.
// Returns:
//   Pointer to the matching debugger variable.
//   NULL if no matching variable is found.
// ==========================================================================================
ST_DebuggerVariable * getShowVariableByC(char * cVar){
    ST_DebuggerVariable * search = DebuggerVariable;
    while(search!=NULL){        
        if(search->variablesByCobol!=NULL && strcmp(search->variablesByC, cVar)==0)
            break;
        search = search->next;
    }
    return search;
}

// ==========================================================================================
// Compare two file names using the platform-specific rules.
//
// Windows:
//   Case-insensitive comparison.
//
// Linux:
//   Case-sensitive comparison.
//
// Returns:
//   0 if equal
//   <0 if fileOne < fileTwo
//   >0 if fileOne > fileTwo
// ==========================================================================================
int fileNameCompare(char * fileOne, char * fileTwo){
#if defined(_WIN32)
    int result=0;
    result = strcasecmp(fileOne,fileTwo);
    return result;
#elif defined(__linux__)
    return strcmp(fileOne,fileTwo);
#endif // Windows/Linux 
}

// ==========================================================================================
// Build the COBOL-to-C source mapping database.
//
// Parses generated C files and extracts:
//
//   - source line mappings
//   - variable definitions
//   - attributes
//   - procedure information
//
// Returns TRUE on success.
// ==========================================================================================
void SourceMap(char fileGroup[][512]){
    int q = 0;
    LineDebug = NULL;
    Attributes = NULL;
    DebuggerVariable = NULL;

    strcpy(cob.name_file,""); // To assign the initial COBOL file
    while(strlen(fileGroup[q])>0){
        strcpy(cwd,"");
   	    //printf("\n\n Arquivo: %s\n",fileGroup[q]);
        //while(key_press(MOUSE_OFF)<=0);
        parser(fileGroup[q], 0);
        q++;

    }
}
