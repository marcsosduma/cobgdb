// This will only have effect on Windows with MSVC
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
#include "cobgdb.h"

char* toLower(char* str) {
  int j = 0;
  char ch;
 
  while (str[j]) {
        str[j]=(char) tolower(str[j]);
        j++;
  }
  return str;
}

char* toUpper(char* str) {
  int j = 0;
  char ch;
 
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

char* subString (const char* input, int offset, int len, char* dest)
{
  int input_len = strlen (input);
  if ((offset + len) > input_len){
     if(input!=NULL) strcpy(dest, input);
     return NULL;
  }
  char * org = strdup(input);
  memcpy (dest, org + offset, len);
  dest[len]='\0';
  free(org);
  return dest;
}

void normalizePath(char * path){
    char sep = '/';
    for(size_t i=0; i < strlen(path);i++) {
            if (path[i] == '\\')
            {
                path[i] = '/';
            }
        }
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
    // Check if either line, len or fp are NULL pointers
    if(line == NULL || len == NULL || fp == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    // Use a chunk array of 128 bytes as parameter for fgets
    char chunk[128];

    // Allocate a block of memory for *line if it is NULL or smaller than the chunk array
    if(*line == NULL || *len < sizeof(chunk)) {
        *len = sizeof(chunk);
        if((*line = malloc(*len)) == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    // "Empty" the string
    (*line)[0] = '\0';

    while(fgets(chunk, sizeof(chunk), fp) != NULL) {
        // Resize the line buffer if necessary
        size_t len_used = strlen(*line);
        size_t chunk_used = strlen(chunk);

        if(*len - len_used < chunk_used) {
            // Check for overflow
            if(*len > SIZE_MAX / 2) {
                errno = EOVERFLOW;
                return -1;
            } else {
                *len *= 2;
            }
            
            if((*line = realloc(*line, *len)) == NULL) {
                errno = ENOMEM;
                return -1;
            }
        }

        // Copy the chunk to the end of the line buffer
        memcpy(*line + len_used, chunk, chunk_used);
        len_used += chunk_used;
        (*line)[len_used] = '\0';

        // Check if *line contains '\n', if yes, return the *line length
        if((*line)[len_used - 1] == '\n') {
            return len_used;
        }
    }

    return -1;
}

typedef struct lista Lista;

int readCodFile(struct program_file * program) {
    FILE *fp = fopen(program->name_file, "r");
    if(fp == NULL) {
        perror("Unable to open file!");
        exit(1);
    }

    // Read lines from a text file using our own a portable getline implementation
    char *line = NULL;
    size_t len = 0;
    program->lines = NULL;

    Lines * line_before = NULL;
    int qtd = 0;
    while(my_getline(&line, &len, fp) != -1) {
        // fputs(line, stdout);
        // fputs("|*\n", stdout);
        //printf("line length: %zd -> %s\n", strlen(line), line);
        Lines* new_line = (Lines*) malloc(sizeof(Lines));
        new_line->line_after=NULL;
        new_line->line_before=NULL;
        new_line->high=NULL;
        if(program->lines == NULL){
             program->lines = new_line;             
        }else{
            new_line->line_before = line_before;
            line_before->line_after = new_line;            
        }
        line_before = new_line;
        new_line->line = line;
        new_line->file_line=qtd+1;
        qtd++;
        line = NULL;
    }
    fclose(fp);
    //free(line);
    program->qtd_lines=qtd;
}

/////////////////////////////////////////////////////////
//
// Example:
// Given path == "C:\\dir1\\dir2\\dir3\\file.exe"
// will return path_ as   "C:\\dir1\\dir2\\dir3"
// Will return base_ as   "file"
// Will return ext_ as    "exe"
//
/////////////////////////////////////////////////////////
void GetFileParts(char *path, char *path_, char *base_, char *ext_)
{
    char *base;
    char *ext;
    char nameKeep[256];
    char pathKeep[256];
    char pathKeep2[256]; //preserve original input string
    char File_Ext[40];
    char baseK[40];
    int lenFullPath, lenExt_, lenBase_;
    char *sDelim={0};
    int   iDelim=0;
    int  rel=0, i;

    if(path)
    {   //determine type of path string (C:\\, \\, /, ./, .\\)
        if(  (strlen(path) > 1) &&

            (
            ((path[1] == ':' ) &&
             (path[2] == '\\'))||

             (path[0] == '\\') ||

             (path[0] == '/' ) ||

            ((path[0] == '.' ) &&
             (path[1] == '/' ))||

            ((path[0] == '.' ) &&
             (path[1] == '\\'))
            )
        )
        {
            sDelim = calloc(5, sizeof(char));
            /*  //   */if(path[0] == '\\') iDelim = '\\', strcpy(sDelim, "\\");
            /*  c:\\ */if(path[1] == ':' ) iDelim = '\\', strcpy(sDelim, "\\"); // also satisfies path[2] == '\\'
            /*  /    */if(path[0] == '/' ) iDelim = '/' , strcpy(sDelim, "/" );
            /* ./    */if((path[0] == '.')&&(path[1] == '/')) iDelim = '/' , strcpy(sDelim, "/" );
            /* .\\   */if((path[0] == '.')&&(path[1] == '\\')) iDelim = '\\' , strcpy(sDelim, "\\" );
            /*  \\\\ */if((path[0] == '\\')&&(path[1] == '\\')) iDelim = '\\', strcpy(sDelim, "\\");
            if(path[0]=='.')
            {
                rel = 1;
                path[0]='*';
            }

            if(!strstr(path, "."))  // if no filename, set path to have trailing delim,
            {                      //set others to "" and return
                lenFullPath = strlen(path);
                if(path[lenFullPath-1] != iDelim)
                {
                    strcat(path, sDelim);
                    path_[0]=0;
                    base_[0]=0;
                    ext_[0]=0;
                }
            }
            else
            {
                nameKeep[0]=0;         //works with C:\\dir1\file.txt
                pathKeep[0]=0;
                pathKeep2[0]=0;        //preserves *path
                File_Ext[0]=0;
                baseK[0]=0;

                //Get lenth of full path
                lenFullPath = strlen(path);

                strcpy(nameKeep, path);
                strcpy(pathKeep, path);
                strcpy(pathKeep2, path);
                strcpy(path_, path); //capture path

                //Get length of extension:
                for(i=lenFullPath-1;i>=0;i--)
                {
                    if(pathKeep[i]=='.') break; 
                }
                lenExt_ = (lenFullPath - i) -1;

                base = strtok(path, sDelim);
                while(base)
                {
                    strcpy(File_Ext, base);
                    base = strtok(NULL, sDelim);
                }


                strcpy(baseK, File_Ext);
                lenBase_ = strlen(baseK) - lenExt_;
                baseK[lenBase_-1]=0;
                strcpy(base_, baseK);

                path_[lenFullPath -lenExt_ -lenBase_ -1] = 0;

                ext = strtok(File_Ext, ".");
                ext = strtok(NULL, ".");
                if(ext) strcpy(ext_, ext);
                else strcpy(ext_, "");
            }
            memset(path, 0, lenFullPath);
            strcpy(path, pathKeep2);
            if(rel)path_[0]='.';//replace first "." for relative path
            free(sDelim);
        }
    }
}