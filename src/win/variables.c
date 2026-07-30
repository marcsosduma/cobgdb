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
#include "win/variables.h"
#include "win/window.h"
#include "gdb/cobgdb.h"

// Prototypes
static void update_variable_text(GtkTreeStore *store, GtkTreeIter *iter, ST_DebuggerVariable *var, gboolean changed);

// Global variables
extern ST_DebuggerVariable *DebuggerVariable;
extern int (*sendCommandGdb1)(char *);
extern struct st_cobgdb cob;
extern int processing_gdb;
extern int upd_var;

// Structure to hold data for flashing variable changes
typedef struct {
    WcobgdbWindow *self;
    ST_DebuggerVariable *var;
} FlashData;

// =====================================================
// Get variable key for caching and node reference
// =====================================================
static const char *get_variable_key(ST_DebuggerVariable *var){
    if (!var) return "";
    // Adicione a verificação de nulo antes do strlen
    if (var->cName && var->cName[0] != '\0') return var->cName;
    if (var->cobolName && var->cobolName[0] != '\0') return var->cobolName;
    return "";
}

// =====================================================
// First variable in the list
// =====================================================
ST_DebuggerVariable *firstVar(ST_DebuggerVariable *var){
    var = DebuggerVariable;
    while (var != NULL) {
        if (var->variablesByCobol != NULL && var->parent == NULL){
            return var;
        }
        var = var->next;
    }
    return NULL;
}

// =====================================================
// Variable value changed compared to cache
// =====================================================
static gboolean variable_value_changed(WcobgdbWindow *self, ST_DebuggerVariable *var){
    // CREATE HASH IF NULL
    if (self->variable_values == NULL) {
        self->variable_values = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    }
    const char *key = get_variable_key(var);
    const char *new_value = var->value ? var->value : "";
    char *old_value = g_hash_table_lookup( self->variable_values, key);
    // FIRST TIME
    if (!old_value) {
        return TRUE;
    }
    // VALUE CHANGED
    if (strcmp(old_value, new_value) != 0){
        return TRUE;
    }
    return FALSE;
}

// =====================================================
// Update variable value in cache
// =====================================================
static void update_variable_cache(WcobgdbWindow *self, ST_DebuggerVariable *var){
    // CREATE HASH IF NULL
    if (self->variable_values == NULL) {
        self->variable_values = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    }
    const char *key = get_variable_key(var);
    const char *value = var->value ? var->value : "";
    g_hash_table_replace(self->variable_values, g_strdup(key), g_strdup(value));
}

// =====================================================
// Get variable iterator from TreeStore using cache reference
// =====================================================
static gboolean get_variable_iter(WcobgdbWindow *self, GtkTreeStore *store, 
    ST_DebuggerVariable *var, GtkTreeIter *iter){
    const char *key = get_variable_key(var);
    GtkTreeRowReference *ref = g_hash_table_lookup(self->variable_nodes, key);
    if (!ref) {
        return FALSE;
    }
    if (!gtk_tree_row_reference_valid(ref)) {
        return FALSE;
    }
    GtkTreePath *path = gtk_tree_row_reference_get_path(ref);
    if (!path)
        return FALSE;
    gboolean ok = gtk_tree_model_get_iter( GTK_TREE_MODEL(store), iter, path );
    gtk_tree_path_free(path);
    return ok;
}

// =====================================================
// Reset variable text to normal after flashing change
// =====================================================
static gboolean reset_variable_flash(gpointer data) {
    FlashData *flash_data = (FlashData *)data;
    if (!GTK_IS_WIDGET(flash_data->self->tree_view)) {
        g_free(flash_data);
        return FALSE;
    }
    GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(flash_data->self->tree_view)));
    GtkTreeIter iter;
    if (get_variable_iter(flash_data->self, store, flash_data->var, &iter)) {
        update_variable_text(store, &iter, flash_data->var, FALSE);
        update_variable_cache(flash_data->self, flash_data->var);
    }
    g_free(flash_data);
    return FALSE;
}

