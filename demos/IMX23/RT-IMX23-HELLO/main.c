/*
    ChibiOS - Copyright (C) 2006-2026 Xael South.

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
#include "tlsf_bindings.h"

#if defined(BOARD_ENC28J60)
#include "hal_mac.h"
#endif

#if HAL_USE_USB
#include "usb_rndis.h"
#include "usb_net_lwip.h"
#include "telnetd.h"
#include "tftpd_lfs.h"
#include "httpsd.h"
#include "net_shell.h"
#include "sio_sio.h"
#include "lwip/ip4_addr.h"
#endif

#if defined(BOARD_IMX23_EVK)
#include "imx23_lcdif.h"
#include "imx23_pxp.h"
#include "imx23_lradc.h"
#include "imx23_lvgl.h"
#include "lvgl/lvgl.h"
#include "keypad.h"
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

/*
 * LVGL port object + partial draw buffer (RGB565, one full-width band of
 * 40 lines). The flush callback blits through the PXP into lcd_fb.
 */
static imx23_lvgl_t lvgl1;
static uint16_t lvgl_draw_buf[IMX23_LCD_WIDTH * 40U];

static void btn_event_cb(lv_event_t *e) {
  static unsigned count;
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);

  count++;
  lv_label_set_text_fmt(label, "clicked %u times", count);
}

static void lvgl_ui_init(void) {
  lv_obj_t *scr = lv_screen_active();
  lv_obj_t *label, *btn, *btn_label, *slider;

  lv_obj_set_style_bg_color(scr, lv_color_hex(0x003a57), 0);

  label = lv_label_create(scr);
  lv_label_set_text_fmt(label, "ChibiOS/RT on i.MX23 EVK\nLVGL %d.%d.%d",
                        LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
                        LVGL_VERSION_PATCH);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

  btn = lv_button_create(scr);
  lv_obj_set_size(btn, 160, 50);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, -10);
  btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "click me");
  lv_obj_center(btn_label);

  {
    lv_obj_t *cnt = lv_label_create(scr);
    lv_obj_align(cnt, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_color(cnt, lv_color_hex(0xFFD700), 0);
    lv_label_set_text(cnt, "clicked 0 times");
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, cnt);
  }

  slider = lv_slider_create(scr);
  lv_obj_set_width(slider, 200);
  lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_slider_set_value(slider, 60, LV_ANIM_OFF);
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

#if defined(BOARD_IMX23_EVK)
/* "adc <pad 0-7>" - single LRADC conversion; "adc touch" - raw touch x/y. */
static void cmd_adc(BaseSequentialStream *chp, int argc, char *argv[]) {

  imx23_lradc_init();
  if (argc >= 1U && strcmp(argv[0], "touch") == 0) {
    static const imx23_lradc_ts_t ts = {
      .x_sense = BOARD_TOUCH_X_SENSE,
      .y_sense = BOARD_TOUCH_Y_SENSE,
      .raw_min = BOARD_TOUCH_RAW_MIN,
      .raw_max = BOARD_TOUCH_RAW_MAX,
    };
    uint16_t xr, yr;
    int rc = imx23_lradc_touch_read(&ts, &xr, &yr);
    chprintf(chp, "touch: %s x_raw=%u y_raw=%u (detect=%d)\r\n",
             rc == 0 ? "pressed" : "released",
             (unsigned)xr, (unsigned)yr, imx23_lradc_touch_detect());
    return;
  }
  if (argc >= 1U) {
    unsigned pad = (unsigned)strtoul(argv[0], NULL, 0);
    if (pad > 7U) {
      chprintf(chp, "pad 0..7\r\n");
      return;
    }
    chprintf(chp, "lradc pad %u: %lu (0x%lx)\r\n", pad,
             (unsigned long)imx23_lradc_read((uint8_t)pad, TIME_MS2I(100)),
             (unsigned long)imx23_lradc_read((uint8_t)pad, TIME_MS2I(100)));
    return;
  }
  chprintf(chp, "usage: adc <pad 0-7> | adc touch\r\n");
}

/* "keypad" - scan the matrix keypad (rows GPIO, cols via LRADC). */
static uint32_t keypad_adc_cb(void *ctx, uint8_t ch, sysinterval_t to) {

  (void)ctx;
  return imx23_lradc_read(ch, to);
}

