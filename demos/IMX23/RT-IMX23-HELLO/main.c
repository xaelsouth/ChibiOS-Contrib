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
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "board.h"
#include "imx23_clock.h"
#include "imx23_audioout.h"

#if defined(BOARD_IMX23_EVK)
#include "imx23_lcdif.h"
#include "imx23_pxp.h"
#endif

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
 * LCD framebuffer (XRGB8888) and test pattern.
 */
static uint32_t lcd_fb[IMX23_LCD_WIDTH * IMX23_LCD_HEIGHT]
    __attribute__((aligned(32)));
static volatile uint32_t lcd_frames;

static void lcd_vsync_cb(void) {
  lcd_frames++;
}

static const imx23_lcdif_timing_t lcd_timing = {
  .width      = IMX23_LCD_WIDTH,
  .height     = IMX23_LCD_HEIGHT,
  .hsync_len  = IMX23_LCD_HSYNC,
  .hfp        = IMX23_LCD_HFP,
  .hbp        = IMX23_LCD_HBP,
  .vsync_len  = IMX23_LCD_VSYNC,
  .vfp        = IMX23_LCD_VFP,
  .vbp        = IMX23_LCD_VBP,
  .pixclk_hz  = IMX23_LCD_PIXCLK_HZ,
  .flags      = IMX23_LCD_FLAGS,
};

static void lcd_test_pattern(void) {
  uint32_t x, y;

  for (y = 0U; y < IMX23_LCD_HEIGHT; y++) {
    for (x = 0U; x < IMX23_LCD_WIDTH; x++) {
      uint32_t r = (x * 255U) / IMX23_LCD_WIDTH;
      uint32_t g = (y * 255U) / IMX23_LCD_HEIGHT;
      uint32_t b = ((x + y) * 255U) / (IMX23_LCD_WIDTH + IMX23_LCD_HEIGHT);
      lcd_fb[y * IMX23_LCD_WIDTH + x] = 0xFF000000U | (r << 16) | (g << 8) | b;
    }
  }
}

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

#if defined(BOARD_IMX23_EVK)
/*
 * "pxp" - blit the top half of the framebuffer onto the bottom half
 * through the PXP (exercises the mem2mem path used for LVGL flushes).
 */
static void cmd_pxp(BaseSequentialStream *chp, int argc, char *argv[]) {
  imx23_pxp_job_t job;
  msg_t msg;

  (void)argc; (void)argv;

  memset(&job, 0, sizeof(job));
  job.src     = &lcd_fb[0];
  job.src_w   = IMX23_LCD_WIDTH;
  job.src_h   = IMX23_LCD_HEIGHT / 2U;
  job.src_fmt = IMX23_PXP_S0_FMT_RGB888;
  job.dst     = &lcd_fb[(IMX23_LCD_HEIGHT / 2U) * IMX23_LCD_WIDTH];
  job.dst_w   = IMX23_LCD_WIDTH;
  job.dst_h   = IMX23_LCD_HEIGHT / 2U;
  job.dst_fmt = IMX23_PXP_OUT_FMT_RGB888;

  msg = imx23_pxp_run(&job, TIME_MS2I(1000));
  chprintf(chp, "pxp blit: %s\r\n", (msg == MSG_OK) ? "OK" : "FAILED");
  log_line("pxp blit %s", (msg == MSG_OK) ? "OK" : "FAILED");
}
#endif

/*
 * "tone [hz] [ms]" - play a sine wave on the mxs-builtin DAC (44100 Hz,
 * 16-bit stereo) through the APBX DMA channel.
 */
#define TONE_RATE_HZ    44100U
#define TONE_BUF_FRAMES 2048U

static int16_t tone_buf[TONE_BUF_FRAMES * 2U];

