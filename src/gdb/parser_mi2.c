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
#include <string.h>
#include <stdlib.h>
#include "cobgdb.h"
//#define DEBUG 

char m[10][MAX_MATCH_LENGTH];

void parseValue(char * output, ST_TableValues * values);
struct ST_TableValues * parseResult(char * output);
struct ST_TableValues * parseCommaResult(char * output);
struct ST_TableValues * parseCommaValue(char * output);

char * asyncRecordTypeOrg[] = { "*","+","="};
char * asyncRecordTypeDest[]= { "exec","status","notify"};

// ==========================================================================================
// Convert a GDB asynchronous record symbol to its textual representation.
//
// Supported symbols:
//   * -> exec
//   + -> status
//   = -> notify
//
// Returns a dynamically allocated string that must be freed by the caller.
// Returns NULL if the symbol is not recognized.
// ==========================================================================================
char * asyncRecordType(char * type){
    for(int a=0;a<3;a++){
        if(strcmp(type, asyncRecordTypeOrg[a])==0) 
            return strdup(asyncRecordTypeDest[a]);
    }
    return NULL;
}

char * streamRecordTypeOrg[] = { "~","@","&"};
char * streamRecordTypeDest[]= { "console","target","log"};
// ==========================================================================================
// Convert a GDB stream record symbol to its textual representation.
//
// Supported symbols:
//   ~ -> console
//   @ -> target
//   & -> log
//
// Returns a dynamically allocated string that must be freed by the caller.
// Returns NULL if the symbol is not recognized.
// ==========================================================================================
char * streamRecordType(char * type){
    for(int a=0;a<3;a++){
        if(strcmp(type, streamRecordTypeOrg[a])==0) 
         return strdup(streamRecordTypeDest[a]);
    }
    return NULL;
}

// ==========================================================================================
// Allocate and initialize a ST_TableValues structure.
//
// All fields are initialized to NULL.
//
// Returns a newly allocated structure.
// ==========================================================================================
ST_TableValues * newTableValues(){
    ST_TableValues * values = malloc(sizeof(ST_TableValues));
    values->next=NULL; values->array=NULL; values->next_array=NULL;
    values->value=NULL; values->key = NULL;
    return values;
}

// ==========================================================================================
// Check whether the beginning of a string contains a valid 3-digit
// octal escape sequence.
//
// Example:
//   123
//   377
//
// Returns 1 if a valid octal sequence is found, otherwise -1.
// ==========================================================================================
//char octalMatch[] = "^[0-7]{3}";
int fOctalMatch(char * line, char mm[][MAX_MATCH_LENGTH]){
    if(strlen(line)<3) return -1;
    for(int a=0;a<3;a++){
        if(line[a]>='0' && line[a]<='7'){
            mm[0][a]=line[a];
        }else{
            return -1;
        }
    }
    mm[0][3]='\0';
    return 1;
}

// ==========================================================================================
// Decode a quoted C-style string.
//
// Supported escape sequences:
//   \\  \"  \'  \n  \r  \t  \b  \f  \v
// and octal escape sequences.
//
// Returns a newly allocated decoded string.
// ==========================================================================================
char * parseString(char * str){
    char chrUTF8[4];
    char * ret = malloc(strlen(str)*4+1);
    strcpy(ret,"");
    if(str[0]!='"' || str[strlen(str)-1]!='"'){
        free(ret);
        return strdup("");
    }
    int tot=strlen(str)-2;
    subString(str,1, strlen(str)-2, str);
    str[tot]='\0';
    boolean escaped = FALSE;
    int x=0;
    for(int i=0;i<(int) strlen(str);i++){
        if(escaped){
            if (str[i] == '\\'){
               ret[x++] = '\\';}
            else if (str[i] == '\"'){
                ret[x++] = '\"';}
            else if (str[i] == '\''){
                ret[x++] = '\'';}
            else if (str[i] == 'n'){
                ret[x++] = '\n';}
            else if (str[i] == 'r'){
                ret[x++] = '\r';}
            else if (str[i] == 't'){
                ret[x++] = '\t';}
            else if (str[i] == 'b'){
                ret[x++] = '\b';}
            else if (str[i] == 'f'){
                ret[x++] = '\f';}
            else if (str[i] == 'v'){
                ret[x++] = '\v';}
            else if (str[i] == '0'){
                ret[x++] = '\0';}
            else{                
                char match[10][512];
                char * aux=malloc(strlen(str)+1);
                subString(str,i, strlen(str)-i, aux);
                //int qtd=regex(octalMatch, aux, match);
                int qtd=fOctalMatch(aux, &match[0]);
                if(qtd>0){
                    strncpy(chrUTF8, (char *) &match[0], 3);
                    chrUTF8[3] = '\0';
                    int valUTF8 = strtol(chrUTF8, NULL, 8);
                    ret[x++]= (char)valUTF8;
                    i += 2;
                }else{
                  ret[x++]=str[i];  
                }
                free(aux);
            }
            escaped = FALSE;
        }else{
            if(str[i] == '\\')
                escaped = TRUE;
            else if(str[i] == '"')
                printf("erro");
            else{
                ret[x++]=str[i];
            }
        }
    }
    ret[x]='\0';
    return ret;
}

