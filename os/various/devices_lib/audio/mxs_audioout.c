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
 * @file    IMX23/IMX23xx/mxs_audioout.c
 * @brief   i.MX23 mxs-builtin audio DAC playback (ported from Linux
 *          sound/soc/mxs + sound/soc/codecs/mxs-builtin-codec.c).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#if defined(__has_include)
#if __has_include("imx23_dma.h")
#include "imx23_dma.h"   /* APBX DMA engine (port-level) */
#endif
#endif
#include "mxs_audioout.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define MXS_AOUT_REG32(addr)   (*(volatile uint32_t *)(addr))

/* DACSRR values from the i.MX23 RM table 29-1 (BASEMULT, SRC_HOLD, SRC_INT,
   SRC_FRAC) for the common sample rates. */
typedef struct {
  uint32_t rate;
  uint8_t  basemult;
  uint8_t  src_hold;
  uint8_t  src_int;
  uint16_t src_frac;
} mxs_dacsr_t;

static const mxs_dacsr_t dacsr_table[] = {
  { 192000U, 4U, 0U, 0x0FU, 0x13FFU },
  { 176400U, 4U, 0U, 0x11U, 0x0037U },
  { 128000U, 4U, 0U, 0x17U, 0x0E00U },
  {  96000U, 2U, 0U, 0x0FU, 0x13FFU },
  {  88200U, 2U, 0U, 0x11U, 0x0037U },
  {  64000U, 2U, 0U, 0x17U, 0x0E00U },
  {  48000U, 1U, 0U, 0x0FU, 0x13FFU },
  {  44100U, 1U, 0U, 0x11U, 0x0037U },
  {  32000U, 1U, 0U, 0x17U, 0x0E00U },
  {  24000U, 1U, 1U, 0x0FU, 0x13FFU },
  {  22050U, 1U, 1U, 0x11U, 0x0037U },
  {  16000U, 1U, 1U, 0x17U, 0x0E00U },
  {  12000U, 1U, 3U, 0x0FU, 0x13FFU },
  {  11025U, 1U, 3U, 0x11U, 0x0037U },
  {   8000U, 1U, 3U, 0x17U, 0x0E00U },
};

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static mxs_audio_config_t audio_cfg;
static imx23_dma_ccw_t audio_ccw;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static uint32_t dacsr_for_rate(uint32_t rate) {
  unsigned i;

  for (i = 0U; i < sizeof(dacsr_table) / sizeof(dacsr_table[0]); i++) {
    if (dacsr_table[i].rate == rate) {
      return MXS_AOUT_DACSRR_BASEMULT(dacsr_table[i].basemult) |
             MXS_AOUT_DACSRR_SRC_HOLD(dacsr_table[i].src_hold) |
             MXS_AOUT_DACSRR_SRC_INT(dacsr_table[i].src_int) |
             MXS_AOUT_DACSRR_SRC_FRAC(dacsr_table[i].src_frac);
    }
  }
  /* Not in the table: fall back to 44100. */
  return MXS_AOUT_DACSRR_BASEMULT(1U) | MXS_AOUT_DACSRR_SRC_HOLD(0U) |
         MXS_AOUT_DACSRR_SRC_INT(0x11U) | MXS_AOUT_DACSRR_SRC_FRAC(0x0037U);
}

/* Digital volume: 0..100 -> 0xFF (mute, -100dB) .. 0x00 (0dB), 0.5dB steps. */
static uint8_t vol_to_dac(uint8_t vol) {
  uint32_t v;

  if (vol > 100U) {
    vol = 100U;
  }
  v = 200U - (uint32_t)vol * 2U;    /* 0..100 -> 200..0 */
  if (v > 0xFFU) {
    v = 0xFFU;
  }
  return (uint8_t)v;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void mxs_audio_config(const mxs_audio_config_t *cfg) {
  uint8_t dv;

  audio_cfg = *cfg;

  /* Sample rate + word length. */
  MXS_AOUT_REG32(MXS_AOUT_DACSRR) = dacsr_for_rate(cfg->rate);
  if (cfg->bits == 16U) {
    MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x04U) = MXS_AOUT_CTRL_WORD_LENGTH; /* SET */
  }
  else {
    MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x08U) = MXS_AOUT_CTRL_WORD_LENGTH; /* CLR */
  }

  /* Digital volume (0.5 dB steps, 0 = max). */
  dv = vol_to_dac(cfg->volume);
  MXS_AOUT_REG32(MXS_AOUT_DACVOLUME) =
      MXS_AOUT_DACVOLUME_VOL_LEFT(dv) | MXS_AOUT_DACVOLUME_VOL_RIGHT(dv);
}

