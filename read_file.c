/* 
   This code includes the 'my_getline' function obtained from the following site: 
   https://solarianprogrammer.com/2019/04/03/c-programming-read-file-lines-fgets-getline-implement-portable-getline/
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS 1
    #define restrict __restrict
#endif

#ifndef EOVERFLOW
#define EOVERFLOW 132
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#if defined(_WIN32)
    #include <direct.h>
#endif
#include "cobgdb.h"


// Function to get the current directory
char* getCurrentDirectory() {
    char* currentDirectory = NULL;

    #ifdef _WIN32
        // Windows environment
        currentDirectory = (char*)_getcwd(NULL, 0);
        if (currentDirectory == NULL) {
            perror("Error getting current directory");
        }
    #else
        // POSIX environment (Linux, macOS, etc.)
        size_t size = pathconf(".", _PC_PATH_MAX);
        if ((currentDirectory = (char *)malloc((size_t)size)) != NULL) {
            if (getcwd(currentDirectory, size) == NULL) {
                perror("Error getting current directory");
                free(currentDirectory);
                currentDirectory = NULL;
            }
        } else {
            perror("Error allocating memory for current directory");
        }
    #endif

    return currentDirectory;
}


void fileNameWithoutExtension(char * file, char * onlyName){
    int qtd=strlen(file);
    if(strchr(file,'.')!=NULL){
        while(file[qtd]!='.') qtd--;
        onlyName[qtd--]='\0';
        while(qtd>=0){
            onlyName[qtd]=file[qtd];
            qtd--;
        }   
    }else{
        strcpy(onlyName, file);
    }
}

void fileExtension(char * file, char * onlyExtension){
    int offset=strlen(file);
    int qtt=1;
    if(strchr(file,'.')!=NULL){
        while(file[offset]!='.'){offset--; qtt++;}
        if(qtt>100) {
            strcpy(onlyExtension, "");
            return;    
        }
        memcpy (onlyExtension, file + offset, qtt);
    }else{
        strcpy(onlyExtension, "");
    }
}

char *istrstr(const char *haystack, const char *needle) {
    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }
        if (!*n)
            return (char *)haystack;
        haystack++;
    }
    return NULL;
}


char* toLower(char* str) {
  int j = 0;
 
  while (str[j]) {
        str[j]=(char) tolower(str[j]);
        j++;
  }
  return str;
}

char* toUpper(char* str) {
  int j = 0;
 
  while (str[j]) {
        str[j]=(char) toupper(str[j]);
        j++;
  }
  return str;
}

boolean isSpace(char c) {
    switch (c) {
        case ' ':
        case '\n':
        case '\t':
        case '\f':
        case '\r':
            return TRUE;
            break;
        default:
            return FALSE;
            break;
    }
}

char * Trim(char * s){
    int ix, jx;
    char * buf;
    int len = strlen(s);  /* possibly should use strnlen */
    buf = (char *) malloc(strlen(s)+1);

    for(ix=0, jx=0; ix < len; ix++)
       if(!isSpace(s[ix]))
          buf[jx++] = s[ix];
        buf[jx] = '\0';
        strncpy(s, buf, jx);  /* always looks as far as the null, but who cares? */
        free(buf);            /* no good leak goes unpunished */
        return s;             /* modifies s in place *and* returns it for swank */  
}

char* subString(const char* input, int offset, int len, char* dest)
{
  int input_len = strlen (input);
  if ((offset + len) > input_len){
     if(input!=NULL) strcpy(dest, input);
     return dest;
  }  
  char * org = strdup(input);
  memcpy (dest, org + offset, len);
  dest[len]='\0';
  free(org);
  return dest;
}

void normalizePath(char * path){
    char bef = ' ';
    char * temp = strdup(path);
    int x=0;
    for(size_t i=0; i < strlen(path);i++) {
            if (temp[i] == '\\') {
                if(bef!='\\') path[x++] = '/';
            }else{
                path[x++]=temp[i];
            }
            bef=temp[i];
        }
    path[x]='\0';
    free(temp);
}

char* getFileNameFromPath(char* path) {
        char sep = '/';
        for(size_t i = strlen(path) - 1; i; i--)  
        {
            if (path[i] == sep)
            {
                return &path[i+1];
            }
        }
        return path;
}

void getPathName(char * path, char * org) {
   char sep = '/';
    int final_pos=0;
    for(size_t i = strlen(org) - 1; i; i--){
            if (org[i] == sep)
            {
                final_pos=i;
                break;
            }
   }
   subString (org, 0, final_pos, path);
   path[final_pos]='\0';
}

int isAbsolutPath(char * path){
    if(strstr(path, "/")!=NULL) return 1;
    if(strstr(path, "\\")!=NULL) return 1;
    return 0;
}

int64_t my_getline(char **restrict line, size_t *restrict len, FILE *restrict fp) {
    if (line == NULL || len == NULL || fp == NULL) {
        errno = EINVAL;
        return -1;
    }
    char chunk[512];
    if (*line == NULL || *len < sizeof(chunk)) {
        *len = sizeof(chunk);
        if ((*line = malloc(*len)) == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }
    (*line)[0] = '\0';
    while (fgets(chunk, sizeof(chunk), fp) != NULL) {
        size_t len_used = strlen(*line);
        size_t chunk_used = strlen(chunk);
        if (*len - len_used < chunk_used) {
            if (*len > SIZE_MAX / 2) {
                errno = EOVERFLOW;
                return -1;
            } else {
                *len *= 2;
            }
            char *new_line = realloc(*line, *len);
            if (new_line == NULL) {
                free(*line);
                errno = ENOMEM;
                return -1;
            }
            *line = new_line;
        }
        memcpy(*line + len_used, chunk, chunk_used);
        len_used += chunk_used;
        (*line)[len_used] = '\0';
        if ((*line)[len_used - 1] == '\n') {
            return len_used;
        }
        //If something was read before EOF, return it even without a \n
        if (len_used > 0) {
            return (int64_t)len_used;
        }
    }
    return -1;
}

int readCodFile(struct st_cobgdb *program) {
    FILE *fp = fopen(program->name_file, "r");
    if (fp == NULL) {
        perror("Unable to open file!");
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    Lines *lines = NULL;
    Lines *line_before = NULL;
    int qtd = 0;

    while (my_getline(&line, &len, fp) != -1) {
        Lines *new_line = (Lines *)malloc(sizeof(Lines));
        new_line->line_after = NULL;
        new_line->line_before = NULL;
        new_line->breakpoint='N';
        new_line->high = NULL;

        if (lines == NULL) {
            lines = new_line;
        } else {
            new_line->line_before = line_before;
            line_before->line_after = new_line;
        }

        line_before = new_line;
        new_line->line = line;
        new_line->file_line = qtd + 1;
        qtd++;
        line = NULL;
    }

    fclose(fp);
    program->lines = lines;
    program->qtd_lines = qtd;
    return TRUE;
}

//Make sure the file exists
int file_exists(const char *filepath) {
    struct stat fileStat; // Structure to store file information
    // Attempt to get file status
    if (stat(filepath, &fileStat) == 0) {
        return TRUE;
    }
    return FALSE;
}
