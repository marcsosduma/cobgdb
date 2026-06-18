/*
 * COBGDB - GnuCOBOL Debugger
 *
 * Linux terminal management and debugger I/O redirection.
 *
 * License:
 * This software is provided "as is", without warranty of any kind,
 * express or implied. Redistribution and modification are permitted.
 *
 * Author:
 * Marcos Martins Duma
 */

#if defined(__linux__)
#include <dirent.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include "cobgdb.h"

#define BUFFER 512
extern char * gdbOutput;
extern int VIEW_LINES;
extern int VIEW_COLS;

// ==========================================================================================
// Execute a system command and capture its standard output.
//
// The returned buffer is dynamically allocated and must be freed by
// the caller.
// ==========================================================================================
char *execCommand(const char *cmd) {
    char initialColumns[100];
    char *result = malloc(BUFFER);
    result[0] = '\0';
    if(getenv("COLUMNS")!=NULL)
        strcpy(initialColumns,getenv("COLUMNS"));
    else
        strcpy(initialColumns,"80");
    setenv("COLUMNS", "500", 1);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        perror("popen");
        return NULL;
    }
    int count=1;
    while ( !feof( pipe ) ) {
        char buffer[BUFFER] = {0};
        fgets(buffer, sizeof(buffer), pipe);
        if(count>1){
            char * aux=malloc(BUFFER*count);
            aux[0]='\0';
            strcat(aux,result);
            free(result);
            result=aux;
        }
        strcat(result, buffer);
        count++;
    }
    pclose(pipe);
    setenv("COLUMNS", initialColumns, 1);
    return result;
}

// ==========================================================================================
// Suspend execution for the specified number of milliseconds.
// ==========================================================================================
void sleepMillis(int milliseconds) {
    usleep(milliseconds * 1000);
}

// ==========================================================================================
// Generate a deterministic numeric identifier from a string.
//
// The generated value is used as a unique token to identify debugger
// terminal instances.
//
// The returned string must be freed by the caller.
// ==========================================================================================
char *hashCode(const char *target) {
    unsigned int code = 0;
    int i;
    char strCode[20];

    for (i = 0; target[i] != '\0'; i++) {
        code = code * 31 + target[i];
    }

    //if (code < 0) code *= -1;
    if (code < 900000) code += 900000;

    sprintf(strCode, "%u", code);
    return strdup(strCode);
}


// ==========================================================================================
// Locate the terminal device associated with the debugger output process
// using the "ps" command.
//
// Returns the full terminal device path (for example "/dev/pts/3")
// or NULL if no matching terminal is found.
//
// The returned string must be freed by the caller.
// ==========================================================================================
char *findTtyNameByPS(const char *target) {
    char *xterm_device=NULL;
    char command[512];
    sprintf(command, "ps -u");
    char *result = execCommand(command);
    char * sleepVal = hashCode(target);
    char findValue[50];
    sprintf(findValue,"sleep %s", sleepVal);
    char *line;
    char *saveptr1, *saveptr2;
    line = strtok_r(result, "\n", &saveptr1);
    while (line != NULL) {
        if (strstr(line, findValue)) {
            char *pts = strtok_r(line, " ", &saveptr2);
            while (pts != NULL) {
                if (strstr(pts, "pts") || strstr(pts, "tty")) {
                    char buffer[512];
                    sprintf(buffer, "/dev/%s", pts);
                    xterm_device = strdup(buffer);
                }
                pts = strtok_r(NULL, " ", &saveptr2);
            }
        }
        line = strtok_r(NULL, "\n", &saveptr1);    
    }
    free(sleepVal);
    free(result);
    return xterm_device;
}

