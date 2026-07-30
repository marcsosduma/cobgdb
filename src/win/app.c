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
#include "win/app.h"

struct _WcobgdbApp
{
    GtkApplication parent_instance;
};

G_DEFINE_TYPE(
    WcobgdbApp,
    wcobgdb_app,
    GTK_TYPE_APPLICATION
)

// =====================================================
// COMMAND LINE
// =====================================================
static int
wcobgdb_app_command_line(
    GApplication *app,
    GApplicationCommandLine *cmdline
)
{
    int argc;
    char **argv;

    argv = g_application_command_line_get_arguments(
        cmdline,
        &argc
    );

    initialize_cobgdb(
        argc,
        argv
    );

    g_strfreev(argv);

    return 0;
}

// =====================================================
// CLASS INIT
// =====================================================
static void
wcobgdb_app_class_init(
    WcobgdbAppClass *class
)
{
    G_APPLICATION_CLASS(class)->command_line =
        wcobgdb_app_command_line;
}

// =====================================================
// INIT
// =====================================================
static void
wcobgdb_app_init(
    WcobgdbApp *self
)
{
}

// =====================================================
// NEW
// =====================================================
WcobgdbApp *
wcobgdb_app_new(void)
{
    return g_object_new(
        WCOBGDB_APP_TYPE,
        "application-id",
        "org.wcobgdb.app",
        "flags",
        G_APPLICATION_HANDLES_COMMAND_LINE,
        NULL
    );
}