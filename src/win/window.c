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
#include <gtk/gtk.h>
#include <locale.h>
#include <wchar.h>
#include <stdarg.h>
#include "win/window.h"
#include "gdb/cobgdb.h"

// =====================================================
// COLOR MAP
// =====================================================
static const char *color_map[TOTAL_COLORS] = {
    "#D4D4D4", // default text
    "#569CD6", // keyword blue
    "#6A9955", // comment green
    "#4EC9B0", // type cyan
    "#C586C0", // magenta
    "#D16969", // red
    "#CE9178", // string
    "#DCDCDC", // light gray
    "#808080", // gray
    "#569CD6", // blue
    "#4EC9B0", // aqua
    "#9CDCFE", // light cyan
    "#F44747", // strong red
    "#C586C0", // pink
    "#DCDCAA", // yellow
    "#FFFFFF", // white
    "#3C3C3C", // dark gray
    "#505050"
};

// =====================================================
// GLOBAL VARIABLES AND STRUCTS
// =====================================================
extern struct st_cobgdb cob;
extern ST_DebuggerVariable * DebuggerVariable;
//int last_line = -1;
//static int last_line_pos = 0;
static int last_debug_line  = 0;

// =====================================================
// Prototypes
// =====================================================
static void wcobgdb_window_dispose(GObject *object);
static void update_debug_highlight(GtkTextBuffer *buffer);
void refresh_line(GtkTextBuffer *buffer,int line);
void cobgdb_set_css(char css[], size_t css_size);

// =====================================================
// Variables
// =====================================================
typedef struct {
    WcobgdbWindow *self;
    int line;
} ScrollData;

typedef struct {
    char *text;
} LogData;

G_DEFINE_TYPE( 
    WcobgdbWindow,
    wcobgdb_window,
    GTK_TYPE_APPLICATION_WINDOW
)
static WcobgdbWindow *g_window = NULL;
static GtkCssProvider *font_provider = NULL;
static int current_font_size = 10;

// =====================================================
// Key press event handler for global shortcuts and commands
// =====================================================
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data){
    // CTRL
    if (event->state & GDK_CONTROL_MASK)
    {
        switch (event->keyval)
        {
            // Font increase
            case GDK_KEY_plus:
            case GDK_KEY_equal:
            case GDK_KEY_KP_Add:
                apply_global_font_size(current_font_size + 1);
                return TRUE;

            // Font decrease
            case GDK_KEY_minus:
            case GDK_KEY_KP_Subtract:
                apply_global_font_size(current_font_size - 1);
                return TRUE;

            // RESET
            case GDK_KEY_0:
            case GDK_KEY_KP_0:
                apply_global_font_size(14);
                return TRUE;

            // LOG - GDB
            case GDK_KEY_l:
            case GDK_KEY_L:
                show_gdb();
                return TRUE;

            case GDK_KEY_g:
            case GDK_KEY_G:
                goto_line();
                return TRUE;

            // Load Breakpoint
            case GDK_KEY_b:
            case GDK_KEY_B:
                load_breakpoint();
                return TRUE;

            // Save Breakpoint
            case GDK_KEY_s:
            case GDK_KEY_S:
                save_breakpoint();
                return TRUE;
                
            // FIND TEXT
            case GDK_KEY_f:
            case GDK_KEY_F:
                gtk_widget_set_no_show_all(g_window->search_bar, FALSE);
                gtk_widget_show_all(g_window->search_bar);
                gtk_widget_grab_focus(GTK_WIDGET(g_window->search_entry));
                return TRUE;

            // Autostep
            case GDK_KEY_a:
            case GDK_KEY_A:
                init_autostep();
                return TRUE;

            default:
                break;
        }
    }
    // Key Commands
    switch (event->keyval)
    {
        case GDK_KEY_r:
        case GDK_KEY_R:
            show_log("Run");
            run_program();
            return TRUE;

        case GDK_KEY_n:
        case GDK_KEY_N:
            show_log("Step Over");
            next_step();
            return TRUE;

        case GDK_KEY_s:
        case GDK_KEY_S:
            show_log("Step Into");
            step_into();
            return TRUE;

        case GDK_KEY_f:
        case GDK_KEY_F:
            show_log("Load File");
            load_file();
            return TRUE;

        case GDK_KEY_g:
        case GDK_KEY_G:
            show_log("Continue");
            continue_exec();
            return TRUE;

        case GDK_KEY_o:
        case GDK_KEY_O:
            show_log("Change Focus");
            output_focus();
            return TRUE;

        case GDK_KEY_a:
        case GDK_KEY_A:
            show_log("Connect to GdbServer or PID");
            connect_process();
            return TRUE;

        case GDK_KEY_q:
        case GDK_KEY_Q:
            show_log("Quit");
            quit_program();
            return TRUE;

        case GDK_KEY_j:
        case GDK_KEY_J:
            show_log("Jump to Line");
            jump_exec();
            return TRUE;

        case GDK_KEY_b:
        case GDK_KEY_B:
            show_log("Breakpoint change");
            breakpoint_change();
            return TRUE;           

        case GDK_KEY_Escape:
            show_log("ESC");
            cob.auto_step=FALSE; // Cancel Autostep
            return TRUE;

        case GDK_KEY_plus:
        case GDK_KEY_KP_Add:
            increase_autostep_speed();
            return TRUE;

        case GDK_KEY_minus:
        case GDK_KEY_KP_Subtract:
            decrease_autostep_speed();
            return TRUE;


        default:
            break;
    }

    return FALSE;
}

// =====================================================
// Button click handlers for main toolbar buttons
// =====================================================
static void on_run_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Run");
    run_program();
}

static void on_step_over_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Step Over");
    next_step();
}

static void on_step_into_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Step Into");
    step_into();
}

static void on_step_out_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Step Out");
    continue_exec();
}

static void on_stop_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Quit");
    quit_program();
}

