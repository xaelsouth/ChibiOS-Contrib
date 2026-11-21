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
 * @file    IMX23/IMX23xx/imx23_audioout.h
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

#ifndef IMX23_AUDIOOUT_H
#define IMX23_AUDIOOUT_H

#include "imx23.h"

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
} imx23_audio_config_t;

/**
 * @brief   Powers up the AUDIOOUT block and the analog DAC/headphone/speaker
 *          path, mute off.
 *
 * @notapi
 */
void imx23_audio_init(const imx23_audio_config_t *cfg);

/**
 * @brief   Reconfigures rate/bits/volume.
 *
 * @notapi
 */
void imx23_audio_config(const imx23_audio_config_t *cfg);

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
msg_t imx23_audio_play(const void *buf, size_t bytes, sysinterval_t timeout);

/**
 * @brief   Stops playback and the DAC run bit.
 *
 * @notapi
 */
void imx23_audio_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* IMX23_AUDIOOUT_H */

/** @} */