// ==========================================================================================
// Check whether a string contains only numeric characters.
// ==========================================================================================
static int is_number_str(const char *s) {
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

// ==========================================================================================
// Locate the terminal device associated with the debugger output process.
//
// The function first searches the /proc filesystem for a process
// matching the generated debugger token and retrieves its terminal.
//
// Returns the terminal device path or NULL if not found.
//
// The returned string must be freed by the caller.
// ==========================================================================================
char *findTtyName(const char *target) {
    if (!target) return NULL;
    uid_t myuid = getuid();
    char *sleepVal = hashCode(target);
    if (!sleepVal) return NULL;
    char needle[128];
    if (snprintf(needle, sizeof(needle), "sleep %s", sleepVal) >= (int)sizeof(needle)) {
        free(sleepVal);
        return findTtyNameByPS(target);
    }
    DIR *d = opendir("/proc");
    if (!d) {
        free(sleepVal);
        return findTtyNameByPS(target);
    }
    struct dirent *de;
    char *xterm_device = NULL;
    while ((de = readdir(d)) != NULL) {
        if (!is_number_str(de->d_name)) continue;
        char procdir_path[PATH_MAX];
        snprintf(procdir_path, sizeof(procdir_path), "/proc/%s", de->d_name);
        struct stat st;
        if (stat(procdir_path, &st) != 0) {
            continue;
        }
        if (st.st_uid != myuid) {
            continue;
        }
        char cmdline_path[PATH_MAX];
        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", de->d_name);
        FILE *f = fopen(cmdline_path, "rb");
        if (!f) continue;
        char cmdline[4096];
        size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, f);
        fclose(f);
        if (n == 0) continue;
        cmdline[n] = '\0';
        for (size_t i = 0; i < n; i++) {
            if (cmdline[i] == '\0') cmdline[i] = ' ';
        }
        if (!strstr(cmdline, needle)) continue;
        char fd0_path[PATH_MAX];
        snprintf(fd0_path, sizeof(fd0_path), "/proc/%s/fd/0", de->d_name);
        char linkbuf[PATH_MAX];
        ssize_t ln = readlink(fd0_path, linkbuf, sizeof(linkbuf) - 1);
        if (ln <= 0) continue;
        linkbuf[ln] = '\0';
        if (strncmp(linkbuf, "/dev/", 5) == 0) {
            free(xterm_device);
            xterm_device = strdup(linkbuf);
        }
    }

    closedir(d);
    free(sleepVal);
    return xterm_device;
}

