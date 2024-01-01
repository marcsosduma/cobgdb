/* 
   This code is based on the GnuCOBOL Debugger extension available at: 
   https://github.com/OlegKunitsyn/gnucobol-debug
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include "cobgdb.h"

extern char m[10][512];

#define ZERO_SIGN_CHAR_CODE 112

char * formatNumber(char *valueStr, int fieldSize, int scale, int isSigned) {
    char *value = strdup(valueStr);
    int isNegative = 0;
    if (value[0] == '-' || value[0] == '+') {
        isNegative = (value[0] == '-');
        memmove(value, value + 1, strlen(value));  // Remove the signal
    }
    char *wholeNumber = strtok(value, ".");
    char *decimals = strtok(NULL, ".");
    if (decimals == NULL) {
        decimals = "";
    }
    if (scale < 0) {
        decimals = "";
        int len = strlen(wholeNumber);
        if (len > abs(scale)) {
            wholeNumber[len - abs(scale)] = '\0';  // Trunc right
        }
    } else if (scale > fieldSize) {
        int len = strlen(decimals);
        if (len > (scale - fieldSize)) {
            decimals += len - (scale - fieldSize);  // Trunc left
        } else {
            decimals = "";
        }
    }
    char *valueResult = malloc(fieldSize + 1);
    if(strlen(wholeNumber)>fieldSize){
        strncpy(valueResult, wholeNumber, fieldSize);
        valueResult[fieldSize]='\0';
    }else{
        strcpy(valueResult, wholeNumber);
    }
    strcat(valueResult, decimals);

    int diff = fieldSize - strlen(valueResult);
    if (diff > 0) {
        char append[diff + 1];
        memset(append, '0', diff);
        append[diff] = '\0';
        if (fieldSize - scale < 0) {
            strcat(valueResult, append);
        } else {
            memmove(valueResult + diff, valueResult, strlen(valueResult) + 1);
            memcpy(valueResult, append, diff);
        }
    } else if (diff < 0) {
        if (fieldSize - scale < 0) {
            int len = strlen(valueResult);
            memmove(valueResult, valueResult + abs(diff), len - abs(diff) + 1);
        } else {
            int len = strlen(valueResult);
            memmove(valueResult, valueResult + abs(diff), len - abs(diff) + 1);
        }
    }
    if (isSigned && isNegative) {
        char sign[2];
        sign[0] = valueResult[strlen(valueResult) - 1] + ZERO_SIGN_CHAR_CODE;
        sign[1] = '\0';
        valueResult[strlen(valueResult) - 1] = '\0';
        strcat(valueResult, sign);
    }
    char * result = strdup(valueResult);
    free(value);
    free(valueResult);
    return result;
}


void formatNumberParser(char *valueStr, int fieldSize, int sc) {
    char *value = strdup(valueStr);
    char *originalLocale = setlocale(LC_NUMERIC, NULL);

    int scale = abs(sc);    
    setlocale(LC_NUMERIC, "C");
    if(fieldSize<0)
        fieldSize=strlen(valueStr)-2;
    if (value[0] == '"') {
        memmove(value, value + 1, fieldSize+1);  // Remove comma
        value[fieldSize]='\0';        
        char *endPtr;
        int signCharCode = value[strlen(value) - 1];
        char sign[2];
        sign[0] = '\0';
        if (signCharCode >= ZERO_SIGN_CHAR_CODE) {
            sign[0] = '-';
            sign[1] = '\0';
            value[strlen(value)-1] -= 64;
        }
        if (sc < 0) {
            int diff = scale;
            char suffix[diff+1];
            memset(suffix, '0', diff);
            suffix[diff] = '\0';
            strcat(value, suffix);
        }else if (strlen(value) < sc) {
            int diff = scale - strlen(value);
            char prefix[diff+1];
            memset(prefix, '0', diff);
            prefix[diff] = '\0';
            strcat(prefix, value);
            strcpy(value, prefix);
        } 
        char *wholeNumber = malloc(scale+fieldSize + 3);
        char *decimals = malloc(scale + fieldSize + 3);
        wholeNumber[0] = '\0';
        decimals[0] = '\0';
        if (strlen(value) >= abs(scale)) {
            strcpy(wholeNumber, value);
            if (sc > 0) {
                wholeNumber[strlen(wholeNumber) - abs(scale)] = '\0';
                strcpy(decimals, value + strlen(value) - abs(scale));
            }
        }
        char *numericValue = malloc(fieldSize + scale + 3);  // +3 for . and signal
        numericValue[0] = '\0';
        strcat(numericValue, sign);
        strcat(numericValue, wholeNumber);
        if (strlen(decimals) > 0) {
            strcat(numericValue, ".");
            strcat(numericValue, decimals);
        }
        if(sc>0)
            sprintf(valueStr, "%.*f", scale, atof(numericValue));
        else
            sprintf(valueStr, "%.*f", 0, atof(numericValue));
        free(wholeNumber);
        free(decimals);
        free(numericValue);
    }
    free(value);
    setlocale(LC_NUMERIC, originalLocale);
}

//char repeatTimeRegex[] = "(\"\\,\\s|^)\\'(\\s|0)\\'\\s\\<repeats\\s(\\[0-9]+)\\stimes\\>";
int fRepeatTimeRegex(char * line, char mm[][MAX_MATCH_LENGTH]){
    if(line==NULL) return 0;
    int len=strlen(line);
    if(len<8) return 0;
    if(strstr(line,"<repeats")==NULL) return 0;
    if(strstr(line,"times>")==NULL) return 0;
    char ch = line[0];
    int pos=0, pos1=0, pos2=0;
    while(ch==' ' || ch==','){
        pos++;
        ch = line[pos];
    }
    if(ch=='\'' || ch=='"'){
        pos++;
        ch = line[pos];
        while(ch!='\'' && ch!='"'){
            mm[0][pos1++]=ch;
            ch = line[++pos];
        }
        mm[0][pos1]='\0';
    }
    ch=line[++pos];
    while(ch==' ') ch = line[++pos];
    len = strlen(&line[pos]);
    if(len<8 || strncmp(&line[pos],"<repeats",8)!=0) return 0;
    pos+=8;
    ch=line[pos];
    while(ch!=' ') ch = line[++pos];
    ch=line[++pos];
    if(ch<'0' || ch>'9') return 0;
    do{
        mm[1][pos2++]=ch;
        ch=line[++pos];
    }while(ch>='0' && ch<='9');
    mm[1][pos2]='\0';
    return 2;
}

char *CobolFieldDataParser(char *valueStr) {
    char *value = strdup(valueStr);
    char *toFree=value;

    if (strchr(value, ' ') == NULL) {
        free(value);
        return NULL;
    }
    value = strchr(value, ' ') + 1;
    if (value[0] == '<') {
        if (strchr(value, ' ') == NULL) {
            free(toFree);
            return NULL;
        }
        value = strchr(value, ' ') + 1;
    }
    //int qtd = regex(repeatTimeRegex, value, m);
    int qtd = fRepeatTimeRegex(value, m);
    if (qtd>0) {
        int size = atoi(m[1]);
        char * replacement = malloc(size+3);
        strcpy(replacement,"\"");
        for(int i=0;i<size;i++){
            strcat(replacement,m[0]);            
        }
        strcat(replacement,"\"");
        free(value);
        value = replacement;
    }
    return value;
}

char* formatAlpha(const char* valueStr, int fieldSize) {
    const char* value = valueStr;
    if (value[0] == '"') {
        value = &value[1];
    }
    size_t length = strlen(value);
    if (value[length - 1] == '"') {
        length--;
    }
    int diff = fieldSize - length;
    if (diff > 0) {
        char* formattedValue = (char*)malloc(fieldSize + 1);
        strncpy(formattedValue, value, length);
        for (int i = 0; i < diff; i++) {
            formattedValue[length + i] = ' ';
        }
        formattedValue[fieldSize] = '\0';
        return formattedValue;
    } else if (diff < 0) {
        char* formattedValue = (char*)malloc(fieldSize + 1);
        strncpy(formattedValue, value, fieldSize);
        formattedValue[fieldSize] = '\0';
        return formattedValue;
    } else {
        char* formattedValue = strdup(value);
        return formattedValue;
    }
}

char* formatValueVar(char* valueStr, int fieldSize, int scale, char* type) {
    if (!valueStr) return NULL;
    if (strcmp(type, "numeric") == 0) {
        return formatNumber(valueStr, fieldSize, scale, 0);
    } else if (
        strcmp(type, "group") == 0 ||  strcmp(type, "numeric edited") == 0 ||
        strcmp(type, "alphanumeric") == 0 || strcmp(type, "alphanumeric edited") == 0 ||
        strcmp(type, "national") == 0 || strcmp(type, "national edited") == 0
    ){
        return formatAlpha(valueStr, fieldSize);
    }else if (strcmp(type, "integer") == 0 ) {
        return strdup(valueStr);
    }else {
        return formatAlpha(valueStr, fieldSize);
    }
    return strdup(valueStr);
}


char* dbStringValues(char* valueStr, int fieldSize) {
    char* value = valueStr;
    int shift = 0;
    if (value[0] == '"') {
        shift = 1;
    }
    int size = (fieldSize + shift) < strlen(valueStr) ? (fieldSize + shift) : strlen(valueStr);
    char* result = malloc(size + 3);  // +3 for 2 x double quote and null character
    snprintf(result, size + 3, "\"%.*s\"", size-shift, value + shift);
    return result;
}

char* debugParse(char* valueStr, int fieldSize, int scale, char* type) {
    if (!valueStr) return NULL;
    if (strncmp(valueStr, "0x", 2) == 0) valueStr=CobolFieldDataParser(valueStr);
    if (!valueStr) return NULL;
    if (strcmp(type, "numeric") == 0) {
        formatNumberParser(valueStr, fieldSize, scale);
        return valueStr;
    } else if (
        strcmp(type, "numeric edited") == 0 ||
        strcmp(type, "alphanumeric") == 0 ||
        strcmp(type, "alphanumeric edited") == 0 ||
        strcmp(type, "national") == 0 ||
        strcmp(type, "national edited") == 0
    ) {
        // AlphanumericValueParser parse to do
        return dbStringValues(valueStr, fieldSize);
    }else if (
        strcmp(type, "integer") == 0 ||
        strcmp(type, "group") == 0
    ) {
        return strdup(valueStr);
    } else {
        return strdup(valueStr);
    }
    return strdup(valueStr);
}

char* parseUsage(char* valueStr, char* type) {
    if (!valueStr || strlen(valueStr) == 0) {
        return NULL;
    }
    if (strncmp(valueStr, "0x", 2) == 0) {
       valueStr=CobolFieldDataParser(valueStr);
    }
    if (!valueStr || strlen(valueStr) == 0) {
        return NULL;
    }
    return  strdup(valueStr);;
}