static void cmd_keypad(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const ioline_t rows[4] = BOARD_KEYPAD_ROWS;
  static const uint8_t cols[4] = BOARD_KEYPAD_COLS;
  static Keypad kp;
  char k;

  (void)argc; (void)argv;
  keypadInit(&kp, keypad_adc_cb, NULL, rows, 4U, cols, 4U,
             BOARD_KEYPAD_MAP, BOARD_KEYPAD_THRESHOLD);
  k = keypadScan(&kp);
  chprintf(chp, "keypad: %c\r\n", k != 0 ? k : '-');
}
#endif /* BOARD_IMX23_EVK */

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

#if defined(BOARD_ENC28J60)
/*
 * ENC28J60 on SSP0 (SPID1), CS = GPIO pad from board.h.
 */
static const SPIConfig enc_spi_cfg = {
  .ssport = GPIOB,                 /* BOARD_ENC28J60_CS_BANK -> GPIOB (bank1) */
  .sspad  = BOARD_ENC28J60_CS_PIN,
  .clock  = BOARD_ENC28J60_SPI_CLK_HZ,
  .mode   = 0U,                    /* SPI mode 0 */
};

static const ETHSPIConfig eth_spi_cfg = {
  .driver             = &SPID1,    /* SSP0 */
  .config             = &enc_spi_cfg,
  .reset_trc_delay_ms = 1U,
  .reset_tpl_delay_ms = 10U,
  .reset_line         = PAL_LINE(GPIOB, BOARD_ENC28J60_RESET_PIN),
#if ETH_USE_CALLBACKS == TRUE
  .irq_line           = PAL_LINE(GPIOC, BOARD_ENC28J60_IRQ_PIN),
#endif
};

static const ETHPhyConfig eth_phy_cfg = {
  .conf_by = ETH_PHY_CONF_BY_SW,
  .mode    = ETH_PHY_MODE_AUTO,
  .speed   = ETH_PHY_SPEED_100,
  .duplex  = ETH_PHY_DUPLEX_FULL,
};

MACDriver ETHD1 = {
  .spi_config = &eth_spi_cfg,
  .phy_config = &eth_phy_cfg,
};

static const uint8_t mac_addr[6] = { 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U };

static const MACConfig mac_cfg = {
  .mac_address = mac_addr,
};

static void cmd_mac(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argc; (void)argv;
  chprintf(chp, "ENC28J60 link: %s\r\n",
           mac_lld_poll_link_status(&ETHD1) ? "UP" : "DOWN");
}
#endif /* BOARD_ENC28J60 */

#include "imx23_dcp.h"

#if defined(BOARD_IMX23_EVK)
#include "imx23_gpmi_nand.h"
#include "nand_lfs.h"
#include "imx23_romboot.h"

static bool nand_ready;
static nand_lfs_t nand_lfs;
static imx23_romboot_t romboot;

/* LittleFS volume starts past the ROM boot area (NCB/LDLB/DBBT + SB). */
static uint32_t lfs_base_lblock(void) {

  return imx23_romboot_lfs_base(&IMX23NAND1, IMX23_ROMBOOT_RESERVE_BLOCKS);
}

/* Lazily initialize the GPMI NAND (the BBT scan takes a moment). */
static int nand_ensure(void) {

  if (!nand_ready) {
    int rc = imx23_nand_init(&IMX23NAND1, 0U);
    if (rc != IMX23_NAND_OK) {
      return rc;
    }
    nand_ready = true;
  }
  return IMX23_NAND_OK;
}

static void hexdump(BaseSequentialStream *chp, const uint8_t *p, uint32_t len) {
  uint32_t i;

  for (i = 0U; i < len; i += 16U) {
    uint32_t j;
    chprintf(chp, "%08lx: ", (unsigned long)(i));
    for (j = 0U; j < 16U; j++) {
      if (i + j < len) {
        chprintf(chp, "%02x ", p[i + j]);
      }
      else {
        chprintf(chp, "   ");
      }
    }
    chprintf(chp, "\r\n");
  }
}

/* bootwrite source: stream an lfs file into the SB writer. */
typedef struct { lfs_t *lfs; lfs_file_t *f; } bw_ctx_t;

