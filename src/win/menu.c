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
#include "win/window.h"
#include "gdb/cobgdb.h"

// Function pointer to send commands to GDB
extern int (*sendCommandGdb1)(char *);

// =====================================
// Menu item "Edit" callback
// =====================================
void on_menu_edit( GtkMenuItem *item, gpointer data){
    WcobgdbWindow *self = WCOBGDB_WINDOW(data);
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->tree_view));
    GtkTreeIter iter;
    if (!self->popup_path) {
        return;
    }
    if (!gtk_tree_model_get_iter(model, &iter, self->popup_path)){
        return;
    }
    gchar *current_value = NULL;
    gtk_tree_model_get(model, &iter, 1, &current_value, -1);
    if (!current_value) {
        return;
    }
    ST_DebuggerVariable *var = getShowVariableByC(current_value);
    if (!var) {
        g_free(current_value);
        return;
    }
    MI2evalVariable(sendCommandGdb1, var, 0, 0);
    char value_to_edit[1024];
    value_to_edit[0] = '\0';
    if (var->value != NULL) {
        if (var->value[0] == '"') {
            size_t max_copy = MIN(strlen(var->value) - 2, sizeof(value_to_edit) - 1);
            memset(value_to_edit,' ', MIN(var->size, sizeof(value_to_edit) - 1));
            value_to_edit[MIN(var->size, sizeof(value_to_edit) - 1)] = '\0';
            strncpy(value_to_edit, &var->value[1], max_copy);
            value_to_edit[max_copy] = '\0';
        } else {
            g_snprintf( value_to_edit, sizeof(value_to_edit), "%*s", MIN(var->size, 1000), var->value);
        }
    }
    // OPEN EDIT DIALOG
    char *new_value = show_text_edit_dialog( GTK_WINDOW(self), "Edit Value", value_to_edit);
    // USER CONFIRMED
    if (new_value)
    {
        MI2editVariable( sendCommandGdb1, var, new_value );
        g_timeout_add(50, refresh_variables_idle, get_main_window());
        /*
        char text[4096];
        char keyName[1024];
        const char *bg_color = "#10335c";
        snprintf(
            text,
            sizeof(text),
            "<span foreground=\"#4EC9B0\">%s</span> "
            "<span foreground=\"#6A9955\">= </span>"
            "<span background=\"%s\" foreground=\"#D4D4D4\"> %s </span>",
            var->cobolName,
            bg_color,
            new_value
        );
        snprintf(
            keyName,
            sizeof(keyName),
            "%s.%s",
            var->functionName,
            var->cName
        );
        gtk_tree_store_set(
            GTK_TREE_STORE(model),
            &iter,
            0, text,
            1, keyName,
            -1
        );
        */
        show_log("New value: %s", new_value);
        g_free(new_value);
    }
    g_free(current_value);
}

// =====================================
// Menu item "Watch" callback
// =====================================
void on_menu_watch(GtkMenuItem *item, gpointer data) {
    show_log("Add to watch");
    WcobgdbWindow *self = WCOBGDB_WINDOW(data);
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->tree_view));
    GtkTreeIter iter;
    if (!self->popup_path) {
        return;
    }
    if (!gtk_tree_model_get_iter(model, &iter, self->popup_path)){
        return;
    }
    gchar *current_value = NULL;
    gtk_tree_model_get(model, &iter, 1, &current_value, -1);
    if (!current_value) {
        return;
    }
    ST_DebuggerVariable *var = getShowVariableByC(current_value);
    if (!var) {
        g_free(current_value);
        // TODO: Error message
        return;
    }
    //g_timeout_add(50, refresh_variables_idle, get_main_window());
    refresh_variables_watch_tree(self, var);
    g_free(current_value);
    
}

// =====================================
// Menu creation for TreeView right-click
// =====================================
void create_popup_menu(WcobgdbWindow *self) {
    self->menu_popup = gtk_menu_new();
    GtkWidget *item_edit = gtk_menu_item_new_with_label("Edit");
    GtkWidget *item_watch = gtk_menu_item_new_with_label("Watch");
    gtk_menu_shell_append(GTK_MENU_SHELL(self->menu_popup), item_edit);
    gtk_menu_shell_append(GTK_MENU_SHELL(self->menu_popup), item_watch);
    g_signal_connect(item_edit, "activate", G_CALLBACK(on_menu_edit), self);
    g_signal_connect(item_watch, "activate", G_CALLBACK(on_menu_watch), self);
    gtk_widget_show_all(self->menu_popup);
}