static void on_find_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Find");
    gtk_widget_set_no_show_all(
        g_window->search_bar,
        FALSE
    );
    gtk_widget_show_all(
        g_window->search_bar
    );
    gtk_widget_grab_focus(
        GTK_WIDGET(g_window->search_entry)
    );
}

static void on_output_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Change Focus");
    output_focus();
}

static void on_gdb_clicked(GtkWidget *btn, gpointer user_data){
    show_log("GDB Log");
    show_gdb();
}

static void on_help_clicked(GtkWidget *btn, gpointer user_data){
    show_log("Help");
    show_help_gtk();
}

// =====================================================
// Delimiters for word extraction in tooltip
// =====================================================
static gboolean is_delimiter(gunichar c) {
    if (g_unichar_isspace(c)) {
        return TRUE;
    }
    switch (c) {
        case '.':
        case ',':
        case '!':
        case ';':
        case ':':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '"':
        case '\'':
        case '=':
        case '+':
        case '*':
        case '/':
            return TRUE;
        default:
            return FALSE;
    }
}

// =====================================================
// Tooltip query handler for showing variable information on hover
// =====================================================
static gboolean on_query_tooltip(GtkWidget  *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data){
    GtkTextView *text_view = GTK_TEXT_VIEW(widget);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter iter;
    gint bx, by;
    gtk_text_view_window_to_buffer_coords(text_view, GTK_TEXT_WINDOW_TEXT, x, y, &bx, &by);
    // Get iterator at mouse location
    gtk_text_view_get_iter_at_location(text_view, &iter, bx, by);
    if (is_delimiter(gtk_text_iter_get_char(&iter)))
        return FALSE;
    GtkTextIter start = iter;
    GtkTextIter end   = iter;
    while (!gtk_text_iter_is_start(&start)) {
        gunichar c = gtk_text_iter_get_char(&start);
        if (is_delimiter(c)) {
            gtk_text_iter_forward_char(
                &start
            );
            break;
        }
        if (!gtk_text_iter_backward_char(&start))
            break;
    }
    while (!gtk_text_iter_is_end(&end)) {
        gunichar c = gtk_text_iter_get_char(&end);
        if (is_delimiter(c)) break;
        if (!gtk_text_iter_forward_char(&end)) break;
    }
    // Extract word from buffer
    char *word = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    if (word == NULL || strlen(word) == 0){
        g_free(word);
        return FALSE;
    }
    // Check if the word is a COBOL variable
    if (isVariableByCobol(word)) {
        ST_DebuggerVariable *hvar = findFieldVariableByCobol(cob.function, word, DebuggerVariable);
        if (hvar != NULL) {
            char *type = (hvar->attribute != NULL && hvar->attribute->type != NULL) ? hvar->attribute->type : "";
            char *value = (hvar->value != NULL) ? hvar->value : "";
            char *safe_name = g_markup_escape_text(hvar->cobolName, -1);
            char *safe_type = g_markup_escape_text(type, -1);
            char *safe_value = g_markup_escape_text(value, -1);
            // Build tooltip markup
            char *tooltip_text =
                g_strdup_printf(
                "<span foreground='#4EC9B0' font_desc='%s %d'>"
                "%s"
                "</span>\n\n"
                "<span foreground='#569CD6'> TYPE:</span> <span foreground='#D4D4D4'>"
                "%s"
                "</span>\n"
                "<span foreground='#DCDCAA'>VALUE:</span> <span foreground='#CE9178'>"
                "%s"
                "</span>",
                APP_FONT_FAMILY,
                current_font_size + 4,
                safe_name,
                safe_type,
                safe_value
            );
            gtk_tooltip_set_markup(tooltip, tooltip_text);
            g_free(safe_name);
            g_free(safe_type);
            g_free(safe_value);
            g_free(tooltip_text);
            g_free(word);
            return TRUE;
        }
    }
    g_free(word);
    return FALSE;
}

// =====================================================
// Line click handler for toggling breakpoints when 
// clicking on the line number area
// =====================================================
static gboolean line_source_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data){
    GtkTextIter iter;
    gint buffer_x, buffer_y;
    gint line, column;

    // Convert window coordinates to buffer coordinates
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT, (gint)event->x, (gint)event->y, &buffer_x, &buffer_y);
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget), &iter, buffer_x, buffer_y);
    line   = gtk_text_iter_get_line(&iter);
    column = gtk_text_iter_get_line_offset(&iter);
    // g_print("Line=%d Column=%d\n", line + 1, column);
    if (column < 6)    {
        // g_print("Clicked line number: %d\n", line + 1);
        cob.line_pos = line + 1;
        breakpoint_change();
    }
    return FALSE;
}

// =========================================
// Find text in source view
// =========================================
void find_text(GtkWidget *widget, gpointer data)
{
    WcobgdbWindow *self = WCOBGDB_WINDOW(data);

    const gchar *text_to_find = gtk_entry_get_text(GTK_ENTRY(self->search_entry));
    if (!text_to_find || !*text_to_find) return;
    GtkTextIter start;
    GtkTextIter match_start;
    GtkTextIter match_end;
    GtkTextIter end;
    gboolean found = FALSE;
    // START SEARCH FROM CURRENT LINE
    int line = cob.line_pos > 0 ? cob.line_pos : 0;
    gtk_text_buffer_get_iter_at_line(self->source_buffer, &start, line);
    gtk_text_buffer_get_end_iter(self->source_buffer, &end);
    // SEARCH FROM CURRENT POSITION TO EOF
    found = gtk_text_iter_forward_search(&start, text_to_find, GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, &end);
    // CIRCULAR SEARCH
    if (!found){
        GtkTextIter begin;
        gtk_text_buffer_get_start_iter(self->source_buffer, &begin);
        gtk_text_buffer_get_iter_at_line(self->source_buffer, &end, line);
        found = gtk_text_iter_forward_search(&begin, text_to_find, GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_start, &match_end, &end);
    }
    // FOUND
    if (found){
        gtk_text_buffer_select_range(self->source_buffer, &match_start, &match_end);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(self->source_view), &match_start, 0.2, FALSE, 0, 0);
        // UPDATE CURRENT LINE
        cob.line_pos = gtk_text_iter_get_line( &match_start ) + 1;
        refresh_line(self->source_buffer, cob.line_pos);
    }
}

