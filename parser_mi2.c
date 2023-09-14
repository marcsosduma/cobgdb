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
//#define DEBUG 

char octalMatch[] = "^[0-7]{3}";
char outOfBandRecordRegex[] = "^(([0-9]*|undefined)([\\*\\+\\=])|([\\~\\@\\&]))";
char resultRecordRegex[] = "^([0-9]*)\\^(done|running|connected|error|exit)";
char newlineRegex[] = "^\r\n?";
char variableRegex[] = "^([a-zA-Z_\\-][a-zA-Z0-9_\\-]*)";
char asyncClassRegex[] = "^([a-zA-Z0-9_\\-]*),";

char m[10][512];
char * pathRegex = "^\\.?([a-zA-Z_\\-][a-zA-Z0-9_\\-]*)";
char * indexRegex = "^\\[([0-9]+)\\]($|\\.)";


void parseValue(char * output, ST_TableValues * values);
struct ST_TableValues * parseResult(char * output);
struct ST_TableValues * parseCommaResult(char * output);
struct ST_TableValues * parseCommaValue(char * output);

char * asyncRecordTypeOrg[] = { "*","+","="};
char * asyncRecordTypeDest[]= { "exec","status","notify"};
char * asyncRecordType(char * type){
    for(int a=0;a<3;a++){
        if(strcmp(type, asyncRecordTypeOrg[a])==0) 
            return strdup(asyncRecordTypeDest[a]);
    }
    return NULL;
}
char * streamRecordTypeOrg[] = { "~","@","&"};
char * streamRecordTypeDest[]= { "console","target","log"};
char * streamRecordType(char * type){
    for(int a=0;a<3;a++){
        if(strcmp(type, streamRecordTypeOrg[a])==0) 
         return strdup(streamRecordTypeDest[a]);
    }
    return NULL;
}

ST_TableValues * newTableValues(){
    ST_TableValues * values = malloc(sizeof(ST_TableValues));
    values->next=NULL;
    values->array=NULL;
    values->next_array=NULL;
    values->value=NULL;
    values->key = NULL;
    return values;
}

