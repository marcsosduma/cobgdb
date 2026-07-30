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
#include <dirent.h> 
#include "win/window.h"
#include "gdb/cobgdb.h"

#define MAX_FILES 100
extern struct st_cobgdb cob;
char files[MAX_FILES][512];
int isFirstFile=FALSE;

// =====================================================
// File was selected
// =====================================================
void file_selected(size_t file_sel){
    char baseName[256];
    char nameCFile[1024];
    freeFile();
    strcpy(cob.file_cobol, files[file_sel]);
    if(isFirstFile) strcpy(cob.first_file, cob.file_cobol);
    if(!file_exists(cob.file_cobol)){
        show_log("File COB not found.");
        return;
    }  
    if(!isFirstFile){
        fileNameWithoutExtension(files[file_sel], &baseName[0]);
        normalizePath(baseName);
        strcpy(baseName,getFileNameFromPath(baseName));
        // C File
        snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
        if(file_exists(nameCFile)){
            parser(nameCFile, 0);
        }          
    }
    loadfile(files[file_sel]);
    highlightParse();

}

// =====================================================
// Button on listbox was pressed
// =====================================================
static gboolean on_listbox_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data){
    if (event->type == GDK_2BUTTON_PRESS) {
        GtkListBoxRow *row = gtk_list_box_get_selected_row( GTK_LIST_BOX(widget) );
        if (row) {
            //GtkWidget *child = gtk_bin_get_child( GTK_BIN(row) );
            //const char *text = gtk_label_get_text(GTK_LABEL(child));
            //g_print("Duplo clique: %s\n",text);
            gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT);
        }
    }
    return FALSE;
}

// =====================================================
// Search changed in entry
// =====================================================
static void on_search_changed(GtkEntry *entry, gpointer data) {
    GtkListBox *listbox = GTK_LIST_BOX(data);
    const char *search = gtk_entry_get_text(entry);
    char *search_low = g_ascii_strdown(search, -1);
    
    GList *rows = gtk_container_get_children(GTK_CONTAINER(listbox));
    for (GList *l = rows; l != NULL; l = l->next) {
        GtkWidget *row = GTK_WIDGET(l->data);
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(row));
        const char *text = gtk_label_get_text(GTK_LABEL(child));
        char *text_low = g_ascii_strdown(text, -1);
        gboolean visible = (g_strstr_len(text_low, -1, search_low) != NULL);
        g_free(text_low);
        gtk_widget_set_visible(row, visible);
    }
    g_free(search_low);
    g_list_free(rows);
}

// =====================================================
// Key press in dialog
// =====================================================
static gboolean on_dialog_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Escape) {
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_ACCEPT);
        return TRUE;
    }
    return FALSE;
}

// =====================================================
// Create and show the file selection dialog
// =====================================================
void show_my_dialog(GtkWindow *parent, char fileGroup[][512], size_t num_files) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *listbox;
    GtkWidget *scrolled_window;

    /* Create the dialog */
    dialog = gtk_dialog_new_with_buttons(
                        "Select File Source",
                        parent,
                        GTK_DIALOG_MODAL |
                        GTK_DIALOG_DESTROY_WITH_PARENT,
                        "_OK",
                        GTK_RESPONSE_ACCEPT,
                        "_Cancel",
                        GTK_RESPONSE_CANCEL,
                        NULL
                    );
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    /* List Box configuration */
    listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode( GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE );
    /* callback selection */
    g_signal_connect( listbox, "button-press-event", G_CALLBACK(on_listbox_button_press), dialog );
    g_signal_connect( dialog, "key-press-event", G_CALLBACK(on_dialog_key_press), NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 350);
    GtkWidget *search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text( GTK_ENTRY(search_entry), "Search..." );
    gtk_box_pack_start( GTK_BOX(content_area), search_entry, FALSE, FALSE, 5 );
    /* Scroll configuration */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), listbox);
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 5);
    g_signal_connect( search_entry, "changed", G_CALLBACK(on_search_changed), listbox);
    /* Add Item */
    for (int q = 0; q < num_files; q++) {
        GtkWidget *label = gtk_label_new(fileGroup[q]);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_widget_set_hexpand(label, TRUE);
        gtk_list_box_insert(GTK_LIST_BOX(listbox),label,-1);
    }
    gtk_widget_show_all(dialog);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        GtkListBoxRow *row = gtk_list_box_get_selected_row( GTK_LIST_BOX(listbox));
        if (row) {
            //GtkWidget *child = gtk_bin_get_child( GTK_BIN(row) );
            //const char *text = gtk_label_get_text( GTK_LABEL(child) );
            //g_print( "Selecionado ao sair: %s\n", text);
            int index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row));
            file_selected(index);
        }
    }
    /* Destroy dialog */
    gtk_widget_destroy(dialog);
}

// =====================================================
// Show sources files in listbox and parse if necessary
// =====================================================
void show_sources(int (*sendCommandGdb)(char *), int mustParse){
    char fileCGroup[100][512];
    isFirstFile=TRUE;

    size_t qtd_files = MI2sourceFiles(sendCommandGdb,files);
    if(mustParse){
        char baseName[256];
        char nameCFile[1024];
        int nfile=0;
        for(int a=0;a<qtd_files;a++){
            fileNameWithoutExtension(files[a], &baseName[0]);
            normalizePath(baseName);
            strcpy(baseName,getFileNameFromPath(baseName));
            // C File
            snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
            if(!file_exists(nameCFile)){
                show_log("File C not found.");
                return;
            }
            strcpy(fileCGroup[nfile],nameCFile);
            nfile++;
        }
        strcpy(fileCGroup[nfile], "");
        printf("Parser starting...\n");
		SourceMap(fileCGroup);
        printf("Parser end...\n");
    }
    show_my_dialog(GTK_WINDOW(get_main_window()), files, qtd_files);
}

// =====================================================
// Load source files into the listbox
// for 'F' key command
// =====================================================
void load_file(){
    char str[512];
    char cwd[512];
    char baseName[256];
    char nameCFile[1024];
    int qtd_files=0;
    DIR *d;
    struct dirent *dir;

    isFirstFile=FALSE;
    str[0] = '\0';
    getPathName(cwd, cob.first_file);
    d = opendir(cwd);
    if (d) {
        qtd_files = 0;
        while ((dir = readdir(d)) != NULL) {
            if(strlen(str)>0 && istrstr(dir->d_name, str)==NULL) continue;
            if(istrstr(dir->d_name,".cbl")!=NULL || istrstr(dir->d_name,".cob")!=NULL){
                sprintf(files[qtd_files],"%s/%s",cwd,dir->d_name);
                normalizePath(files[qtd_files]);
                fileNameWithoutExtension(files[qtd_files], &baseName[0]);
                strcpy(baseName,getFileNameFromPath(baseName));
                // C File
                snprintf(nameCFile, sizeof(nameCFile), "%s/%s.c", cob.cwd, baseName);
                if(!file_exists(nameCFile)){
                    continue;
                }
                qtd_files++;
            }
        }
        closedir(d);
    }
    show_my_dialog(GTK_WINDOW(get_main_window()), files, qtd_files);
}