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
#include "gdb/cobgdb.h"
#include "win/window.h"

// =====================================================
// Show Dialog
// =====================================================
// Return:
// 1 = Confirm
// 0 = Cancel
// =====================================================
int show_confirm_dialog(GtkWindow *parent, const char *title, const char *message){
    GtkWidget *dialog;
    gint response;
    int result = 0;

    dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "%s",
        message
    );
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancelar", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Confirmar", GTK_RESPONSE_OK );
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        result = 1;
    }
    gtk_widget_destroy(dialog);
    return result;
}

// =====================================================
// Show Message Dialog
// =====================================================
void show_message_dialog(GtkWindow *parent, const char *title, const char *message){
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s",
        message
    );
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// =====================================================
// Auto Close Timeout
// =====================================================
static gboolean close_dialog_timeout(gpointer data)
{
    GtkWidget *dialog = GTK_WIDGET(data);
    if (GTK_IS_DIALOG(dialog)) {
        gtk_dialog_response(
            GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE
        );
    }
    g_object_unref(dialog);
    return FALSE;
}

// =====================================================
// Show Cob Message
// =====================================================
int showCobMessage(char *message, int type)
{
    GtkWidget *dialog;
    guint timeout_id;
    (void) type;
    dialog = gtk_message_dialog_new(
        GTK_WINDOW(get_main_window()),
        GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s",
        message
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "WCobGDB");
    g_object_ref(dialog);
    timeout_id = g_timeout_add_seconds( 3, close_dialog_timeout, dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    if (g_main_context_find_source_by_id(NULL, timeout_id)) {
        g_source_remove(timeout_id);
        g_object_unref(dialog);
    }
    gtk_widget_destroy(dialog);
    return 1;
}