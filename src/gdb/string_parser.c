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
#include <ctype.h>
#include <wctype.h>
#include "cobgdb.h"

// ==========================================================================================
// Default empty token.
//
// Used as a safe fallback when a requested token position does not
// exist in a parsed token array.
// ==========================================================================================
struct st_parse parse_blank = {
    .token = "" ,       // Initialize the char * token pointer as NULL
    .size = 0,          // Initialize the int size as 0
    .type = 0,          // Initialize the int type as 0
    .next = NULL        // Initialize the struct st_parse * next pointer as NULL

};

// ==========================================================================================
// Convert all alphabetic characters in a string to uppercase.
//
// The conversion is performed in-place, modifying the original buffer.
//
// Returns a pointer to the modified string.
// ==========================================================================================
char * toStUpper(char* str) {
  int j = 0;
 
  while (str[j]) {
        str[j]=toupper(str[j]);
        j++;
  }
  return str;
}

// ==========================================================================================
// Count leading whitespace characters.
//
// Counts consecutive spaces and tab characters starting from the
// beginning of the supplied string.
//
// Returns the number of whitespace characters found.
// ==========================================================================================
int psSpaces(char * ch){
    int count=0;
    while(*ch==' ' || *ch=='\t'){
        ch = ch + 1;
        count++;
    }
    return count;
}

// ==========================================================================================
// Determine the length of a quoted string literal.
//
// Supports both single-quoted and double-quoted strings and includes
// the opening and closing quote characters in the count.
//
// Returns the total length of the string token.
// ==========================================================================================
int psString(char * ch){
    int count=0;
    if(*ch=='\''){
        count++;
        ch = ch+1;
        while(*ch!='\'' && *ch!='\0'  && *ch!='\n'){
            ch = ch + 1;
            count++;
        }
        if(*ch=='\''){
            ch = ch + 1;
            count++;
        }
    }else{
        count++;
        ch = ch+1;
        while(*ch!='"' && *ch!='\0'  && *ch!='\n'){
            ch = ch + 1;
            count++;
        }
        if(*ch=='"'){
            ch = ch + 1;
            count++;
        }
    }
    return count;
}

// ==========================================================================================
// Determine the length of a generic symbol sequence.
//
// A symbol sequence consists of consecutive non-whitespace characters
// that are not recognized as delimiters or grouping symbols.
//
// Returns the number of characters in the symbol token.
// ==========================================================================================
int psSymbols(char * ch){
    int count=0;
    while(*ch!=' ' && *ch!='\t' && *ch!='\n' && *ch!='\0' &&
           *ch != '(' && *ch != '[' && *ch != '{' && *ch != ')' 
           && *ch != ']' && *ch != '}' && *ch != '=' ){
        ch = ch + 1;
        count++;
    }
    return count;
}

// ==========================================================================================
// Determine the length of an alphanumeric identifier.
//
// Valid characters include:
//
//   a-z A-Z 0-9 . _ -
//
// Returns the number of characters in the identifier.
// ==========================================================================================
int psAlpha(char * ch){
    int count=0;
    while(isalpha(*ch) || *ch=='.' || *ch=='-' || *ch=='_' || isdigit(*ch)){
        ch = ch + 1;
        count++;
    }
    return count;
}

// ==========================================================================================
// Determine the length of a numeric token.
//
// Supports:
//
//   Decimal numbers
//   Floating-point numbers
//   Hexadecimal-style values containing 'x' and a-f digits
//
// Returns the number of characters in the numeric token.
// ==========================================================================================
int psNumber(char * ch){
    int count=0;
    int hex = 0;
    while(isdigit(*ch) || *ch=='.' || *ch=='x' || 
            ((*ch=='a' || *ch=='b' || *ch=='c' || *ch=='d' || *ch=='e' || *ch=='f') && hex==1)){
        if(*ch=='x') hex = 1;
        ch = ch + 1;
        count++;
    }
    return count;
}

// ==========================================================================================
// Safely retrieve a token from a parsed token array.
//
// If the requested position is outside the valid range, a predefined
// empty token structure is returned instead.
//
// Returns a pointer to the requested token.
// ==========================================================================================
struct st_parse * tk_val(struct st_parse * line_parsed, int qtt_tk, int pos){
    struct st_parse * tk = NULL;
    if(pos>=0 && pos<qtt_tk){
        tk = &line_parsed[pos];
    }
    if(tk==NULL){ tk=&parse_blank; }
    return tk;
}

