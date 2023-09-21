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

extern char m[10][512];

char repeatTimeRegex[] = "(\"\\,\\s|^)\\'(\\s|0)\\'\\s\\<repeats\\s(\\[0-9]+)\\stimes\\>";

#define ZERO_SIGN_CHAR_CODE 112

void formatNumber(char *valueStr, int fieldSize, int scale, int isSigned, char *result) {
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
    strcpy(valueResult, wholeNumber);
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
    strcpy(result, valueResult);
    free(value);
    free(valueResult);
}


void formatNumberParser(char *valueStr, int fieldSize, int scale) {
    char *value = strdup(valueStr);
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
            value[strlen(value) - 1] = '\0';
        }
        if (strlen(value) < scale) {
            int diff = scale - strlen(value);
            char prefix[diff + 1];
            memset(prefix, '0', diff);
            prefix[diff] = '\0';
            strcat(prefix, value);
            strcpy(value, prefix);
        } else if (scale < 0) {
            int diff = abs(scale);
            char suffix[diff + 1];
            memset(suffix, '0', diff);
            suffix[diff] = '\0';
            strcat(value, suffix);
        }
        char *wholeNumber = malloc(fieldSize + 1);
        char *decimals = malloc(scale + 1);
        wholeNumber[0] = '\0';
        decimals[0] = '\0';
        if (strlen(value) >= abs(scale)) {
            strcpy(wholeNumber, value);
            if (abs(scale) > 0) {
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
        sprintf(valueStr, "%.*f", scale, atof(numericValue));
        free(wholeNumber);
        free(decimals);
        free(numericValue);
    }
    free(value);
}


static void replacestr(char *line, const char *search, const char *replace)
{
     char *sp;
     if ((sp = strstr(line, search)) == NULL) return;
     int search_len = strlen(search);
     int replace_len = strlen(replace);
     int tail_len = strlen(sp+search_len);
     memmove(sp+replace_len,sp+search_len,tail_len+1);
     memcpy(sp, replace, replace_len);
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
    int qtd = regex(repeatTimeRegex, value, m);
    if (qtd>0) {
        char replacement[250];
        strcpy(replacement,"");
        int size = atoi(m[3]);
        for(int i=0;i<size;i++){
            strcat(replacement,m[2]);            
        }
        strcat(replacement,"\"");
        replacestr(value, m[0], replacement);
        if(strncmp(value,"\"",1)!=0)
            sprintf(value,"\"%s",value);
    }
    return value;
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
        return strdup(valueStr);
    }else if (
        strcmp(type, "integer") == 0 ||
        strcmp(type, "group") == 0
    ) {
        return strdup(valueStr);
    } else {
        fprintf(stderr, "Type error: %s\n", type);
        return NULL;
    }
    return strdup(valueStr);
}
