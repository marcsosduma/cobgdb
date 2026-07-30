#include <gtk/gtk.h>
#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define ISATTY _isatty
    #define FILENO _fileno
#else
    #include <unistd.h>
    #define ISATTY isatty
    #define FILENO fileno
#endif
#include "win/window.h"

typedef struct {
    const char *text;
    const char *color;
    gboolean is_bold;
} HelpItem;

int supports_unicode(void)
{
#ifdef _WIN32
    return GetConsoleOutputCP() == CP_UTF8;
#else
    const char *locale = setlocale(LC_CTYPE, NULL);

    return locale &&
           (strstr(locale, "UTF-8") ||
            strstr(locale, "utf8") ||
            strstr(locale, "UTF8"));
#endif
}

// Use "❓" : "?" 
const char *replace_query(const char *text)
{
    static char buffer[1024];
    const char *query = supports_unicode() ? "❓" : "?";
    char *pos = strstr(text, "{Q}");
    if (!pos) return text;
    size_t prefix = pos - text;
    snprintf(buffer, sizeof(buffer), "%.*s%s%s", (int)prefix, text, query, pos + 3);
    return buffer;
}

static const HelpItem help_content[] = {
    {"                                {Q} Main Commands", "#00FFFF", TRUE},
    {"", "#000000", FALSE},
    {"B - Breakpoint:", "#FFD700", TRUE},
    {"    Toggles the breakpoint at the current selected line.", "#DDDDDD", FALSE},
    {"R - Run:", "#FFD700", TRUE},
    {"    Runs the program from the first statement until a breakpoint is encountered.", "#DDDDDD", FALSE},
    {"C - Cursor or Continue:", "#FFD700", TRUE},
    {"    Runs the program until it reaches the selected line.", "#DDDDDD", FALSE},
    {"J - Jump:", "#FFD700", TRUE},
    {"    Runs the program until it reaches the specified line.", "#DDDDDD", FALSE},
    {"N - Next:", "#FFD700", TRUE},
    {"    Runs the program until the next line but does not enter a subroutine.", "#DDDDDD", FALSE},
    {"S - Step:", "#FFD700", TRUE},
    {"    Runs the program until the next line.", "#DDDDDD", FALSE},
    {"G - Go:", "#FFD700", TRUE},
    {"    Continues the program execution until it encounters a stopping point.", "#DDDDDD", FALSE},
    {"F - File:", "#FFD700", TRUE},
    {"    Allows selecting the source file for debugging.", "#DDDDDD", FALSE},
    {"A - Attach:", "#FFD700", TRUE},
    {"    Attach to GDBSERVER or Application PID.", "#DDDDDD", FALSE},
    {"O - Focus Window:", "#FFD700", TRUE},
    {"    Changes focus to the debugged application window.", "#DDDDDD", FALSE},
    {"CTRL-A - Autostep:", "#FFD700", TRUE},
    {"    Automatically executes STEP until next breakpoint.", "#DDDDDD", FALSE},
    {"CTRL-F - Find Text:", "#FFD700", TRUE},
    {"    Searches text in the source code.", "#DDDDDD", FALSE},
    {"CTRL-G - Go To Line:", "#FFD700", TRUE},
    {"    Moves cursor to a specified line.", "#DDDDDD", FALSE},
    {"CTRL-S - Save Breakpoints:", "#FFD700", TRUE},
    {"    Saves all breakpoints from current session.", "#DDDDDD", FALSE},
    {"CTRL-B - Load Breakpoints:", "#FFD700", TRUE},
    {"    Loads all saved breakpoints.", "#DDDDDD", FALSE},
    {"CTRL-L - GDB Log:", "#FFD700", TRUE},
    {"    Displays responses from GDB commands.", "#DDDDDD", FALSE},
    {"CTRL-+ - Increase Font Size:", "#FFD700", TRUE},
    {"    Increases the font size in all windows.", "#DDDDDD", FALSE},
    {"CTRL-- - Decrease Font Size:", "#FFD700", TRUE},
    {"    Decreases the font size in all windows.", "#DDDDDD", FALSE},
    {"Q - Quit:", "#FFD700", TRUE},
    {"    Quits the program.", "#DDDDDD", FALSE},
    {"----------------------------------------", "#555555", FALSE},
    {"                         {Q} WCobGDB with Multiple Sources", "#00FFFF", TRUE},
    {"WCobGDB takes one or more programs with COB/CBL extension as parameters", "#6495ED", FALSE},
    {"and runs the GnuCOBOL compiler with the following format:", "#6495ED", FALSE},
    {"cobc -g -fsource-location -ftraceall -v -O0 -x prog.cob prog2.cob ...\n", "#f13f3f", FALSE},
    {"Example:", "#6495ED", FALSE},
    {"wcobgdb prog.cob subprog1.cob subprog2.cob\n", "#f13f3f", FALSE},
    {"                       {Q} WCobGDB Attach (GDBServer or PID)", "#00FFFF", TRUE},
    {"You can run GDB/GDBSERVER remotely using the 'A' key.", "#6495ED", FALSE},
    {"Provide the server and port (server:port) or the application PID.", "#6495ED", FALSE},
    {"Example:", "#6495ED", FALSE},
    {"wcobgdb --connect localhost:5555 prog.cob\n", "#f13f3f", FALSE},
    {"                  {Q} WCobGDB Debugging Pre-compiled Executables", "#00FFFF", TRUE},
    {"You can also use WCobGDB to debug a previously generated executable.", "#6495ED", FALSE},
    {"Compile the program with these options:", "#6495ED", FALSE},
    {"cobc -g -fsource-location -ftraceall -v -O0 -x prog.cob\n", "#f13f3f", FALSE},
    {"Start debugging using the --exe directive:", "#6495ED", FALSE},
    {"wcobgdb --exe prog", "#f13f3f", FALSE}
};