// ==========================================================================================
// Extract and decode a quoted string from the beginning of a buffer.
//
// The extracted portion is removed from the original buffer.
//
// Example:
//   Input : "hello",next
//   Output: hello
//   Remaining buffer: ,next
//
// Returns a newly allocated decoded string.
// ==========================================================================================
char * parseCString(char * output) {
    if (output[0] != '"') return NULL;

    int stringEnd = 1;
    int escaped = 0;
    while (output[stringEnd] != '\0') {
        if (escaped) {
            escaped = 0;
        } else if (output[stringEnd] == '\\') {
            escaped = 1;
        } else if (output[stringEnd] == '"') {
            stringEnd++;
            break;
        }
        stringEnd++;
    }

    char *str = malloc(stringEnd + 1);
    memcpy(str, output, stringEnd);
    str[stringEnd] = '\0';

    size_t lenResto = strlen(output + stringEnd);
    memmove(output, output + stringEnd, lenResto + 1);

    char *aux = parseString(str); 
    
    free(str); 
    return aux;
}

// ==========================================================================================
// Legacy implementation of parseCString().
//
// Extracts a quoted string from the beginning of the buffer,
// decodes escape sequences and removes the parsed text.
//
// Returns a newly allocated string.
// ==========================================================================================
char * parseCStringOld(char * output){
    //char remaining[8192];
    if(output[0]!='"')
        return NULL;
    int stringEnd = 1;
    boolean inString = TRUE;
    char * remaining = output;
    remaining++;
    boolean escaped = FALSE;
    while(inString){
        if(escaped)
            escaped=FALSE;
        else if(remaining[0]=='\\')
            escaped=TRUE;
        else if(remaining[0]=='"')
            inString = FALSE;
        remaining++;
        stringEnd++;
    }
    char * str = malloc(stringEnd+1);
    subString(output,0, stringEnd, str);
    str[stringEnd]='\0';
    int lenOut= strlen(output);
    subString(output,stringEnd, lenOut-stringEnd, output);
    char * aux = parseString(str);
    if(aux!=NULL){
        strcpy(str, aux);
        free(aux);
    }
    return str;
} 

