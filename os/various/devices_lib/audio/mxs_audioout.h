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
 * @file    IMX23/IMX23xx/mxs_audioout.h
 * @brief   i.MX23 mxs-builtin audio DAC playback driver (playback only).
 *
 * @details Ported from the Linux sound/soc/mxs/mxs-builtin-{dai,pcm}.c and
 *          sound/soc/codecs/mxs-builtin-codec.c. Stereo interleaved PCM
 *          (16 or 32 bit) is streamed to HW_AUDIOOUT_DATA by the APBX DMA
 *          channel 1 (audio-dac). Headphone + speaker outputs are powered.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef MXS_AUDIOOUT_H
#define MXS_AUDIOOUT_H

#include "hal.h"

#include <stdint.h>

#define MXS_AOUT_BASE         0x80048000U
#define MXS_AOUT_REG(off)     (MXS_AOUT_BASE + (off))
#define MXS_AOUT_REG32(addr)  (*(volatile uint32_t *)(uintptr_t)(addr))

#define MXS_AOUT_REG(off)     (MXS_AOUT_BASE + (off))
#define MXS_AOUT_CTRL         MXS_AOUT_REG(0x000U)
#define MXS_AOUT_STAT         MXS_AOUT_REG(0x010U)
#define MXS_AOUT_DACSRR       MXS_AOUT_REG(0x020U)
#define MXS_AOUT_DACVOLUME    MXS_AOUT_REG(0x030U)
#define MXS_AOUT_HPVOL        MXS_AOUT_REG(0x050U)
#define MXS_AOUT_PWRDN        MXS_AOUT_REG(0x070U)
#define MXS_AOUT_REFCTRL      MXS_AOUT_REG(0x080U)
#define MXS_AOUT_ANACTRL      MXS_AOUT_REG(0x090U)
#define MXS_AOUT_ANACLKCTRL   MXS_AOUT_REG(0x0E0U)
#define MXS_AOUT_DATA         MXS_AOUT_REG(0x0F0U)
#define MXS_AOUT_SPEAKERCTRL  MXS_AOUT_REG(0x100U)
#define MXS_AOUT_VERSION      MXS_AOUT_REG(0x200U)
#define MXS_AOUT_CTRL_SFTRST              (1U << 31)
#define MXS_AOUT_CTRL_CLKGATE             (1U << 30)
#define MXS_AOUT_CTRL_DMAWAIT_COUNT(v)    (((v) & 0x1FU) << 16)
#define MXS_AOUT_CTRL_WORD_LENGTH         (1U << 6)   /* 1=16-bit, 0=32-bit */
#define MXS_AOUT_CTRL_FIFO_UNDERFLOW_IRQ  (1U << 3)
#define MXS_AOUT_CTRL_FIFO_OVERFLOW_IRQ   (1U << 2)
#define MXS_AOUT_CTRL_FIFO_ERROR_IRQ_EN   (1U << 1)
#define MXS_AOUT_CTRL_RUN                 (1U << 0)
#define MXS_AOUT_DACSRR_OSR               (1U << 31)
#define MXS_AOUT_DACSRR_BASEMULT(v)       (((v) & 0x7U) << 28)
#define MXS_AOUT_DACSRR_SRC_HOLD(v)       (((v) & 0x7U) << 24)
#define MXS_AOUT_DACSRR_SRC_INT(v)        (((v) & 0x1FU) << 16)
#define MXS_AOUT_DACSRR_SRC_FRAC(v)       ((v) & 0xFFFFU)
#define MXS_AOUT_DACVOLUME_EN_ZCD         (1U << 25)
#define MXS_AOUT_DACVOLUME_MUTE_LEFT      (1U << 24)
#define MXS_AOUT_DACVOLUME_VOL_LEFT(v)    (((v) & 0xFFU) << 16)
#define MXS_AOUT_DACVOLUME_MUTE_RIGHT     (1U << 8)
#define MXS_AOUT_DACVOLUME_VOL_RIGHT(v)   ((v) & 0xFFU)
#define MXS_AOUT_HPVOL_EN_MSTR_ZCD        (1U << 25)
#define MXS_AOUT_HPVOL_MUTE               (1U << 24)
#define MXS_AOUT_HPVOL_VOL_LEFT(v)        (((v) & 0x7FU) << 8)
#define MXS_AOUT_HPVOL_VOL_RIGHT(v)       ((v) & 0x7FU)
#define MXS_AOUT_PWRDN_SPEAKER            (1U << 24)
#define MXS_AOUT_PWRDN_DAC                (1U << 12)
#define MXS_AOUT_PWRDN_CAPLESS            (1U << 4)
#define MXS_AOUT_PWRDN_HEADPHONE          (1U << 0)
#define MXS_AOUT_ANACTRL_HP_CLASSAB       (1U << 4)
#define MXS_AOUT_ANACTRL_HP_HOLD_GND      (1U << 5)
#define MXS_AOUT_ANACLKCTRL_CLKGATE       (1U << 31)
#define MXS_AOUT_ANACLKCTRL_DACDIV(v)     ((v) & 0x7U)
#define MXS_AOUT_SPEAKERCTRL_MUTE         (1U << 24)


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Audio stream configuration.
 */
typedef struct {
  uint32_t  rate;       /**< sample rate in Hz (8k..48k common set)      */
  uint8_t   bits;       /**< 16 or 32                                     */
  uint8_t   volume;     /**< 0..100 (DAC digital volume)                  */
} mxs_audio_config_t;

/**
 * @brief   Powers up the AUDIOOUT block and the analog DAC/headphone/speaker
 *          path, mute off.
 *
 * @notapi
 */
void mxs_audio_init(const mxs_audio_config_t *cfg);

/**
 * @brief   Reconfigures rate/bits/volume.
 *
 * @notapi
 */
void mxs_audio_config(const mxs_audio_config_t *cfg);

/**
 * @brief   Plays a buffer of interleaved stereo PCM (blocking, DMA).
 *
 * @param[in] buf       PCM data (L/R interleaved, cfg->bits wide)
 * @param[in] bytes     number of bytes (multiple of 4)
 * @param[in] timeout   timeout
 * @return              MSG_OK when all bytes were clocked out.
 *
 * @notapi
 */
msg_t mxs_audio_play(const void *buf, size_t bytes, sysinterval_t timeout);

/**
 * @brief   Stops playback and the DAC run bit.
 *
 * @notapi
 */
void mxs_audio_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* MXS_AUDIOOUT_H */

/** @} */