// =========================================
// Close find bar and return focus to source view
// =========================================
void close_find_bar(GtkWidget *widget, gpointer data){
    WcobgdbWindow *self = WCOBGDB_WINDOW(data);
    gtk_widget_hide(self->search_bar);
    gtk_widget_set_no_show_all(self->search_bar,TRUE);
    gtk_widget_grab_focus(GTK_WIDGET(self->source_view));
}

// =========================================
// Key press handler for find entry to allow 
// closing with ESC
// =========================================
static gboolean on_search_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data){
    WcobgdbWindow *self = WCOBGDB_WINDOW(data);
    if (event->keyval == GDK_KEY_Escape){
        close_find_bar(NULL, self);
        return TRUE;
    }
    return FALSE;
}

// =====================================================
// Get main application window
// =====================================================
WcobgdbWindow *get_main_window(void){
    return g_window;
}

// =====================================================
// Get GTK window from main application window
// =====================================================
GtkWindow *get_gtk_window(void)
{
    return GTK_WINDOW(g_window);
}

// =====================================================
// TreeView button press handler for showing variable context menu
// =====================================================
gboolean on_treeview_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    WcobgdbWindow *self = WCOBGDB_WINDOW(user_data);
    if (event->type == GDK_BUTTON_PRESS &&
        event->button == 3) {
        GtkTreePath *path = NULL;
        gint bx;
        gint by;
        // Converts widget coordinates to TreeView internal coordinates
        gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(widget), (gint)event->x, (gint)event->y, &bx, &by);
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bx, by, &path, NULL, NULL, NULL)) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_unselect_all(selection);
            gtk_tree_selection_select_path(selection, path);
            if (self->popup_path) {
                gtk_tree_path_free(self->popup_path);
            }
            self->popup_path = gtk_tree_path_copy(path);
            // Retrieves the selected item
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter(model, &iter, path)) {
                gchar *text = NULL;
                gtk_tree_model_get(model, &iter, 1, &text, -1);
                gchar *path_str = gtk_tree_path_to_string(path);
                //g_print( "Selected item:\n  Path : %s\n Text : %s\n", path_str, text ? text : "(null)");
                g_free(path_str);
                g_free(text);
            }
            gtk_tree_path_free(path);
#if GTK_CHECK_VERSION(3,22,0)
            gtk_menu_popup_at_pointer(GTK_MENU(self->menu_popup), (GdkEvent *)event);
#else
            gtk_menu_popup(GTK_MENU(self->menu_popup), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
            return TRUE;
        }
    }
    return FALSE;
}

// =====================================================
// Set font size and family and style for the entire 
// application using CSS
// =====================================================
void apply_global_font_size(int size){
    if (size < 6)
        size = 6;
    if (size > 40)
        size = 40;
    current_font_size = size;
    // CSS
    char css[2048];
    snprintf( css,
        sizeof(css),
        "* {"
        "   font-family: \"%s\";"
        "   font-size: %dpt;"
        "}",
        APP_FONT_FAMILY,
        size
    );
    // PROVIDER
    if (font_provider == NULL) {
        font_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(font_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    gtk_css_provider_load_from_data(font_provider, css, -1, NULL);
}

// =====================================================
// Create text tags for syntax highlighting and other purposes
// =====================================================
static void create_all_tags(GtkTextBuffer *buffer, GtkTextTag **tags){
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    for (int i = 0; i < TOTAL_COLORS; i++) {
        char name[32];
        snprintf(name, sizeof(name), "color_%d", i);
        GtkTextTag *tag = gtk_text_tag_table_lookup(table, name);
        if (!tag) {
            tag = gtk_text_buffer_create_tag(buffer, name, "foreground", color_map[i], NULL);
        }
        tags[i] = tag;
    }
    // running_pointer
    if (!gtk_text_tag_table_lookup(table, "running_pointer")) {
        gtk_text_buffer_create_tag(
            buffer,
            "running_pointer",
            "foreground", "#FF5555",
            "weight", PANGO_WEIGHT_BOLD,
            NULL
        );
    }
    // running_line
    if (!gtk_text_tag_table_lookup(table, "running_line")) {
        gtk_text_buffer_create_tag(
            buffer,
            "running_line",
            "paragraph-background", "#220000",
            NULL
        );
    }
    // debug_pointer
    if (!gtk_text_tag_table_lookup(table, "debug_pointer")) {
        gtk_text_buffer_create_tag(buffer, "debug_pointer", "foreground", "#00FF7F", "weight", PANGO_WEIGHT_BOLD, NULL);
    }
    // debug_line
    if (!gtk_text_tag_table_lookup(table, "debug_line")) {
        gtk_text_buffer_create_tag(buffer, "debug_line", "paragraph-background", "#000c07", NULL);
    }
    // cursor_line
    if (!gtk_text_tag_table_lookup(table, "cursor_line")) {
        gtk_text_buffer_create_tag(buffer, "cursor_line", "paragraph-background", "#2D2D30", "background-full-height", TRUE, NULL);
    }
    // breakpoint
    if (!gtk_text_tag_table_lookup(table, "breakpoint")) {
        gtk_text_buffer_create_tag(buffer, "breakpoint", "foreground", "#FF5555", "weight", PANGO_WEIGHT_BOLD, NULL);
    }
    // line_number
    if (!gtk_text_tag_table_lookup(table, "line_number")) {
        gtk_text_buffer_create_tag(buffer, "line_number", "foreground", "#808080", NULL);
    }
}

// ==========================================================================
// Insert a token with the appropriate syntax highlighting based on its type
// ==========================================================================
static void insert_token(GtkTextBuffer *buffer, GtkTextIter *iter, struct st_highlt *h, GtkTextTag **tags){
    wchar_t wcBuffer[2048];
    char utf8[8192];
    int qtd = h->size;
    if (qtd <= 0)
        return;
    if (qtd > 2047)
        qtd = 2047;
    wmemcpy(wcBuffer, h->token, qtd);
    wcBuffer[qtd] = L'\0';
    size_t len = wcstombs(utf8, wcBuffer, sizeof(utf8) - 1);
    if (len == (size_t)-1)
        return;
    utf8[len] = '\0';
    GtkTextTag *tag = NULL;
    if (h->color >= 0 && h->color < TOTAL_COLORS){
        tag = tags[h->color];
    }
    if (tag) {
        gtk_text_buffer_insert_with_tags(buffer, iter, utf8, -1, tag, NULL);
    } else {
        gtk_text_buffer_insert(buffer, iter, utf8, -1);
    }
}

// ==========================================================================
// Update the highlight for the current debug line, removing it from the 
// previous line and applying it to the new line
// ==========================================================================
static void update_cursor_highlight(WcobgdbWindow *self){
    GtkTextBuffer *buffer = self->source_buffer;
    GtkTextIter cursor_iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, gtk_text_buffer_get_insert(buffer));
    // SYNCHRONIZE CURRENT LINE
    cob.line_pos = gtk_text_iter_get_line(&cursor_iter) + 1;
    // REMOVE ALL PREVIOUS HIGHLIGHTS
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "cursor_line", &start, &end);
    //  GET CURRENT LINE RANGE
    GtkTextIter line_start = cursor_iter;
    GtkTextIter line_end   = cursor_iter;
    gtk_text_iter_set_line_offset(&line_start, 0);
    // move to end of current line
    if (!gtk_text_iter_ends_line(&line_end)) {
        gtk_text_iter_forward_to_line_end(&line_end);
    }
    // do not advance at EOF
    if (!gtk_text_iter_is_end(&line_end)) {
        gtk_text_iter_forward_char(&line_end);
    }
    // APPLY CURRENT LINE HIGHLIGHT
    gtk_text_buffer_apply_tag_by_name(buffer, "cursor_line", &line_start, &line_end);
}

