#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "commands.h"

#include <stdio.h>
#include <wchar.h>

typedef struct RuntimeState {
    HMODULE module;
    int implementation;
} RuntimeState;

static int library_path(int implementation, wchar_t *path, size_t capacity) {
    const wchar_t *name = implementation == 1 ? L"lab5_impl1.dll" : L"lab5_impl2.dll";
    DWORD length = GetModuleFileNameW(NULL, path, (DWORD)capacity);
    if (length == 0 || length >= capacity) {
        return 0;
    }

    wchar_t *separator = wcsrchr(path, L'\\');
    if (separator == NULL) {
        return 0;
    }
    size_t prefix_length = (size_t)(separator - path) + 1;
    size_t name_length = wcslen(name);
    if (prefix_length + name_length >= capacity) {
        return 0;
    }
    wmemcpy(path + prefix_length, name, name_length + 1);
    return 1;
}

static int load_implementation(int implementation, LibraryFunctions *functions, HMODULE *module_out) {
    wchar_t path[32768];
    if (!library_path(implementation, path, sizeof(path) / sizeof(path[0]))) {
        fputs("Could not determine the DLL path.\n", stderr);
        return 0;
    }

    HMODULE module = LoadLibraryW(path);
    if (module == NULL) {
        fprintf(stderr, "Could not load implementation %d (Windows error %lu).\n",
                implementation, (unsigned long)GetLastError());
        return 0;
    }

    Lab5GcfFn gcf = (Lab5GcfFn)GetProcAddress(module, "GCF");
    Lab5SortFn sort = (Lab5SortFn)GetProcAddress(module, "Sort");
    if (gcf == NULL || sort == NULL) {
        fprintf(stderr, "Implementation %d does not export GCF and Sort.\n",
                implementation);
        FreeLibrary(module);
        return 0;
    }

    functions->gcf = gcf;
    functions->sort = sort;
    *module_out = module;
    return 1;
}

static int switch_implementation(LibraryFunctions *functions, void *context) {
    RuntimeState *state = context;
    int next = state->implementation == 1 ? 2 : 1;
    HMODULE new_module;
    if (!load_implementation(next, functions, &new_module)) {
        puts("The current implementation is still active.");
        return 0;
    }

    FreeLibrary(state->module);
    state->module = new_module;
    state->implementation = next;
    printf("Using implementation %d.\n", next);
    return 1;
}

int main(void) {
    LibraryFunctions functions;
    RuntimeState state = {NULL, 1};
    if (!load_implementation(1, &functions, &state.module)) {
        return 1;
    }

    puts("Using implementation 1.");
    int status = run_commands(&functions, switch_implementation, &state);
    FreeLibrary(state.module);
    return status;
}