// =====================================================
// Update variable text in TreeStore with color based on change
// =====================================================
static void update_variable_text(GtkTreeStore *store, GtkTreeIter *iter, 
    ST_DebuggerVariable *var, gboolean changed){
    if (!var) return;
    const char *name = var->cobolName ? var->cobolName : "";
    char *raw_value = var->value ? var->value : "";
    char *safe_value = g_markup_escape_text(raw_value, -1);
    // Define background color
    const char *bg_color = changed ? "#10335c" : "#252526";
    char text[8192];
    char keyName[2048];
    
    snprintf(text, sizeof(text),
        "<span foreground=\"#4EC9B0\">%s</span> "
        "<span foreground=\"#6A9955\">= </span>" 
        "<span background=\"%s\" foreground=\"#D4D4D4\"> %s </span>",
        name,
        bg_color,
        safe_value
    );
    g_free(safe_value);
    snprintf(keyName, sizeof(keyName), "%s.%s", 
             var->functionName ? var->functionName : "unknown", 
             var->cName ? var->cName : "unknown");
    gtk_tree_store_set(store, iter, 0, text, 1, keyName, -1);
}

// =====================================================
// Create variable node in TreeStore and cache reference
// =====================================================
static void create_variable_node(WcobgdbWindow *self, GtkTreeStore *store, 
    GtkTreeIter *parent, ST_DebuggerVariable *var, GtkTreeIter *iter){
    gtk_tree_store_append(store, iter, parent);
    update_variable_text(store, iter, var, FALSE);
    update_variable_cache(self, var);
    // CREATE TREE ROW REFERENCE
    GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
    GtkTreeRowReference *ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(store), path);
    gtk_tree_path_free(path);
    g_hash_table_insert(self->variable_nodes, g_strdup(get_variable_key(var)), ref);
    //printf("INSERT NODE: %s\n", get_variable_key(var));
}

// =====================================================
// Add or update variable recursively in TreeStore
// =====================================================
static void add_variable_recursive(WcobgdbWindow *self, GtkTreeStore *store, 
    GtkTreeIter *parent, ST_DebuggerVariable *var){
    if (!var) return;
    if(var->value == NULL){
        wait_process_gdb();
        processing_gdb = TRUE;
        MI2evalVariable(sendCommandGdb1,var,0,0);
        processing_gdb = FALSE;
    }
    GtkTreeIter iter;
    gboolean exists = get_variable_iter( self, store, var, &iter );
    // UPDATE EXISTING NODE
    if (exists) {
        gboolean changed = variable_value_changed(self, var);
        update_variable_text(store, &iter, var, changed);
        update_variable_cache(self, var);
        /*
        update_variable_text(store, &iter, var, changed);
        update_variable_cache(self, var);
        */
        if (changed) {
            FlashData *data = g_new0(FlashData, 1);
            data->self = self;
            data->var = var;
            g_timeout_add(1500, reset_variable_flash, data);
        }
    } else {
        create_variable_node(self, store, parent, var, &iter);
    }
    // PROCESS CHILD VARIABLES
    if(var->first_children != NULL) {
        add_variable_recursive(self, store, &iter, var->first_children);
    }
    // PROCESS SIBLING VARIABLES
    if (var->brother != NULL) {
        add_variable_recursive(self, store, parent, var->brother);
    }
}

// =====================================================
// Setup TreeView for variables with model and columns
// =====================================================
void setup_variables_treeview(WcobgdbWindow *self){
    GtkTreeStore *store;
    store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(self->tree_view), GTK_TREE_MODEL(store));
    g_object_unref(store);
    // CREATE CELL RENDERER
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "family", "monospace", "ypad", 1, NULL);
    // CREATE COLUMN
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes(
            "",
            renderer,
            "markup",
            0,
            "cell-background", 1,
            "cell-background-set", 1,
            NULL
        );
    gtk_tree_view_append_column( GTK_TREE_VIEW(self->tree_view), column);
    // CREATE HASH TABLES
    self->variable_nodes = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_row_reference_free);
    self->variable_values = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    // TREE VIEW VISUAL SETTINGS
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->tree_view), FALSE);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(self->tree_view), TRUE);
}