// =======================================================================================
// Current line change handler to update the highlight when the cursor moves to a new line
// =======================================================================================
static void on_cursor_moved(GtkTextBuffer *buffer, GtkTextIter *new_location, GtkTextMark *mark, gpointer user_data){
    WcobgdbWindow *self = user_data;
    if (!mark)
        return;
    const char *name = gtk_text_mark_get_name(mark);
    if (!name)
        return;
    // only react to the insert cursor
    if (strcmp(name, "insert") != 0)
        return;
    update_cursor_highlight(self);
}

// =======================================================================================
// Insert a line of code into the source view with syntax highlighting, breakpoint and 
// debug indicators, and line numbers
// =======================================================================================
static void insert_highlighted_line(WcobgdbWindow *self, GtkTextBuffer *buffer, GtkTextIter *iter, Lines *line, GtkTextTag **tags){
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *bp_tag = gtk_text_tag_table_lookup(table, "breakpoint");
    GtkTextTag *ln_tag = gtk_text_tag_table_lookup(table, "line_number");
    gboolean is_current_line = FALSE;
    if (cob.debug_line == line->file_line){
        is_current_line = TRUE;
    }
    // BREAKPOINT INDICATOR
    if (line->breakpoint == 'S') {
        gtk_text_buffer_insert_with_tags(buffer, iter, "● ", -1, bp_tag, NULL);
    } else {
        gtk_text_buffer_insert(buffer, iter, "  ", -1);
    }
    // DEBUG POINTER
    GtkTextTag *debug_pointer_tag = gtk_text_tag_table_lookup(table, "debug_pointer");
    if (is_current_line && cob.debug_line > 0) {
        gtk_text_buffer_insert_with_tags(buffer, iter, "> ", -1, debug_pointer_tag, NULL);
    } else {
        gtk_text_buffer_insert(buffer, iter, "  ", -1);
    }
    // LINE NUMBER
    char num[64];
    snprintf(num, sizeof(num), "%4d ", line->file_line);
    gtk_text_buffer_insert_with_tags(buffer, iter, num, -1, ln_tag, NULL);
    // PLAIN LINE (NO HIGHLIGHT)
    if (line->high == NULL) {
        if (line->line) {
            char clean_line[8192];
            g_strlcpy(clean_line, line->line, sizeof(clean_line));
            clean_line[strcspn(clean_line, "\r\n")] = 0;
            gtk_text_buffer_insert(buffer, iter, clean_line, -1);
        }
        gtk_text_buffer_insert(buffer, iter, "\n", 1);
        return;
    }
    // SYNTAX HIGHLIGHT TOKENS
    struct st_highlt *h = line->high;
    while (h != NULL) {
        insert_token(buffer, iter, h, tags);
        h = h->next;
    }
    // END OF LINE
    gtk_text_buffer_insert(buffer, iter, "\n", 1);
}

// =======================================================================================
// Set the cursor to the current line in the source view and update the highlight when the 
// user clicks on a line in the tree view or when the current line changes in the debugger
// =======================================================================================
static gboolean focus_source_view(gpointer data)
{
    WcobgdbWindow *self = WCOBGDB_WINDOW(data);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(self->source_buffer, &iter, cob.line_pos > 0 ? cob.line_pos - 1 : 0);
    gtk_text_buffer_place_cursor(self->source_buffer, &iter);
    update_cursor_highlight(self);
    gtk_widget_grab_focus(GTK_WIDGET(self->source_view));
    return G_SOURCE_REMOVE;
}

