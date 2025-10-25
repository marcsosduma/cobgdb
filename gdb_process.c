/*
 * COBGDB GnuCOBOL Debugger:
 *
 * License:
 * This code is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 * 
 * Author: Marcos Martins Duma
 */

#if defined(_WIN32)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#elif defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#define TRUE 1
#define FALSE 0
#define clrscr() printf("\e[1;1H\e[2J")
#endif // Windows/Linux
#include "cobgdb.h"
#define BUFSIZE 2048
#define BUFFER_OUTPUT_SIZE 1024
//#define DEBUG 1

extern struct st_cobgdb cob;
extern char * gdbOutput;
int token=1;

#if defined(_WIN32)

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;
TCHAR command[512];

int debug(int (*sendCommandGdb)(char *));
void CreateChildProcess(void);
int sendCommandGdb(char * command);
void ErrorExit(char *);
//TCHAR szCmdline[]=TEXT("gdb --interpreter=mi2 client.exe");

int start_gdb(char * name, char * cwd)
{
   char fixCommand[] = "gdb -nx --quiet --interpreter=mi2 ";
   sprintf(command, "%s", fixCommand);
   SECURITY_ATTRIBUTES saAttr;

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   // Create a pipe for the child process's STDOUT.
   if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
      ErrorExit("StdoutRd CreatePipe");

   if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
      ErrorExit("Stdout SetHandleInformation");

   DWORD dwMode = PIPE_NOWAIT;
   SetNamedPipeHandleState(
      g_hChildStd_OUT_Rd,
      &dwMode,
      NULL,
      NULL);

   // Create a pipe for the child process's STDIN.
   if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
      ErrorExit("Stdin CreatePipe");

   if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
      ErrorExit("Stdin SetHandleInformation");

   // Create the child process.
   CreateChildProcess();

   char env_dir[512];
   char f_symb[512];
   sprintf(env_dir, "-environment-directory \"%s\"\n", cwd);
   sprintf(f_symb, "-file-exec-and-symbols \"%s/%s\"\n", cwd, name);

   char * gdbSet[] = {
      "-gdb-set target-async on\n",
      "-gdb-set mi-async on\n",
      "-gdb-set print repeats 1000\n",
      "-gdb-set charset UTF-8\n",
      "-gdb-set mi-notify-stdio on\n",
      "-gdb-set print sevenbit-strings off\n",
      "-gdb-set print static-members off\n",
      "-gdb-set print pretty on\n",
      "-gdb-set new-console on\n",
      env_dir,
      f_symb,
      ""
   };

   int idx = 0;
   while (strlen(gdbSet[idx]) >= 1) {
      sendCommandGdb(gdbSet[idx++]);
   }

   cob.waitAnswer = 1;
   debug(&sendCommandGdb);

   printf("\n->End of parent execution.\n");
   return 0;
}

void CreateChildProcess()
{
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE;

   ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
   ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = g_hChildStd_OUT_Wr;
   siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

   bSuccess = CreateProcess(NULL,
      command,
      NULL,
      NULL,
      TRUE,
      0,
      NULL,
      NULL,
      &siStartInfo,
      &piProcInfo);

   if (!bSuccess)
      ErrorExit(TEXT("CreateProcess"));
   else {
      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);

      WaitForSingleObject(piProcInfo.hProcess, INFINITE);

      CloseHandle(g_hChildStd_OUT_Wr);
      CloseHandle(g_hChildStd_IN_Rd);
   }
}

int sendCommandGdb(char * command)
{
   DWORD dwRead, dwWritten;
   CHAR chBuf[BUFSIZE];
   BOOL bSuccess = FALSE;
   int qtdAlloc = 0;
   int mustReturn = 1;
   int count0 = 0;

   #ifdef DEBUG
   HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
   #endif

   int tk = token;
   if (strlen(command) > 0) {
      if (strstr(command, "-gdb-exit\n") != NULL || strstr(command, "-gdb-set") != NULL ||
          strstr(command, "-environment-directory") != NULL ||
          strstr(command, "-file-exec-and-symbols") != NULL) {
         strcpy(chBuf, command);
         mustReturn = 0;
         cob.waitAnswer = 1;
      } else {
         sprintf(chBuf, "%d-%s", token++, command);
      }

      bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, strlen(chBuf), &dwWritten, NULL);
      if (!bSuccess) return 0;
      if (mustReturn) return tk;
   }

   if (gdbOutput != NULL) {
      strcat(gdbOutput, "\n");
      if (strcmp(gdbOutput, "\n") == 0) strcpy(gdbOutput, "");
      qtdAlloc = strlen(gdbOutput);
   } else {
      qtdAlloc = 0;
      gdbOutput = malloc(BUFFER_OUTPUT_SIZE);
      strcpy(gdbOutput, "");
   }

   strcpy(chBuf, "");
   for (;;) {
      bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      #ifdef DEBUG
      bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
      #endif

      qtdAlloc = (gdbOutput != NULL) ? strlen(gdbOutput) + 1 : 0;
      if (dwRead > 0) {
         chBuf[dwRead] = '\0';
         qtdAlloc += dwRead + 1;
         if (qtdAlloc > BUFFER_OUTPUT_SIZE) {
            gdbOutput = realloc(gdbOutput, qtdAlloc);
         }
         strcat(gdbOutput, chBuf);
         count0 = 0;
      } else {
         if (count0 > 0)
            Sleep(1);
         count0++;
      }

      if (strncmp(chBuf, "^exit\n", 6) == 0) return 0;
      if ((strstr(chBuf, "(gdb) ") != NULL && dwRead == 0) || (count0 > 2)) {
         return -1;
      }
   }
}