// ==========================================================================================
// Parse a GDB/MI tuple or list structure.
//
// Supported examples:
//
//   {name="value"}
//   [1,2,3]
//   [{...},{...}]
//
// Returns a tree of ST_TableValues structures.
// ==========================================================================================
struct ST_TableValues * parseTupleOrList(char * output){
    if(output[0] != '{' && output[0] != '[')
        return FALSE;
    boolean canBeValueList = (output[0]=='[');
    subString(output,1, strlen(output)-1, output);
    if(output[0]=='}' || output[0]==']'){
        subString(output,1, strlen(output)-1, output);
        return NULL;
    }
    if(canBeValueList){
        struct ST_TableValues * result = newTableValues();
        result->key=NULL;
        result->array=NULL;
        result->next_array=NULL;
        result->next=NULL;
        result->value=NULL;
        parseValue(output, result);
        if(result->array!=NULL || result->value!=NULL){
            struct ST_TableValues * returnArray=NULL;
            if(result->value!=NULL){
                returnArray=result;
            }else{
                returnArray=result->array;        
                free(result);
            }
            while(TRUE){
                struct ST_TableValues * portionValue = parseCommaValue(output);
                if(portionValue!=NULL){
                    if(portionValue->value!=NULL){
                        if(returnArray==NULL) {
                            returnArray=portionValue;
                        }else{
                            struct ST_TableValues * out=returnArray;
                            while(out->next!=NULL) out=out->next;
                            out->next=portionValue;
                        }
                    }else{
                        struct ST_TableValues * out=returnArray;
                        while(out->next_array!=NULL) out=out->next_array;
                        out->next_array=portionValue;
                    }
                }else{
                    subString(output,1, strlen(output)-1, output);
                    break;
                }
            }
            return returnArray;
        }
    }    
    struct ST_TableValues * result = parseResult( output);
    if(result!=NULL){
        while(TRUE){
                struct ST_TableValues * valuesArray = parseCommaResult(output);
                if(valuesArray!=NULL){
                    struct ST_TableValues * out=result;
                    while(out->next!=NULL) out=out->next;
                    out->next=valuesArray;
                }else{
                    subString(output,1, strlen(output)-1, output);
                    return result;
                }
        }
    }
    char canBeValueListChar = (canBeValueList)?'[':'{';
    char * temp = malloc(strlen(output)+2);
    sprintf(temp,"%c%s",canBeValueListChar, output);
    free(output);
    output=temp;
    return NULL;
}

// ==========================================================================================
// Parse a single GDB/MI value.
//
// Supported value types:
//   - string
//   - tuple
//   - list
//
// Parsed data is stored in the supplied structure.
// ==========================================================================================
void parseValue(char * output, struct ST_TableValues * values){
    if(output[0]=='\"'){
        values->value=parseCString(output);
    }else if (output[0] == '{' || output[0] == '['){
        values->array=parseTupleOrList(output);
    }
}

// ==========================================================================================
// Parse a comma-prefixed value.
//
// Example:
//
//   ,"value"
//
// Returns a parsed ST_TableValues structure or NULL.
// ==========================================================================================
ST_TableValues * parseCommaValue(char * output){
    if(output[0]!=',')
        return NULL;
    subString(output,1, strlen(output)-1, output);
    ST_TableValues * values = newTableValues();
    parseValue(output,values);
    return values;
}

// ==========================================================================================
// Extract and validate a GDB/MI variable name.
//
// Supported characters:
//   a-z A-Z 0-9 _ -
//
// Returns 1 when a valid identifier is found,
// otherwise returns -1.
// ==========================================================================================
//char variableRegex[] = "^([a-zA-Z_\\-][a-zA-Z0-9_\\-]*)";
int fVariableRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
    if(line==NULL) return -1;
    int len=strlen(line);
    if(len<1) return -1;
    int qtt=0;
    char ch = line[0];
    if(!(ch>='a' && ch<='z' ) && !(ch>='A' && ch<='Z') && ch!='_' && ch!='-') return -1;
    int ret = 1;
    int pos=0;
    while(qtt<len){
        ch=line[qtt++];
        if((ch>='a' && ch<='z' ) || (ch>='A' && ch<='Z') || ch=='_' || ch=='-' || (ch>='0' && ch<='9')){
            mm[0][pos++]=ch;
        }else{
            break;
        }
    }
    mm[0][pos]='\0';
    return ret;
}

// ==========================================================================================
// Parse a GDB/MI result entry.
//
// Example:
//
//   frame={...}
//   level="0"
//
// Returns a populated ST_TableValues structure.
// ==========================================================================================
ST_TableValues * parseResult(char * output){
    char match[10][512];
    struct ST_TableValues * values = NULL;
    //int qtd=regex(variableRegex, output, match);
    int qtd = fVariableRegex(output,match);
    if(qtd>0){
        subString(output,strlen(match[0])+1, strlen(output)-strlen(match[0])-1, output);
        values = newTableValues();
        values->key = malloc(strlen(match[0])+1);
        strcpy(values->key, match[0]);
        parseValue(output, values);
    }
    return values;
}