// =========================================================================================
// WCOBGDB Window initialization function to set up the GUI components, syntax highlighting,
// event handlers, and other settings when the main application window is created
// =========================================================================================
static void wcobgdb_window_init(WcobgdbWindow *self){
    setlocale(LC_ALL, "");
    // IMPROVE FONT RENDERING
    GtkSettings *settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-theme-name", "Adwaita", "gtk-icon-theme-name", "Adwaita", NULL);
    g_object_set(gtk_settings_get_default(), "gtk-xft-antialias", TRUE, "gtk-xft-hinting", TRUE, "gtk-xft-hintstyle", "hintslight", "gtk-xft-rgba", "rgb", NULL);

    GtkIconTheme *theme = gtk_icon_theme_get_default();
    gtk_icon_theme_add_resource_path(theme, "/wcobgdb/icons");

    gtk_widget_init_template(GTK_WIDGET(self));

    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_run), "media-playback-start");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_step_over),"go-next");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_stop),"process-stop");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_step_into),"go-down");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_step_out),"go-up");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_find),"edit-find");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_help),"help-browser");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_gdb),"dialog-information");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(self->btn_output),"view-refresh");

    g_window = self;
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource("/wcobgdb/icons/app.png", &error);
    if (pixbuf) {
        gtk_window_set_icon(GTK_WINDOW(g_window), pixbuf);
        g_object_unref(pixbuf);
    } else {
        g_warning("Falha ao carregar ícone: %s", error ? error->message : "desconhecido");
        g_clear_error(&error);
    }
    self->output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->output_view));
    // TREEVIEW VARIABLES
    GtkTreeStore *store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    // TREEVIEW RENDERER
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,"family", APP_FONT_FAMILY, "ypad", 0, "xpad", 4, NULL);
    // TREEVIEW COLUMN
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("", renderer, "markup", 0, NULL);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(self->tree_view), column);
    // SET TREEVIEW MODEL
    gtk_tree_view_set_model(GTK_TREE_VIEW(self->tree_view), NULL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(self->tree_view), GTK_TREE_MODEL(store));
    g_object_unref(store);
    // TREEVIEW VISUAL SETTINGS
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->tree_view), FALSE);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(self->tree_view), TRUE);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(self->tree_view), TRUE);
    gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(self->tree_view), TRUE);
    gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(self->tree_view), 12);


    // WATCH TREEVIEW VARIABLES
    GtkTreeStore *store_watch = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    // WATCH TREEVIEW RENDERER
    GtkCellRenderer *renderer_watch = gtk_cell_renderer_text_new();
    g_object_set(renderer_watch,"family", APP_FONT_FAMILY, "ypad", 0, "xpad", 4, NULL);
    // WATCH TREEVIEW COLUMN
    GtkTreeViewColumn *column_watch = gtk_tree_view_column_new_with_attributes("", renderer_watch, "markup", 0, NULL);
    gtk_tree_view_column_set_sizing(column_watch, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_expand(column_watch, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(self->tree_view_watch), column_watch);
    // WATCH SET TREEVIEW MODEL
    gtk_tree_view_set_model(GTK_TREE_VIEW(self->tree_view_watch), NULL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(self->tree_view_watch), GTK_TREE_MODEL(store_watch));
    g_object_unref(store_watch);
    // WATCH TREEVIEW VISUAL SETTINGS
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->tree_view_watch), FALSE);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(self->tree_view_watch), TRUE);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(self->tree_view_watch), TRUE);
    gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(self->tree_view_watch), TRUE);
    gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(self->tree_view_watch), 12);




    // TREEVIEW -> Menu
    create_popup_menu(self);
    // CREATE SOURCE BUFFER
    self->source_buffer = gtk_text_buffer_new(NULL);
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(self->source_buffer, &iter);
    // CREATE TEXT TAGS
    memset(self->tags, 0, sizeof(self->tags));
    create_all_tags(self->source_buffer, self->tags);
    // RENDER SOURCE LINES - FIRST TIME
    Lines *line = cob.lines;
    while (line != NULL) {
        insert_highlighted_line(self, self->source_buffer, &iter, line, self->tags);
        line = line->line_after;
    }
    // ASSIGN BUFFER TO SOURCE VIEW
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(self->source_view), self->source_buffer);
    update_debug_highlight(self->source_buffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(self->source_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(self->source_view), FALSE);
    // ENABLE SOURCE VIEW FOCUS
    gtk_widget_set_can_focus(self->source_view, TRUE);
    gtk_widget_grab_focus(self->source_view);
    gtk_text_view_set_overwrite(GTK_TEXT_VIEW(self->source_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(self->source_view), TRUE);
    // SOURCE VIEW MARGINS
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(self->source_view), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(self->source_view), 10);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(self->source_view), 2);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(self->source_view), 2);
    // OUTPUT VIEW MARGINS
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(self->output_view), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(self->output_view), 10);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(self->output_view), 2);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(self->output_view), 2);
    // APPLY DARK THEME
    gtk_widget_set_name(self->source_view, "sourceview");
    gtk_widget_set_has_tooltip(self->source_view, TRUE);
    GtkCssProvider *provider = gtk_css_provider_new();
    char css[16384];
    cobgdb_set_css(css, sizeof(css));
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    // CONNECT SIGNAL HANDLERS
    // BUTTONS
    g_signal_connect(self->btn_run, "clicked", G_CALLBACK(on_run_clicked), self);
    g_signal_connect(self->btn_step_over, "clicked", G_CALLBACK(on_step_over_clicked), self);
    g_signal_connect(self->btn_step_into, "clicked", G_CALLBACK(on_step_into_clicked), self);
    g_signal_connect(self->btn_step_out, "clicked", G_CALLBACK(on_step_out_clicked), self);
    g_signal_connect(self->btn_stop, "clicked", G_CALLBACK(on_stop_clicked), self);
    g_signal_connect(self->btn_find, "clicked", G_CALLBACK(on_find_clicked), self);
    g_signal_connect(self->btn_output, "clicked", G_CALLBACK(on_output_clicked), self);
    g_signal_connect(self->btn_help, "clicked", G_CALLBACK(on_help_clicked), self);
    g_signal_connect(self->btn_gdb, "clicked", G_CALLBACK(on_gdb_clicked), self);
    g_signal_connect(self->btn_find_next, "clicked", G_CALLBACK(find_text), self);
    // Mouse
    g_signal_connect(self->source_buffer, "mark-set", G_CALLBACK(on_cursor_moved), self);
    // Keyboard
    g_signal_connect(self->source_view, "key-press-event", G_CALLBACK(on_key_press), self);
    // Source View
    g_signal_connect(self->source_view, "button-press-event", G_CALLBACK(line_source_clicked), NULL);
    g_signal_connect(self->source_view, "query-tooltip", G_CALLBACK(on_query_tooltip), self);
    // Find Text
    g_signal_connect(self->search_entry, "activate", G_CALLBACK(find_text), self);
    g_signal_connect(self->search_entry, "key-press-event", G_CALLBACK(on_search_key_press), self);
    g_signal_connect(self->btn_find_close, "clicked", G_CALLBACK(close_find_bar), self);
    // TreeView Button Press -> Menu
    g_signal_connect(self->tree_view, "button-press-event", G_CALLBACK(on_treeview_button_press), self);
    // DISPLAY ALL WINDOW WIDGETS
    gtk_widget_show_all( GTK_WIDGET(self));
    // HIDE FIND BAR
    gtk_widget_hide(self->search_bar);
    gtk_widget_set_no_show_all(self->search_bar, TRUE);
    // FORCE FOCUS AFTER WINDOW IS READY
    g_timeout_add(50, focus_source_view, self);
    // APPLY GLOBAL FONT SIZE
    apply_global_font_size(APP_FONT_SIZE);
}