/* 256-entry sine table, amplitude 127. */
static const int8_t sine_tab[256] = {
   0,  3,  6,  9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46,
  49, 52, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88,
  90, 92, 94, 96, 98,100,102,104,106,108,109,111,113,114,116,117,
 119,120,121,122,124,125,126,127,127,127,126,125,124,122,121,120,
 119,117,116,114,113,111,109,108,106,104,102,100, 98, 96, 94, 92,
  90, 88, 85, 83, 81, 78, 76, 73, 71, 68, 65, 63, 60, 57, 54, 52,
  49, 46, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 12,  9,  6,  3,
   0, -3, -6, -9,-12,-16,-19,-22,-25,-28,-31,-34,-37,-40,-43,-46,
 -49,-52,-54,-57,-60,-63,-65,-68,-71,-73,-76,-78,-81,-83,-85,-88,
 -90,-92,-94,-96,-98,-100,-102,-104,-106,-108,-109,-111,-113,-114,
 -116,-117,-119,-120,-121,-122,-124,-125,-126,-127,-127,-127,-126,
 -125,-124,-122,-121,-120,-119,-117,-116,-114,-113,-111,-109,-108,
 -106,-104,-102,-100,-98,-96,-94,-92,-90,-88,-85,-83,-81,-78,-76,
 -73,-71,-68,-65,-63,-60,-57,-54,-52,-49,-46,-43,-40,-37,-34,-31,
 -28,-25,-22,-19,-16,-12, -9, -6, -3,  0,  3,  6,  9, 12, 16, 19
};
/* note: entries 128..255 mirror 127..0 with sign flip via (256 - i) trick
   handled below; simpler: use index (phase & 255) and negate when in
   second half of the table range. */

static void cmd_tone(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const imx23_audio_config_t audio_cfg = {
    .rate = TONE_RATE_HZ, .bits = 16U, .volume = 60U,
  };
  unsigned hz = (argc > 0) ? (unsigned)atoi(argv[0]) : 440U;
  unsigned ms = (argc > 1) ? (unsigned)atoi(argv[1]) : 500U;
  unsigned total = (TONE_RATE_HZ * ms) / 1000U;
  unsigned done = 0U;
  msg_t msg = MSG_OK;

  if (hz == 0U) hz = 440U;

  imx23_audio_init(&audio_cfg);

  while (done < total && msg == MSG_OK) {
    unsigned n = total - done;
    unsigned i;
    if (n > TONE_BUF_FRAMES) n = TONE_BUF_FRAMES;
    for (i = 0U; i < n; i++) {
      /* 256 steps per period */
      unsigned phase = ((i + done) * hz * 256U) / TONE_RATE_HZ;
      unsigned idx = phase & 0xFFU;
      int v = sine_tab[idx];
      int16_t smp = (int16_t)(v * 100);
      tone_buf[2U * i]      = smp;
      tone_buf[2U * i + 1U] = smp;
    }
    msg = imx23_audio_play(tone_buf, n * 4U, TIME_MS2I(2000));
    done += n;
  }

  imx23_audio_stop();
  chprintf(chp, "tone %u Hz %u ms: %s\r\n", hz, ms,
           (msg == MSG_OK) ? "done" : "FAILED");
  log_line("tone %u Hz %u ms %s", hz, ms, (msg == MSG_OK) ? "done" : "FAILED");
}

static const ShellCommand commands[] = {
  { "freq", cmd_freq },
  { "tone", cmd_tone },
#if defined(BOARD_IMX23_EVK)
  { "backlight", cmd_backlight },
  { "pxp", cmd_pxp },
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
  /* LCD: framebuffer + LCDIF + backlight PWM2 on GPIO_1_28. */
  lcd_test_pattern();
  imx23_pxp_init();
  imx23_lcdif_init(&lcd_timing, lcd_fb);
  imx23_lcdif_set_vsync_callback(lcd_vsync_cb);
  imx23_iomux_set_pad(1U, 28U, 0U, 0x4U);
  pwmStart(&PWMD1, &pwm_backlight_cfg);
  pwmEnableChannel(&PWMD1, BOARD_BACKLIGHT_PWM_CHANNEL,
                   BOARD_BACKLIGHT_PWM_PERIOD / 2U);   /* 50% */
  log_line("LCD up: %ux%u @ %lu Hz pixclk", IMX23_LCD_WIDTH, IMX23_LCD_HEIGHT,
           (unsigned long)IMX23_LCD_PIXCLK_HZ);
#endif

  /* xshell on the debug serial port. */
  shellInit();
  chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO + 1,
                    shellThread, (void *)&shell_cfg);

  /* Heartbeat with timestamped log. */
  while (true) {
    palTogglePad(BOARD_LED_PORT, BOARD_LED_PAD);
#if defined(BOARD_IMX23_EVK)
    log_line("heartbeat, CPU = %lu Hz, profile %u, LCD frames = %lu",
             (unsigned long)imx23_clock_cpu_hz(),
             imx23_clock_current_profile(),
             (unsigned long)lcd_frames);
#else
    log_line("heartbeat, CPU = %lu Hz, profile %u",
             (unsigned long)imx23_clock_cpu_hz(),
             imx23_clock_current_profile());
#endif
    chThdSleepMilliseconds(2000);
  }

  return 0;
}
