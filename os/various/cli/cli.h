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
 * @file    cli.h
 * @brief   Common helpers for CLI modules usable with both shell and xshell.
 *
 * @details ChibiOS has two shells with incompatible command signatures:
 *          - classic shell (os/various/shell):      shellcmd_t =
 *              void (*)(BaseSequentialStream *chp, int argc, char *argv[])
 *            registered in a ShellCommand table (ShellConfig.sc_commands)
 *          - xshell (os/various/xshell):            xshellcmd_t =
 *              void (*)(xshell_t *xshp, int argc, char *argv[], char *envp[])
 *            registered in an xshell_command_t table
 *            (xshell_manager_config.commands)
 *
 *          To support both, every CLI command is implemented once as a
 *          "core" function with the classic shell signature:
 *
 *              void cli_cmd_xxx(BaseSequentialStream *chp, int argc, char *argv[]);
 *
 *          A core function can be registered directly in a ShellCommand
 *          table via CLI_SHELL_ENTRY(). For xshell, a thin adapter that
 *          forwards xshp->stream is generated with CLI_XSHELL_ADAPTER() and
 *          registered via CLI_XSHELL_ENTRY(). Both can be used at the same
 *          time (both shells active in different threads).
 *
 *          The xshell adapters are compiled only when xshell.h is on the
 *          include path (i.e. the project includes xshell.mk), detected
 *          with __has_include.
 *
 * @addtogroup CLI
 * @{
 */

#ifndef CLI_H
#define CLI_H

#include "hal.h"

/* Detect whether the classic shell / xshell are part of the build (their
   include directories are on the include path via shell.mk / xshell.mk). */
#if defined(__has_include)
#  if __has_include("shell.h")
#    define CLI_HAS_SHELL   1
#  endif
#  if __has_include("xshell.h")
#    define CLI_HAS_XSHELL  1
#  endif
#endif
#ifndef CLI_HAS_SHELL
#  define CLI_HAS_SHELL     0
#endif
#ifndef CLI_HAS_XSHELL
#  define CLI_HAS_XSHELL    0
#endif

#if CLI_HAS_XSHELL
#include "xshell.h"

/**
 * @brief   Generates an xshell adapter function for a core command.
 * @details Place in a .c file: CLI_XSHELL_ADAPTER(cli_cmd_clocks)
 *          generates cli_xshell_cmd_clocks(xshell_t*, int, char**, char**)
 *          which forwards to cli_cmd_clocks(xshp->stream, argc, argv).
 */
#define CLI_XSHELL_ADAPTER(corefn)                                        \
  void cli_xshell_##corefn(xshell_t *xshp, int argc, char *argv[],        \
                           char *envp[]) {                                \
    (void)envp;                                                           \
    corefn(xshp->stream, argc, argv);                                     \
  }

/**
 * @brief   xshell_command_t table entry for a core command, e.g.:
 *          static const xshell_command_t cmds[] = {
 *            CLI_XSHELL_ENTRY(cli_cmd_clocks, "clocks")
 *            {NULL, NULL}
 *          };
 */
#define CLI_XSHELL_ENTRY(corefn, name)  { name, cli_xshell_##corefn },
#else
#define CLI_XSHELL_ADAPTER(corefn)
#define CLI_XSHELL_ENTRY(corefn, name)
#endif /* CLI_HAS_XSHELL */

/**
 * @brief   ShellCommand table entry for a core command — the core function
 *          already has the shellcmd_t signature, no adapter needed, e.g.:
 *          static const ShellCommand cmds[] = {
 *            CLI_SHELL_ENTRY(cli_cmd_clocks, "clocks")
 *            {NULL, NULL}
 *          };
 */
#define CLI_SHELL_ENTRY(corefn, name)   { name, corefn },

/*===========================================================================*/
/* Merging command tables.                                                   */
/*===========================================================================*/

/**
 * @details Both shells resolve a command by searching a NULL-terminated
 *          table (ShellConfig.sc_commands / xshell_manager_config.commands).
 *          To add the commands of a cli module to an application's existing
 *          table without editing it, merge the two tables at run-time:
 *          @code
 *          static ShellCommand merged[24];
 *          scfg.sc_commands =
 *            cli_shell_merge(app_commands, cli_shell_clock_commands, merged);
 *          @endcode
 *          The output buffer must hold cli_shell_count(a) +
 *          cli_shell_count(b) + 1 entries; merge aborts (osalDbgCheck) on
 *          overflow.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if CLI_HAS_SHELL
#include "shell.h"
  size_t cli_shell_count(const ShellCommand *t);
  const ShellCommand *cli_shell_merge(const ShellCommand *a,
                                      const ShellCommand *b,
                                      ShellCommand *out);
#endif

#if CLI_HAS_XSHELL
  size_t cli_xshell_count(const xshell_command_t *t);
  const xshell_command_t *cli_xshell_merge(const xshell_command_t *a,
                                           const xshell_command_t *b,
                                           xshell_command_t *out);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CLI_H */

/** @} */
