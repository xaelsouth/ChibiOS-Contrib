/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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

#include <stdarg.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "board.h"
#include "imx23_clock.h"

/*
 * Console + xshell on the PL011 DUART (pins configured in board.c).
 */
static const SIOConfig sio_console = {
  .baud     = 115200U,
  .framecfg = IMX23_SIO_LLD_8N1,
};

/*
 * Timestamped log over the debug serial port.
 * ST runs at 32768 Hz, so convert ticks to milliseconds.
 */
static void log_line(const char *fmt, ...) {
  va_list ap;
  systime_t t = chVTGetSystemTimeX();
  uint32_t ms = (uint32_t)(((uint64_t)t * 1000U) / CH_CFG_ST_FREQUENCY);

  chprintf((BaseSequentialStream *)&SIOD1, "[%lu.%03lu] ",
           (unsigned long)(ms / 1000U), (unsigned long)(ms % 1000U));
  va_start(ap, fmt);
  chvprintf((BaseSequentialStream *)&SIOD1, fmt, ap);
  va_end(ap);
  chprintf((BaseSequentialStream *)&SIOD1, "\r\n");
}

/*
 * "freq [0|1|2]" - switch CPU clock profile (480/240/120 MHz).
 */
static void cmd_freq(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc == 0) {
    unsigned i;
    chprintf(chp, "current profile: %u, CPU = %lu Hz\r\n",
             imx23_clock_current_profile(),
             (unsigned long)imx23_clock_cpu_hz());
    for (i = 0U; i < IMX23_CLOCK_PROFILE_COUNT; i++) {
      chprintf(chp, "  %u: CPU=%lu Hz HBUS=%lu Hz VDDD=%u mV\r\n", i,
               (unsigned long)(IMX23_PLL0_HZ / imx23_clock_profiles[i].div_cpu),
               (unsigned long)(IMX23_PLL0_HZ / imx23_clock_profiles[i].div_hbus),
               (unsigned)imx23_clock_profiles[i].vddd_mV);
    }
    return;
  }

  unsigned p = (unsigned)atoi(argv[0]);
  uint32_t hz = imx23_clock_set_profile(p);
  log_line("freq -> profile %u, CPU = %lu Hz", p, (unsigned long)hz);
}

#if defined(BOARD_IMX23_EVK)
/*
 * LCD backlight on PWM channel 2 (pad GPIO_1_28, function 0).
 * "backlight [0-100]" - duty cycle in percent.
 */
#define BOARD_BACKLIGHT_PWM_CHANNEL   2U
#define BOARD_BACKLIGHT_PWM_FREQ_HZ   1000000U   /* timer clock */
#define BOARD_BACKLIGHT_PWM_PERIOD    100U      /* -> 10 kHz PWM   */

static const PWMConfig pwm_backlight_cfg = {
  .frequency = BOARD_BACKLIGHT_PWM_FREQ_HZ,
  .period    = BOARD_BACKLIGHT_PWM_PERIOD,
  .callback  = NULL,
  .channels  = {
    [0 ... IMX23_PWM_CHANNELS - 1] = { PWM_OUTPUT_DISABLED, NULL },
    [BOARD_BACKLIGHT_PWM_CHANNEL]  = { PWM_OUTPUT_ACTIVE_HIGH, NULL },
  },
};

static void cmd_backlight(BaseSequentialStream *chp, int argc, char *argv[]) {
  unsigned pct;

  if (argc == 0) {
    chprintf(chp, "usage: backlight <0-100>\r\n");
    return;
  }
  pct = (unsigned)atoi(argv[0]);
  if (pct > 100U) {
    pct = 100U;
  }
  pwmEnableChannel(&PWMD1, BOARD_BACKLIGHT_PWM_CHANNEL,
                   (pwmcnt_t)((BOARD_BACKLIGHT_PWM_PERIOD * pct) / 100U));
  log_line("backlight -> %u%%", pct);
}
#endif /* BOARD_IMX23_EVK */

static const ShellCommand commands[] = {
  { "freq", cmd_freq },
#if defined(BOARD_IMX23_EVK)
  { "backlight", cmd_backlight },
#endif
  { NULL, NULL }
};

static const ShellConfig shell_cfg = {
  .sc_channel  = (BaseSequentialStream *)&SIOD1,
  .sc_commands = commands,
};

static THD_WORKING_AREA(waShell, 1024);

/*
 * Application entry point.
 */
int main(void) {

  halInit();
  chSysInit();

  /* Console / debug serial. */
  sioStart(&SIOD1, &sio_console);
  log_line("ChibiOS/RT on i.MX23 (%s) ready, CPU = %lu Hz",
           BOARD_NAME, (unsigned long)imx23_clock_cpu_hz());

  /* LED. */
  palSetPadMode(BOARD_LED_PORT, BOARD_LED_PAD, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(BOARD_LED_PORT, BOARD_LED_PAD);

#if defined(BOARD_IMX23_EVK)
  /* LCD backlight PWM2 on GPIO_1_28. */
  imx23_iomux_set_pad(1U, 28U, 0U, 0x4U);
  pwmStart(&PWMD1, &pwm_backlight_cfg);
  pwmEnableChannel(&PWMD1, BOARD_BACKLIGHT_PWM_CHANNEL,
                   BOARD_BACKLIGHT_PWM_PERIOD / 2U);   /* 50% */
#endif

  /* xshell on the debug serial port. */
  shellInit();
  chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO + 1,
                    shellThread, (void *)&shell_cfg);

  /* Heartbeat with timestamped log. */
  while (true) {
    palTogglePad(BOARD_LED_PORT, BOARD_LED_PAD);
    log_line("heartbeat, CPU = %lu Hz, profile %u",
             (unsigned long)imx23_clock_cpu_hz(),
             imx23_clock_current_profile());
    chThdSleepMilliseconds(2000);
  }

  return 0;
}
