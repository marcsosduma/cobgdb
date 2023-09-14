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
#include <regex.h>

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

int er_error(int i, regex_t reg)
{
	size_t length;
	char *buffer=NULL;

	/* size of error message */
	length = regerror (i, &reg, NULL, 0);

	/* space allocate to error message  */
	if ((buffer = (char *)malloc(length)) == NULL) {
		fprintf(stderr, "error: malloc buffer\n");
		exit(1);
	}
	
	/* message to buffer */
	regerror (i, &reg, buffer, length);	
	fprintf(stderr,"Erro: %s\n",buffer);
	free(buffer);
	exit(1);
}


char re [128];
char buf [1024];
int qtd_match;

int regex(char * tx_regex, char * text, char mm[][512])
{
    FILE *fp;
    regex_t     regex;
    regmatch_t  pmatch [100]; 
    regoff_t    offset, length;
    int ret;
    strcpy (re, tx_regex);
    char m[100][512];

    if (ret = regcomp (&regex, re, REG_EXTENDED | REG_ICASE | REG_NEWLINE)) {
        (void) regerror (ret, &regex, buf, sizeof (buf));
        fprintf (stderr, "Error: regcomp: %s\n", buf);
        exit(EXIT_FAILURE);
    }
    char *s = text;
    qtd_match=0;
    if(s==NULL) return qtd_match;
    while (strlen(s)>0) {
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
            offset = pmatch [0].rm_so + (s - buf);
            length = pmatch [0].rm_eo - pmatch[0].rm_so;
            for (int j = 0; j < ARRAY_SIZE(pmatch); j++) {
                if (pmatch [j].rm_so == -1)
                    break;
                offset = pmatch [j].rm_so + (s - buf);
                length = pmatch [j].rm_eo - pmatch [j].rm_so;
                if(length>512) length=510;
                sprintf(m[qtd_match++],"%.*s", length, s + pmatch[j].rm_so); 
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
