/*
 * COBGDB/WCOBGDB GnuCOBOL Debugger:
 * This code is based on the GnuCOBOL Debugger extension available at:
 * https://github.com/OlegKunitsyn/gnucobol-debug
 *
 * License:
 * This code is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 * * Author: Marcos Martins Duma
 */
#include <gtk/gtk.h>
#include "win/window.h"

// =====================================================
// Handle key press events in dialogs
// =====================================================
static gboolean on_dialog_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Escape) {
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}

// ======================================
// TEXT EDIT DIALOG
// ======================================
char *show_text_edit_dialog(GtkWindow *parent, const char *title, const char *initial_value) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *entry;
    char *result = NULL;

    dialog = gtk_dialog_new_with_buttons(
        title,
        parent,
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    g_signal_connect( dialog, "key-press-event", G_CALLBACK(on_dialog_key_press), NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    entry = gtk_entry_new();
    if (initial_value) {
        gtk_entry_set_text(
            GTK_ENTRY(entry),
            initial_value
        );
    }
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        result = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    }
    gtk_widget_destroy(dialog);
    return result;
}

// =====================================================
// Filter: Allow only numeric digits
// =====================================================
static void on_insert_text(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data) {
    for (int i = 0; i < length; i++) {
        // Stop emission if the character is not a digit
        if (!g_ascii_isdigit(text[i])) {
            g_signal_stop_emission_by_name(editable, "insert-text");
            return;
        }
    }
}

// =====================================================
// Generic dialog runner
// =====================================================
int run_dialog(char *title, char *msg, char *var, int size, gboolean numeric_only) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, get_gtk_window(), GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_OK, NULL);
    
    g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_dialog_key_press), NULL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 10); // Add padding
    gtk_container_add(GTK_CONTAINER(content), gtk_label_new(msg));

    GtkWidget *entry = gtk_entry_new();
    if (var && *var) gtk_entry_set_text(GTK_ENTRY(entry), var);
    
    // Ensure we respect buffer size and prevent overflow
    gtk_entry_set_max_length(GTK_ENTRY(entry), size - 1);
    gtk_entry_set_width_chars(GTK_ENTRY(entry), size > 20 ? 20 : size);

    if (numeric_only) {
        g_signal_connect(entry, "insert-text", G_CALLBACK(on_insert_text), NULL);
    }
    
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        strncpy(var, gtk_entry_get_text(GTK_ENTRY(entry)), size - 1);
        var[size - 1] = '\0';
    }
    
    gtk_widget_destroy(dialog);
    return (response == GTK_RESPONSE_OK);
}

// =====================================================
// Wrappers for ease of use
// =====================================================

// Standard text input dialog
int dialogBox(char *title, char *msg, char *var, int size) {
    return run_dialog(title, msg, var, size, FALSE);
}

// Numeric only input dialog
int dialogBoxNum(char *title, char *msg, char *var, int size) {
    return run_dialog(title, msg, var, size, TRUE);
}