void mxs_audio_init(const mxs_audio_config_t *cfg) {

  /* Ungate the AUDIOOUT digital + analog clocks, release reset. */
  MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x08U) = MXS_AOUT_CTRL_SFTRST |
                                         MXS_AOUT_CTRL_CLKGATE;   /* CLR */
  MXS_AOUT_REG32(MXS_AOUT_ANACLKCTRL + 0x08U) = MXS_AOUT_ANACLKCTRL_CLKGATE;

  /* Power up DAC, headphone (class-AB) and speaker path. */
  MXS_AOUT_REG32(MXS_AOUT_PWRDN + 0x08U) = MXS_AOUT_PWRDN_HEADPHONE |
                                          MXS_AOUT_PWRDN_DAC |
                                          MXS_AOUT_PWRDN_SPEAKER |
                                          MXS_AOUT_PWRDN_CAPLESS;  /* CLR */
  MXS_AOUT_REG32(MXS_AOUT_ANACTRL + 0x04U) = MXS_AOUT_ANACTRL_HP_CLASSAB;

  /* DMA wait count max (errata 2349 workaround), error IRQ on. */
  MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x04U) = MXS_AOUT_CTRL_DMAWAIT_COUNT(31U) |
                                         MXS_AOUT_CTRL_FIFO_ERROR_IRQ_EN;

  /* Headphone analog volume: unmute, default 0x0C (0 dB-ish). */
  MXS_AOUT_REG32(MXS_AOUT_HPVOL + 0x08U) = MXS_AOUT_HPVOL_MUTE;  /* CLR */
  MXS_AOUT_REG32(MXS_AOUT_HPVOL) = MXS_AOUT_HPVOL_VOL_LEFT(0x0CU) |
                                  MXS_AOUT_HPVOL_VOL_RIGHT(0x0CU);
  /* Speaker amp unmute. */
  MXS_AOUT_REG32(MXS_AOUT_SPEAKERCTRL + 0x08U) = MXS_AOUT_SPEAKERCTRL_MUTE;

  mxs_audio_config(cfg);
}

msg_t mxs_audio_play(const void *buf, size_t bytes, sysinterval_t timeout) {
  systime_t t0;
  msg_t msg = MSG_OK;

  osalDbgCheck((bytes & 3U) == 0U);

  /* Stop any previous run (errata 2349: clear RUN before starting DMA). */
  MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x08U) = MXS_AOUT_CTRL_RUN;   /* CLR */

  /* One-shot memory -> AUDIOOUT DATA transfer on APBX channel 1.
     Per the Linux mxs driver: reset + (re)init the DMA channel BEFORE
     setting the playback RUN bit, otherwise the channel can stall. */
  imx23_dma_reset(IMX23_DMA_APBX, IMX23_DMA_CH_AUDIO_DAC);
  audio_ccw.next    = 0U;
  audio_ccw.bits    = IMX23_CCW_IRQ | IMX23_CCW_SEMAPHORE |
                      IMX23_CCW_COMMAND_READ;   /* mem -> peripheral */
  audio_ccw.xfer    = (uint16_t)bytes;
  audio_ccw.bufaddr = (uint32_t)(uintptr_t)buf;
  audio_ccw.pio[0]  = 0U;

  imx23_dma_start(IMX23_DMA_APBX, IMX23_DMA_CH_AUDIO_DAC, &audio_ccw);

  /* Only now RUN the DAC. */
  MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x04U) = MXS_AOUT_CTRL_RUN;   /* SET */

  /* Wait for DMA completion (semaphore count drops to 0). */
  t0 = chVTGetSystemTimeX();
  while (!imx23_dma_done(IMX23_DMA_APBX, IMX23_DMA_CH_AUDIO_DAC)) {
    if ((chVTGetSystemTimeX() - t0) > timeout) {
      msg = MSG_TIMEOUT;
      break;
    }
    chThdSleepMilliseconds(1);
  }

  /* Let the FIFO drain, then stop.
     Per the Linux mxs driver: deinit the DMA channel FIRST, then clear
     the RUN bit - clearing RUN while the DMA may still be live can hang
     the channel (see the "potentially hang DMA channel" comment in
     mxs_trigger_stop()). */
  if (msg == MSG_OK) {
    chThdSleepMilliseconds(20);
  }
  imx23_dma_stop(IMX23_DMA_APBX, IMX23_DMA_CH_AUDIO_DAC);
  imx23_dma_reset(IMX23_DMA_APBX, IMX23_DMA_CH_AUDIO_DAC);
  MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x08U) = MXS_AOUT_CTRL_RUN;   /* CLR */

  return msg;
}

void mxs_audio_stop(void) {

  /* DMA off first, then RUN off (same ordering as mxs_audio_play). */
  imx23_dma_stop(IMX23_DMA_APBX, IMX23_DMA_CH_AUDIO_DAC);
  imx23_dma_reset(IMX23_DMA_APBX, IMX23_DMA_CH_AUDIO_DAC);
  MXS_AOUT_REG32(MXS_AOUT_CTRL + 0x08U) = MXS_AOUT_CTRL_RUN;   /* CLR */
}

/** @} */
