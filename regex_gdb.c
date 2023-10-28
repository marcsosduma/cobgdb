/* 
   This code is based on information from: https://thobias.org/doc/er_c.html
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(_WIN32)
#include "libgnurx/regex.h"
#else
#include <regex.h>
#endif

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

#define MAX_MATCHES 100
#define MAX_MATCH_LENGTH 512

int regex(char * tx_regex, char * text, char mm[][MAX_MATCH_LENGTH])
{
    regex_t     regex;
    regmatch_t  pmatch [MAX_MATCHES]; 
    regoff_t    length;
    int ret;
    char m[MAX_MATCHES][MAX_MATCH_LENGTH];
    char re [128];
    char buf [1024];
    int  qtd_match=0;

    strcpy (re, tx_regex);
    if ((ret = regcomp(&regex, re, REG_EXTENDED | REG_ICASE | REG_NEWLINE)) != 0) {
        perror("regcomp");
        return qtd_match;
    }
    char *s = text;
    if(s==NULL) return qtd_match;
    while (s[0]!='\0'){
        for (int i = 0; ; i++) {
            if (ret = regexec (&regex, s, ARRAY_SIZE(pmatch), pmatch, 0) || qtd_match>9) {
                if (ret != REG_NOMATCH) {
                    (void) regerror (ret, &regex, buf, sizeof (buf));
                    fprintf (stderr, "Error: regexec: %s\n", buf);
                    exit (EXIT_FAILURE);
                }else{
                    regfree (&regex);
                    if(qtd_match>10) qtd_match=10;
                    for(int i=0;i<qtd_match;i++){
                        strcpy(mm[i],m[i]);
                    }
                    return qtd_match;
                }
                break;
	        }
            for (int j = 0; j < ARRAY_SIZE(pmatch); j++) {
                if (pmatch [j].rm_so == -1)
                    break;
                length = pmatch [j].rm_eo - pmatch [j].rm_so;
                if(length>MAX_MATCH_LENGTH) length=MAX_MATCH_LENGTH-2;
                memcpy(m[qtd_match], s + pmatch[j].rm_so, length);
                m[qtd_match][length] = '\0';
                qtd_match++;
            }
            s += pmatch[0].rm_eo;
        }
    }
    // free internal storage fields associated with regex
    regfree (&regex);
    if(qtd_match>10) qtd_match=10;
    for(int i=0;i<qtd_match;i++){
        strcpy(mm[i],m[i]);
    }
    return qtd_match;
}
