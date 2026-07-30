/*
 * COBGDB/WCOBGDB GnuCOBOL Debugger:
 * This code is based on the GnuCOBOL Debugger extension available at:
 * https://github.com/OlegKunitsyn/gnucobol-debug
 *
 * License:
 * This code is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 * 
 * Author: Marcos Martins Duma
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include "gdb/cobgdb.h"
#include <stdarg.h>
#if defined(__linux__)
#include <unistd.h>
#include <stdint.h>
#else
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#define access _access
#define F_OK 0
#include <mmsystem.h>
#endif
#include "win/window.h"
#include "win/variables.h"

#if __has_include(<X11/Xlib.h>)
#ifndef HAVE_X11
#define HAVE_X11
#endif
#include <X11/Xlib.h>
#endif
#define __WITH_TESTS_
#define WCOBGDB_VERSION "1.0"

#define STEP_DELAY_INITIAL_INDEX 6

// =====================================================
// Variables
// =====================================================
int VIEW_COLS=80;
int VIEW_LINES=24;
int WAIT_GDB=100;
int first_load = TRUE;
int DEBUG_GDB=FALSE;
int upd_var=FALSE;
static int stdout_pipe[2];

ST_Line * LineDebug=NULL;
ST_Attribute * Attributes=NULL;
ST_DebuggerVariable * DebuggerVariable=NULL;
ST_bk * BPList=NULL;
ST_Watch * Watching=NULL;
Lines * lines = NULL;
struct st_cfiles * parsed_cfiles = NULL;
char * gdbOutput = NULL;
extern int last_line;
int processing_gdb = FALSE;
int sava_cfg = FALSE;
int highlight_bar = 45;
int auto_step_timer = 0;

// =====================================================
// Cobol Debugger State
// =====================================================
struct st_cobgdb cob = {
    .debug_line = 19,
    .ctlVar = -1,
    .running = TRUE,
    .waitAnswer = FALSE,
    .changeLine = FALSE,
    .showFile = TRUE,
    .isStepOver = -1,
    .mouse = 0,
    .num_dig = 4,
    .showVariables = FALSE,
    .status_bar = 0,
    .dragY = -1,
    .auto_step = FALSE,
    .auto_step_delay = STEP_DELAY_INITIAL_INDEX
};

int step_delays[] = {2000,1750,1500,1250,1000,750,500,250,100,25};

// =====================================================
// Prototypes
// =====================================================
void load_source_file(WcobgdbWindow *self, Lines *lines);
int (*sendCommandGdb1)(char *);
void freeWatchingList();
int set_first_break(int (*sendCommandGdb)(char *));
gboolean check_gdb_status_callback(gpointer data);
int start_gdb(char * name, char * cwd);

// =====================================================
// Load First Source for Debugging Executable File
// =====================================================
gboolean load_first_source(gpointer data){
    show_sources(sendCommandGdb1, TRUE);
    if(cob.lines==NULL){
        show_log("No source file found.\nPlease check if the file was compiled with debug information.");
    }else{
        cob.line_pos=set_first_break(sendCommandGdb1);
        WcobgdbWindow *window = get_main_window();
        if (window) {
            refresh_source_view(window);
            gtk_widget_grab_focus(GTK_WIDGET(window->source_view));
            if(cob.connect[0]!='\0') {
                connect_process();
            }
        }
    }
    return FALSE;
}

// =====================================================
// Set callback for GDB output and start main loop
// =====================================================
int debug(int (*f)(char *)){
    sendCommandGdb1 = f;
    g_timeout_add(50, check_gdb_status_callback, NULL);
    if(cob.lines==NULL){
        sleep_ms(900);
        g_timeout_add(100, load_first_source, NULL);
    }else{
        cob.line_pos=set_first_break(sendCommandGdb1);
        if(cob.connect[0]!='\0') {
            connect_process();
        }
    }
    return TRUE;
}

// =====================================================
// Returm current time in milliseconds
// =====================================================
#ifdef _WIN32
long long current_time_ms() {
    return GetTickCount64();
}
#else
long long current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)(ts.tv_sec * 1000LL) +
           (ts.tv_nsec / 1000000LL);
}
#endif

// =====================================================
// Wait for GDB process to finish with a timeout
// =====================================================
void wait_process_gdb(){
    long long start = current_time_ms();
    while (processing_gdb) {
        if (current_time_ms() - start >= 800) {
            break;
        }
        sleep_ms(10);
    }
}

// =====================================================
// INITIAL BREAKPOINT
// =====================================================
int set_first_break(int (*sendCommandGdb)(char *)){
    ST_Line * debug = LineDebug;
    int ret = 1;
    
    while(debug!=NULL){
        if(debug->isEntry==TRUE && strcasecmp(debug->fileCobol,cob.file_cobol)==0) break;
        debug=debug->next;
    }
    if(debug!=NULL){
        MI2addBreakPoint(sendCommandGdb, cob.name_file, debug->lineCobol);
        Lines * line = cob.lines;
        while(line!=NULL){
            if(line->file_line==debug->lineCobol){
                line->breakpoint='S';
                ret= line->file_line;
                if(cob.qtd_lines>5){
                    initialize_line_pos(line->file_line);
                    ret = line->file_line;                
                }
                break;
            }
            line=line->line_after;
        }
    }
    return ret;
}

// =====================================================
// LOAD FILE
// =====================================================
int loadfile(char *nameCobFile)
{
    char baseName[500];
    char temp_name[1024];
    cob.debug_line = -1;
    if (!file_exists(nameCobFile))
        return FALSE;
    strcpy(cob.name_file, nameCobFile);
    fileNameWithoutExtension(nameCobFile, &baseName[0]);
    normalizePath(baseName);
    strcpy(baseName,getFileNameFromPath(baseName));
    snprintf(temp_name, sizeof(temp_name), "%s/%s.c", cob.cwd, baseName);
    if(!file_exists(temp_name))
        return FALSE;
    strcpy(cob.cfile, temp_name);        
    show_log("Loading...");
    show_log(nameCobFile);
    readCodFile(&cob);
    freeWatchingList();
    highlightParse();
    lines = cob.lines;
    char aux[10];
    sprintf(aux,"%d",cob.qtd_lines);
    cob.num_dig=strlen(aux);
    if(BPList!=NULL){
        Lines * line = cob.lines;
        line->breakpoint='N';
        while(line!=NULL){
            ST_bk * search = BPList;
            while(search!=NULL){
                if(search->line==line->file_line && strcasecmp(search->name, cob.file_cobol)==0){
                    int check=hasCobolLine(line->file_line);
                    if(check>0)
                        line->breakpoint='S';
                }
                search=search->next;
            }            
            line=line->line_after;
        }
    }
    //cob.dragSize = 3;
    clear_variables_tree(get_main_window());
    // UPDATE SOURCE VIEW
    WcobgdbWindow *main_win = get_main_window();
    if (main_win != NULL && !first_load) {
        load_source_file(main_win, cob.lines);
    }
    first_load = FALSE;

    return TRUE;
}

// =========================================
// IDLE REFRESH
// =========================================
gboolean refresh_variables_idle(gpointer data){
    WcobgdbWindow *self = (WcobgdbWindow *)data;
    int aux1=cob.debug_line;
    int aux2=cob.running;
    int aux3=cob.line_pos;
    refresh_variables_tree(self);
    cob.debug_line=aux1;
    cob.running=aux2;    
    cob.line_pos=aux3;
    return FALSE;
}

// =========================================
// PROCESS GDB
// =========================================
gboolean check_gdb_status_callback(gpointer data) {
    if(processing_gdb || upd_var) return TRUE;

    processing_gdb = TRUE;
    MI2verifyGdb(sendCommandGdb1);
    processing_gdb = FALSE;
    if ((cob.showFile && cob.changeLine && !cob.running) ) {
        file_line_changed(cob.debug_line);
        cob.changeLine = FALSE;
        cob.showFile = FALSE;
        cob.isStepOver = -1;
        cob.ctlVar = (cob.ctlVar > 10000) ? 1 : cob.ctlVar + 1;
        cob.waitAnswer = FALSE;
        g_timeout_add(50, refresh_variables_idle, get_main_window());
        gtk_window_present(GTK_WINDOW(get_main_window()));
        return TRUE;
    }
    if (!cob.waitAnswer && cob.debug_line == -1 && strcmp(cob.connect, "attaching") == 0) {
        cob.waitAnswer = TRUE;
    }
    return TRUE; 
}

// =====================================================
// Run the program in GDB
// =====================================================
void run_program(){
    if(!cob.waitAnswer || cob.running==FALSE){
        if(cob.debug_line>0){
            if (!show_confirm_dialog(get_gtk_window(), "Confirm", "Would you like to \"Run\" the program again ? (= Restart)")){
                return;
            }
        }
        wait_process_gdb();
        processing_gdb = TRUE;
        MI2start(sendCommandGdb1);
        processing_gdb = FALSE;
        file_line_changed(cob.debug_line);
        cob.waitAnswer = TRUE;
        WAIT_GDB=100;
        cob.status_bar = 0;
    }
}

// =====================================================
// Step into next line in GDB
// =====================================================
void step_into(){
    if(!cob.waitAnswer){
        wait_process_gdb();
        processing_gdb = TRUE;
        MI2stepInto(sendCommandGdb1);
        if(cob.running){
            cob.waitAnswer = TRUE;
        }
        processing_gdb = FALSE;
        file_line_changed(cob.debug_line);
        cob.status_bar = 0;
    }
}

// =====================================================
// Step over next line in GDB
// =====================================================
void next_step(){
    if(!cob.waitAnswer){
        wait_process_gdb();
        processing_gdb = TRUE;
        MI2stepOver(sendCommandGdb1);
        if(cob.running){
            cob.waitAnswer = TRUE;
        }
        processing_gdb = FALSE;
        file_line_changed(cob.debug_line);
        cob.status_bar = 0;
    }
}

// =====================================================
// Load breakpoint from file
// =====================================================
void load_breakpoint(){
    int tempValue= cob.debug_line;
    BPList = load_breakpoints(sendCommandGdb1, BPList, cob.first_file);
    breakpoints_changed();
    cob.debug_line = tempValue;
    window_wait_user();
}

// =====================================================
// Sava breakpoint to file (cobfilename.bkp)
// =====================================================
void save_breakpoint(){
    if(save_breakpoints(BPList, cob.first_file)){
        showCobMessage("Breakpoints saved",1);
    }
}

// =====================================================
// Go to line 
// =====================================================
void goto_line(){
    Lines *line = cob.lines;
    char aux[100];
    aux[0]='\0';
    int found=FALSE;
    if(!dialogBoxNum("Goto Line", "Line", aux, 50)){
        showCobMessage("Not a valid line.",1);
    }
    int new_line = atoi(aux);
    while (line != NULL) {
        if(new_line==line->file_line){
            cob.line_pos=line->file_line;
            found = TRUE;
            break;
        }
        line = line->line_after;
    }
    if(found){
        force_line_visibility(get_main_window(), cob.line_pos);
    }else{
        showCobMessage("Line not found.",1);
    }
}
// =====================================================
// Jump debug to line specified
// =====================================================
void jump_exec(){
    if(!cob.waitAnswer){ 
        wait_process_gdb();
        processing_gdb = TRUE;
        int aux = cob.debug_line;
        int ret = MI2lineToJump(sendCommandGdb1);
        if(ret==0){
            showCobMessage("Not a debuggable line.",2);
            processing_gdb = FALSE;
        }else if(ret==1){
            showCobMessage("Jump operation canceled.",2);
            processing_gdb = FALSE;
        } else if(ret==2){
            cob.waitAnswer = TRUE;
            processing_gdb = FALSE;
            cob.debug_line = aux;
            window_wait_user();
        }
        cob.status_bar = 0;
    }
}

// =====================================================
// Continue execution the program in GDB
// =====================================================
void continue_exec(){
    if(!cob.waitAnswer){ 
        wait_process_gdb();
        processing_gdb = TRUE;
        MI2stepOut(sendCommandGdb1);
        if(cob.running){
            cob.waitAnswer = TRUE;
        }
        processing_gdb = FALSE;
        file_line_changed(cob.debug_line);
    }

}

// ====================================================================
// Attaches the debugger to the configured GDB server or process.
// Performs the attach request and updates the internal debugger state.
// ====================================================================
void connect_process(){
    if(!cob.waitAnswer){
        wait_process_gdb();
        MI2attach(sendCommandGdb1);
        if(cob.running){
            cob.waitAnswer = TRUE;
        }
        processing_gdb = FALSE;
        file_line_changed(cob.debug_line);
    }
}

// =====================================================
// Set or remove breakpoint for the current line in GDB
// =====================================================
void breakpoint_change(){
    if(!cob.waitAnswer){
        Lines * lb = lines;     
        for(int a=0;a<cob.line_pos-1;a++) lb=lb->line_after;
        if(lb==NULL)
            return;
        int check=hasCobolLine(lb->file_line);
        if(check>0){
            int store_line = cob.debug_line;
            lb->breakpoint=(lb->breakpoint=='S')?'N':'S';
            if(lb->breakpoint=='S'){
                MI2addBreakPoint(sendCommandGdb1, cob.name_file, lb->file_line);
            }else{
                MI2removeBreakPoint(sendCommandGdb1, cob.name_file, lb->file_line);
            }  
            cob.showFile=TRUE;
            MI2getStack(sendCommandGdb1,1);
            cob.debug_line = store_line;
            window_wait_user();
            cob.waitAnswer = FALSE;
        }else{
            show_log("Not a debugeable line");
        } 
    }
}

// =====================================================
// Set focus to the debugged application window
// =====================================================
void output_focus(){
        show_log("Change focus to the debug output");
        focus_window_by_title(cob.title);
}

// =====================================================
// Quit the program
// =====================================================
void quit_program(){
        show_log("Change focus to the debug output");
        if (show_confirm_dialog(get_gtk_window(), "Confirm","Would you like to exit the program?")){
            sendCommandGdb1("gdb-exit\n");
        }else{
            return;
        }
        if(sava_cfg){
            save_cfg_value(highlight_bar, cob.auto_step_delay);
        }
        cob.showFile=TRUE; 
        //freeAll();
        GtkApplication *app;

        app = gtk_window_get_application(
            GTK_WINDOW(get_gtk_window())
        );

        if (app) {
            g_application_quit(
                G_APPLICATION(app)
            );
        }
}

// =========================================
// Autostep Callback Function
// =========================================
gboolean auto_step_callback(gpointer data) {
    if(!cob.auto_step) return FALSE;
    step_into();
    return cob.auto_step; 
}

// =====================================================
// Enable Auto Step
// =====================================================
void init_autostep(){
    if(cob.debug_line>0){
        show_log("Autostep");
        cob.auto_step = TRUE;                
        auto_step_timer = g_timeout_add(step_delays[cob.auto_step_delay], auto_step_callback, NULL);
    }
}

void increase_autostep_speed(){
    if(cob.auto_step){
        int speed_array_size = sizeof(step_delays) / sizeof(step_delays[0]);
        cob.auto_step_delay=(cob.auto_step_delay==(speed_array_size-1))?cob.auto_step_delay:cob.auto_step_delay+1;
        g_source_remove(auto_step_timer);
        auto_step_timer = g_timeout_add(step_delays[cob.auto_step_delay], auto_step_callback, NULL);
        sava_cfg=TRUE;
    }
}

void decrease_autostep_speed(){
    if(cob.auto_step){
        if (cob.auto_step_delay > 0) {
            cob.auto_step_delay--;
            g_source_remove(auto_step_timer);
            auto_step_timer = g_timeout_add(step_delays[cob.auto_step_delay], auto_step_callback, NULL);
            sava_cfg=TRUE;
        }
    }
}

// =====================================================
// Show Cobgdb Version
// =====================================================
void show_version(){
    show_wcobgdb_version(WCOBGDB_VERSION);
}

// =====================================================
// Show or Hide GDB Log Output 
// =====================================================
void show_gdb(){
    DEBUG_GDB = !DEBUG_GDB;
}

// =====================================================
// Focus the debugged application window if possible
// =====================================================
#if  defined(__linux__)
#ifdef HAVE_X11
int is_x11_environment() {
    const char *display = getenv("DISPLAY");
    return display != NULL && display[0] != '\0';
}

Window find_window_by_title(Display *display, Window root, const char *title) {
    Window root_return, parent_return;
    Window *children;
    unsigned int nchildren;

    if (!XQueryTree(display, root, &root_return, &parent_return, &children, &nchildren)) {
        return 0;
    }
    for (unsigned int i = 0; i < nchildren; i++) {
        char *window_name = NULL;
        if (XFetchName(display, children[i], &window_name)) {
            if (window_name && strstr(window_name, title)) {
                Window found = children[i];
                XFree(window_name);
                return found;
            }
            XFree(window_name);
        }
        // Recurse
        Window child = find_window_by_title(display, children[i], title);
        if (child) return child;
    }
    if (children) XFree(children);
    return 0;
}

void focus_window_by_title_linux(const char *title_to_find) {
    if (!is_x11_environment()) {
        return;
    }
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        return;
    }
    Window root = DefaultRootWindow(display);
    Window win = find_window_by_title(display, root, title_to_find);
    if (win != 0) {
        //printf("Janela encontrada: 0x%lx\n", win);
        XRaiseWindow(display, win); // Traz para frente
        XSetInputFocus(display, win, RevertToParent, CurrentTime); // Foca
    }
    XCloseDisplay(display);
}
#else
void focus_window_by_title_linux(const char *title_to_find) {
    (void)title_to_find; 
}
#endif
#endif  

// =====================================================
// Focus the debugged application window if possible
// =====================================================
void focus_window_by_title(const char *window_title) {
    #if defined(_WIN32)
    HWND hwnd = FindWindowA(NULL, window_title);
    if (hwnd != NULL) {
        ShowWindow(hwnd, SW_SHOW); 
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        SetActiveWindow(hwnd);
    }
    #else
    (void)window_title; 
    #ifdef HAVE_X11
    char linux_title[300];
    snprintf(linux_title, sizeof(linux_title) - 1, "GnuCOBOL Debug - %s", cob.name_file);
    if (!is_x11_environment()) {
        showCobMessage("Works only in X11 environments", 1);
    }else{
        const char *session_type = getenv("XDG_SESSION_TYPE");
        // Standard check for Wayland session
        int is_wayland = (session_type && strcmp(session_type, "wayland") == 0);
        if(!is_wayland){
            focus_window_by_title_linux(linux_title);
        }else{
            showCobMessage("Not supported in Wayland", 1);
        }
    }
    #endif
    #endif
}

// =====================================================
// Application Output Terminal in Linux
// =====================================================
void message_output_linux(char *sleepVal) {
    char message[1000];
    GtkClipboard *clipboard;

    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    snprintf(message, sizeof(message), "sleep %s;", sleepVal);
    gtk_clipboard_set_text(clipboard, message, -1);
    snprintf(
        message,
        sizeof(message),
        "CobGDB will open a terminal to perform the application's output. "
        "If you want to use another terminal, open the terminal and type the "
        "following command:\n\n"
        "sleep %s;\n\n"
        "After that, click the 'OK' button in this window.\n"
        "(This value is already in the clipboard).",
        sleepVal
    );
    show_message_dialog(get_gtk_window(), "Application Output Terminal", message);
}

// =====================================================
// Free memory allocated for source lines and highlights
// =====================================================
void free_memory()
{
    Lines * lines = cob.lines;
    Lines *current_line;

    while (lines != NULL) {
        current_line = lines;
        lines = lines->line_after;
        if (current_line->line != NULL) {
            free(current_line->line);
            current_line->line= NULL;
        }
        if (current_line->high != NULL) {
            freeHighlight(current_line->high);
            current_line->high= NULL;
        }
        free(current_line);
    }
    cob.lines = NULL;
}

// =====================================================
// Free c files parsed.
// =====================================================
void free_cfiles(void) {
    struct st_cfiles * curr = parsed_cfiles;
    while (curr) {
        struct st_cfiles *next = (struct st_cfiles *) curr->next;
        free(curr->file);
        free(curr);
        curr = next;
    }
    parsed_cfiles = NULL;
}

// =====================================================
// Free watching list
// =====================================================
void freeWatchingList()
{
    ST_Watch * search = Watching;
    ST_Watch *current;

    while (search != NULL) {
        current = search;
        search = search->next;
        free(current);
    }
    Watching = NULL;
}

// =====================================================
// Free Breakpoint List
// =====================================================
void freeBKList()
{
    ST_bk * search = BPList;
    ST_bk *current;

    while (search != NULL) {
        current = search;
        search = search->next;
        free(current->name);
        free(current);
    }
    BPList = NULL;
}

// =====================================================
// Free file and all related data
// =====================================================
int freeFile(){
    free_memory();
    return TRUE;
}

// =====================================================
// Free all allocated memory and reset variables
// =====================================================
int freeAll(){
    freeBKList();
    freeFile();
    freeWatchingList();
    free_cfiles();
    return TRUE;
}

// =====================================================
// Function to check the availability of a command
// =====================================================
int isCommandInstalled(const char *command) {
#ifdef _WIN32
    // Windows
    char fullPath[MAX_PATH];
    DWORD result = SearchPath(NULL, command, ".exe", MAX_PATH, fullPath, NULL);
    return result != 0;
#else
    // Linux
    char whichCommand[100];
    snprintf(whichCommand, sizeof(whichCommand) - 1, "which %s > /dev/null 2> /dev/null", command);
    return system(whichCommand) == 0;
#endif
}

// =====================================================
// Start cobc compiler with specified files and 
// parameters and capture the output
// =====================================================
int cobc_compile(char file[][512], char values[10][256], int arg_count){
    char *param[20]; 

    // Check if the cobc command is available
    if(!isCommandInstalled("cobc")){
        exit_with_message("The GnuCOBOL cobc command is not available!\n", 0);
    }
    // Initialize the first elements with the initial values.
    char *initial_params[] = {
        "-g ",
        "-fsource-location ",
        "-ftraceall ",
        "-v ",
        "-O0 ",
        "-x "
    };
    int param_count = sizeof(initial_params) / sizeof(initial_params[0]);
    int initFree = param_count;
    for (int i = 0; i < param_count; i++) {
        param[i] = initial_params[i];
    }
    for (int i = 0; i < arg_count && param_count < 20; i++) {
        char *param_value = malloc(strlen(values[i]) + 8);
        if (param_value == NULL) {
            return 1;
        }
        snprintf(param_value, strlen(values[i]) + 8, "%s ", values[i]);
        param[param_count] = param_value;
        param_count++;
    }
    char compiler[2048] = "cobc ";    
    for (int a = 0; a < param_count; a++) {
        strcat(compiler, param[a]);
        if(a>=initFree) free(param[a]); 
    }
    int nfile=0;
    while(strlen(file[nfile])>1){
        strcat(compiler, file[nfile++]);
        strcat(compiler, " ");
    }
    strcat(compiler, " 2>&1");
    show_log("%s\n", compiler);
    FILE *pfd = popen(compiler, "r");
    if (pfd == NULL) {
        show_log("failed to open pipe\n");
        return 1;
    }
    while (!feof(pfd)) {
        char buf[1024] = {0};
        fgets(buf, sizeof(buf) - 1, pfd);
        show_log("%s", buf);
    }
    int ret = pclose(pfd);
    return ret;
}

int show_line_var(struct st_highlt * high, char * functionName, int (*sendCommandGdb)(char *)){
    (void) high;
    (void) functionName;
    (void) sendCommandGdb;
    showCobMessage("Feature not implemented.", 2);
    return FALSE;
}

// =====================================================
// Thread function to read from the stdout pipe and 
// log the output
// =====================================================
static gpointer stdout_reader_thread(gpointer data)
{
    char read_buffer[256];
    char line_buffer[8192];

    size_t line_len = 0;
    while (1) {
        ssize_t n = read(stdout_pipe[0], read_buffer, sizeof(read_buffer) - 1);
        if (n <= 0)
            continue;
        for (ssize_t i = 0; i < n; i++) {
            char c = read_buffer[i];
            line_buffer[line_len++] = c;
            if (line_len >= sizeof(line_buffer) - 1) {
                line_len = 0;
            }
            if (c == '\n') {
                line_buffer[line_len] = '\0';
                show_log("%s", line_buffer);
                line_len = 0;
            }
        }
    }
    return NULL;
}

// =====================================================
// Redirect stdout to a pipe and start a thread to 
// read from it
// =====================================================
void redirect_stdout_to_log()
{
    #ifdef _WIN32
        _pipe(stdout_pipe, 4096, _O_TEXT);
    #else
        pipe(stdout_pipe);
    #endif
    dup2(stdout_pipe[1], STDOUT_FILENO);
    setvbuf(stdout, NULL, _IONBF, 0);
    g_thread_new("stdout-reader", stdout_reader_thread, NULL);
}

// =====================================================
// Handle --connect argument to connect to a remote 
// GDB server
// =====================================================
int handle_connect_args(int argc, char **argv) {
    strcpy(cob.connect,"");
    if (argc < 3)
        return 0;

    if (strcmp(argv[1], "--connect") == 0) {
        if(argc<3){
            show_log("Please provide the parameters.\n");
            return 0;
        }
        strcpy(cob.connect,argv[2]);
        return 1;
    }
    return 0;
}

// =====================================================
// Handle special arguments like --test, --version, 
// and --help and execute the corresponding actions
// and output in terminal
// =====================================================
int handle_special_args(int argc, char **argv, int arg_init) {
    if (argc < arg_init - 1 + 2){
        printf("Please provide the GnuCOBOL file.\n");
        //showCobMessage("Please provide the GnuCOBOL file.", 1);
        return 1;
    }

    if (strcmp(argv[arg_init], "--test") == 0) {
#ifdef __WITH_TESTS_
        testParser();
        testMI2();
#else
        printf("Tests not included.\n");
#endif
        return 1;
    }
    if (strcmp(argv[arg_init], "--version") == 0) {
        show_version();
        return 1;
    }
    if (strcmp(argv[arg_init], "--help") == 0) {
        show_help(TRUE);
        return 1;
    }
    return 0;
}

// =====================================================
// Initialize the COBGDB context with the current working
// directory and the name of the executable file
// =====================================================
void init_cob_context(const char *cwd, const char *nameExecFile) {
    strcpy(cob.cwd, cwd);
    normalizePath(cob.cwd);
#if defined(_WIN32)
    snprintf(cob.title, sizeof(cob.title), "%s\\%s", cwd, nameExecFile);
#else
    snprintf(cob.title, sizeof(cob.title), "%s/%s", cwd, nameExecFile);
#endif
    strcpy(cob.file_cobol,"");
    sprintf(cob.find_text, "%61s", "");
}

// =====================================================
// Handle --exe argument to debug an executable file directly
// =====================================================
int handle_exe_mode(char *exePath) {
    if (access(exePath, F_OK) < 0)
        exit_with_message("Executable not found.", 0);

    char *cwd = getCurrentDirectory();
    char baseName[256];
    fileNameWithoutExtension(exePath, baseName);
    normalizePath(baseName);

    #if defined(_WIN32)
    strcat(baseName, ".exe");
    #endif

    show_log("Utilize the parameters below to compile your COBOL program:\n");
    show_log("%-*s\r",60,"cobc -g -fsource-location -ftraceall -v -O0 -x prog.cob");
    
    init_cob_context(cwd, baseName);
    start_gdb(baseName, cob.cwd);
    WcobgdbWindow *window = get_main_window();
    if (window) {
        refresh_source_view(window);
    }
    return 0;
}

// =====================================================
// Handle COBOL files passed as arguments, compile them 
// with cobc, and start GDB with the compiled executable
// =====================================================
int handle_cob_files(int argc, char **argv, int arg_init) {
    char *cwd = getCurrentDirectory();
    char nameExecFile[256], baseName[256];
    char fileCGroup[10][512], fileCobGroup[10][512];
    char nameCFile[1024];
    char values[10][256];
    int arg_count = 0, nfile = 0;

    init_cob_context(cwd, "");
    free(cwd);

    for (int i = arg_init; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (arg_count < 10)
                strncpy(values[arg_count++], argv[i], sizeof(values[0]) - 1);
            continue;
        }
        char ext[16];
        fileExtension(argv[i], ext);
        if (strcasecmp(ext, ".cob")!=0 && strcasecmp(ext, ".cbl")!=0)
            continue;

        fileNameWithoutExtension(argv[i], &baseName[0]);
        normalizePath(baseName);
        strcpy(baseName,getFileNameFromPath(baseName));
        if (nfile == 0) {
            strcpy(cob.name_file, argv[i]);
            strcpy(nameExecFile, baseName);
            #if defined(_WIN32)
            strcat(nameExecFile, ".exe");
            #endif
        }
        // C File
        snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
        strcpy(fileCGroup[nfile],nameCFile);
        // Cobol File
        realpath(argv[i], fileCobGroup[nfile]);
        normalizePath(fileCobGroup[nfile]);
        nfile++;
    }
    
    if (nfile == 0)
        exit_with_message("Enter a file to debug.", 1);

    fileCobGroup[nfile][0] = '\0';
    fileCGroup[nfile][0] = '\0';

    int ret = cobc_compile(fileCobGroup, values, arg_count);
    if (ret) {
        show_log("CobGDB: Issue in cobc, code %d\n", ret);
        return ret;
    }

    show_log("Parser starting...\n");
    SourceMap(fileCGroup);
    show_log("Parser end...\n");

    strcpy(cob.file_cobol, cob.name_file);
    strcpy(cob.first_file, cob.file_cobol);
    loadfile(cob.file_cobol);

    strcat(cob.title, nameExecFile);
    start_gdb(nameExecFile, cob.cwd);
    WcobgdbWindow *window = get_main_window();
    if (window) {
        refresh_source_view(window);
    }
    return 0;
}


// =====================================================
// Initialize cobgdb application, create the main 
// window, handle command-line arguments, check for GDB, 
// and start the debugging session
// =====================================================
void initialize_cobgdb(int argc, char **argv){
    GtkApplication *app;

    WcobgdbWindow *window;

    int arg_init = 1;
    // APP GTK
    app = GTK_APPLICATION(g_application_get_default());
    // WINDOW
    window = wcobgdb_window_new(app);
    gtk_widget_show_all(GTK_WIDGET(window));
    // INITIAL LOG
    show_log("CobGDB starting...");
    show_log("Initializing debugger...");
    // CONNECT
    if (handle_connect_args(argc, argv)){
        arg_init = 3;
        show_log("Connect mode enabled");
    }
    // CHECK GDB
    if (!isCommandInstalled("gdb")) {
        show_log(
            "ERROR: GDB not installed"
        );
        return;
    }
    show_log("GDB found");
    highlight_bar=load_cfg_value();
    // EXE MODE or COB FILES
    if (argc >= 3 && strcmp(argv[arg_init], "--exe") == 0) {
        show_log("Opening executable: %s", argv[arg_init + 1]);
        handle_exe_mode(argv[arg_init + 1]);
    }else {
        if (argc > arg_init){
            show_log("Opening COBOL file: %s", argv[arg_init]);
        }
        handle_cob_files(argc, argv, arg_init);
    }
    show_log("Initialization complete");
}
