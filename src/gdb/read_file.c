/* 
 * This code includes the 'my_getline' function obtained from the following site: 
 * https://solarianprogrammer.com/2019/04/03/c-programming-read-file-lines-fgets-getline-implement-portable-getline/
 * 
 * It is provided without any warranty, express or implied. 
 * You may modify and distribute it at your own risk.
 * 
 * Author: Marcos Martins Duma
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

// =====================================================
// Global Variables
// =====================================================
extern struct st_cobgdb cob;
extern struct st_cfiles * parsed_cfiles;

// ==========================================================================================
// Get the current working directory.
//
// Retrieves the current process working directory using the
// appropriate platform-specific API.
//
// On Windows, _getcwd() is used.
// On POSIX systems, getcwd() is used.
//
// Returns:
//   Dynamically allocated string containing the current directory.
//   NULL on failure.
//
// The returned buffer must be freed by the caller.
// ==========================================================================================
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

// ==========================================================================================
// Extract the filename without its extension.
//
// Example:
//
//   Input : "program.cbl"
//   Output: "program"
//
// Parameters:
//   file      - Original filename.
//   onlyName  - Destination buffer.
//
// Returns:
//   Nothing.
// ==========================================================================================
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

// ==========================================================================================
// Extract the file extension from a filename.
//
// Example:
//
//   Input : "program.cbl"
//   Output: ".cbl"
//
// Parameters:
//   file           - Original filename.
//   onlyExtension  - Destination buffer.
//
// Returns:
//   Nothing.
// ==========================================================================================
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

// ==========================================================================================
// Perform a case-insensitive substring search.
//
// Similar to strstr(), but ignores character case.
//
// Parameters:
//   haystack - String to search in.
//   needle   - String to search for.
//
// Returns:
//   Pointer to the first occurrence.
//   NULL if not found.
// ==========================================================================================
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

// ==========================================================================================
// Convert a string to lowercase in place.
//
// Parameters:
//   str - String to convert.
//
// Returns:
//   Pointer to the modified string.
// ==========================================================================================
char* toLower(char* str) {
  int j = 0;
 
  while (str[j]) {
        str[j]=(char) tolower(str[j]);
        j++;
  }
  return str;
}

// ==========================================================================================
// Convert a string to uppercase in place.
//
// Parameters:
//   str - String to convert.
//
// Returns:
//   Pointer to the modified string.
// ==========================================================================================
char* toUpper(char* str) {
  int j = 0;
 
  while (str[j]) {
        str[j]=(char) toupper(str[j]);
        j++;
  }
  return str;
}

// ==========================================================================================
// Check whether a character is considered whitespace.
//
// Recognized characters:
//
//   ' '
//   '\n'
//   '\r'
//   '\t'
//   '\f'
//
// Returns:
//   TRUE  if the character is whitespace.
//   FALSE otherwise.
// ==========================================================================================
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

// ==========================================================================================
// Remove all whitespace characters from a string.
//
// Unlike a traditional trim operation, this function removes
// every whitespace character found in the string.
//
// Example:
//
//   "A B C" -> "ABC"
//
// Parameters:
//   s - String to modify.
//
// Returns:
//   Pointer to the modified string.
// ==========================================================================================
char * Trim(char * s){
    int ix, jx;
    char * buf;
    int len = strlen(s);  /* possibly should use strnlen */
    buf = (char *) malloc(strlen(s)+1);

    for(ix=0, jx=0; ix < len; ix++)
       if(!isSpace(s[ix]))
          buf[jx++] = s[ix];
        buf[jx] = '\0';
        strncpy(s, buf, jx);  
        free(buf);            
        return s;             
}

// ==========================================================================================
// Extract a substring from a string.
//
// Parameters:
//   input  - Source string.
//   offset - Starting position.
//   len    - Number of characters to copy.
//   dest   - Destination buffer.
//
// Returns:
//   Pointer to the destination buffer.
// ==========================================================================================
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

// ==========================================================================================
// Normalize a filesystem path.
//
// Converts Windows path separators ('\') into POSIX-style
// separators ('/') and removes duplicate separators.
//
// Parameters:
//   path - Path string to normalize.
//
// Returns:
//   Nothing.
// ==========================================================================================
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

// ==========================================================================================
// Extract the filename portion from a full path.
//
// Example:
//
//   "/tmp/test.cbl" -> "test.cbl"
//
// Parameters:
//   path - Full file path.
//
// Returns:
//   Pointer to the filename inside the original buffer.
// ==========================================================================================
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