// ==========================================================================================
// Parse a comma-prefixed GDB/MI result entry.
//
// Example:
//
//   ,frame={...}
//
// Returns a populated ST_TableValues structure or NULL.
// ==========================================================================================
struct ST_TableValues * parseCommaResult(char * output){
    if(output[0]!=',')
        return NULL;
    subString(output,1, strlen(output)-1, output);
    struct ST_TableValues * values = parseResult(output);
    return values;
}

// ==========================================================================================
// Parse a GDB/MI out-of-band record prefix.
//
// Supported record types:
//
//   *stopped
//   +download
//   =thread-created
//   ~"console text"
//
// Returns the number of matched elements.
// ==========================================================================================
//char outOfBandRecordRegex[] = "^(([0-9]*|undefined)([\\*\\+\\=])|([\\~\\@\\&]))";
int fOutOfBandRecordRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
   if(line==NULL) return 0;
   int len=strlen(line);
   char ch = line[0];
   int pos=0, ret=0;
   if((ch>='0' && ch<='9') || (ch=='*' || ch=='+' || ch =='=')){
        while(ch>='0' && ch<='9' && pos<len){
            mm[0][pos]=mm[1][pos]=mm[2][pos]=ch;
            ch=line[++pos];
        }
        mm[0][pos]=mm[1][pos]=mm[2][pos]='\0';
        int p1=0;
        if(pos<len){
            ch = line[pos];
            if(ch=='*' || ch=='+' || ch =='='){
                mm[0][pos]=mm[1][pos]=mm[3][p1++]=ch;
                pos++;
                ret=4;
            }
        }
        mm[0][pos]= mm[1][pos]=mm[3][p1]='\0';        
   }else if(ch=='~' || ch=='@' || ch =='&'){
        mm[0][0]=mm[1][0]=ch;
        mm[0][1]=mm[1][1]='\0';
        ret=2;
   }else if(len>=9 && strncmp(line, "undefined", 9)==0){
        strcpy(mm[0],"undefined");
        strcpy(mm[1],"undefined");
        ret=2;
   }
   return ret;
}

// ==========================================================================================
// Parse an asynchronous record class.
//
// Examples:
//
//   stopped,
//   running,
//   thread-created,
//
// Returns the number of matched elements.
// ==========================================================================================
//char asyncClassRegex[] = "^([a-zA-Z0-9_\\-]*),";
int fAsyncClassRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
   if(line==NULL) return -1;
    int len=strlen(line);
    if(len<1) return -1;
    int qtt=0;
    char ch = line[0];
    if(!(ch>='a' && ch<='z' ) && !(ch>='A' && ch<='Z') && ch!='_' && ch!='-') return -1;
    int ret = 0;
    int pos=0, pos1=0;
    while(qtt<len){
        ch=line[qtt++];
        if((ch>='a' && ch<='z' ) || (ch>='A' && ch<='Z') || ch=='_' || ch=='-' || (ch>='0' && ch<='9') || ch==','){
            mm[0][pos++]=ch;
            if(ch!=','){
                mm[1][pos1++]=ch;
            }else{
                ret=2;
                break;
            }
        }else{
            break;
        }
    }
    mm[0][pos]= mm[1][pos1]='\0';
    return ret;
}

// ==========================================================================================
// Parse a GDB/MI result record.
//
// Supported records:
//
//   ^done
//   ^running
//   ^connected
//   ^error
//   ^exit
//
// Returns the number of matched elements.
// ==========================================================================================
//char resultRecordRegex[] = "^([0-9]*)\\^(done|running|connected|error|exit)";
int fResultRecordRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
   char * values[]={"done","running","connected","error","exit"};
   if(line==NULL || line[0]=='\0') return 0;
   int len=strlen(line);
   char ch = line[0];
   int pos=0, pos1=0, ret=0;
   if((ch>='0' && ch<='9') || (ch=='^')){
        while(((ch>='0' && ch<='9') || (ch=='^')) && pos<len){
            mm[0][pos++]=ch;
            if(ch!='^')
                mm[1][pos1++]=ch;
            ch=line[pos];
        }
        mm[0][pos]=  mm[1][pos1]='\0';
        strcpy(mm[2],"");
        for(int x=0;x<5;x++){
            int len = strlen(values[x]);
            int l = strlen(&line[pos]);
            if(l>=len && strncmp(&line[pos], values[x], len)==0){
                strcat(mm[0], values[x]);
                strcat(mm[2], values[x]);
                ret=3;
                break;
            }
        }
   }
   return ret;
}

