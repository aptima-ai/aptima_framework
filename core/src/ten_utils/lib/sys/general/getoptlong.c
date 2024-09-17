//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include "ten_utils/lib/getoptlong.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int get_short_flag(const char *str) {
  if (str == NULL || str[0] == '\0') {
    return 0;
  }

  if (str[0] != '-' || str[1] == '\0' || str[1] == '-') {
    return 0;
  }

  if (str[2] != '\0') {
    return 0;
  }

  return str[1];
}

static const char *get_long_flag(const char *str) {
  if (str == NULL || str[0] == '\0') {
    return NULL;
  }

  if (str[0] != '-' || str[1] != '-') {
    return NULL;
  }

  if (str[2] == '\0' || str[2] == '-') {
    return NULL;
  }

  return (str + 2);
}

static const ten_opt_long_t *get_opt_short(const ten_opt_long_t *opts,
                                           int short_flag) {
  if (short_flag <= 0) {
    return NULL;
  }
  const ten_opt_long_t *itor = opts;
  while (itor->short_name != 0) {
    if (itor->short_name == short_flag) {
      return itor;
    }
    itor++;
  }
  return NULL;
}

static const ten_opt_long_t *get_opt_long(const ten_opt_long_t *opts,
                                          const char *long_flag) {
  if (!long_flag) {
    return NULL;
  }
  char *key = strdup(long_flag);
  char *equal = strstr(key, "=");
  if (equal) {
    *equal = '\0';
  }
  const ten_opt_long_t *itor = opts;
  while (itor->long_name) {
    if (strcmp(itor->long_name, key) == 0) {
      free(key);
      return itor;
    }
    itor++;
  }
  free(key);
  return NULL;
}

void ten_print_help(const char *exec_name, const ten_opt_long_t *opts) {
  const char *tmp = strrchr(exec_name, '\\');
  if (!tmp) {
    tmp = strrchr(exec_name, '/');
  }
  if (!tmp) {
    tmp = exec_name;
  } else {
    tmp++;
  }
  fprintf(stderr, "\nUsage: %s [options]\n\n", tmp);
  const ten_opt_long_t *itor = opts;
  while (itor->long_name || itor->short_name) {
    int summary_len = 0;
    fprintf(stderr, "  ");
    summary_len += 2;
    if (isascii(itor->short_name)) {
      fprintf(stderr, "-%c", (char)itor->short_name);
      summary_len += 2;
      if (itor->has_param) {
        fprintf(stderr, " <value>");
        summary_len += 8;
      }
      if (itor->long_name) {
        fprintf(stderr, ", ");
        summary_len += 2;
      }
    }

    if (itor->long_name) {
      fprintf(stderr, "--%s", itor->long_name);
      summary_len += 2 + strlen(itor->long_name);
      if (itor->has_param) {
        fprintf(stderr, "=<value>");
        summary_len += 8;
      }
    }
    fprintf(stderr, ": ");
    summary_len += 2;
    if (itor->help_msg) {
      if (summary_len <= 40) {
        for (int i = 0; i < (40 - summary_len); i++) {
          fprintf(stderr, " ");
        }
      } else {
        fprintf(stderr, "\n");
        for (int i = 0; i < 40; i++) {
          fprintf(stderr, " ");
        }
      }
      fprintf(stderr, "%s\n", itor->help_msg);
    }
    itor++;
  }
  fprintf(stderr, "\n");
}

static int current_argv_index = 0;

int ten_getopt_long(int argc, const char **argv, const ten_opt_long_t *opts,
                    char **p) {
  // skip first parameter (usually exec name)
  argc--;
  argv++;

  if (current_argv_index >= argc) {
    return -1;
  }

  int idx = current_argv_index++;
  int short_flag = get_short_flag(argv[idx]);
  const ten_opt_long_t *opt = get_opt_short(opts, short_flag);

  if (opt) {
    if (opt->has_param) {
      if (current_argv_index >= argc) {
        return 0;
      }

      if (p) {
        *p = (char *)argv[current_argv_index];
      }

      current_argv_index++;
    } else if (p) {
      *p = NULL;
    }

    return opt->short_name;
  }

  const char *long_flag = get_long_flag(argv[idx]);
  opt = get_opt_long(opts, long_flag);
  if (!opt) {
    return 0;
  }

  if (opt->has_param) {
    char *equal_mark = strstr(argv[idx], "=");
    if (!equal_mark) {
      return 0;
    }

    if (p) {
      *p = equal_mark + 1;
    }

  } else if (p) {
    *p = NULL;
  }

  return opt->short_name;
}