// =========================================================================================
// WCOBGDB class initialization function to set up the class structure, template, and child 
// widget bindings
// =========================================================================================
static void wcobgdb_window_class_init(WcobgdbWindowClass *class){
    GObjectClass *object_class = G_OBJECT_CLASS(class);
    object_class->dispose = wcobgdb_window_dispose;
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    gtk_widget_class_set_template_from_resource(widget_class, "/wcobgdb/glade/window.ui");
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, source_view);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, output_view);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, tree_view);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, tree_view_watch);
    // Buttons
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_run);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_step_over);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_step_into);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_step_out);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_stop);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_find);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_output);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_help);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_gdb);
    // Status Bar
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, main_statusbar);
    // Find Bar
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, search_bar);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, search_entry);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_find_next);
    gtk_widget_class_bind_template_child(widget_class, WcobgdbWindow, btn_find_close);
}

// ===========================================================================================
// WCOBGDB Window constructor function to create a new instance of the main application window
// ===========================================================================================
WcobgdbWindow *wcobgdb_window_new(GtkApplication *app){
    return g_object_new(WCOBGDB_TYPE_WINDOW, "application", app, NULL);
}

// ===========================================================================================
// Window dispose function to clean up resources when the main application window is closed, 
// such as destroying hash tables used for variable tracking and calling the parent class 
// dispose method
// ===========================================================================================
static void wcobgdb_window_dispose(GObject *object){
    WcobgdbWindow *self = WCOBGDB_WINDOW(object);
    g_clear_pointer(&self->variable_nodes, g_hash_table_destroy);
    g_clear_pointer(&self->variable_values, g_hash_table_destroy);
    G_OBJECT_CLASS(wcobgdb_window_parent_class)->dispose(object);
    if(font_provider!=NULL) g_clear_object(&font_provider);
}

// ==========================================================================================
// Refresh the source view by clearing it and re-rendering all lines with syntax highlighting 
// and other indicators, such as breakpoints and the current debug line
// ==========================================================================================
void refresh_source_view(WcobgdbWindow *self)
{
    GtkTextBuffer *buffer = self->source_buffer;
    g_object_freeze_notify(G_OBJECT(buffer));
    gtk_text_buffer_set_text(buffer, "", -1);
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);
    GtkTextTag *tags[TOTAL_COLORS] = {0};
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    for (int i = 0; i < TOTAL_COLORS; i++) {
        char name[32];
        snprintf(name, sizeof(name), "color_%d", i);
        tags[i] = gtk_text_tag_table_lookup(table, name);
    }
    // Re-render all source lines
    Lines *line = cob.lines;
    while (line) {
        insert_highlighted_line(self, buffer, &iter, line, tags);
        line = line->line_after;
    }
    g_object_thaw_notify(G_OBJECT(buffer));
}

// ==========================================================================================
// Scroll output view to the last line after appending new log messages, ensuring that the 
// most recent output is visible to the user
// ==========================================================================================
static gboolean scroll_to_end(gpointer data)
{
    GtkTextView *view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buf = gtk_text_view_get_buffer(view);
    GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
    gtk_text_view_scroll_to_mark(view, mark, 0.0, TRUE, 0.0, 1.0);
    return G_SOURCE_REMOVE;
}

// ==========================================================================================
// Structure and functions for appending log messages to the output view from other threads, 
// using g_idle_add to ensure that UI updates happen in the main thread and do not cause 
// thread safety issues
// ==========================================================================================
static gboolean append_log_idle(gpointer data){
    LogData *log = data;
    show_log("%s", log->text);
    g_free(log->text);
    g_free(log);
    return FALSE;
}

// ==========================================================================================
// Function to log output from GDB or other sources to the output view, creating a LogData
// ==========================================================================================
void log_gdb_output(const char *text){
    if (!text || text[0] == '\0')
        return;
    LogData *log = g_new0(LogData, 1);
    log->text = g_strdup(text);
    g_idle_add(append_log_idle, log);
}