char * parseString(char * str){
    char chrUTF8[4];
    char * ret = malloc(strlen(str)*4);
    strcpy(ret,"");
    if(str[0]!='"' || str[strlen(str)-1]!='"')
        return "";
    int tot=strlen(str)-2;
    subString(str,1, strlen(str)-2, str);
    str[tot]='\0';
    int bufIndex = 0;
    boolean escaped = FALSE;
    int x=0;
    int i=0;
    for(int i=0;i<strlen(str);i++){
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
                int qtd=regex(octalMatch, aux, match);
                free(aux);
                if(qtd>0){
                    strncpy(chrUTF8, (char *) &match[0], 3);
                    chrUTF8[3] = '\0';
                    int valUTF8 = strtol(chrUTF8, NULL, 8);
                    ret[x++]= (char)valUTF8;
                    i += 2;
                }else{
                  ret[x++]=str[i];  
                }
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

char * parseCString(char * output){
    if(output[0]!='"')
        return NULL;
    int stringEnd = 1;
    boolean inString = TRUE;
    char * remaining = strdup(output);
    int len=strlen(output);
    subString(output,1, (--len), remaining);
    boolean escaped = FALSE;
    while(inString){
        if(escaped)
            escaped=FALSE;
        else if(remaining[0]=='\\')
            escaped=TRUE;
        else if(remaining[0]=='"')
            inString = FALSE;
        subString(remaining,1, (--len), remaining); 
        stringEnd++;
    }
    char * str = malloc(stringEnd+1);
    subString(output,0, stringEnd, str);
    str[stringEnd]='\0';
    int lenOut= strlen(output);
    subString(output,stringEnd, lenOut-stringEnd, output);
    free(remaining);
    char * aux = parseString(str);
    strcpy(str, aux);
    return str;

} 

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
            struct ST_TableValues * retornArray=NULL;
            if(result->value!=NULL){
                retornArray=result;
            }else{
                retornArray=result->array;        
                free(result);
            }
            while(TRUE){
                struct ST_TableValues * portionValue = parseCommaValue(output);
                if(portionValue!=NULL){
                    if(portionValue->value!=NULL){
                        if(retornArray==NULL) {
                            retornArray=portionValue;
                        }else{
                            struct ST_TableValues * out=retornArray;
                            while(out->next!=NULL) out=out->next;
                            out->next=portionValue;
                        }
                    }else{
                        struct ST_TableValues * out=retornArray;
                        while(out->next_array!=NULL) out=out->next_array;
                        out->next_array=portionValue;
                    }
                }else{
                    subString(output,1, strlen(output)-1, output);
                    break;
                }
            }
            return retornArray;
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
    char * temp = malloc(strlen(output)+1);
    sprintf(temp,"%c%s",canBeValueListChar, output);
    free(output);
    output=temp;
    return NULL;
}

 void parseValue(char * output, struct ST_TableValues * values){
    if(output[0]=='\"'){
        values->value=parseCString(output);
    }else if (output[0] == '{' || output[0] == '['){
        values->array=parseTupleOrList(output);
    }
}

ST_TableValues * parseCommaValue(char * output){
    if(output[0]!=',')
        return NULL;
    subString(output,1, strlen(output)-1, output);
    ST_TableValues * values = newTableValues();
    parseValue(output,values);
    return values;
}

ST_TableValues * parseResult(char * output){
    char match[10][512];
    struct ST_TableValues * values = NULL;
    int qtd=regex(variableRegex, output, match);
    if(qtd>0){
        subString(output,strlen(match[0])+1, strlen(output)-strlen(match[0])-1, output);
        values = newTableValues();
        values->key = malloc(strlen(match[1])+1);
        strcpy(values->key, match[1]);
        parseValue(output, values);
    }
    return values;
}

struct ST_TableValues * parseCommaResult(char * output){
    if(output[0]!=',')
        return NULL;
    subString(output,1, strlen(output)-1, output);
    struct ST_TableValues * values = parseResult(output);
    return values;
}

ST_MIInfo * parseMI(char * out){
    #ifdef DEBUG
    printf("%s\n", out);
    #endif
    char * output = strdup(out);
    char match[10][512];
    ST_MIInfo * MIInfo = malloc(sizeof(ST_MIInfo));
    MIInfo->token= 0;
    MIInfo->outOfBandRecord=NULL;
    MIInfo->resultRecords=NULL;
    int qtd=0;

    ST_OutOfBandRecord * outOfBandRecord= NULL;
    ST_ResultRecords * resultRecords = NULL;
    while(1){
        qtd=regex(outOfBandRecordRegex, output, match);
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
            int qtd2 = regex(asyncClassRegex, output, classMatch);
            subString(output,strlen(classMatch[1]), strlen(output)-strlen(classMatch[1]), output);
            ST_OutOfBandRecord * asyncRecord = malloc(sizeof(ST_OutOfBandRecord));            
            if(outOfBandRecord==NULL) outOfBandRecord = asyncRecord;
            asyncRecord->isStream=FALSE;
            asyncRecord->type = asyncRecordType(match[3]);
            asyncRecord->asyncClass=malloc(strlen(classMatch[1])+1);
            strcpy(asyncRecord->asyncClass, classMatch[1]);
            asyncRecord->output=NULL;
            asyncRecord->content=NULL;
            asyncRecord->next=NULL;
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
    qtd = regex(resultRecordRegex, output, match);
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


void freeTableValues(ST_TableValues * start){
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

void freeParsed(ST_MIInfo * parsed) {
    //return;
    if(parsed==NULL)
        return;    
    if(parsed->outOfBandRecord!=NULL){
        if(parsed->outOfBandRecord->asyncClass!=NULL) free(parsed->outOfBandRecord->asyncClass);
        if(parsed->outOfBandRecord->content!=NULL) free(parsed->outOfBandRecord->content);
        if(parsed->outOfBandRecord->type!=NULL) free(parsed->outOfBandRecord->type);
        if(parsed->outOfBandRecord->output!=NULL){
            freeTableValues(parsed->outOfBandRecord->output);
            free(parsed->outOfBandRecord->output);
        } 
        free(parsed->outOfBandRecord);
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

ST_TableValues * parseMIvalueOf(ST_TableValues * start, char * p, char * key, boolean * find) {
    if(start==NULL)
        return start;    
    char * path = strdup(p);
    int idx = -1;
    ST_TableValues * search=NULL;
    Trim(path);
    while(!*find){
        if(key==NULL){
            if(start==NULL || strlen(path)<1)
                return start;
            int qtd=regex(pathRegex, path, m);
            if(qtd>0){
                subString(path, strlen(m[0]),strlen(path)-strlen(m[0]), path);
                key = strdup(m[1]);
            }else if (path[0]=='@')
            {
                subString(path, 1,strlen(path)-1, path);
                continue;
            }else {
                qtd=regex(indexRegex, path, m);
                if(qtd>0){
                    subString(path, strlen(m[0]),strlen(path)-strlen(m[0]), path);
                    idx=atoi(m[1]);
                    int qtd=regex(pathRegex, path, m);
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
        Trim(path);
    }
    free(path);
    return search;
}
