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
#include "win/app.h"
#include <glib.h> // Required for g_setenv

// Prototype
int handle_special_args(int argc, char **argv, int arg_init);

int main(int argc, char **argv)
{
    // =====================================================
    // Environment variables for portable execution.
    // =====================================================
    #if defined(_WIN32)
    g_setenv("GTK_DATA_PREFIX", ".", TRUE);
    g_setenv("GDK_PIXBUF_MODULEDIR", "lib/gdk-pixbuf-2.0/2.10.0/loaders", TRUE);
    g_setenv("GDK_PIXBUF_MODULE_FILE", "lib/gdk-pixbuf-2.0/2.10.0/loaders.cache", TRUE);
    g_setenv("XDG_DATA_DIRS", "share", TRUE);
    g_setenv("DBUS_SESSION_BUS_ADDRESS", "0", TRUE);
    #endif

    // =====================================================
    // Handle arguments like --test, --version, 
    // --help and execute the corresponding actions
    // =====================================================
    if (handle_special_args(argc, argv, 1))
        return 0;

    // =====================================================
    // Start the GTK application and set the default icon
    // =====================================================
    #if defined(__linux__)
    gtk_window_set_default_icon_from_file("icons/app.png", NULL);
    #endif
    return g_application_run(G_APPLICATION(wcobgdb_app_new()), argc, argv);
}