// ==========================================================================================
// Create a detached terminal process using fork() and execvp().
// ==========================================================================================
void createTerminalProcess(char *xterm_args[]) {
    int child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (child_pid == 0) { // Child process
        if (execvp(xterm_args[0], xterm_args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
}

// ==========================================================================================
// Launch an XFCE Terminal instance configured for debugger output.
// ==========================================================================================
void createXFCETerminal(char *sleepVal, const char *target) {
    char title[100]; // Maximum title length.
    snprintf(title, sizeof(title) - 1, "GnuCOBOL Debug - %s", target);
    char param[100];
    snprintf(param, sizeof(param) - 1, "bash -c \"echo 'GnuCOBOL DEBUG'; sleep %s;\"", sleepVal);

    char *xfce4_terminal_args[] = {
        "xfce4-terminal",
        "--title", title,
        "--font=DejaVu Sans Mono 14",
        "--command", param,
        NULL
    };
    createTerminalProcess(xfce4_terminal_args);
}


// ==========================================================================================
// Launch an xterm instance configured for debugger output.
// ==========================================================================================
void createTerminal(char * sleepVal, const char *target) {
    char title[100]; // Maximum Title Length.
    snprintf(title, sizeof(title)-1, "GnuCOBOL Debug - %s", target);
    char param[100]; 
    snprintf(param, sizeof(param)-1, "echo 'GnuCOBOL DEBUG'; sleep %s;", sleepVal);
    char *xterm_args[] = {
        "xterm",
        "-title",title,
        "-fa", "DejaVu Sans Mono",
        "-fs", "14",
        "-e", param,
        NULL
    };
    createTerminalProcess(xterm_args);
    sleep(1);
}

// ==========================================================================================
// Launch a Konsole terminal instance configured for debugger output.
// ==========================================================================================
void createKDETerminal(char *sleepVal, const char *target) {
    char title[100]; // Maximum title length.
    snprintf(title, sizeof(title) - 1, "GnuCOBOL Debug - %s", target);
    char param[100];
    snprintf(param, sizeof(param) - 1, "bash -c 'echo \"GnuCOBOL DEBUG\"; sleep %s;'", sleepVal);

    char *konsole_args[] = {
        "konsole",
        "--title", title,
        "--separate",  // Open in a new tab
        "--nofork",    // Do not fork a new process (useful for waiting)
        //"--hold",    // Keep the terminal open after the command finishes
        "-e", param,   // Execute the command
        NULL
    };
    createTerminalProcess(konsole_args);    
    sleep(1);
}

// ==========================================================================================
// Launch a GNOME Terminal instance configured for debugger output.
// ==========================================================================================
void createGNOMETerminal(char *sleepVal, const char *target) {
    char title[100];
    snprintf(title, sizeof(title) - 1, "GnuCOBOL Debug - %s", target);
    char param[100];
    snprintf(param, sizeof(param) - 1, "echo 'GnuCOBOL DEBUG'; sleep %s;", sleepVal);
    char *gnome_terminal_args[] = {
        "gnome-terminal",
        "--title", title,
        "--",                // Indicates the end of gnome-terminal arguments and the start of the command to be executed
        "bash", "-c", param, // Command to be executed
        NULL
    };
    createTerminalProcess(gnome_terminal_args);
    sleep(1);
}

// ==========================================================================================
// Launch a Ptyxis terminal instance configured for debugger output.
// ==========================================================================================
void createPTYXISTerminal(char *sleepVal, const char *target) {
    setenv("GSK_RENDERER", "cairo", 1);
    char param[512]; 
    snprintf(param, sizeof(param), 
             "echo -ne '\\033]0;GnuCOBOL Debug - %s\\007'; "
             "echo 'GnuCOBOL DEBUG'; "
             "sleep %s;", 
             target, sleepVal);
    char *ptyxis_args[] = {
        "ptyxis",
        "--",                
        "bash", "-c", param,
        NULL
    };
    createTerminalProcess(ptyxis_args);    
    sleep(1);
    unsetenv("GSK_RENDERER");
}

// ==========================================================================================
// Check whether a terminal emulator is available in the current system.
// ==========================================================================================
int isTerminalInstalled(const char *terminalCommand) {
    char command[100];
    snprintf(command, sizeof(command) - 1, "which %s >/dev/null 2>/dev/null", terminalCommand);
    return system(command) == 0;
}

// ==========================================================================================
// Copy text to the system clipboard using xclip.
//
// Returns 1 on success or 0 on failure.
// ==========================================================================================
int copyToClipboard(const char *text) {
    // Check if xclip is available
    if (system("which xclip > /dev/null 2> /dev/null") == 0) {
        FILE *clipboard = popen("xclip -selection clipboard", "w");
        if (clipboard) {
            fprintf(clipboard, "%s", text);
            pclose(clipboard);
            return 1;
        }
    }
    return 0;
}

// ==========================================================================================
// Configure the GDB inferior terminal used for program input/output.
//
// The function attempts to locate an existing debugger terminal or
// creates a new terminal emulator instance when necessary. Once the
// terminal is available, its device path is assigned to GDB through
// the "inferior-tty" setting.
// ==========================================================================================
void openOuput(int (*sendCommandGdb)(char *), char *target){
    char aux[512];
    int status, tk=0;
    char * sleepVal=hashCode(target);
    char *xterm_device = findTtyName(target);

    //disableEcho();
    if(xterm_device == NULL){
        message_output_linux(sleepVal);
        xterm_device = findTtyName(target);
    }
    if (xterm_device == NULL) {
        // Find terminal
        if (isTerminalInstalled("xterm")) {
            createTerminal(sleepVal, target);
        }else if (isTerminalInstalled("gnome-terminal")) {
            createGNOMETerminal(sleepVal, target);
        }else if (isTerminalInstalled("xfce4-terminal")) {
            createXFCETerminal(sleepVal, target);
        }else if (isTerminalInstalled("konsole")) {
            createKDETerminal(sleepVal, target);
        }else if (isTerminalInstalled("ptyxis")) {
            createPTYXISTerminal(sleepVal, target);
        }
        int try_find = 0;
        while (try_find < 4) {
            sleepMillis(500);
            xterm_device = findTtyName(target);
            try_find++;
            if (xterm_device != NULL){
                break;
            }
        }
    }
    if(xterm_device!=NULL){
        sprintf(aux,"-gdb-set inferior-tty %s\n", xterm_device);
        sendCommandGdb(aux);
        while(TRUE){
            sendCommandGdb("");
            if(strlen(gdbOutput)<2) continue;
            MI2onOuput(sendCommandGdb, tk, &status);
            break;
        }
        setenv("TERM","xterm",1);
        free(xterm_device);
    }else{
        exit(1);
    }
    free(sleepVal);
}
#endif