static int bw_lfs_read(void *vctx, uint32_t off, uint8_t *buf, uint32_t len) {
  bw_ctx_t *c = (bw_ctx_t *)vctx;

  if (lfs_file_seek(c->lfs, c->f, off, LFS_SEEK_SET) < 0) {
    return -1;
  }
  return (lfs_file_read(c->lfs, c->f, buf, len) == (lfs_ssize_t)len) ? 0 : -1;
}

/* U-Boot-style: nand info | nand erase <block> |
   nand read <addr> <block> <page> [len] [raw] |
   nand write <addr> <block> <page> [len] [raw]                        */
static void cmd_nand(BaseSequentialStream *chp, int argc, char *argv[]) {
  const imx23_nand_geo_t *g;
  int rc;

  rc = nand_ensure();
  if (rc != IMX23_NAND_OK) {
    chprintf(chp, "nand init failed: %d\r\n", rc);
    return;
  }
  g = imx23_nand_geo(&IMX23NAND1);

  if (argc < 1U) {
    chprintf(chp, "usage: nand info|erase <blk>|read <addr> <blk> <page> [len] [raw]|write <addr> <blk> <page> [len] [raw]|bootwrite <lfs .sb file> [slot 0|1]|bootslot <0|1> <sectors>\r\n");
    return;
  }

  /* Re-point the LDLB at an existing boot slot (roll back / switch). */
  if (strcmp(argv[0], "bootslot") == 0 && argc >= 3U) {
    uint32_t slot = (uint32_t)strtoul(argv[1], NULL, 0);
    uint32_t sect = (uint32_t)strtoul(argv[2], NULL, 0);
    int rc2;

    imx23_romboot_init(&romboot, &IMX23NAND1);
    rc2 = imx23_romboot_set_boot_slot(&romboot, slot, sect);
    chprintf(chp, "bootslot %lu (%lu sectors): %s (%d)\r\n",
             (unsigned long)slot, (unsigned long)sect,
             rc2 == IMX23_NAND_OK ? "OK" : "FAILED", rc2);
    return;
  }

  /* Write an SB bootstream image from LittleFS into the ROM boot area:
     NCB (Hamming) at blocks 0-7, LDLB at 8-15, DBBT at 16-23, image into
     slot 0 (block 24) or slot 1 (block 40). The .sb is produced on the
     host by elftosb. */
  if (strcmp(argv[0], "bootwrite") == 0 && argc >= 2U) {
    bw_ctx_t ctx;
    lfs_file_t f;
    lfs_ssize_t sz;
    uint32_t slot = (argc >= 3U) ? (uint32_t)strtoul(argv[2], NULL, 0) : 0U;
    int rc2;

    if (!nand_lfs.mounted) {
      chprintf(chp, "lfs not mounted\r\n");
      return;
    }
    ctx.lfs = nand_lfs_handle(&nand_lfs);
    ctx.f   = &f;
    if (lfs_file_open(ctx.lfs, &f, argv[1], LFS_O_RDONLY) != LFS_ERR_OK) {
      chprintf(chp, "bootwrite: cannot open %s\r\n", argv[1]);
      return;
    }
    sz = lfs_file_size(ctx.lfs, &f);
    if (sz <= 0) {
      chprintf(chp, "bootwrite: %s is empty\r\n", argv[1]);
      lfs_file_close(ctx.lfs, &f);
      return;
    }
    imx23_romboot_init(&romboot, &IMX23NAND1);
    chprintf(chp, "bootwrite: writing %ld bytes of %s to slot %lu (NCB/LDLB/DBBT + image from block %lu)...\r\n",
             (long)sz, argv[1], (unsigned long)slot,
             (unsigned long)imx23_romboot_bootstream_block(g, slot));
    rc2 = imx23_romboot_write_sb(&romboot, bw_lfs_read, &ctx, (uint32_t)sz,
                                  slot);
    lfs_file_close(ctx.lfs, &f);
    chprintf(chp, "bootwrite: %s (%d)\r\n",
             rc2 == IMX23_NAND_OK ? "OK - reset to boot from NAND" : "FAILED",
             rc2);
    return;
  }

  if (strcmp(argv[0], "info") == 0) {
    uint32_t b, bad = 0U;
    chprintf(chp, "NAND: %s\r\n", g->name);
    chprintf(chp, "JEDEC ID: %02x %02x %02x %02x %02x\r\n",
             g->id[0], g->id[1], g->id[2], g->id[3], g->id[4]);
    chprintf(chp, "page %lu + oob %lu, block %lu (%lu pages), %lu blocks (%lu pages)\r\n",
             (unsigned long)g->page_size, (unsigned long)g->oob_size,
             (unsigned long)g->block_size, (unsigned long)g->pages_per_block,
             (unsigned long)g->blocks, (unsigned long)g->pages);
    chprintf(chp, "BCH: ecc %u bits/512B, %u chunks, layout0=0x%08lx layout1=0x%08lx\r\n",
             g->ecc_strength, g->ecc_chunks,
             (unsigned long)g->layout0, (unsigned long)g->layout1);
    for (b = 0U; b < g->blocks; b++) {
      if (imx23_nand_is_bad(&IMX23NAND1, b)) {
        bad++;
      }
    }
    chprintf(chp, "BBT: %lu bad of %lu blocks, %lu good (logical) blocks\r\n",
             (unsigned long)bad, (unsigned long)g->blocks,
             (unsigned long)g->good_blocks);
    for (b = 0U; b < g->blocks; b++) {
      if (imx23_nand_is_bad(&IMX23NAND1, b)) {
        chprintf(chp, "  bad block %lu\r\n", (unsigned long)b);
      }
    }
    return;
  }

  if (strcmp(argv[0], "erase") == 0 && argc >= 2U) {
    uint32_t blk = (uint32_t)strtoul(argv[1], NULL, 0);
    rc = imx23_nand_erase_block(&IMX23NAND1, blk);
    chprintf(chp, "erase block %lu: %s\r\n", (unsigned long)blk,
             rc == IMX23_NAND_OK ? "OK" : "FAILED");
    if (rc == IMX23_NAND_ERR_ERASE) {
      imx23_nand_mark_bad(&IMX23NAND1, blk);
      chprintf(chp, "block %lu marked bad in BBT\r\n", (unsigned long)blk);
    }
    return;
  }

  if ((strcmp(argv[0], "read") == 0 || strcmp(argv[0], "write") == 0) &&
      argc >= 4U) {
    int writing = (strcmp(argv[0], "write") == 0);
    uint8_t *addr = (uint8_t *)(uintptr_t)strtoul(argv[1], NULL, 0);
    uint32_t blk   = (uint32_t)strtoul(argv[2], NULL, 0);
    uint32_t page  = (uint32_t)strtoul(argv[3], NULL, 0);
    uint32_t len   = (argc >= 5U) ? (uint32_t)strtoul(argv[4], NULL, 0)
                                  : g->page_size;
    int raw = 0;
    uint32_t i, done = 0U;
    uint8_t tmp[IMX23_NAND_MAX_PAGE_SIZE];

    if (argc >= 6U && strcmp(argv[5], "raw") == 0) {
      raw = 1;
    }
    if (blk >= g->blocks || page >= g->pages_per_block ||
        len == 0U || len > 64U * 1024U) {
      chprintf(chp, "bad args\r\n");
      return;
    }
    while (done < len) {
      uint32_t pp = blk * g->pages_per_block + page + (done / g->page_size);
      uint32_t chunk = g->page_size;
      if (chunk > len - done) {
        chunk = len - done;
      }
      if (writing) {
        memcpy(tmp, addr + done, chunk);
        rc = raw ? imx23_nand_write_page_raw(&IMX23NAND1, pp, tmp, NULL)
                 : imx23_nand_write_page(&IMX23NAND1, pp, tmp, NULL);
      }
      else {
        rc = raw ? imx23_nand_read_page_raw(&IMX23NAND1, pp, tmp, NULL)
                 : imx23_nand_read_page(&IMX23NAND1, pp, tmp, NULL, NULL);
        if (rc == IMX23_NAND_OK) {
          memcpy(addr + done, tmp, chunk);
        }
      }
      if (rc != IMX23_NAND_OK) {
        chprintf(chp, "%s page %lu failed: %d\r\n",
                 writing ? "write" : "read", (unsigned long)pp, rc);
        return;
      }
      done += chunk;
    }
    chprintf(chp, "%s %lu bytes %s block %lu page %lu%s: OK\r\n",
             writing ? "wrote" : "read", (unsigned long)len,
             writing ? "to" : "from", (unsigned long)blk, (unsigned long)page,
             raw ? " (raw)" : "");
    if (!writing) {
      hexdump(chp, addr, len > 64U ? 64U : len);
    }
    return;
  }

  chprintf(chp, "usage: nand info|erase <blk>|read <addr> <blk> <page> [len] [raw]|write <addr> <blk> <page> [len] [raw]\r\n");
}

