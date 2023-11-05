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

void createXtermProcess(char *xterm_args[]) {
    int child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (child_pid == 0) { // Child process
        if (execvp("xterm", xterm_args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
}

// Open an xterm terminal if necessary.
void createTerminal(char * sleepVal, const char *target) {
    char title[100]; // Maximum Title Length.
    snprintf(title, sizeof(title)-1, "GnuCOBOL Debug - %s", target);
    char param[100]; 
    snprintf(param, sizeof(param)-1, "/usr/bin/tty; echo 'GnuCOBOL DEBUG'; sleep %s;", sleepVal);
    char *xterm_args[] = {
        "xterm",
        "-title",title,
        "-fa", "DejaVu Sans Mono",
        "-fs", "14",
        "-e", param,
        NULL
    };
    createXtermProcess(xterm_args);
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
        createTerminal(sleepVal, target);
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