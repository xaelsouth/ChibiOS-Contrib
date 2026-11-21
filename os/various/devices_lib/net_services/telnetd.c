/*
    ChibiOS - Copyright (C) 2026 Xael South.

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
 * @file    devices_lib/net_services/telnetd.c
 * @brief   Minimal telnet server feeding an xshell session.
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#include "hal.h"
#include "telnetd.h"

#include <string.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Telnet protocol bytes. */
#define IAC                     255U
#define DONT                    254U
#define DO                      253U
#define WONT                    252U
#define WILL                    251U
#define SB                      250U
#define SE                      240U

#define OPT_ECHO                1U
#define OPT_SGA                 3U    /* suppress go ahead              */
#define OPT_LINEMODE            34U

#define TELNETD_STACKSIZE       2048U

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/* BaseSequentialStream over a connected socket. */
typedef struct {
  BaseSequentialStream  stream;       /* must be first                    */
  int                   fd;
} telnet_stream_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static const ShellCommand *telnetd_commands;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* Read exactly one byte; returns < 0 on EOF/error. */
static int sock_read1(int fd) {
  uint8_t b;
  int n = lwip_read(fd, &b, 1);

  return (n == 1) ? (int)b : -1;
}

/* Read one byte, stripping telnet IAC sequences. Returns < 0 on EOF. */
static int telnet_read1(int fd) {
  int c = sock_read1(fd);

  if (c == (int)IAC) {
    int cmd = sock_read1(fd);
    if (cmd == (int)IAC) {
      return (int)IAC;              /* escaped 0xFF data byte           */
    }
    if (cmd == (int)SB) {
      /* Sub-negotiation: skip until IAC SE. */
      int prev = -1;
      while (true) {
        c = sock_read1(fd);
        if (c < 0) {
          return -1;
        }
        if (prev == (int)IAC && c == (int)SE) {
          break;
        }
        prev = c;
      }
    }
    else if (cmd == (int)WILL || cmd == (int)WONT ||
             cmd == (int)DO || cmd == (int)DONT) {
      (void)sock_read1(fd);         /* skip the option byte             */
    }
    /* Any other 2-byte command: nothing more to skip. */
    return telnet_read1(fd);        /* next real data byte              */
  }
  return c;
}

/* BaseSequentialStream VMT over the telnet connection. */
static size_t ts_write(void *instance, const uint8_t *bp, size_t n) {
  telnet_stream_t *tsp = (telnet_stream_t *)instance;
  size_t done = 0U;

  while (done < n) {
    int w = lwip_write(tsp->fd, bp + done, (int)(n - done));
    if (w <= 0) {
      break;
    }
    done += (size_t)w;
  }
  return done;
}

static size_t ts_read(void *instance, uint8_t *bp, size_t n) {
  telnet_stream_t *tsp = (telnet_stream_t *)instance;
  size_t done = 0U;

  while (done < n) {
    int c = telnet_read1(tsp->fd);
    if (c < 0) {
      break;
    }
    bp[done++] = (uint8_t)c;
  }
  return done;
}

static msg_t ts_put(void *instance, uint8_t b) {

  return (ts_write(instance, &b, 1U) == 1U) ? (msg_t)b : MSG_RESET;
}

static msg_t ts_get(void *instance) {
  int c = telnet_read1(((telnet_stream_t *)instance)->fd);

  return (c < 0) ? MSG_RESET : (msg_t)c;
}

static const struct BaseSequentialStreamVMT telnet_stream_vmt = {
  .write = ts_write,
  .read  = ts_read,
  .put   = ts_put,
  .get   = ts_get
};

/* Telnet daemon thread: accept one client at a time, run xshell on it. */
static THD_WORKING_AREA(telnetd_wa, TELNETD_STACKSIZE);
static THD_FUNCTION(telnetd_thread, arg) {
  const ShellCommand *commands = (const ShellCommand *)arg;
  int lfd;

  chRegSetThreadName("telnetd");

  lfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (lfd >= 0) {
    int one = 1;
    struct sockaddr_in addr;

    lwip_setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(TELNETD_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (lwip_bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        lwip_listen(lfd, 1) < 0) {
      lwip_close(lfd);
      lfd = -1;
    }
  }
  if (lfd < 0) {
    return;     /* cannot listen: give up */
  }

  while (!chThdShouldTerminateX()) {
    int cfd = lwip_accept(lfd, NULL, NULL);
    if (cfd < 0) {
      chThdSleepMilliseconds(100);
      continue;
    }

    /* Negotiate: we echo, we suppress go-ahead, no linemode. */
    {
      static const uint8_t neg[] = {
        IAC, WILL, OPT_ECHO,
        IAC, WILL, OPT_SGA,
        IAC, DO,   OPT_SGA,
        IAC, WONT, OPT_LINEMODE,
        IAC, DONT, OPT_LINEMODE
      };
      (void)lwip_write(cfd, neg, sizeof(neg));
    }

    /* Run the shell on this connection until the client goes away. */
    {
      telnet_stream_t ts;
      ShellConfig cfg;

      ts.stream.vmt = &telnet_stream_vmt;
      ts.fd         = cfd;

      cfg.sc_channel  = &ts.stream;
      cfg.sc_commands = commands;
#if SHELL_USE_HISTORY == TRUE
      cfg.sc_histbuf  = NULL;
      cfg.sc_histsize = 0;
#endif
#if SHELL_USE_COMPLETION == TRUE
      cfg.sc_completion = NULL;
#endif
      shellThread(&cfg);      /* returns when the stream resets         */
    }

    lwip_close(cfd);
  }

  lwip_close(lfd);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void telnetdStart(const ShellCommand *commands) {

  telnetd_commands = commands;
  chThdCreateStatic(telnetd_wa, sizeof(telnetd_wa),
                    NORMALPRIO - 2U, telnetd_thread, (void *)commands);
}

/** @} */