/* lfs mount [format] | lfs ls [path] | lfs cat <path> | lfs write <path> <text> |
   lfs rm <path> | lfs unmount                                             */
static void cmd_lfs(BaseSequentialStream *chp, int argc, char *argv[]) {
  lfs_t *lfs;
  int rc;

  if (argc < 1U) {
    chprintf(chp, "usage: lfs mount [format]|ls [path]|cat <path>|write <path> <text>|rm <path>|unmount\r\n");
    return;
  }

  if (strcmp(argv[0], "mount") == 0) {
    bool format = (argc >= 2U && strcmp(argv[1], "format") == 0);
    rc = nand_ensure();
    if (rc != IMX23_NAND_OK) {
      chprintf(chp, "nand init failed: %d\r\n", rc);
      return;
    }
    rc = nand_lfs_mount(&nand_lfs, &IMX23NAND1, lfs_base_lblock(), format);
    chprintf(chp, "lfs mount: %s (%d)\r\n",
             rc == LFS_ERR_OK ? "OK" : "FAILED", rc);
    return;
  }

  if (!nand_lfs.mounted) {
    chprintf(chp, "lfs not mounted\r\n");
    return;
  }
  lfs = nand_lfs_handle(&nand_lfs);

  if (strcmp(argv[0], "ls") == 0) {
    const char *path = (argc >= 2U) ? argv[1] : "/";
    lfs_dir_t dir;
    struct lfs_info info;
    rc = lfs_dir_open(lfs, &dir, path);
    if (rc != LFS_ERR_OK) {
      chprintf(chp, "ls %s: %d\r\n", path, rc);
      return;
    }
    chprintf(chp, "%s:\r\n", path);
    while ((rc = lfs_dir_read(lfs, &dir, &info)) > 0) {
      chprintf(chp, "  %s %s %lu bytes\r\n",
               info.type == LFS_TYPE_DIR ? "dir " : "file",
               info.name, (unsigned long)info.size);
    }
    lfs_dir_close(lfs, &dir);
    return;
  }

  if (strcmp(argv[0], "cat") == 0 && argc >= 2U) {
    lfs_file_t f;
    uint8_t buf[128];
    lfs_ssize_t n;
    rc = lfs_file_open(lfs, &f, argv[1], LFS_O_RDONLY);
    if (rc != LFS_ERR_OK) {
      chprintf(chp, "cat %s: %d\r\n", argv[1], rc);
      return;
    }
    while ((n = lfs_file_read(lfs, &f, buf, sizeof(buf))) > 0) {
      streamWrite(chp, buf, (size_t)n);
    }
    chprintf(chp, "\r\n");
    lfs_file_close(lfs, &f);
    return;
  }

  if (strcmp(argv[0], "write") == 0 && argc >= 3U) {
    lfs_file_t f;
    rc = lfs_file_open(lfs, &f, argv[1], LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (rc != LFS_ERR_OK) {
      chprintf(chp, "write %s: %d\r\n", argv[1], rc);
      return;
    }
    {
      int i;
      for (i = 2; i < argc; i++) {
        lfs_file_write(lfs, &f, argv[i], (lfs_size_t)strlen(argv[i]));
        if (i + 1 < argc) {
          lfs_file_write(lfs, &f, " ", 1U);
        }
      }
      lfs_file_write(lfs, &f, "\r\n", 2U);
    }
    lfs_file_close(lfs, &f);
    chprintf(chp, "wrote %s\r\n", argv[1]);
    return;
  }

  if (strcmp(argv[0], "rm") == 0 && argc >= 2U) {
    rc = lfs_remove(lfs, argv[1]);
    chprintf(chp, "rm %s: %d\r\n", argv[1], rc);
    return;
  }

  if (strcmp(argv[0], "unmount") == 0) {
    rc = nand_lfs_unmount(&nand_lfs);
    chprintf(chp, "lfs unmount: %d\r\n", rc);
    return;
  }

  chprintf(chp, "usage: lfs mount [format]|ls [path]|cat <path>|write <path> <text>|rm <path>|unmount\r\n");
}
#endif /* BOARD_IMX23_EVK */

/*
 * "dcp ..." - DCP hardware crypto: info | sha256 <text> | aes selftest |
 *             unique roundtrip | reencrypt (zero-key -> device-unique key).
 */
static void cmd_dcp(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const uint8_t fips_key[16] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
  };
  static const uint8_t fips_pt[16] = {
    0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
    0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff
  };
  static const uint8_t fips_ct[16] = {
    0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,
    0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a
  };
  /* SHA-256("abc") */
  static const uint8_t sha_abc[32] = {
    0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,
    0x22,0x23,0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,
    0xf2,0x00,0x15,0xad
  };
  uint8_t buf[64U];
  int rc;
  unsigned i;

  if (imx23_dcp_init(&IMX23DCP1) != IMX23_DCP_OK) {
    chprintf(chp, "dcp init failed\r\n");
    return;
  }

  if (argc < 1U) {
    chprintf(chp, "usage: dcp info|sha256 <text>|aes|aes-enc <hexkey> <hexdata>|unique|reencrypt\r\n");
    return;
  }

  if (strcmp(argv[0], "info") == 0) {
    chprintf(chp, "DCP capability1 = 0x%08lx (AES128=%lu SHA1=%lu SHA256=%lu)\r\n",
             (unsigned long)IMX23_REG(IMX23_DCP_CAPABILITY1),
             (unsigned long)(IMX23_REG(IMX23_DCP_CAPABILITY1) & 1U),
             (unsigned long)((IMX23_REG(IMX23_DCP_CAPABILITY1) >> 16) & 1U),
             (unsigned long)((IMX23_REG(IMX23_DCP_CAPABILITY1) >> 16) & 4U));
    return;
  }

  if (strcmp(argv[0], "sha256") == 0) {
    const char *msg = (argc >= 2U) ? argv[1] : "abc";
    rc = imx23_dcp_sha256(&IMX23DCP1, (const uint8_t *)msg, strlen(msg), buf);
    chprintf(chp, "sha256(\"%s\") rc=%d: ", msg, rc);
    for (i = 0U; i < 32U; i++) {
      chprintf(chp, "%02x", buf[i]);
    }
    chprintf(chp, "\r\n%s\r\n",
             (memcmp(buf, sha_abc, 32U) == 0 && strcmp(msg, "abc") == 0)
             ? "SELFTEST OK" : "");
    return;
  }

  if (strcmp(argv[0], "aes") == 0) {
    uint8_t ct[16], pt[16];
    rc = imx23_dcp_aes128(&IMX23DCP1, fips_pt, ct, 16U, fips_key, 0U,
                          true, false, NULL);
    if (rc != IMX23_DCP_OK || memcmp(ct, fips_ct, 16U) != 0U) {
      chprintf(chp, "AES-128-ECB encrypt FAILED rc=%d\r\n", rc);
      return;
    }
    rc = imx23_dcp_aes128(&IMX23DCP1, ct, pt, 16U, fips_key, 0U,
                          false, false, NULL);
    chprintf(chp, "AES-128-ECB %s\r\n",
             (rc == IMX23_DCP_OK && memcmp(pt, fips_pt, 16U) == 0U)
             ? "SELFTEST OK" : "FAILED");
    return;
  }

  if (strcmp(argv[0], "unique") == 0) {
    uint8_t ct[16], pt[16];
    memcpy(buf, fips_pt, 16U);
    rc = imx23_dcp_aes128(&IMX23DCP1, buf, ct, 16U, NULL,
                          IMX23_DCP_KEY_UNIQUE, true, false, NULL);
    if (rc != IMX23_DCP_OK) {
      chprintf(chp, "unique-key encrypt FAILED rc=%d\r\n", rc);
      return;
    }
    rc = imx23_dcp_aes128(&IMX23DCP1, ct, pt, 16U, NULL,
                          IMX23_DCP_KEY_UNIQUE, false, false, NULL);
    chprintf(chp, "device-unique-key roundtrip %s\r\n",
             (rc == IMX23_DCP_OK && memcmp(pt, buf, 16U) == 0U)
             ? "OK" : "FAILED");
    return;
  }

  if (strcmp(argv[0], "reencrypt") == 0) {
    /* Decrypt with the all-zero distribution key, re-encrypt with the
       device-unique DCP key (CBC, zero IV). */
    static const uint8_t zero_key[16] = { 0 };
    static const uint8_t iv[16] = { 0 };
    uint8_t ct[16], pt[16];
    memcpy(ct, fips_pt, 16U);   /* pretend this is the zero-key ciphertext */
    rc = imx23_dcp_aes128(&IMX23DCP1, ct, pt, 16U, zero_key, 0U,
                          false, true, iv);
    if (rc != IMX23_DCP_OK) {
      chprintf(chp, "zero-key decrypt FAILED rc=%d\r\n", rc);
      return;
    }
    rc = imx23_dcp_aes128(&IMX23DCP1, pt, ct, 16U, NULL,
                          IMX23_DCP_KEY_UNIQUE, true, true, iv);
    chprintf(chp, "reencrypt zero->unique %s (rc=%d)\r\n",
             rc == IMX23_DCP_OK ? "OK" : "FAILED", rc);
    return;
  }

  chprintf(chp, "unknown dcp command\r\n");
}