// =====================================================
// Clear variables tree and cache, called when loading 
// new source or refreshing variables
// =====================================================
void clear_variables_tree(WcobgdbWindow *self){
    if (!self) return;
    if (self->variable_values) {
        g_hash_table_remove_all(self->variable_values);
    }
    if (!self->tree_view) return;
    GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->tree_view)));
    if (store) {
        gtk_tree_store_clear(store);
    }
    // CLEAR NODE REFERENCES HASH
    if (self->variable_nodes) {
        g_hash_table_remove_all(self->variable_nodes);
    }
}

// =====================================================
// Refresh variables tree by requesting variable list 
// from GDB and updating/creating nodes accordingly
// =====================================================
void refresh_variables_tree(WcobgdbWindow *self){
    if (!DebuggerVariable) return;

    while(upd_var==TRUE){
        sleep_ms(10);
    }    
    upd_var=TRUE;
    if (self == NULL || self->tree_view == NULL){
        return;
    }
    if (self->variable_nodes == NULL) {
        self->variable_nodes = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_row_reference_free);
    }
    GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->tree_view)));
    // REQUEST VARIABLES FROM GDB
    wait_process_gdb();
    processing_gdb = TRUE;
    MI2variablesRequest(sendCommandGdb1);
    processing_gdb = FALSE;
    // CREATE ROOT NODE
    GtkTreeIter root;
    gboolean root_exists = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &root);
    if (!root_exists) {
        gtk_tree_store_append(store, &root, NULL);
        gtk_tree_store_set(store, &root, 0, "Local", 1, "", -1);
    }
    // WALK THROUGH VARIABLES
    ST_DebuggerVariable *var = firstVar(NULL);
    while (var != NULL) {
        if (var->parent == NULL && strcmp(var->functionName, cob.function)==0) {
            add_variable_recursive(self, store, &root, var);
        }
        var = var->next;
    }
    // EXPAND ROOT NODE
    GtkTreePath *path = gtk_tree_path_new_first();
    gtk_tree_view_expand_row(GTK_TREE_VIEW(self->tree_view),path,FALSE);
    gtk_tree_path_free(path);
    upd_var = FALSE;
}


// =====================================================
// Refresh variables watch tree by requesting variable list 
// from GDB and updating/creating nodes accordingly
// =====================================================
void refresh_variables_watch_tree(WcobgdbWindow *self, ST_DebuggerVariable *var){
    if (!DebuggerVariable) return;

    while(upd_var==TRUE){
        sleep_ms(10);
    }    
    upd_var=TRUE;
    if (self == NULL || self->tree_view_watch == NULL){
        return;
    }
    if (self->variable_nodes == NULL) {
        self->variable_nodes = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_row_reference_free);
    }
    GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->tree_view_watch)));
    // CREATE ROOT NODE
    GtkTreeIter root;
    gboolean root_exists = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &root);
    if (!root_exists) {
        gtk_tree_store_append(store, &root, NULL);
        gtk_tree_store_set(store, &root, 0, "Watch", 1, "", -1);
    }
    // WALK THROUGH VARIABLES
    if (var->parent == NULL && strcmp(var->functionName, cob.function)==0) {
        add_variable_recursive(self, store, &root, var);
    }
    // EXPAND ROOT NODE
    GtkTreePath *path = gtk_tree_path_new_first();
    gtk_tree_view_expand_row(GTK_TREE_VIEW(self->tree_view_watch),path,FALSE);
    gtk_tree_path_free(path);
    upd_var = FALSE;
}