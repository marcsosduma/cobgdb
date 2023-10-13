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
//#include </usr/include/linux/fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#define TRUE 1
#define FALSE 0
#define clrscr() printf("\e[1;1H\e[2J")
#endif // Windows/Linux
#define BUFSIZE 8192
//#define DEBUG 1

extern char * gdbOutput;
extern int waitAnswer;
extern char * ttyName;
int token=1;

#if defined(_WIN32)

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;
TCHAR command[512];

int debug(int line_pos, int (*sendCommandGdb)(char *));
void CreateChildProcess(void);
int sendCommandGdb(char * command);
void ErrorExit(char *);
//TCHAR szCmdline[]=TEXT("gdb --interpreter=mi2 client.exe");
char * gdbSet[]={"gdb-set target-async on\n", //old format
                 "gdb-set mi-async on\n",
                 "gdb-set print repeats 1000\n",
                 "gdb-set charset UTF-8\n",
                 "gdb-set new-console on\n",
                 //"-environment-directory "+cwd+"\n",
                 //"-file-exec-and-symbols "+file+"\n",

                 ""
};

int start_gdb(char * name)
{

   char fixCommand[] = "gdb -q --interpreter=mi2 ";
   sprintf(command, "%s%s", fixCommand, name);
   SECURITY_ATTRIBUTES saAttr;
   int looking[]={0}; 

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   // Create a pipe for the child process's STDOUT.
   if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) )
      ErrorExit("StdoutRd CreatePipe");

   // Ensure the read handle to the pipe for STDOUT is not inherited.
   if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
       ErrorExit("Stdout SetHandleInformation");
   // Non blocking mode
   DWORD dwMode = PIPE_NOWAIT;
   BOOL fSuccess = SetNamedPipeHandleState( 
      g_hChildStd_OUT_Rd,    // pipe handle 
      &dwMode,  // new pipe mode 
      NULL,     // don't set maximum bytes 
      NULL);    // don't set maximum time 

   // Create a pipe for the child process's STDIN.
   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))

   // Ensure the write handle to the pipe for STDIN is not inherited.
   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit("Stdin SetHandleInformation");
   // Create the child process.
   CreateChildProcess();

   int idx=0;
   while(strlen(gdbSet[idx])>=1){
        strcpy(command, gdbSet[idx++]);
        sendCommandGdb(command);
   }
   waitAnswer=1;
   debug(0, &sendCommandGdb );
   printf("\n->End of parent execution.\n");
   return 0;
}

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE;

// Set up members of the PROCESS_INFORMATION structure.

   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

// Set up members of the STARTUPINFO structure.
// This structure specifies the STDIN and STDOUT handles for redirection.

   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = g_hChildStd_OUT_Wr;
   siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

// Create the child process.

   bSuccess = CreateProcess(NULL,
      command,     // command line
      NULL,          // process security attributes
      NULL,          // primary thread security attributes
      TRUE,          // handles are inherited
      0,             // creation flags
      NULL,          // use parent's environment
      NULL,          // use parent's current directory
      &siStartInfo,  // STARTUPINFO pointer
      &piProcInfo);  // receives PROCESS_INFORMATION

   // If an error occurs, exit the application.
   if ( ! bSuccess )
      ErrorExit(TEXT("CreateProcess"));
   else
   {
      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
      //
      WaitForSingleObject(piProcInfo.hProcess, INFINITE);

      CloseHandle(g_hChildStd_OUT_Wr);
      CloseHandle(g_hChildStd_IN_Rd);
   }
}

