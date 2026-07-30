#ifndef WCOBGDB_APP_H
#define WCOBGDB_APP_H

#include <gtk/gtk.h>

#define WCOBGDB_APP_TYPE \
    (wcobgdb_app_get_type())

G_DECLARE_FINAL_TYPE(
    WcobgdbApp,
    wcobgdb_app,
    WCOBGDB,
    APP,
    GtkApplication
)

// =====================================================
// CONSTRUCTOR
// =====================================================
WcobgdbApp *
wcobgdb_app_new(void);

// =====================================================
// COBGDB INIT
// =====================================================
void initialize_cobgdb(
    int argc,
    char **argv
);

#endif