// ==========================================================================================
// Parse a complete GDB/MI response.
//
// Supports:
//   - result records
//   - async records
//   - stream records
//
// Builds a structured ST_MIInfo tree representing the response.
//
// Returns a newly allocated ST_MIInfo structure.
// ==========================================================================================
ST_MIInfo * parseMI(char * out){
    #ifdef DEBUG
    printf("%s\n", out);
    #endif
    char * output = strdup(out);
    char match[10][512];
    //char match1[10][512];
    ST_MIInfo * MIInfo = malloc(sizeof(ST_MIInfo));
    MIInfo->token= 0;
    MIInfo->outOfBandRecord=NULL;
    MIInfo->resultRecords=NULL;
    int qtd=0;

    ST_OutOfBandRecord * outOfBandRecord= NULL;
    ST_ResultRecords * resultRecords = NULL;
    while(TRUE){
        //qtd=regex(outOfBandRecordRegex, output, match);
        qtd=fOutOfBandRecordRegex(output, match);
        if(qtd==0) break;
        #ifdef DEBUG
        printf("m[0]: %s\n", match[0]);
        printf("m[1]: %s\n", match[0]);
        printf("Tamanho: %d\n", (int) strlen(output));
        #endif
        
        subString(output,strlen(match[0]), strlen(output)-strlen(match[0]), output);

        if (MIInfo->token == 0 && qtd>2 && strcmp(match[2],"undefined")!=0) {
            MIInfo->token = atoi(match[2]);
        }
        if(qtd>3 && strlen(match[3])>0){
            char classMatch[10][512];
            fAsyncClassRegex(output, classMatch);
            subString(output,strlen(classMatch[1]), strlen(output)-strlen(classMatch[1]), output);
            ST_OutOfBandRecord * asyncRecord = malloc(sizeof(ST_OutOfBandRecord));            
            if(outOfBandRecord==NULL) outOfBandRecord = asyncRecord;
            asyncRecord->isStream=FALSE;
            asyncRecord->type = asyncRecordType(match[3]);
            asyncRecord->asyncClass=malloc(strlen(classMatch[1])+1);
            strcpy(asyncRecord->asyncClass, classMatch[1]);
            asyncRecord->output=NULL; asyncRecord->content=NULL; asyncRecord->next=NULL;
            while(TRUE){
                    ST_TableValues * values = parseCommaResult(output);
                    if(values!=NULL){
                        ST_TableValues * out=asyncRecord->output;
                        if(out==NULL){
                            asyncRecord->output=values;
                        }else{
                            while(out->next!=NULL) out=out->next;
                            out->next=values;
                        }
                    }else{
                        break;
                    }
            }
        }else if (qtd>1 && strlen(match[1])>0) {
            struct ST_OutOfBandRecord * streamRecord = malloc(sizeof(struct ST_OutOfBandRecord));            
            if(outOfBandRecord==NULL) outOfBandRecord = streamRecord;
            streamRecord->isStream=TRUE;
            streamRecord->type = streamRecordType(match[1]);
            streamRecord->asyncClass=NULL;
            streamRecord->output=NULL;
            streamRecord->next=NULL;
            streamRecord->content=parseCString(output);
        }
    }
    //qtd = regex(resultRecordRegex, output, match);
    qtd = fResultRecordRegex(output, match);
    if(qtd>0){
        subString(output,strlen(match[0]), strlen(output)-strlen(match[0]), output);
        if (strlen(match[1])>0 && MIInfo->token == 0) {
            MIInfo->token = atoi(match[1]);
        }
        resultRecords = malloc(sizeof(ST_ResultRecords));
        resultRecords->resultClass=malloc(strlen(match[2])+1);
        strcpy(resultRecords->resultClass, match[2]);
        resultRecords->results=NULL;
        while(TRUE){
            struct ST_TableValues * values = parseCommaResult(output);
            if(values!=NULL){
                struct ST_TableValues * out=resultRecords->results;
                if(out==NULL){
                    resultRecords->results=values;
                }else{
                    while(out->next!=NULL) out=out->next;
                    out->next=values;
                }
            }else{
                break;
            }
        }
    }
    if(output!=NULL) {free(output); output=NULL;}
    MIInfo->outOfBandRecord=outOfBandRecord;
    MIInfo->resultRecords=resultRecords;
    return MIInfo;
}

