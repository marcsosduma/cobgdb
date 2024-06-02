/* 
This code is based on the GnuCOBOL Debugger extension available at:https://github.com/OlegKunitsyn/gnucobol-debug
and also on the number handling routines found inGnuCOBOL: termio.c and numbers.c. 
Written by Keisuke Nishida, Roger While, Simon Sobisch, Edward Hart.
It is provided without any warranty, express or implied.
You may modify and distribute it at your own risk.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stdint.h>
#include <math.h>
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
    if(strlen(wholeNumber)>(size_t) fieldSize){
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
    char *originalLocale = setlocale(LC_NUMERIC, NULL);

    int scale = abs(sc);    
    char *value = malloc(fieldSize+scale+3);
    strcpy(value,valueStr);
    setlocale(LC_NUMERIC, "C");
    if(fieldSize<0)
        fieldSize=strlen(valueStr)-2;
    if (value[0] == '"') {
        memmove(value, value + 1, fieldSize+1);  // Remove comma
        value[fieldSize]='\0'; 
    }       
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
    }else if (strlen(value) < (size_t) sc) {
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
    int size = (fieldSize + shift) < (int) strlen(valueStr) ? (int) (fieldSize + shift) : (int) strlen(valueStr);
    char* result = malloc(size + 3); 
    snprintf(result, size + 3, "\"%.*s\"", size-shift, value + shift);
    return result;
}

double my_pow(double base, int exponent) {
    if (exponent == 0) return 1;
    double result = 1.0;
    int exp = exponent;    
    if (exponent < 0) exp = -exponent;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result *= base;
        }
        base *= base;
        exp /= 2;
    }    
    if (exponent < 0) return 1.0 / result;
    return result;
}

#if	defined(_WIN32) && !defined(__MINGW32__)
#define	cob_s64_t		__int64
#define	cob_u64_t		unsigned __int64
#else
#define	cob_s64_t		long long
#define	cob_u64_t		unsigned long long
#endif

// Function to reverse the byte order of a 64-bit number
uint64_t COB_BSWAP_64(uint64_t x) {
    return (((x) << 56) |                         \
            (((x) << 40) & 0xff000000000000ULL) | \
            (((x) << 24) & 0xff0000000000ULL) |   \
            (((x) <<  8) & 0xff00000000ULL) |     \
            (((x) >>  8) & 0xff000000ULL) |       \
            (((x) >> 24) & 0xff0000ULL) |         \
            (((x) >> 40) & 0xff00ULL) |           \
            ((x)  >> 56));
}

cob_s64_t cob_binary_get_sint64(const char* data, size_t size, unsigned int flags) {
    cob_s64_t n = 0;
    const size_t fsiz = 8U - size;
    #ifndef WORDS_BIGENDIAN
    if (flags&(1U << 5)) {
		 memcpy(&n, data, size);
         n = COB_BSWAP_64(n);
	} else {
		memcpy ((char *)&n + fsiz, data, size);
	}
    #else
    memcpy((char *)&n + fsiz, data, size);
    #endif
    n >>= (cob_s64_t)8 * fsiz;
    return n;
}

cob_u64_t cob_binary_get_uint64(const char* data, size_t size, unsigned int flags) {
    cob_u64_t n = 0;
    const size_t fsiz = 8U - size;

    #ifndef WORDS_BIGENDIAN
    if (flags&(1U << 5)) {
        memcpy((char *)&n + fsiz, data, size);
        n = COB_BSWAP_64(n);
    }else{
        memcpy (&n, data, size);
    }
    #else
    memcpy ((char *)&n + fsiz, data, size);
    #endif
    return n;
}

char* debugParseBuilIn(char* valueStr, int fieldSize, int scale, char* type, unsigned int flags) {
    char wrk[50];

    if (fieldSize == 0) return NULL;
    if (strcmp(type, "alphanumeric") == 0 || strcmp(type, "numeric edited") == 0  || strcmp(type, "group") == 0
        || strcmp(type, "boolean") == 0 || strstr(type, "national") != NULL) {
        if(valueStr[0]=='\"')
            return valueStr;
        char * new_value = malloc(strlen(valueStr)+3);
        sprintf(new_value,"\"%s\"", valueStr);
        return new_value;
    }
    if (strcmp(type, "numeric l double") == 0) {
        long double lval;
        memcpy(&lval, valueStr, sizeof(long double));
        snprintf(wrk, sizeof(wrk), "\"%-.32Lf\"", lval);
        return strdup(wrk);
    } else if (strcmp(type, "numeric double") == 0) {
        double dval;
        memcpy(&dval, valueStr, sizeof(double));
        sprintf(wrk, "\"%-.16G\"", dval);
        return strdup(wrk);
    } else if (strcmp(type, "numeric float") == 0) {
        float fval;
        memcpy(&fval, valueStr, sizeof(float));
        sprintf(wrk, "\"%-.8G\"", (double)fval);
        return strdup(wrk);
    } else if (strcmp(type, "numeric packed") == 0) {
        // Is COMP-3 or COMP-6
        int is_comp3 = FALSE;
        unsigned char last_byte = valueStr[fieldSize - 1];
        unsigned char last_nibble = last_byte & 0x0F;        
        if (last_nibble == 0xC || last_nibble == 0xD || last_nibble == 0xF) is_comp3 = TRUE;
        if (is_comp3) {
            // Convert COMP-3
            int len = (fieldSize * 2) - 1;
            char* buf = malloc(len + 2);
            int i, j = 0;
            int is_negative = 0;
            for (i = 0; i < fieldSize - 1; i++) {
                buf[j++] = ((valueStr[i] & 0xF0) >> 4) + '0';
                buf[j++] = (valueStr[i] & 0x0F) + '0';
            }
            buf[j++] = ((valueStr[fieldSize - 1] & 0xF0) >> 4) + '0';
            char sign = valueStr[fieldSize - 1] & 0x0F;
            if (sign == 0x0D || sign == 0x0B) {
                is_negative = 1;
            }
            buf[j] = '\0';
            double comp3_value = atof(buf) / my_pow(10, scale);
            if (is_negative) {
                comp3_value = -comp3_value;
            }
            sprintf(wrk, "%.*f", scale, comp3_value);
            free(buf);
        } else {
            // Convert COMP-6
            int64_t value = 0;
            for (int i = 0; i < fieldSize; i++) {
                int highDigit = (valueStr[i] >> 4) & 0x0F;
                int lowDigit = valueStr[i] & 0x0F;
                value = (value * 100) + (highDigit * 10) + lowDigit;
            }
            double comp6_value = (double)value / my_pow(10, scale);
            sprintf(wrk, "%.*f", scale, comp6_value);
        }
        return strdup(wrk);
    } else if (strcmp(type, "numeric FP DEC64") == 0 || strcmp(type, "numeric FP DEC128") == 0) {
        // TODO: Implementation for these data types is not available yet
        return NULL;
    } else {
        strcpy(wrk, "");
    }
    if (strstr(type, "numeric") != NULL) {
        if ((flags & (1U<<7)) && strcmp(type, "numeric binary") == 0) {
            char *p;
            int n;
            char temp[3];
            strcpy(wrk, "\"0x");
            #ifdef WORDS_BIGENDIAN
            p = valueStr;
            for (n = 0; n < sizeof(void *); ++n, ++p) {
            #else
            p = valueStr + sizeof(void *) - 1;
            for (n = sizeof(void *) - 1; n >= 0; --n, --p) {
            #endif
                snprintf(temp, sizeof(temp), "%x%x", (*p >> 4) & 0xF, *p & 0xF);
                strcat(wrk, temp);
            }
            strcat(wrk,"\"");
            return strdup(wrk);
        }else if ((flags & (1U<<6)) || strcmp(type, "numeric binary") == 0) {
            double converted_value;
            char* wrk = (char*)malloc(50 * sizeof(char));
            if(flags&0x1){  
                cob_s64_t val = cob_binary_get_sint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double) val / my_pow(10, scale);
            }else{
                const cob_u64_t	uval = cob_binary_get_uint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double)uval / my_pow(10, scale);
            }
            sprintf(wrk, "\"%.*f\"", scale, converted_value);
            return strdup(wrk);
        }else{ 
            memcpy(wrk, valueStr, fieldSize);
            wrk[fieldSize] = '\0';
            formatNumberParser(valueStr, fieldSize, scale);
            wrk[0]='\"';
            strcpy(&wrk[1], valueStr);
            strcat(wrk,"\"");
        }
    }
    return strdup(wrk);
}


char* parseUsage(char* valueStr) {
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