#define CONTENT_COUNT (sizeof(help_content) / sizeof(HelpItem))

// Function to enable ANSI colors on Windows
void enable_terminal_colors() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

void show_help_gtk() {
    HelpWindow *hw = help_window_new(get_gtk_window());
    for (int i = 0; i < CONTENT_COUNT; i++) {
        char tag_name[32];
        snprintf(tag_name, sizeof(tag_name), "hlp_%d", i);
        const char *text = replace_query(help_content[i].text);
        help_add_text(hw, text, tag_name, help_content[i].color, help_content[i].is_bold);
        help_add_text(hw, "\n", tag_name, help_content[i].color, FALSE);
    }
    help_window_show(hw);
}

void print_colored(const char *text, const char *hex) {
    if (strcmp(hex, "#00FFFF") == 0) printf("\033[1;36m%s\033[0m", text);   // Cyan
    else if (strcmp(hex, "#FFD700") == 0) printf("\033[1;33m%s\033[0m", text); // Yellow
    else if (strcmp(hex, "#6495ED") == 0) printf("\033[1;34m%s\033[0m", text); // Blue
    else if (strcmp(hex, "#f13f3f") == 0) printf("\033[1;31m%s\033[0m", text); // Red
    else if (strcmp(hex, "#555555") == 0) printf("\033[0;90m%s\033[0m", text); // Gray
    else printf("%s", text);
}

int supports_colors() {
    if (!ISATTY(FILENO(stdout))) return 0;
    
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        return 1;
    }
    return 0;
#else
    return 1; // Linux terminals
#endif
}

void show_help(int use_color) {
    int use_colors = (use_color && supports_colors());
    for (int i = 0; i < CONTENT_COUNT; i++) {
        const char *text = replace_query(help_content[i].text);
        if (use_colors) {
            print_colored(text, help_content[i].color);
            printf("\n");
        } else {
            printf("%s\n", text);
        }
    }
}


#if defined(__clang__)
    #define COMPILER_NAME "Clang " __clang_version__
#elif defined(__MINGW64__)
    #define COMPILER_NAME "MinGW-w64 GCC " __VERSION__
#elif defined(__MINGW32__)
    #define COMPILER_NAME "MinGW32 GCC " __VERSION__
#elif defined(__GNUC__)
    #define COMPILER_NAME "GCC " __VERSION__
#elif defined(_MSC_VER)
    #define COMPILER_NAME "Microsoft Visual C++"
#else
    #define COMPILER_NAME "Unknown Compiler"
#endif

// =====================================================
// Show version information for the WCobGDB debugger, 
// including copyright and license details
// =====================================================
void show_wcobgdb_version(char *version)
{
    int use_colors = supports_colors();
    char version_info[500];
    snprintf(version_info,
             sizeof(version_info),
             "WCobGDB - GnuCOBOL Debugger - version %s\n",
             version);
    HelpItem version_content[] = {
        {version_info, "#00FFFF", TRUE},
        {"Copyright (C) 2025 Free Software Foundation, Inc.\n", "#FFD700", FALSE},
        {"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n", "#FFD700", FALSE},
        {"This is free software: you are free to change and redistribute it.\n", "#FFD700", FALSE},
        {"This WCobGDB was configured with \"", "#FFD700", FALSE},
        {COMPILER_NAME, "#6495ED", FALSE},
        {"\".\n", "#FFD700", FALSE},
        {"For bug reporting instructions, please see:\n", "#FFD700", FALSE},
        {"https://github.com/marcsosduma/wcobgdb\n", "#f13f3f", FALSE}
    };
    int content_count = sizeof(version_content) / sizeof(version_content[0]);
    for (int i = 0; i < content_count; i++) {
        if (use_colors) {
            print_colored(version_content[i].text, version_content[i].color);
        } else {
            printf("%s", version_content[i].text);
        }
    }
}