// ==========================================================================================
// Function to show a log message in the output view, using printf-style formatting and 
// ensuring thread safety by using g_idle_add to schedule the UI update in the main thread
// ==========================================================================================
void show_log(const char *fmt, ...){
    if (!g_window)
        return;
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    GtkTextBuffer *buf = g_window->output_buffer;
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buf, &iter);
    gtk_text_buffer_insert(buf, &iter, buffer, -1);
    gtk_text_buffer_insert(buf, &iter, "\n", -1);
    g_idle_add(scroll_to_end, g_window->output_view);
}

// ===============================================================================================
// Scroll the source view to ensure that a specific line is visible, adjusting the scroll position
// only if the line is outside of the current visible area, and placing the cursor on that line
// ===============================================================================================
static gboolean do_scroll_to_line(gpointer data){
    ScrollData *sd = data;
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(sd->self->source_buffer, &iter, sd->line - 1);
    GtkTextView *view = GTK_TEXT_VIEW(sd->self->source_view);
    GtkAdjustment *vadj = gtk_scrollable_get_vadjustment( GTK_SCROLLABLE(view) );
    GdkRectangle rect;
    gtk_text_view_get_iter_location(view, &iter, &rect);
    gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_WIDGET, rect.x, rect.y, NULL, &rect.y);
    gint line_height = rect.height;
    double visible_height = gtk_adjustment_get_page_size(vadj);
    double top_limit = line_height * 2;
    double bottom_limit = visible_height - (line_height * 3);
    double current_scroll = gtk_adjustment_get_value(vadj);
    double line_y = rect.y;
    // ADJUST ONLY WHEN NECESSARY
    if (line_y < top_limit){
        gtk_adjustment_set_value(vadj, current_scroll - (top_limit - line_y));
    }else if (line_y > bottom_limit){
        gtk_adjustment_set_value(vadj, current_scroll + (line_y - bottom_limit));
    }
    gtk_text_buffer_place_cursor(sd->self->source_buffer, &iter);
    g_free(sd);
    return FALSE;
}

// ===============================================================================================
// Force scroll to a specific line and show it as the current debug line, ensuring that the line 
// is visible in the source view and that the debug highlight is updated accordingly
// ===============================================================================================
void force_line_visibility(WcobgdbWindow *self, int line){
    if (line <= 0)
        return;
    update_debug_highlight(self->source_buffer);
    ScrollData *sd = g_malloc(sizeof(ScrollData));
    sd->self = self;
    sd->line = line;
    g_idle_add(do_scroll_to_line, sd);
}

// ==============================================================================================    
// Find the Lines structure for a specific file line number, used to retrieve information about
// ==============================================================================================
static Lines *find_line(int file_line){
    Lines *line = cob.lines;
    while (line) {
        if (line->file_line == file_line)
            return line;
        line = line->line_after;
    }
    return NULL;
}

// ==============================================================================================
// Get the start and end iterators for a specific line in the text buffer, used for applying tags
// ==============================================================================================
static void get_line_bounds(GtkTextBuffer *buffer, int line, GtkTextIter *start, GtkTextIter *end){
    gtk_text_buffer_get_iter_at_line(buffer, start, line - 1);
    *end = *start;
    gtk_text_iter_forward_to_line_end(end);
}

// ===============================================================================================
// Clear all line-specific tags (cursor_line, debug_line, running_line) from a specific line, used
// before applying new tags to ensure that only the correct line is highlighted
// ===============================================================================================
static void clear_line_tags(GtkTextBuffer *buffer, int line){
    if (line <= 0)
        return;
    GtkTextIter start;
    GtkTextIter end;
    get_line_bounds(buffer, line, &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "cursor_line", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "debug_line", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "running_line", &start, &end);
}

// ==============================================================================================
// Update the line prefix for a specific line, showing breakpoint and debug pointers as needed, 
// and ensuring that the correct indicators are displayed based on the current debug state and
// whether the line is the current debug line
// ==============================================================================================
static void update_line_prefix(GtkTextBuffer *buffer, int line, gboolean show_pointer, gboolean running){
    GtkTextIter start;
    GtkTextIter end;
    get_line_bounds(buffer, line, &start, &end);
    GtkTextIter prefix_start = start;
    GtkTextIter prefix_end   = start;
    gtk_text_iter_forward_chars(&prefix_end, 4);
    gtk_text_buffer_delete(buffer, &prefix_start, &prefix_end);
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *bp_tag = gtk_text_tag_table_lookup(table, "breakpoint");
    Lines *ln = find_line(line);
    // Breakpoint
    if (ln && ln->breakpoint == 'S') {
        gtk_text_buffer_insert_with_tags(buffer, &prefix_start, "● ", -1, bp_tag, NULL);
    } else {
        gtk_text_buffer_insert(buffer, &prefix_start, "  ", -1);
    }
    // Pointer
    if (show_pointer) {
        if (running) {
            gtk_text_buffer_insert_with_tags_by_name(buffer, &prefix_start, "! ", -1, "running_pointer", NULL);
        } else {
            gtk_text_buffer_insert_with_tags_by_name(buffer, &prefix_start, "> ", -1, "debug_pointer", NULL);
        }
    } else {
        gtk_text_buffer_insert(buffer, &prefix_start, "  ", -1);
    }
}


// ============================================================================================
// Get Visible Lines in the Source View
// ============================================================================================
void get_visible_lines(GtkTextView *view, int *first, int *last)
{
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    GdkRectangle rect;

    gtk_text_view_get_visible_rect(view, &rect);
    gtk_text_view_get_iter_at_location(view, &start_iter, rect.x, rect.y);
    gtk_text_view_get_iter_at_location(view, &end_iter, rect.x, rect.y + rect.height);
    *first = gtk_text_iter_get_line(&start_iter) + 1;
    *last  = gtk_text_iter_get_line(&end_iter) + 1;
}