void ErrorExit(char *msg)
{
   printf("%s\n", msg);
   exit(1);
}


#elif defined(__linux__)

int stInPid, stOutPid;
int debug(int (*sendCommandGdb)(char *));

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT.
int sendCommandGdb(char * command)
{
   char chBuf[BUFSIZE];
   int qtdAlloc = BUFSIZE;  
   int mustReturn=1;
   fd_set read_fds;

   int tk = token;
   if(strlen(command)>0){
      if(strstr(command,"-gdb-exit\n")!=NULL || strstr(command,"-gdb-set")!=NULL ||
         strstr(command,"-environment-directory")!=NULL || 
        // strstr(command,"set inferior-tty")!=NULL ||         
         strstr(command,"-file-exec-and-symbols")!=NULL){
         mustReturn=0;
         strcpy(chBuf,command);
         cob.waitAnswer = 1;
      }else{
         sprintf(chBuf,"%d-%s", token++, command);
      }
      #ifdef DEBUG
      printf("%s",chBuf);
      #endif
      write(stOutPid, chBuf, strlen(chBuf));
      if(mustReturn) return tk;
   }
   if(gdbOutput!=NULL){
      strcat(gdbOutput,"\n");
      if(strcmp(gdbOutput,"\n")==0) strcpy(gdbOutput,"");
      qtdAlloc=strlen(gdbOutput);
   }else{
      qtdAlloc=0;
      gdbOutput=malloc(BUFFER_OUTPUT_SIZE);
      strcpy(gdbOutput,"");
   }
   #ifdef DEBUG
   //clrscr();
   #endif
 for (;;)
   {
      FD_ZERO(&read_fds);
      FD_SET(stInPid, &read_fds);

      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;

      // Use select para verificar se há dados disponíveis
      int dwRead = select(stInPid + 1, &read_fds, NULL, NULL, &timeout);
      if (dwRead == -1) {
         break;
      }
      dwRead = read(stInPid, chBuf, sizeof(chBuf));
      if(dwRead<0){
         break;
      }else{
         chBuf[dwRead]='\0';
         qtdAlloc+=dwRead+1;
         if(qtdAlloc>BUFFER_OUTPUT_SIZE){
           gdbOutput = realloc(gdbOutput, qtdAlloc+3);
         }
         strcat(gdbOutput, chBuf);
         #ifdef DEBUG
         printf("Recebendo: %s", gdbOutput);
         #endif
      }
   }
   return TRUE;
}

int start_gdb(char * name, char * cwd){

    int fd1[2];
    int p2[2];
    char *cmd[] = {"gdb", 
                   //"-nx",
                   "--quiet", 
                   "--interpreter=mi2",
                   NULL
                   };
    pipe(fd1);
    pipe(p2);
    int flf=fcntl(p2[0],F_GETFL,0) | O_NONBLOCK;
    fcntl(p2[0], F_SETFL, flf);
    flf = fcntl(fd1[1], F_GETFL, 0) | O_NONBLOCK;
    fcntl(fd1[1], F_SETFL, flf);

    //to_gdb=fdopen(fd1[1],"w");
    //from_gdb=fdopen(p2[0],"r");
    int pid = fork();
    if(pid==0){
        dup2(fd1[0], 0);
        dup2(p2[1], 1);
        close(fd1[1]);
        close(p2[0]);
        execvp("gdb",cmd);
    }else{
        close(fd1[0]);
        close(p2[1]);

        char env_dir[512]; 
        char f_symb[512];
        sprintf(env_dir,"-environment-directory \"%s\"\n", cwd);
        sprintf(f_symb,"-file-exec-and-symbols \"%s/%s\"\n", cwd, name);

        char * gdbSet[]={"-gdb-set target-async on\n", //old format
                         "-gdb-set mi-async on\n",
                         "-gdb-set print repeats 1000\n",
                         "-gdb-set charset UTF-8\n",
                         "-gdb-set env TERM=xterm\n",
                         "-gdb-set mi-notify-stdio on\n",
                         //"-gdb-set print sevenbit-strings off\n",
                         "-gdb-set print static-members off\n",
                         "-gdb-set print pretty on\n",
                         //"-gdb-set mi-auto-solib-add off\n",
                         env_dir,
                         f_symb,
                         ""
               };
        stInPid = p2[0];
        stOutPid = fd1[1];

        int idx=0;
        while(strlen(gdbSet[idx])>=1){
           sendCommandGdb(gdbSet[idx++]);
        }        
        cob.waitAnswer=1;
        debug(&sendCommandGdb );
        printf("\n->End of parent execution.\n");
        waitpid(pid, NULL, 1);
    }
    return 0;   
}
#endif