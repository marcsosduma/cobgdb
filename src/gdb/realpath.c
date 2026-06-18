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

// ==========================================================================================
// Resolve a file path to its absolute canonical form on Windows.
//
// This function provides a Windows implementation compatible with
// the POSIX realpath() API, allowing source code portability across
// platforms.
//
// Internally, the Windows API GetFullPathNameA() is used to obtain
// the absolute path.
//
// Parameters:
//   name     - Input path to resolve.
//   resolved - Destination buffer. If NULL, a buffer is allocated
//              automatically.
//
// Returns:
//   Pointer to the resolved absolute path.
//   NULL if the path cannot be resolved or memory allocation fails.
//
// Notes:
//   - When 'resolved' is NULL, the returned buffer is dynamically
//     allocated and must be freed by the caller.
//   - The returned path may contain symbolic path components already
//     resolved by the operating system.
// ==========================================================================================
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