// ==========================================================================================
// Perform a case-insensitive search in a wide-character string.
//
// Similar to wcsstr(), but comparisons are performed without regard
// to character case.
//
// Returns a pointer to the first matching occurrence or NULL if not found.
// ==========================================================================================
const wchar_t *wcsistr(const wchar_t *haystack, const wchar_t *needle) {
    if (!*needle) return haystack;
    for (; *haystack; haystack++) {
        const wchar_t *h = haystack;
        const wchar_t *n = needle;
        while (*h && *n && towlower(*h) == towlower(*n)) {
            h++;
            n++;
        }
        if (!*n)
            return haystack;
    }
    return NULL;
}

// ==========================================================================================
// Tokenize a source line into parser tokens.
//
// The function scans the input line and classifies tokens into
// categories such as:
//
//   - identifiers
//   - numbers
//   - strings
//   - symbols
//
// The token array is automatically expanded when necessary.
//
// Parameters:
//   line_to_parse - source text to tokenize
//   h             - token array
//   qtt           - receives token count
//   cap           - current token array capacity
// ==========================================================================================
void lineParse(char * line_to_parse, struct st_parse **h, int *qtt, int *cap ){
    char * ch = line_to_parse;
    *qtt=0;
    while(*ch!='\0'){
            if (*qtt >= *cap) {
                *cap *= 2;
                *h = realloc(*h, (*cap) * sizeof(struct st_parse));
            }

            struct st_parse *t = &(*h)[*qtt];

            if(*ch==' ' || *ch=='\t'){
                int size = psSpaces(ch);
                ch+=size;
                continue;
            }else if(isdigit(*ch)){
                t->token=ch;
                t->type=TP_NUMBER;
                t->size = psNumber(ch);
                ch+=t->size;
            }else if(*ch == '&' || *ch == '=' || *ch == '(' || *ch == '[' 
                    || *ch == '{' || *ch == ')' || *ch == ']' || *ch == '}'){
                t->token=ch;
                t->type=TP_SYMBOL;
                t->size = 1;
                ch+=t->size;
            }else if(isalpha(*ch)){
                t->token=ch;
                t->type=TP_ALPHA;
                t->size = psAlpha(ch);
                ch+=t->size;
            }else if(!isalpha(*ch) && *ch!=' ' && *ch!='\t' && *ch!='\n' && *ch!='\0'){
                t->token=ch;
                t->type=TP_ALPHA;
                t->size = psSymbols(ch);
                ch+=t->size;
            }else if(*ch=='\''){
                t->token=ch;
                t->type=TP_STRING1;
                t->size = psString(ch);
                ch+=t->size;
            }else if(*ch=='"'){
                t->token=ch;
                t->type=TP_STRING2;
                t->size = psString(ch);
                ch+=t->size;
            }else{
                t->token=ch;
                t->type=TP_OTHER;
                t->size = 1;
                ch+=t->size;
                break;
            }
            *qtt = *qtt + 1;
        }
}

// ==========================================================================================
// Convert a UTF-8 string to a wide-character string.
//
// On Windows, MultiByteToWideChar() is used.
// On POSIX systems, mbstowcs() is used.
//
// The returned buffer is dynamically allocated and must be freed by
// the caller.
//
// Returns NULL on conversion failure.
// ==========================================================================================
wchar_t *to_wide(const char *src) {
    if (!src) return NULL;
#if defined(_WIN32)
    int needed = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    if (needed <= 0) return NULL;
    wchar_t *w = malloc(needed * sizeof(wchar_t));
    if (!w) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, src, -1, w, needed);
    return w;
#else
    size_t len = mbstowcs(NULL, src, 0);
    if (len == (size_t)-1) return NULL;
    wchar_t *w = malloc((len + 1) * sizeof(wchar_t));
    if (!w) return NULL;
    mbstowcs(w, src, len + 1);
    return w;
#endif
}

// ==========================================================================================
// Perform a case-insensitive substring search.
//
// Similar to strcasestr(), but returns the starting position of the
// first match instead of a pointer.
//
// Returns:
//   >= 0 : position of the match
//   -1   : substring not found
// ==========================================================================================
int my_strcasestr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return -1;

    size_t nh = strlen(haystack);
    size_t nn = strlen(needle);

    if (nn > nh) return -1;

    for (size_t i = 0; i <= nh - nn; i++) {
        size_t j = 0;
        while (j < nn && tolower((unsigned char)haystack[i+j]) ==
                         tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == nn) return j;  // match
    }
    return -1;
}