// ==========================================================================================
// Recursively free a ST_TableValues tree.
//
// Releases:
//   - child arrays
//   - sibling nodes
//   - keys
//   - values
//
// Does not free the root pointer itself.
// ==========================================================================================
void freeTableValues(ST_TableValues * start){
        if(start == NULL) return;
        ST_TableValues * sch=start;
        if(sch->next_array!=NULL){
           freeTableValues(sch->next_array);
           if(sch->next_array!=NULL) free(sch->next_array);
        }
        if(sch->array!=NULL){
           freeTableValues(sch->array);
           if(sch->array!=NULL) free(sch->array);
        }
        if(sch->next!=NULL){
           freeTableValues(sch->next);
           if(sch->next!=NULL) free(sch->next);
        }
        if(sch->key!=NULL) free(sch->key);
        if(sch->value!=NULL) free(sch->value);
        sch->key=NULL;
        sch->value=NULL;
}

// ==========================================================================================
// Release all memory allocated by parseMI().
//
// Frees:
//   - out-of-band records
//   - result records
//   - table values
//   - associated strings
//
// Safe to call with NULL.
// ==========================================================================================
void freeParsed(ST_MIInfo * parsed) {
    if(parsed==NULL)
        return;    
    if(parsed->outOfBandRecord!=NULL){
        ST_OutOfBandRecord * outOfBandRecord = parsed->outOfBandRecord;
        while(outOfBandRecord!=NULL){
            if(outOfBandRecord->asyncClass!=NULL) free(outOfBandRecord->asyncClass);
            if(outOfBandRecord->content!=NULL) free(outOfBandRecord->content);
            if(outOfBandRecord->type!=NULL) free(outOfBandRecord->type);
            if(outOfBandRecord->output!=NULL){
                freeTableValues(outOfBandRecord->output);
                free(outOfBandRecord->output);
            } 
            ST_OutOfBandRecord * next = outOfBandRecord->next;
            free(outOfBandRecord);
            outOfBandRecord = next;
        }
    }
    if(parsed->resultRecords!=NULL){
        if(parsed->resultRecords->results!=NULL){
            freeTableValues(parsed->resultRecords->results);
            free(parsed->resultRecords->results);
        }
        if(parsed->resultRecords->resultClass!=NULL) free(parsed->resultRecords->resultClass);
        free(parsed->resultRecords);
    }
    free(parsed);
}

// ==========================================================================================
// Parse a path component used to navigate MI structures.
//
// Examples:
//
//   frame
//   fullname
//   args
//   .local
//
// Returns the number of matched elements.
// ==========================================================================================
//char * pathRegex = "^\\.?([a-zA-Z_\\-][a-zA-Z0-9_\\-]*)";
int fPathRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
   if(line==NULL || line[0]=='\0') return -1;
    int len=strlen(line);
    int qtt=0, ret = 0, idx=0, pos=0, pos1=0, find=0;
    char ch = line[0];
    while(qtt<len){
        ch=line[qtt];
        if((ch>='a' && ch<='z' ) || (ch>='A' && ch<='Z') || ch=='_' 
            || ch=='-' || (ch>='0' && ch<='9' && pos1>0) 
            || (ch=='.' && pos==0)){
            mm[idx][pos++]=ch;
            if(ch!='.'){
                mm[idx+1][pos1++]=ch;
                find++;
            }
            qtt++;
        }else{
            if(find){
                mm[idx][pos]= mm[idx+1][pos1]='\0';
                pos=pos1=find=0;
                idx+=2; ret+=2;
            }else{
                break;
            }
            if(idx==10) break;
        }
    }
    if(find) ret+=2;
    mm[idx][pos]= mm[idx+1][pos1]='\0';
    return ret;
}

