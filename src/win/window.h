#ifndef WCOBGDB_WINDOW_H
#define WCOBGDB_WINDOW_H
#ifdef _WIN32
    #define APP_FONT_FAMILY "Cascadia Mono"
    #define APP_FONT_SIZE 12
#else
    #define APP_FONT "Monospace 12"
    #define APP_FONT_FAMILY "Monospace"
    #define APP_FONT_SIZE 12
#endif

#include <gtk/gtk.h>
#include <locale.h>
#include <wchar.h>
#include "gdb/cobgdb.h"

#define WCOBGDB_TYPE_WINDOW (wcobgdb_window_get_type())
#define TOTAL_COLORS 18

G_DECLARE_FINAL_TYPE(
    WcobgdbWindow,
    wcobgdb_window,
    WCOBGDB,
    WINDOW,
    GtkApplicationWindow
)

// =====================================================
// TYPE DEFINITIONS
// =====================================================
typedef struct _WcobgdbWindow WcobgdbWindow;

// =====================================================
// STRUCT WINDOW
// =====================================================
struct _WcobgdbWindow {
    GtkApplicationWindow parent_instance;

    GtkWidget *source_view;
    GtkWidget *output_view;

    GtkTextBuffer  *source_buffer;
    GtkTextBuffer *output_buffer;

    GtkWidget *btn_run;
    GtkWidget *btn_step_over;
    GtkWidget *btn_step_into;
    GtkWidget *btn_step_out;
    GtkWidget *btn_output;
    GtkWidget *btn_stop;
    GtkWidget *btn_find;
    GtkWidget *btn_gdb;
    GtkWidget *btn_help;

    GHashTable *variable_nodes;
    GHashTable *variable_values;

    GtkWidget *main_statusbar;

    GtkTextTag *debug_tag;
    GtkTextTag *tags[TOTAL_COLORS];

    // FIND BAR
    GtkWidget *search_bar;
    GtkWidget *search_entry;
    GtkWidget *btn_find_next;
    GtkWidget *btn_find_close;

    // TreeView
    GtkWidget *tree_view;
    GtkWidget *menu_popup;
    GtkTreePath *popup_path;

    GtkWidget *tree_view_watch;

};

typedef struct {
    GtkWidget *dialog;
    GtkWidget *text_view;

    GtkTextBuffer *buffer;
    GtkTextIter iter;
} HelpWindow;

WcobgdbWindow *wcobgdb_window_new(GtkApplication *app);
void refresh_source_view(WcobgdbWindow *self);  
WcobgdbWindow *get_main_window(void);
void file_line_changed(int new_line);
GtkWindow *get_gtk_window(void);
void apply_global_font_size(int size);
int show_confirm_dialog(GtkWindow *parent, const char * title, const char *message);
void show_message_dialog(GtkWindow *parent, const char * title, const char *message);
void initialize_line_pos(int line);
void wait_process_gdb();
void create_popup_menu(WcobgdbWindow *self);
void show_log(const char *fmt, ...);
char *show_text_edit_dialog(GtkWindow *parent, const char *title, const char *initial_value);
gboolean refresh_variables_idle(gpointer data);
void show_sources(int (*sendCommandGdb)(char *), int mustParse);
void force_line_visibility(WcobgdbWindow *self, int line);
void breakpoints_changed();
void refresh_variables_watch_tree(WcobgdbWindow *self, ST_DebuggerVariable *var);

extern char current_file[1024];

HelpWindow *help_window_new(GtkWindow *parent);
void help_add_text(HelpWindow *hw, const char *text, const char *tag_name, const char *color, gboolean bold);
void help_window_show(HelpWindow *hw);
void run_program();
void step_into();
void next_step();
void show_help_gtk();
void continue_exec();
void breakpoint_change();
void jump_exec();
void output_focus();
void window_wait_user();
void connect_process();
void quit_program();
void show_gdb();
void load_file();
void load_breakpoint();
void save_breakpoint();
void show_wcobgdb_version(char * version);
void goto_line();
void init_autostep();
void increase_autostep_speed();
void decrease_autostep_speed();

#endif