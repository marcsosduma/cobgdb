#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cobgdb.h"

struct st_parse parse_blank = {
    .token = "" ,       // Initialize the char * token pointer as NULL
    .size = 0,          // Initialize the int size as 0
    .type = 0,          // Initialize the int type as 0
    .next = NULL        // Initialize the struct st_parse * next pointer as NULL

};

char * toStUpper(char* str) {
  int j = 0;
 
  while (str[j]) {
        str[j]=toupper(str[j]);
        j++;
  }
  return str;
}

int psSpaces(char * ch){
    int count=0;
    while(*ch==' ' || *ch=='\t'){
        ch = ch + 1;
        count++;
    }
    return count;
}

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

int psAlpha(char * ch){
    int count=0;
    while(isalpha(*ch) || *ch=='.' || *ch=='-' || *ch=='_' || isdigit(*ch)){
        ch = ch + 1;
        count++;
    }
    return count;
}

int psNumber(char * ch){
    int count=0;
    while(isdigit(*ch) || *ch=='.' || *ch=='x'){
        ch = ch + 1;
        count++;
    }
    return count;
}

struct st_parse * tk_val(struct st_parse line_parsed[100], int qtt_tk, int pos){
    struct st_parse * tk = NULL;
    if(pos<qtt_tk){
        tk = &line_parsed[pos];
    }
    if(tk==NULL){ tk=&parse_blank; }
    return tk;
}


void lineParse(char * line_to_parse, struct st_parse h[100], int *qtt ){
    char * ch = line_to_parse;
    *qtt=0;
    while(*ch!='\0' && *qtt<100){
            if(*ch==' ' || *ch=='\t'){
                int size = psSpaces(ch);
                ch+=size;
                continue;
            }else if(isdigit(*ch)){
                h[*qtt].token=ch;
                h[*qtt].type=TP_NUMBER;
                h[*qtt].size = psNumber(ch);
                ch+=h[*qtt].size;
            }else if(*ch == '&' || *ch == '=' || *ch == '(' || *ch == '[' 
                    || *ch == '{' || *ch == ')' || *ch == ']' || *ch == '}'){
                h[*qtt].token=ch;
                h[*qtt].type=TP_SYMBOL;
                h[*qtt].size = 1;
                ch+=h[*qtt].size;
            }else if(isalpha(*ch)){
                h[*qtt].token=ch;
                h[*qtt].type=TP_ALPHA;
                h[*qtt].size = psAlpha(ch);
                ch+=h[*qtt].size;
            }else if(!isalpha(*ch) && *ch!=' ' && *ch!='\t' && *ch!='\n' && *ch!='\0'){
                h[*qtt].token=ch;
                h[*qtt].type=TP_ALPHA;
                h[*qtt].size = psSymbols(ch);
                ch+=h[*qtt].size;
            }else if(*ch=='\''){
                h[*qtt].token=ch;
                h[*qtt].type=TP_STRING1;
                h[*qtt].size = psString(ch);
                ch+=h[*qtt].size;
            }else if(*ch=='"'){
                h[*qtt].token=ch;
                h[*qtt].type=TP_STRING2;
                h[*qtt].size = psString(ch);
                ch+=h[*qtt].size;
            }else{
                h[*qtt].token=ch;
                h[*qtt].type=TP_OTHER;
                h[*qtt].size = 1;
                ch+=h[*qtt].size;
                break;
            }
            *qtt = *qtt + 1;
        }
}