// ==========================================================================================
// Parse an array index expression.
//
// Examples:
//
//   [0]
//   [5].
//   [3]$
//
// Returns the number of matched elements.
// ==========================================================================================
//char * indexRegex = "^\\[([0-9]+)\\]($|\\.)";
int fIndexRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
   if(line==NULL) return 0;
    int len=strlen(line);
    if(len<1) return 0;
    char ch = line[0];
    int qtt=1, ret = 0, pos=1, pos1=0, pos2=0;
    if(ch!='[') return 0;
    strcpy(mm[0],"[");
    if(qtt<len){
        ch=line[qtt];
        while(ch>='0' && ch<='9' && qtt<len){
            mm[0][pos++]= mm[1][pos1++]=ch;
            qtt++;
            ch=line[qtt];
        }
        ch=line[qtt];
        if(ch!=']') return 0;
        mm[0][pos++]=ch;
        mm[0][pos]= mm[1][pos1]='\0';
        ret=3;
        if(qtt<len){
            ch=line[++qtt];
            if(ch=='.' || ch=='$'){
                mm[0][pos++]= mm[2][pos2++]=ch;
            }
            mm[0][pos]= mm[2][pos2]='\0';
            qtt++;
        }
    }
    return ret;
}

// ==========================================================================================
// Search a parsed MI tree using a path expression.
//
// Examples:
//
//   frame.args[0].name
//   stack[2].func
//   bkpt.fullname
//
// Traverses nested structures and arrays recursively.
//
// Returns the matching node or an empty node if no match is found.
// ==========================================================================================
ST_TableValues * parseMIvalueOf(ST_TableValues * start, char * p, char * key, boolean * find) {
    if(start==NULL)
        return start;    
    char * path = p;
    int idx = -1;
    ST_TableValues * search=NULL;
    while(!*find){
        while(isSpace(path[0])) path++;
        if(key==NULL){
            if(start==NULL || strlen(path)<1)
                return start;
            //int qtd=regex(pathRegex, path, m);
            int qtd=fPathRegex(path, m);
            if(qtd>0){
                path = path + strlen(m[0]);
                key = strdup(m[1]);
            }else if (path[0]=='@')
            {
                path++;
                continue;
            }else {
                //int qt=regex(indexRegex, path, m);
                int qt=fIndexRegex(path, m);
                if(qt>0){
                    path += strlen(m[0]);
                    idx=atoi(m[1]);
                    fPathRegex(path, m);
                    ST_TableValues * sch=start;
                    while((idx--)>0){
                        sch=sch->array;
                        sch=sch->next_array;
                    }
                    if(sch->array!=NULL){
                        search=parseMIvalueOf(sch->array, path, key, find);
                        if(search!=NULL) break;
                    }   
                    key=NULL;
                }
            }
        }else{
            ST_TableValues * sch=start;
            while(sch!=NULL){
                if(sch->key!=NULL && strcmp(sch->key, key)==0){
                    search=sch;
                    if(strlen(path)<1)
                         *find=TRUE;
                    if(key!=NULL){
                        free(key);
                        key=NULL;
                    }
                    break;
                }
                if(sch->next!=NULL){
                    *find=FALSE;
                    search=parseMIvalueOf(sch->next, path, key, find);
                    if(search!=NULL && search->value!=NULL) break;
                }  
                sch=start;              
                if(sch->array!=NULL){
                    if(sch->array!=NULL){
                       *find=FALSE;
                        search=parseMIvalueOf(sch->array, path, key, find);
                        if(search!=NULL && search->value!=NULL) break;
                    }
                }
                sch=start;
                if(sch->next_array!=NULL){
                    *find=FALSE;
                    search=parseMIvalueOf(sch->next_array->array, path, key, find);
                    if(search!=NULL && search->value!=NULL) break;
                }
                if(search==NULL){
                    search=newTableValues();
                    *find=TRUE;
                    break;
                }
                break;
            }
        }
    }
    return search;
}