static const ShellCommand commands[] = {
  { "freq", cmd_freq },
  { "tone", cmd_tone },
#if defined(BOARD_ENC28J60)
  { "mac", cmd_mac },
#endif
#if defined(BOARD_IMX23_EVK)
  { "backlight", cmd_backlight },
  { "pxp", cmd_pxp },
  { "nand", cmd_nand },
  { "lfs", cmd_lfs },
  { "adc", cmd_adc },
  { "keypad", cmd_keypad },
#endif
  { "dcp", cmd_dcp },
#if HAL_USE_USB
  /* lwIP network commands (also reachable over telnetd). */
  { "ifconfig", net_shell_cmd_ifconfig },
  { "dhcpc",    net_shell_cmd_dhcpc },
  { "ping",     net_shell_cmd_ping },
  { "arp",      net_shell_cmd_arp },
  { "ntp",      net_shell_cmd_ntp },
  { "slip",     net_shell_cmd_slip },
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
#if HAL_USE_USB
/* USB RNDIS gadget + lwIP netif (static 192.168.7.2/24). */
static usb_rndis_t    rndis1;
static usb_net_lwip_t usbnet1;
static const uint8_t  usb_mac[6] = { 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U };

static void usb_net_start(void) {
  ip4_addr_t ip, nm, gw;

  ip4addr_aton("192.168.7.2", &ip);
  ip4addr_aton("255.255.255.0", &nm);
  ip4addr_aton("192.168.7.1", &gw);

  usbObjectInit(&USBD1);
  usb_net_lwip_init(&usbnet1, &rndis1, &USBD1, usb_mac, &ip, &nm, &gw);
  log_line("USB RNDIS gadget started, lwIP netif 192.168.7.2");

  /* Register the application UARTs as lwIP sio devices (for "slip"). */
#if defined(IMX23_SIO_USE_AUART0) && (IMX23_SIO_USE_AUART0 == TRUE)
  sio_sio_register(1U, &SIOD2);
#endif
#if defined(IMX23_SIO_USE_AUART1) && (IMX23_SIO_USE_AUART1 == TRUE)
  sio_sio_register(2U, &SIOD3);
#endif
}
#endif

int main(void) {

  halInit();
  chSysInit();
  tlsfHeapInit();

  /* Console / debug serial. */
  sioStart(&SIOD1, &sio_console);
  log_line("ChibiOS/RT on i.MX23 (%s) ready, CPU = %lu Hz",
           BOARD_NAME, (unsigned long)imx23_clock_cpu_hz());

  /* LED. */
  palSetPadMode(BOARD_LED_PORT, BOARD_LED_PAD, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(BOARD_LED_PORT, BOARD_LED_PAD);

#if defined(BOARD_IMX23_EVK)
  /* LCD: LVGL port over LCDIF + PXP, backlight PWM2 on GPIO_1_28. */
  imx23_lvgl_init(&lvgl1, &lcd_timing, lcd_fb, lvgl_draw_buf,
                  IMX23_LCD_WIDTH * 40U, true);
  imx23_lcdif_set_vsync_callback(lcd_vsync_cb);
  imx23_iomux_set_pad(1U, 28U, 0U, 0x4U);
  pwmStart(&PWMD1, &pwm_backlight_cfg);
  pwmEnableChannel(&PWMD1, BOARD_BACKLIGHT_PWM_CHANNEL,
                   BOARD_BACKLIGHT_PWM_PERIOD / 2U);   /* 50% */
  log_line("LCD up: %ux%u @ %lu Hz pixclk", IMX23_LCD_WIDTH, IMX23_LCD_HEIGHT,
           (unsigned long)IMX23_LCD_PIXCLK_HZ);
  lvgl_ui_init();
  imx23_lvgl_start(&lvgl1);
  log_line("LVGL %d.%d.%d started, flushes via PXP",
           LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  {
    static const imx23_lradc_ts_t ts = {
      .x_sense = BOARD_TOUCH_X_SENSE,
      .y_sense = BOARD_TOUCH_Y_SENSE,
      .raw_min = BOARD_TOUCH_RAW_MIN,
      .raw_max = BOARD_TOUCH_RAW_MAX,
    };
    imx23_lvgl_add_touch(&lvgl1, &ts);
    log_line("LVGL touch indev on LRADC (x_sense=%u y_sense=%u)",
             BOARD_TOUCH_X_SENSE, BOARD_TOUCH_Y_SENSE);
  }
#endif

#if defined(BOARD_ENC28J60)
  /* ENC28J60 on SSP0: pinmux SSP0 pads (bank2 pins 0,2-6 func0) + CS gpio. */
  {
    static const struct { uint32_t bank, pin, func; } ssp0_pins[] = {
      { 2U, 0U, 0U }, { 2U, 2U, 0U }, { 2U, 3U, 0U }, { 2U, 4U, 0U },
      { 2U, 5U, 0U }, { 2U, 6U, 0U },
    };
    unsigned i;
    for (i = 0U; i < sizeof(ssp0_pins) / sizeof(ssp0_pins[0]); i++) {
      imx23_iomux_set_pad(ssp0_pins[i].bank, ssp0_pins[i].pin,
                          ssp0_pins[i].func, 0x4U);
    }
    palSetPadMode(GPIOB, BOARD_ENC28J60_CS_PIN, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPad(GPIOB, BOARD_ENC28J60_CS_PIN);   /* CS idle high */
  }
  spiStart(&SPID1, &enc_spi_cfg);
  macStart(&ETHD1, &mac_cfg);
  log_line("ENC28J60 started on SSP0");
#endif

#if HAL_USE_USB
  /* USB RNDIS + lwIP (telnetd/TFTP/HTTP live on top of this netif). */
  usb_net_start();
#endif

  /* xshell on the debug serial port. */
  shellInit();
  chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO + 1,
                    shellThread, (void *)&shell_cfg);

#if HAL_USE_USB
  /* telnetd: same xshell command table over TCP port 23 (via RNDIS). */
  telnetdStart(commands);
#endif

#if defined(BOARD_IMX23_EVK)
  /* Mount NAND LittleFS at startup and expose it over TFTP (UDP 69). */
  if (nand_ensure() == IMX23_NAND_OK && !nand_lfs.mounted) {
    int rc = nand_lfs_mount(&nand_lfs, &IMX23NAND1, lfs_base_lblock(), false);
    if (rc == LFS_ERR_OK) {
      log_line("NAND LittleFS mounted");
#if HAL_USE_USB
      tftpd_lfs_start(nand_lfs_handle(&nand_lfs));
      httpsd_start(nand_lfs_handle(&nand_lfs));
#endif
    }
    else {
      log_line("lfs mount failed: %d (try 'lfs mount format')", rc);
    }
  }
#endif

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
