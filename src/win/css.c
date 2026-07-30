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
#include "win/window.h"
#include <stdarg.h>
void cobgdb_set_css(char css[], size_t css_size){
    snprintf(
        css,
        css_size,
        /* SOURCE VIEW */
        "textview {"
        "   background-color: #1E1E1E;"
        "   color: #DCDCDC;"
        "   caret-color: #2D2D30;"
        "}"

        "textview text {"
        "   background-color: #1E1E1E;"
        "   color: #DCDCDC;"
        "}"

        /* TREE VIEW */
        "treeview {"
        "   background-color: #252526;"
        "   color: #E0E0E0;"
        "}"

        "treeview.view:selected {"
        "   background-color: #1E1E1E;"
        "   color: #FFFFFF;"
        "}"

        /* OUTPUT VIEW */
        "textview#output_view {"
        "   background-color: #1E1E1E;"
        "   color: #DCDCDC;"
        "}"

        /* HEADER BUTTONS */
        "header button {"
        "   background-color: #2D2D30;"
        "   color: #FFFFFF;"
        "}"

        /* SCROLLBAR */
        "scrollbar slider {"
        "   background-color: #4A4A4A;"
        "   border-radius: 8px;"
        "}"

        /* TOOLTIP */
        "tooltip {"
        "   background-color: #1E1E1E;"
        "   border: 1px solid #3C3C3C;"
        "   border-radius: 6px;"
        "   padding: 10px;"
        "   opacity: 1;"
        "}"

        "tooltip label {"
        "   color: #D4D4D4;"
        "   font-family: '%s';"
        "   font-size: %dpt;"
        "}"

        /* POPUP MENU */
        "menu {"
        "   background-color: #252526;"
        "   border: 1px solid #3C3C3C;"
        "   padding: 4px;"
        "}"

        "menu menuitem {"
        "   background-color: transparent;"
        "   padding: 6px 12px;"
        "}"

        "menu menuitem label {"
        "   color: #DCDCDC;"
        "}"

        "menu menuitem:hover,"
        //"menu menuitem:prelight,"
        "menu menuitem:selected {"
        "   background-color: #094771;"
        "}"

        "menu menuitem:hover label,"
        //"menu menuitem:prelight label,"
        "menu menuitem:selected label {"
        "   color: #FFFFFF;"
        "}"

        /* DIALOG */
        "dialog {"
        "   background-color: #252526;"
        "   color: #DCDCDC;"
        "}"

        "dialog .dialog-action-area {"
        "   padding: 10px;"
        "}"

        "dialog label {"
        "   color: #DCDCDC;"
        "}"

        "dialog entry {"
        "   background-color: #1E1E1E;"
        "   color: #DCDCDC;"
        "   border: 1px solid #3C3C3C;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"

        "dialog entry selection {"
        "   background-color: #094771;"
        "   color: #FFFFFF;"
        "}"

        "dialog button {"
        "   background-color: #DCDCDC;"
        "   color: #000000;"
        "   border: 1px solid #3C3C3C;"
        "   border-radius: 4px;"
        "   padding: 6px 14px;"
        "}"

        "dialog button label {"
        "   color: #000000;"
        "}"

        "dialog button:hover {"
        "   background-color: #C8C8C8;"
        "}"

        "dialog button:focus {"
        "   border-color: #569CD6;"
        "}"

         /* LISTBOX */
        "list {"
        "   background-color: #1E1E1E;"
        "   color: #DCDCDC;"
        "}"

        "list row {"
        "   background: #272730;"
        "   color: #DCDCDC;"
        "}"

        "list row:hover {"
        "   background: #2A2D2E;"
        "}"

        "list row:selected {"
        "   background: #094771;"
        "}"

        "list row:selected label {"
        "   color: #FFFFFF;"
        "}"

        "list label {"
        "   background: transparent;"
        "   color: #DCDCDC;"
        "   padding: 6px;"
        "   opacity: 1;"
        "}"
        ,
        APP_FONT_FAMILY,
        APP_FONT_SIZE + 1
    );
}