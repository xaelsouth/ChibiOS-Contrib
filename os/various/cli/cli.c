/*
    ChibiOS - Copyright (C) 2026 Xael South

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    cli.c
 * @brief   Common helpers for CLI modules: command table merging.
 *
 * @addtogroup CLI
 * @{
 */

#include "cli.h"

#if CLI_HAS_SHELL || CLI_HAS_XSHELL

#if CLI_HAS_SHELL
/**
 * @brief   Counts entries in a NULL-terminated ShellCommand table.
 */
size_t cli_shell_count(const ShellCommand *t) {
  size_t n = 0U;

  if (t != NULL) {
    while (t[n].sc_name != NULL) {
      n++;
    }
  }
  return n;
}

/**
 * @brief   Merges two NULL-terminated ShellCommand tables.
 * @details @p out must hold cli_shell_count(a) + cli_shell_count(b) + 1
 *          entries. Returns @p out.
 */
const ShellCommand *cli_shell_merge(const ShellCommand *a,
                                    const ShellCommand *b,
                                    ShellCommand *out) {
  size_t na = cli_shell_count(a);
  size_t nb = cli_shell_count(b);
  size_t i;

  for (i = 0U; i < na; i++) {
    out[i] = a[i];
  }
  for (i = 0U; i < nb; i++) {
    out[na + i] = b[i];
  }
  out[na + nb].sc_name = NULL;
  out[na + nb].sc_function = NULL;

  return out;
}
#endif /* CLI_HAS_SHELL */

#if CLI_HAS_XSHELL
/**
 * @brief   Counts entries in a NULL-terminated xshell_command_t table.
 */
size_t cli_xshell_count(const xshell_command_t *t) {
  size_t n = 0U;

  if (t != NULL) {
    while (t[n].name != NULL) {
      n++;
    }
  }
  return n;
}

/**
 * @brief   Merges two NULL-terminated xshell_command_t tables.
 * @details @p out must hold cli_xshell_count(a) + cli_xshell_count(b) + 1
 *          entries. Returns @p out.
 */
const xshell_command_t *cli_xshell_merge(const xshell_command_t *a,
                                         const xshell_command_t *b,
                                         xshell_command_t *out) {
  size_t na = cli_xshell_count(a);
  size_t nb = cli_xshell_count(b);
  size_t i;

  for (i = 0U; i < na; i++) {
    out[i] = a[i];
  }
  for (i = 0U; i < nb; i++) {
    out[na + i] = b[i];
  }
  out[na + nb].name = NULL;
  out[na + nb].fn = NULL;

  return out;
}
#endif /* CLI_HAS_XSHELL */

#endif /* CLI_HAS_SHELL || CLI_HAS_XSHELL */

/** @} */
