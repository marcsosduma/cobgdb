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

// =====================================================
// Create help dialog
// =====================================================
HelpWindow *help_window_new(GtkWindow *parent)
{
    HelpWindow *hw = g_malloc0(sizeof(HelpWindow));
    GtkWidget *content_area;
    GtkWidget *scroll;

    hw->dialog = gtk_dialog_new_with_buttons("Help", parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Close", GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_default_size(GTK_WINDOW(hw->dialog), 800, 600);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(hw->dialog));
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(content_area), scroll);
    hw->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(hw->text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hw->text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scroll), hw->text_view);
    hw->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hw->text_view));
    gtk_text_buffer_get_start_iter(hw->buffer, &hw->iter);
    return hw;
}

// =====================================================
// Add text to help dialog
// =====================================================
void help_add_text(HelpWindow *hw, const char *text, const char *tag_name, const char *color, gboolean bold){
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(hw->buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
    if (!tag) {
        gtk_text_buffer_create_tag(hw->buffer, tag_name, "foreground", color, "weight", bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL, NULL);
    }
    gtk_text_buffer_insert_with_tags_by_name(hw->buffer, &hw->iter, text, -1, tag_name, NULL);
}

// =====================================================
// Show help dialog
// =====================================================
void help_window_show(HelpWindow *hw){
    gtk_widget_show_all(hw->dialog);
    gtk_dialog_run(GTK_DIALOG(hw->dialog));
    gtk_widget_destroy(hw->dialog);
    g_free(hw);
}