// ============================================================================================
// Refresh Visible Lines After Loading Breakpoints
// ============================================================================================
void breakpoints_changed(){
    if(!g_window) return;
    int first, last;
    get_visible_lines(GTK_TEXT_VIEW(g_window->source_view), &first, &last);
    for(int line = first; line <= last; line++) {
        refresh_line(
            g_window->source_buffer,
            line
        );
    }
}

// ==============================================================================================
// Update the visual highlight for the current cursor line, ensuring that only the current line is
// highlighted and that the highlight is updated correctly when the cursor moves to a new line
// ==============================================================================================
/*
static void update_cursor_visual(GtkTextBuffer *buffer){
    clear_line_tags(buffer, last_line_pos);
    if (cob.line_pos <= 0)
        return;
    GtkTextIter start;
    GtkTextIter end;
    get_line_bounds(buffer, cob.line_pos, &start, &end);
    gtk_text_buffer_apply_tag_by_name(buffer, "cursor_line", &start, &end);
    last_line_pos = cob.line_pos;
}
    */

// ==============================================================================================
// Update the debug line highlight, removing it from the previous debug line and applying it to the
// new debug line, and ensuring that the correct highlight is used based on whether the program is 
// running or paused in the debugger
// ==============================================================================================
static void update_debug_highlight(GtkTextBuffer *buffer){
    clear_line_tags(buffer, last_debug_line);
    update_line_prefix(buffer, last_debug_line, FALSE, FALSE);
    if (cob.debug_line <= 0)
        return;
    GtkTextIter start;
    GtkTextIter end;
    get_line_bounds(buffer, cob.debug_line, &start, &end);
    if (cob.running) {
        gtk_text_buffer_apply_tag_by_name(buffer, "running_line", &start, &end);
    } else {
        gtk_text_buffer_apply_tag_by_name(buffer, "debug_line", &start, &end);
    }
    update_line_prefix(buffer, cob.debug_line, TRUE, cob.running);
    last_debug_line = cob.debug_line;
}

// ==============================================================================================
// Update the UI to reflect the current line and debug line changes, ensuring that the correct lines
// are highlighted and that the current line is visible in the source view when it changes, and 
// that the debug line is also updated accordingly
// ==============================================================================================
static gboolean update_ui_line(gpointer user_data){
    WcobgdbWindow *win = get_main_window();
    if (!win)
        return FALSE;
    GtkTextBuffer *buffer = win->source_buffer;
    //update_cursor_visual(buffer);
    update_debug_highlight(buffer);
    if(cob.debug_line>0){
        force_line_visibility(win, cob.debug_line);
    }else{
        // If no debug line, ensure cursor line is visible
        force_line_visibility(win, cob.line_pos);
    }
    return FALSE;
}

// ==============================================================================================
// Refresh a specific line in the source view, updating the debug and cursor highlights for that 
// line, and ensuring that the correct indicators are shown based on whether the line is the 
// current debug line
// ==============================================================================================
void refresh_line(GtkTextBuffer *buffer,int line){
    if (line <= 0)
        return;
    update_line_prefix(buffer, line, line == cob.debug_line, cob.running);
    GtkTextIter start;
    GtkTextIter end;
    get_line_bounds(buffer, line, &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "debug_line", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "running_line", &start, &end);
    gtk_text_buffer_remove_tag_by_name(buffer, "cursor_line", &start, &end);
    if (line == cob.line_pos) {
        gtk_text_buffer_apply_tag_by_name(buffer, "cursor_line", &start, &end);
    }
    if (line == cob.debug_line) {
        gtk_text_buffer_apply_tag_by_name(buffer, cob.running ? "running_line" : "debug_line", &start, &end);
    }
}

// ==============================================================================================
// File line change handler to update the current line position and refresh the UI when the 
// user moves
// ==============================================================================================
void file_line_changed(int new_line) {
    cob.line_pos = new_line;
    update_ui_line(NULL);
}

// ================================================================================================
// Wait for the user to move the cursor to a new line in the source view, refreshing the line 
// ================================================================================================
void window_wait_user() {
    if (!g_window) return;
    refresh_line(g_window->source_buffer, cob.line_pos);
}

// ===================================================================================================
// Load the source file into the source view, clearing any existing content and re-rendering all lines
// ===================================================================================================
void load_source_file(WcobgdbWindow *self, Lines *lines){
    if (self == NULL || self->source_buffer == NULL) {
        return;
    }
    // CLEAR SOURCE BUFFER
    gtk_text_buffer_set_text(self->source_buffer, "", -1);
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(self->source_buffer, &iter);
    // INSERT SOURCE LINES
    Lines *line = lines;
    while (line != NULL) {
        insert_highlighted_line(self, self->source_buffer, &iter, line, self->tags);
        line = line->line_after;
    }
    // UPDATE CURRENT DEBUG LINE
    update_debug_highlight(self->source_buffer);
}

// ============================================================================================
// Initially set the scroll position to show the current line when the source file is loaded, 
// ensuring the line is visible in the source view
// ============================================================================================
gboolean initial_pos_line(gpointer user_data) {
    GtkTextIter iter;
    int line_pos = GPOINTER_TO_INT(user_data);
    gtk_text_buffer_get_iter_at_line(g_window->source_buffer, &iter, line_pos-1);
    GdkRectangle location;
    gtk_text_view_get_iter_location(GTK_TEXT_VIEW(g_window->source_view), &iter, &location);
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(g_window->source_view), &alloc);
    int line_height = location.height;
    /* Set position to +/- 10 visible lines */
    double yalign = (double)(line_height * 10) / alloc.height;
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(g_window->source_view), &iter, 0.0, TRUE, 0.0, yalign);
    return FALSE;
}

// ============================================================================================
// Initialize the scroll position to show the current line when the source file is loaded
// ============================================================================================
void initialize_line_pos(int line) {
    if (!g_window) return;
    g_idle_add(initial_pos_line, GINT_TO_POINTER(line));
}
