/*
 * COBGDB GnuCOBOL Debugger:
 *
 * License:
 * This code is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 * 
 * Author: Marcos Martins Duma
 */
#if defined(_WIN32)
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <windows.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Function to emulate the basic functionality of realpath on Linux in a Windows environment
char* realpath(const char* name, char* resolved) {
    char* fullPath = NULL;

    // If 'resolved' is not provided, allocate a buffer
    if (resolved == NULL) {
        fullPath = (char*)malloc(PATH_MAX);
        if (fullPath == NULL) {
            perror("malloc");
            return NULL;
        }
    } else {
        fullPath = resolved;
    }

    // Get the absolute path using GetFullPathNameA
    DWORD result = GetFullPathNameA(name, PATH_MAX, fullPath, NULL);

    if (result == 0 || result >= PATH_MAX) {
        perror("GetFullPathNameA");
        if (resolved == NULL) {
            free(fullPath);
        }
        return NULL;
    }

    return fullPath;
}
#endif