// ==========================================================================================
// Extract the directory portion of a full path.
//
// Example:
//
//   Input : "/tmp/test.cbl"
//   Output: "/tmp"
//
// Parameters:
//   path - Destination buffer.
//   org  - Full path.
//
// Returns:
//   Nothing.
// ==========================================================================================
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

// ==========================================================================================
// Determine whether a path is absolute or relative.
//
// Return values:
//
//   0 - Relative path.
//   1 - Absolute path.
//   2 - Relative path containing directory separators.
//
// Parameters:
//   path - Path to evaluate.
//
// Returns:
//   Classification value.
// ==========================================================================================
int isAbsolutePath(char *path) {
    if (!path || !*path)
        return 0;
#ifdef _WIN32
    if (isalpha((unsigned char)path[0]) && path[1] == ':' &&
        (path[2] == '\\' || path[2] == '/'))
        return 1;    
    if (path[0] == '\\' && path[1] == '\\')
        return 1;
    if (path[0] == '/')
        return 1;
    if(strstr(path, "/")!=NULL) return 2;
    if(strstr(path, "\\")!=NULL) return 2;
#else
    if (path[0] == '/')
        return 1;
    if(strstr(path, "/")!=NULL) 
        return 2;
#endif
    return 0;
}

// ==========================================================================================
// Portable getline() implementation.
//
// Reads a complete line from a file stream and automatically
// expands the destination buffer as needed.
//
// Parameters:
//   line - Pointer to destination buffer.
//   len  - Current buffer size.
//   fp   - File stream.
//
// Returns:
//   Number of bytes read.
//   -1 on error or EOF.
// ==========================================================================================
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


void sanitize_to_utf8(char *str) {
    unsigned char *p = (unsigned char *)str;
    while (*p) {
        // If the byte is greater than 127, it can cause problems in UTF-8 if not properly formatted.
        // We replace control or invalid characters with '?'
        if (*p > 127) {
            *p = '?'; 
        }
        p++;
    }
}

// ==========================================================================================
// Load a COBOL source file into memory.
//
// Reads the source file line by line and builds a doubly linked
// list of Lines structures.
//
// Parameters:
//   program - Debugger program structure.
//
// Returns:
//   TRUE on success.
// ==========================================================================================
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
        //sanitize_to_utf8(line);
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

// ==========================================================================================
// Check whether a file exists.
//
// Parameters:
//   filepath - File path to test.
//
// Returns:
//   TRUE if the file exists.
//   FALSE otherwise.
// ==========================================================================================
int file_exists(const char *filepath) {
    struct stat fileStat; // Structure to store file information
    // Attempt to get file status
    if (stat(filepath, &fileStat) == 0) {
        return TRUE;
    }
    return FALSE;
}

// ==========================================================================================
// Save all breakpoints to a breakpoint file.
//
// Creates a .bkp file associated with the specified source file
// and stores all breakpoint locations.
//
// Parameters:
//   head     - Breakpoint list.
//   filename - Source filename.
//
// Returns:
//   TRUE on success.
//   FALSE on error.
// ==========================================================================================
int save_breakpoints(struct st_bkpoint *head, char *filename) {
    char baseName[512];    
    fileNameWithoutExtension(filename, &baseName[0]);
    normalizePath(baseName);
    strcat(baseName, ".bkp");
    FILE *fp = fopen(baseName, "w");
    if (!fp) {
        perror("fopen");
        return FALSE;
    }
    struct st_bkpoint *cur = head;
    while (cur) {
        fprintf(fp, "%s %d\n", cur->name, cur->line);
        cur = cur->next;
    }
    fclose(fp);
    return TRUE;
}

// ==========================================================================================
// Restore all breakpoints into GDB.
//
// Removes existing breakpoints and recreates every breakpoint
// contained in the provided list.
//
// Parameters:
//   sendCommandGdb - Function used to communicate with GDB.
//   bp_list        - Breakpoint list.
//
// Returns:
//   TRUE on success.
// ==========================================================================================
int set_break_on_file(int (*sendCommandGdb)(char *), struct st_bkpoint *bp_list) {
    struct st_bkpoint *bp = bp_list;
    if(bp_list == NULL) return FALSE;
    MI2removeAllBreakPoint(sendCommandGdb);
    struct st_bkpoint *prev = NULL;
    while(bp!=NULL){
        if(prev==NULL || strcmp(prev->name, bp->name)!=0)
            loadLibrary(bp->name);
        MI2addBreakPoint(sendCommandGdb, bp->name, bp->line);
        prev=bp;
        bp= bp->next;
    }

    bp = bp_list;
    Lines * line = cob.lines;
    while(line!=NULL){
        line->breakpoint='N';
        ST_bk * search = bp;
        while(search!=NULL){
            int check=hasCobolLine(line->file_line);
            if(check>0){
                if(search->line==line->file_line && strcasecmp(search->name, cob.file_cobol)==0)
                    line->breakpoint='S';
            }
            search=search->next;
        }            
        line=line->line_after;
    }
    return TRUE;
}

