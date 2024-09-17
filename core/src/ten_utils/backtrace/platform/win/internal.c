//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include <Windows.h>
#include <assert.h>
#include <dbghelp.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_utils/backtrace/backtrace.h"
#include "include_internal/ten_utils/backtrace/common.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/memory.h"

typedef BOOL(WINAPI *win_SymInitialize_func_t)(HANDLE hProcess,
                                               PCSTR UserSearchPath,
                                               BOOL fInvadeProcess);

typedef BOOL(WINAPI *win_SymCleanup_func_t)(HANDLE hProcess);

typedef DWORD(WINAPI *win_SymGetOptions_func_t)(VOID);

typedef DWORD(WINAPI *win_SymSetOptions_func_t)(DWORD);

typedef BOOL(WINAPI *win_SymFromAddr_func_t)(HANDLE hProcess, DWORD64 Address,
                                             PDWORD64 Displacement,
                                             PSYMBOL_INFO Symbol);

typedef BOOL(WINAPI *win_SymGetLineFromAddr_func_t)(HANDLE hProcess,
                                                    DWORD dwAddr,
                                                    PDWORD pdwDisplacement,
                                                    PIMAGEHLP_LINE Line);

typedef WORD(WINAPI *win_RtlCaptureStackBackTrace_func_t)(DWORD FramesToSkip,
                                                          DWORD FramesToCapture,
                                                          PVOID *BackTrace,
                                                          PDWORD BackTraceHash);

/**
 * @brief The Windows specific ten_backtrace_t implementation.
 */
typedef struct ten_backtrace_win_t {
  ten_backtrace_common_t common;

  // @{
  // The Windows functions used to dump backtrace.

  // From NtDll.dll
  win_RtlCaptureStackBackTrace_func_t RtlCaptureStackBackTrace;

  // From DbgHelp.dll
  win_SymInitialize_func_t SymInitialize;
  win_SymCleanup_func_t SymCleanup;
  win_SymGetOptions_func_t SymGetOptions;
  win_SymSetOptions_func_t SymSetOptions;
  win_SymFromAddr_func_t SymFromAddr;
  win_SymGetLineFromAddr_func_t SymGetLineFromAddr;
  // @}
} ten_backtrace_win_t;

static void retrieve_windows_backtrace_funcs(ten_backtrace_win_t *self) {
  assert(self && "Invalid argument.");

  HMODULE dll_handle = LoadLibraryA("DbgHelp.dll");
  if (dll_handle != NULL) {
    self->SymFromAddr =
        (win_SymFromAddr_func_t)GetProcAddress(dll_handle, "SymFromAddr");
    assert(self->SymFromAddr && "Failed to find DbgHelp.SymFromAddr");

    self->SymGetLineFromAddr = (win_SymGetLineFromAddr_func_t)GetProcAddress(
        dll_handle, "SymGetLineFromAddr64");
    assert(self->SymGetLineFromAddr &&
           "Failed to find DbgHelp.SymGetLineFromAddr");

    self->SymInitialize =
        (win_SymInitialize_func_t)GetProcAddress(dll_handle, "SymInitialize");
    assert(self->SymInitialize && "Failed to find DbgHelp.SymInitialize");

    self->SymCleanup =
        (win_SymCleanup_func_t)GetProcAddress(dll_handle, "SymCleanup");
    assert(self->SymCleanup && "Failed to find DbgHelp.SymCleanup");

    self->SymGetOptions =
        (win_SymGetOptions_func_t)GetProcAddress(dll_handle, "SymGetOptions");
    assert(self->SymGetOptions && "Failed to find DbgHelp.SymGetOptions");

    self->SymSetOptions =
        (win_SymSetOptions_func_t)GetProcAddress(dll_handle, "SymSetOptions");
    assert(self->SymSetOptions && "Failed to find DbgHelp.SymSetOptions");
  } else {
    assert(0 && "Failed to find DbgHelp.dll");
  }

  dll_handle = LoadLibraryA("NtDll.dll");
  if (dll_handle != NULL) {
    self->RtlCaptureStackBackTrace =
        (win_RtlCaptureStackBackTrace_func_t)GetProcAddress(
            dll_handle, "RtlCaptureStackBackTrace");
    assert(self->RtlCaptureStackBackTrace &&
           "Failed to find NtDll.RtlCaptureStackBackTrace");
  } else {
    assert(0 && "Failed to find NtDll.dll");
  }
}

ten_backtrace_t *ten_backtrace_create(void) {
  ten_backtrace_win_t *self =
      ten_malloc_without_backtrace(sizeof(ten_backtrace_win_t));
  assert(self && "Failed to allocate memory.");

  ten_backtrace_common_init(&self->common, ten_backtrace_default_dump_cb,
                            ten_backtrace_default_error_cb);
  retrieve_windows_backtrace_funcs(self);

  return self;
}

void ten_backtrace_destroy(ten_backtrace_t *self) {
  assert(self && "Invalid argument.");

  ten_backtrace_common_deinit(self);

  ten_free_without_backtrace(self);
}

void ten_backtrace_dump(ten_backtrace_t *self, size_t skip) {
  assert(self && "Invalid argument.");

  ten_backtrace_win_t *win_self = (ten_backtrace_win_t *)self;

  if (win_self->SymInitialize == NULL || win_self->SymCleanup == NULL ||
      win_self->SymGetOptions == NULL || win_self->SymSetOptions == NULL ||
      win_self->SymFromAddr == NULL || win_self->SymGetLineFromAddr == NULL ||
      win_self->RtlCaptureStackBackTrace == NULL) {
    return;
  }

  HANDLE process = GetCurrentProcess();

  win_self->SymSetOptions(win_self->SymGetOptions() | SYMOPT_LOAD_LINES |
                          SYMOPT_DEFERRED_LOADS);
  win_self->SymInitialize(process, NULL, TRUE);

  void *stack[100] = {0};
  USHORT frames = win_self->RtlCaptureStackBackTrace(0, 100, stack, NULL);

  SYMBOL_INFO *symbol = (SYMBOL_INFO *)ten_calloc_without_backtrace(
      offsetof(SYMBOL_INFO, Name) + 256 * sizeof(symbol->Name[0]), 1);
  assert(symbol && "Failed to allocate memory.");

  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  for (size_t i = skip; i < frames; i++) {
    BOOL rc = TRUE;

    rc = win_self->SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
    if (rc == FALSE) {
      TEN_LOGE("Failed to SymFromAddr(): %ld", GetLastError());
      assert(0);
    }

    IMAGEHLP_LINE lineInfo = {0};
    lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE);
    DWORD dwLineDisplacement = 0;
    rc = win_self->SymGetLineFromAddr(process, (DWORD64)(stack[i]),
                                      &dwLineDisplacement, &lineInfo);
    if (rc == TRUE) {
      win_self->common.dump_cb(self, symbol->Address, lineInfo.FileName,
                               lineInfo.LineNumber, symbol->Name, NULL);
    } else {
      win_self->common.dump_cb(self, symbol->Address, NULL, 0, symbol->Name,
                               NULL);
    }
  }

  ten_free_without_backtrace(symbol);

  win_self->SymCleanup(process);
}
