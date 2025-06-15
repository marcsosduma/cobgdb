/*
 * COBGDB GnuCOBOL Debugger:
 * This code is based on the GnuCOBOL Debugger extension available at:
 * https://github.com/OlegKunitsyn/gnucobol-debug
 * 
 * Additional Sources:
 * This code is also based on number handling routines from the following GnuCOBOL files:
 * termio.c, intrinsic.c, and numbers.c. Authors:
 * Keisuke Nishida, Roger While, Simon Sobisch, Edward Hart.
 * 
 * License:
 * This code is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include "cobgdb.h"
#define ZERO_SIGN_CHAR_CODE 112

extern char m[10][512];

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
    if (strlen(value) >= (size_t)abs(scale)) {
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
    if(sc>0){
        if (atof(numericValue) > 0) {
            sprintf(valueStr, "+%0*.*f", fieldSize - 1 + scale, scale, atof(numericValue));
        } else {
            sprintf(valueStr, "%0*.*f", fieldSize + scale, scale, atof(numericValue));
        }
    }else{
        sprintf(valueStr, "%0*.*f", fieldSize, 0, atof(numericValue));
    }
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
    //int qty = regex(repeatTimeRegex, value, m);
    int qty = fRepeatTimeRegex(value, m);
    if (qty>0) {
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

char* debugParseBuilIn(char* valueStr, int fieldSize, int scale, char* type, unsigned int flags, int digits) {
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
            if(flags&(1U << 3))
                sprintf(wrk, "%.*f", scale, comp3_value);
            else
                sprintf(wrk, "%0*.*f", fieldSize + scale + is_negative, scale, comp3_value);
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
            if(flags&(1U << 3))
                sprintf(wrk, "%.*f", scale, comp6_value);
            else
                sprintf(wrk, "%0*.*f", fieldSize + scale + (comp6_value<0), scale, comp6_value);
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
        }else if ((flags & (1U<<6)) && strcmp(type, "numeric binary") == 0) {
            double converted_value;
            char* wrk = (char*)malloc(50 * sizeof(char));
            if(flags&0x1){  
                cob_s64_t val = cob_binary_get_sint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double) val; 
            }else{
                const cob_u64_t	uval = cob_binary_get_uint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double)uval; 
            }
            if(flags&(1U << 0))
                sprintf(wrk, "%+0*.*f", 2 * digits + 1, 0, converted_value);
            else
                sprintf(wrk, "%0*.*f", 2 * digits, 0, converted_value);
            return strdup(wrk);
        }else if (strcmp(type, "numeric binary") == 0) {
            double converted_value;
            char* wrk = (char*)malloc(50 * sizeof(char));
            if(flags&0x1){  
                cob_s64_t val = cob_binary_get_sint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double) val / my_pow(10, scale);
            }else{
                const cob_u64_t	uval = cob_binary_get_uint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double)uval / my_pow(10, scale);
            }
            if(flags&(1U << 0))
                sprintf(wrk, "%+0*.*f", digits - scale + 1, scale, converted_value);
            else
                sprintf(wrk, "%0*.*f", digits - scale, scale, converted_value);
            return strdup(wrk);
        }else if (strcmp(type, "numeric COMP5") == 0) {
            double converted_value;
            char* wrk = (char*)malloc(50 * sizeof(char));
            if(flags&0x1){  
                cob_s64_t val = cob_binary_get_sint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double) val / my_pow(10, scale);
            }else{
                const cob_u64_t	uval = cob_binary_get_uint64((const char*)valueStr, fieldSize, flags);
                converted_value = (double)uval / my_pow(10, scale);
            }
            if(flags&(1U << 0))
                sprintf(wrk, "%+0*.*f", digits - scale + 1, scale, converted_value);
            else
                sprintf(wrk, "%0*.*f", digits - scale, scale, converted_value);
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

char *convertStrToCobField(const char *value){
    unsigned char *final_buff;
    unsigned char *p, *p_end;
    size_t digits, decimal_digits, exponent, datasize;
    int decimal_seen, e_seen, plus_minus, e_plus_minus;
    const unsigned char dec_pt = '.';
    char *formattedValue = NULL;

    datasize = strlen(value);
    if (datasize == 0) {
        formattedValue = malloc(2);
        strcpy(formattedValue, "0");
        return formattedValue;
    }
    if (datasize > 35) {
        datasize = 35;
    }

    final_buff = malloc(datasize + 1U);
    plus_minus = 0;
    digits = 0;
    decimal_digits = 0;
    decimal_seen = 0;
    e_seen = 0;
    exponent = 0;
    e_plus_minus = 0;

    p = (unsigned char *)value;
    p_end = p + datasize - 1;

    for (; p <= p_end; ++p) {
        switch (*p) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                if (e_seen) {
                    exponent *= 10;
                    exponent += (*p - '0');
                } else {
                    if (decimal_seen) {
                        decimal_digits++;
                    }
                    final_buff[digits++] = *p;
                    if (digits > 35) {
                        goto game_over;
                    }
                }
                continue;
            case '+':
                if (e_seen) {
                    if (!e_plus_minus) {
                        e_plus_minus = 1;
                    }
                } else {
                    if (!plus_minus) {
                        plus_minus = 1;
                    }
                }
                continue;
            case '-':
                if (e_seen) {
                    if (!e_plus_minus) {
                        e_plus_minus = -1;
                    }
                } else {
                    if (!plus_minus) {
                        plus_minus = -1;
                    }
                }
                continue;
            case 'e': case 'E':
                if (!e_seen) {
                    if (digits == 0 && decimal_digits == 0) {
                        goto game_over;
                    }
                    e_seen = 1;
                }
                continue;
            case ' ':
                continue;
            default:
                if (*p == dec_pt) {
                    if (!decimal_seen) {
                        decimal_seen = 1;
                    }
                }
                continue;
        }
    }
game_over:
    if (!digits) {
        final_buff[0] = '0';
        digits = 1;
    }
    final_buff[digits] = '\0';
    formattedValue = malloc(digits + 3);
    if (plus_minus == -1) {
        final_buff[digits-1]=final_buff[digits-1]|ZERO_SIGN_CHAR_CODE;
        strcpy(formattedValue, (char *)final_buff);
    } else {
        strcpy(formattedValue, (char *)final_buff);
    }
    free(final_buff);
    if (e_seen) {
        int exp_value = (e_plus_minus == -1) ? -exponent : exponent;
        if (exp_value != 0) {
            char *new_formatted_value = malloc(strlen(formattedValue) + abs(exp_value) + 2);
            char *dp = strchr(formattedValue, '.');
            if (dp) {
                memmove(dp, dp + 1, strlen(dp));
                digits--;
            }
            if (exp_value > 0) {
                strcpy(new_formatted_value, formattedValue);
                memset(new_formatted_value + digits, '0', exp_value);
                new_formatted_value[digits + exp_value] = '\0';
            } else {
                int shift = -exp_value;
                new_formatted_value[0] = '0';
                new_formatted_value[1] = '.';
                memset(new_formatted_value + 2, '0', shift - 1);
                strcpy(new_formatted_value + shift + 1, formattedValue);
            }
            free(formattedValue);
            formattedValue = new_formatted_value;
        }
    }

    return formattedValue;
}

// Helper function to remove the comma and point
int64_t stringToInteger(const char *str) {
    char temp[50];
    strncpy(temp, str, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';
    char *comma = strchr(temp, ',');
    if (comma) {
        *comma = '\0';
        strcat(temp, comma + 1);
    }
    comma = strchr(temp, '.');
    if (comma) {
        *comma = '\0';
        strcat(temp, comma + 1);
    }
    return atoll(temp);
}

// Function to convert a string into binary bytes
char *stringToCobNumber(const char *valueStr, size_t fieldSize, const char *type, int flags) {
    size_t dataSize = fieldSize; 
    char *binary_data = (char *)malloc(dataSize);
    if (binary_data == NULL) {
        fprintf(stderr, "Erro: Falha ao alocar memória.\n");
        return NULL;
    }
    if ((flags & (1U << 7)) && strcmp(type, "numeric binary") == 0) {
        int64_t value = stringToInteger(valueStr);
        for (size_t i = 0; i < dataSize; ++i) {
            binary_data[dataSize - 1 - i] = (unsigned char)((value >> (8 * i)) & 0xFF);
        }
    }else if (strcmp(type, "numeric COMP5") == 0) {
        int64_t value = stringToInteger(valueStr);
        for (size_t i = 0; i < dataSize; ++i) {
            binary_data[i] = (unsigned char)((value >> (8 * i)) & 0xFF);
        }
    }else if ((flags & (1U << 6)) || strcmp(type, "numeric binary") == 0) {
        int64_t value = stringToInteger(valueStr);
        if(flags&0x20){  
            for (size_t i = 0; i < dataSize; ++i) {
                binary_data[dataSize - 1 - i] = (unsigned char)((value >> (8 * i)) & 0xFF);
            }
        }else{
            for (size_t i = 0; i < dataSize; ++i) {
                binary_data[i] = (unsigned char)((value >> (8 * i)) & 0xFF);
            }
        }
    }else {
        memcpy(binary_data, valueStr, dataSize);
    }
    return binary_data;
}

// Function to convert a string to COMP-3 binary bytes
char *stringToPacked(char *valueStr, size_t fieldSize, int scale, const char *type) {
    size_t packedSize = fieldSize; 
    unsigned char *packed_data = (unsigned char *)malloc(packedSize+1);
    if (packed_data == NULL) {
        fprintf(stderr, "Erro: Falha ao alocar memória.\n");
        return NULL;
    }
    memset(packed_data, 0, packedSize);
    if (strcmp(type, "numeric packed") == 0) {
        // Convert COMP-3
        int is_negative = 0;
        size_t len = strlen(valueStr);
        size_t packedIndex = packedSize - 1;
        int shift = 1;
        for (int i = len - 1; i >= 0; --i) {
            if (valueStr[i] == '-') {
                is_negative = 1;
                continue;
            }
            if (valueStr[i] == ',' || valueStr[i] == '.') {
                continue;
            }
            if (!isdigit(valueStr[i])) {
                valueStr[i]='0';
            }
            packed_data[packedIndex] |= (valueStr[i] - '0') << (shift * 4);
            if (shift == 0) {
                shift = 1;
            } else {
                shift = 0;
                packedIndex--;
            }
        }
        if (is_negative) {
            packed_data[packedSize - 1] |= 0x0D; // Negative sign
        } else {
            packed_data[packedSize - 1] |= 0x0C; // Positive sign
        }
    } else if (strcmp(type, "COMP-6") == 0) {
        // Convert COMP-6
        double value = atof(valueStr) * my_pow(10, scale);
        // Fills the packed field from left to right
        for (size_t i = 0; i < fieldSize; ++i) {
            int digit = ((int)value % 10) << 4;
            value /= 10;
            digit += (int)value % 10;
            value /= 10;
            packed_data[i] = digit;
        }
    } else {
        fprintf(stderr, "Erro: Tipo desconhecido.\n");
        free(packed_data);
        return NULL;
    }
    return (char *) packed_data;
}

char* convertDoubleValueToBytes(char* ptr){
        char * bytes = malloc(50);
        for(size_t i=0;i<strlen(ptr);i++){
            if(ptr[i]==',') ptr[i]='.';
        }
        double dbl = atof(ptr);
        memcpy(bytes, &dbl, sizeof(double));
        bytes[sizeof(double)]='\0';    
        return bytes;
}

char* formatValueVar(char* valueStr, int fieldSize, int scale, char* type, unsigned int flags) {
    if (!valueStr) return NULL;
    if (strcmp(type, "numeric") == 0) {
        return convertStrToCobField(valueStr);
    }else if(strcmp(type, "numeric double") == 0) {
        return convertDoubleValueToBytes(valueStr); 
    } else if(strcmp(type, "numeric packed") == 0) {
        return stringToPacked(valueStr, fieldSize, scale, type);
    } else if(strcmp(type, "numeric binary") == 0 || strcmp(type, "numeric COMP5") == 0) {
        return stringToCobNumber(valueStr, fieldSize, type, flags);
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