// ==========================================================================================
// Load breakpoints from a .bkp file.
//
// Reads all saved breakpoints and restores them into the debugger.
//
// Parameters:
//   sendCommandGdb - Function used to communicate with GDB.
//   head           - Existing breakpoint list.
//   filename       - Source filename.
//
// Returns:
//   Updated breakpoint list.
// ==========================================================================================
struct st_bkpoint *load_breakpoints(int (*sendCommandGdb)(char *), struct st_bkpoint *head, char *filename) {
    char baseName[512];    
    fileNameWithoutExtension(filename, &baseName[0]);
    normalizePath(baseName);
    strcat(baseName, ".bkp");
    if(!file_exists(baseName)){
        showCobMessage("Breakpoint file not found.",1);
        return head;
    }
    FILE *fp = fopen(baseName, "r");
    if (!fp) return head;
    freeBKList();
    head = NULL;
    struct st_bkpoint *tail = NULL;
    char namebuf[256];
    int line;
    while (fscanf(fp, "%255s %d", namebuf, &line) == 2) {
        struct st_bkpoint *node = malloc(sizeof(*node));
        node->name = strdup(namebuf);
        node->line = line;
        node->next = NULL;
        if (!head)
            head = tail = node;
        else {
            tail->next = node;
            tail = node;
        }
    }
    fclose(fp);
    set_break_on_file(sendCommandGdb, head);
    showCobMessage("Breakpoint file loaded.",1);
    return head;
}

// ==========================================================================================
// Save debugger configuration values.
//
// Stores user preferences in the configuration file located
// in the user's home directory.
//
// Saved values:
//
//   - Color theme
//   - Auto-step delay
//
// Returns:
//   Nothing.
// ==========================================================================================
void save_cfg_value(int color, int speed_delay)
{
    char *home = NULL;
    char msg[1024];

    #if defined(_WIN32)
    home = getenv("USERPROFILE");
    if (!home) home = getenv("HOMEPATH");
    #else
    home = getenv("HOME");
    #endif
    if (!home) {
        showCobMessage("Error: could not determine HOME directory.",1);
        return;
    }
    char path[512];
    #if defined(_WIN32)
    snprintf(path, sizeof(path), "%s\\cobgdb.cfg", home);
    #else
    snprintf(path, sizeof(path), "%s/cobgdb.cfg", home);
    #endif
    FILE *f = fopen(path, "w");
    if (!f) {
        showCobMessage("Error: unable to open file for writing", 1);
        return;
    }
    fprintf(f, "%d\n", color);
    fprintf(f, "%d\n", speed_delay);
    fclose(f);
    snprintf(msg, 1024, "Color: %d  Autostep Speed: %d saved to: %s", color, speed_delay+1, path);
    showCobMessage(msg, 1);
}

// ==========================================================================================
// Load debugger configuration values.
//
// Reads the configuration file from the user's home directory
// and restores previously saved settings.
//
// Returns:
//   Configured color theme.
//   -1 if the configuration file does not exist.
// ==========================================================================================
int load_cfg_value(void)
{
    char *home = NULL;

    #if defined(_WIN32)
    home = getenv("USERPROFILE");
    if (!home) home = getenv("HOMEPATH");
    #else
    home = getenv("HOME");
    #endif
    if (!home) return -1;
    char path[512];
    #if defined(_WIN32)
    snprintf(path, sizeof(path), "%s\\cobgdb.cfg", home);
    #else
    snprintf(path, sizeof(path), "%s/cobgdb.cfg", home);
    #endif
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int color = -1;
    int speed_delay = -1;
    fscanf(f, "%d", &color);
    fscanf(f, "%d", &speed_delay);
    fclose(f);
    if(speed_delay>=0) cob.auto_step_delay = speed_delay;
    return color;
}
