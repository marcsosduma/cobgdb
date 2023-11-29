#if defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cobgdb.h"

#define BUFFER 512

// run system command and read output
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

// sleep
void sleepMillis(int milliseconds) {
    usleep(milliseconds * 1000);
}


char *hashCode(const char *target) {
    unsigned int code = 0;
    int i;
    char strCode[20];

    for (i = 0; target[i] != '\0'; i++) {
        code = code * 31 + target[i];
    }

    if (code < 0) code *= -1;
    if (code < 900000) code += 900000;

    sprintf(strCode, "%u", code);
    return strdup(strCode);
}

// fint the tty for output
char *findTtyName(const char *target) {
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
                if (strstr(pts, "pts")) {
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

// Open an XFCE terminal if necessary.
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


// Open an xterm terminal if necessary.
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
}

// Open a KDE terminal (Konsole) if necessary.
void createKDETerminal(char *sleepVal, const char *target) {
    char title[100]; // Maximum title length.
    snprintf(title, sizeof(title) - 1, "GnuCOBOL Debug - %s", target);
    char param[100];
    snprintf(param, sizeof(param) - 1, "bash -c 'echo \"GnuCOBOL DEBUG\"; sleep %s;'", sleepVal);

    char *konsole_args[] = {
        "konsole",
        "--title", title,
        "--separate", // Open in a new tab
        "--nofork",    // Do not fork a new process (useful for waiting)
        //"--hold",      // Keep the terminal open after the command finishes
        "-e", param,   // Execute the command
        NULL
    };
    createTerminalProcess(konsole_args);    
}

// Open a GNOME terminal if necessary.
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
}

// Function to check the availability of a terminal
int isTerminalInstalled(const char *terminalCommand) {
    // The "which" command returns the path of the executable if it's installed
    char command[100];
    snprintf(command, sizeof(command) - 1, "which %s >/dev/null 2>/dev/null", terminalCommand);

    // system returns 0 if the command is successful
    return system(command) == 0;
}


char * openOuput(char *target){
    char alert1[80];
    char aux[100];
    char * sleepVal=hashCode(target);
    char *xterm_device = findTtyName(target);
    if(xterm_device == NULL){
        clearScreen();
        gotoxy(12,10);print_no_reset("COBGDB will open XTERM to perform the application's output.\n",color_green);
        gotoxy(12,11);printf("If you want to use another terminal, open the terminal  and\n");
        gotoxy(12,12);printf("type the following command:                                \n");
        sprintf(alert1,"sleep %s;\n", sleepVal);
        sprintf(aux,"%-59s",alert1);
        gotoxy(12,13);print(aux,color_red);
        gotoxy(12,14);print("After that, press a key.                                   \n", color_green);
        print_color_reset();
        gotoxy(12,15);while(key_press()==0);
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
        } 
        int try_find = 0;
        while (try_find < 4) {
            sleepMillis(500);
            xterm_device = findTtyName(target);
            try_find++;
            if (xterm_device != NULL) break;
        }
    }else{
       setenv("TERM","xterm",1);
    }
    free(sleepVal);
    return xterm_device;
}
#endif