int sendCommandGdb(char * command)
// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT.
{
   DWORD dwRead, dwWritten;
   CHAR chBuf[BUFSIZE];
   BOOL bSuccess = FALSE;   
   HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
   int line_pos=0;
   int starting=1;
   int setLine=0;
   char test[10];
   int bufOut = 0;
   int qtdAlloc = BUFSIZE;   
   int count0=0;
   int mustReturn=1;
   #if defined(_WIN32)
   int l1 = 8;
   #else
   int l1 = 7;
   #endif
   
   int tk = token;
   if(strlen(command)>0){
      if(strcmp(command,"-gdb-exit\n")==0){
         strcpy(chBuf,command);
         mustReturn=0;
         waitAnswer = 1;
      }else{
         sprintf(chBuf,"%d-%s", token++, command);
      }
      bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, strlen(chBuf), &dwWritten, NULL);
      if( ! bSuccess ) return 0;
      Sleep(50);
      if(mustReturn) return tk;
   }
   if(gdbOutput!=NULL){
      strcat(gdbOutput,"\n");
      qtdAlloc=strlen(gdbOutput);
   }else{
      qtdAlloc=0;
   }
   for (;;)
   {
      bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      //if( ! bSuccess) break;
      #ifdef DEBUG
      bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
      #endif
      //if (! bSuccess ) break;
      if(dwRead>0){
         chBuf[dwRead]='\0';
         qtdAlloc+=sizeof(chBuf)+2;
         char * auxPointer = malloc(qtdAlloc);
         if(gdbOutput!=NULL && strlen(gdbOutput)>0){
            memcpy(auxPointer,gdbOutput, strlen(gdbOutput)+1);
         }else{
            strcpy(auxPointer,"");
         }
         if(gdbOutput!=NULL) free(gdbOutput);
         gdbOutput = auxPointer;
         bufOut+=dwRead;
         strcat(gdbOutput, chBuf);
         count0=0;
      }else{
         Sleep(1);
         count0++;
      }
      if(dwRead>5)
         if(strncmp(chBuf,"^exit\n",6)==0)
             return 0;
      strcpy(test,"");
      int lenOut=(gdbOutput!=NULL)?strlen(gdbOutput):0;
      if(lenOut>=7){
        memcpy(test, &gdbOutput[(lenOut-l1)],6);
        test[6]='\0';
      }
      if((strcmp(test,"(gdb) ")==0 && dwRead==0) || count0==10){
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
int debug(int line_pos, int (*sendCommandGdb)(char *));

int sendCommandGdb(char * command)
// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT.
{
   char chBuf[BUFSIZE];
   char test[10];
   int bufOut = 0;
   int qtdAlloc = BUFSIZE;  
   int count0=0; 
   int mustReturn=1;
   #if defined(_WIN32)
   int l1 = 8;
   #else
   int l1 = 7;
   #endif

   int tk = token;
   if(strlen(command)>0){
      if(strcmp(command,"-gdb-exit\n")==0 || strncmp(command,"-gdb-set",8)==0){
         mustReturn=0;
         strcpy(chBuf,command);
         waitAnswer = 1;
      }else{
         sprintf(chBuf,"%d-%s", token++, command);
      }
      write(stOutPid, chBuf, strlen(chBuf));
      usleep(1500);
      if(mustReturn) return tk;
   }
   if(gdbOutput!=NULL){
      qtdAlloc=strlen(gdbOutput);
   }else{
      qtdAlloc=0;
   }
   #ifdef DEBUG
   clrscr();
   #endif
   while(TRUE){
      int dwRead = read(stInPid, chBuf, sizeof(chBuf));
      if(dwRead>=0){
         chBuf[dwRead]='\0';
         qtdAlloc+=sizeof(chBuf)+2;
         char * auxPointer = malloc(qtdAlloc);
         if(gdbOutput!=NULL && strlen(gdbOutput)>0){
            memcpy(auxPointer,gdbOutput, strlen(gdbOutput)+1);
         }else{
            strcpy(auxPointer,"");
         }
         if(gdbOutput!=NULL) free(gdbOutput);
         gdbOutput = auxPointer;
         bufOut+=dwRead;
         strcat(gdbOutput, chBuf);
         #ifdef DEBUG
         printf("Recebendo: %s", chBuf);
         #endif
         count0=0;
      }else{
         usleep(1500);
         count0++;
      }
      if(dwRead>5)
         if(strncmp(chBuf,"^exit\n",6)==0)
             return 0;
      strcpy(test,"");
      int lenOut=(gdbOutput!=NULL)?strlen(gdbOutput):0;
      if(lenOut>=7){
        memcpy(test, &gdbOutput[(lenOut-l1)],6);
        test[6]='\0';
      }
      #ifdef DEBUG
      printf("%s\n",test);
      #endif
      if((strcmp(test,"(gdb) ")==0 && dwRead<0) || (dwRead<0 && count0==5)){
        return -1;
      }
   }
}

int start_gdb(char * name){

    int fd1[2];
    int p2[2];
    char tty[100];
    if(ttyName!=NULL){
          sprintf(tty,"--tty=%s", ttyName);
    }else{
      strcmp(tty,"");
    }
    char *cmd[] = {"gdb", 
                   "--quiet", 
                   tty,
                   "--interpreter=mi2",
                   name, 
                   NULL
                   };
    //char chBuf[256];

    pipe(fd1);
    pipe(p2);
    fcntl(p2[0], F_SETFL, O_NONBLOCK);
    /*
    #ifdef F_SETPIPE_SZ
    fcntl(fd1[0], F_SETPIPE_SZ, BUFSIZE);
    fcntl(p2[0], F_SETPIPE_SZ, BUFSIZE);
    #endif
    */
    int pid = fork();
    if(pid==0){
        dup2(fd1[0], 0);
        dup2(p2[1], 1);
        close(fd1[1]);
        close(p2[0]);
        execvp("gdb",cmd);
    }else{
        //wait(NULL);
        close(fd1[0]);
        close(p2[1]);

        char chBuf[256];
        char * gdbSet[]={"gdb-set target-async on\n", //old format
                         "gdb-set mi-async on\n",
                         "gdb-set print repeats 1000\n",
                         "gdb-set charset UTF-8\n",
                         //"-environment-directory "+cwd+"\n",
                         //"-file-exec-and-symbols "+file+"\n",
                         ""
               };
        stInPid = p2[0];
        stOutPid = fd1[1];

        int idx=0;
        while(strlen(gdbSet[idx])>=1){
           strcpy(chBuf, gdbSet[idx++]);
           sendCommandGdb(chBuf);
        }        
        waitAnswer=1;
        debug(0, &sendCommandGdb );
        printf("\n->End of parent execution.\n");
        //wait(NULL);
        waitpid(pid, NULL, 1);
    }
    return 0;   
}
#endif