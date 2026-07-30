#ifndef VARIABLES_H
#define VARIABLES_H
#include <gtk/gtk.h>

typedef struct _WcobgdbWindow WcobgdbWindow;

void setup_variables_treeview(WcobgdbWindow *self);
void refresh_variables_tree(WcobgdbWindow *self);
void clear_variables_tree(WcobgdbWindow *self);
#endif