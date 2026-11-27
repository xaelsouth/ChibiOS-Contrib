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

#include <string.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rail.h"
#include "sli_rail_util_callbacks.h"
#include "sl_rail_util_init.h"
#include "sl_rail_util_protocol.h"

extern const RAIL_ChannelConfig_t *channelConfigs[];

static RAIL_Handle_t rail_handle;
static semaphore_t tx_done_sem;
static volatile RAIL_Events_t tx_events;
static uint16_t tx_channel;

static void rail_event_cb(RAIL_Handle_t handle, RAIL_Events_t events) {
  (void)handle;
  if ((events & RAIL_EVENTS_TX_COMPLETION) != 0U) {
    osalSysLockFromISR();
    tx_events = events;
    chSemSignalI(&tx_done_sem);
    osalSysUnlockFromISR();
  }
}


/* Console via SEGGER RTT (up channel 0) — works over J-Link SWD.
   View with: JLinkRTTLoggerExe -Device EFR32FG23B010F512IM48 -If SWD -Speed 4000 -RTTChannel 0 out.log */
#include <string.h>
#include <stdio.h>

typedef struct {
  char           id[16];        /* "SEGGER RTT" */
  uint32_t       num_up;
  uint32_t       num_down;
  struct {
    const char  *name;
    uint8_t     *buf;
    uint32_t     size;
    volatile uint32_t wr;
    volatile uint32_t rd;
    uint32_t     flags;
  } up[1];
} rtt_cb_t;

static rtt_cb_t rtt_cb __attribute__((aligned(32)));
static uint8_t  rtt_buf[1024];

static void console_init(void) {
  memset(&rtt_cb, 0, sizeof(rtt_cb));
  memcpy(rtt_cb.id, "SEGGER RTT", 10);
  rtt_cb.num_up = 1;
  rtt_cb.num_down = 0;
  rtt_cb.up[0].name  = "Terminal";
  rtt_cb.up[0].buf   = rtt_buf;
  rtt_cb.up[0].size  = sizeof(rtt_buf);
  rtt_cb.up[0].flags = 0;
}

static void rtt_write(const char *s, size_t n) {
  uint32_t wp = rtt_cb.up[0].wr;
  for (size_t i = 0; i < n; i++) {
    rtt_buf[wp] = (uint8_t)s[i];
    wp = (wp + 1U) % rtt_cb.up[0].size;
  }
  rtt_cb.up[0].wr = wp;
}

#define PRINTF(...) do {   char _b[256];   int _n = snprintf(_b, sizeof(_b), __VA_ARGS__);   if (_n > 0) rtt_write(_b, (size_t)_n); } while (0)

static bool radio_init(void) {
  RAIL_Config_t rail_config = { .eventsCallback = rail_event_cb };
  chSemObjectInit(&tx_done_sem, 0);
  PRINTF("RAIL_Init...\r\n");
  rail_handle = RAIL_Init(&rail_config, &sl_rail_util_on_rf_ready);
  PRINTF("RAIL_Init returned %p\r\n", (void *)rail_handle);
  if (rail_handle == NULL) {
    return false;
  }

  RAIL_DataConfig_t data_config = {
    .txSource = SL_RAIL_UTIL_INIT_DATA_FORMAT_INST0_TX_SOURCE,
    .rxSource = SL_RAIL_UTIL_INIT_DATA_FORMAT_INST0_RX_SOURCE,
    .txMethod = SL_RAIL_UTIL_INIT_DATA_FORMAT_INST0_TX_MODE,
    .rxMethod = SL_RAIL_UTIL_INIT_DATA_FORMAT_INST0_RX_MODE,
  };
  RAIL_Status_t status = RAIL_ConfigData(rail_handle, &data_config);
  PRINTF("RAIL_ConfigData=%d\r\n", (int)status);
  if (status != RAIL_STATUS_NO_ERROR) {
    return false;
  }
  PRINTF("RAIL_ConfigChannels...\r\n");
  tx_channel = RAIL_ConfigChannels(rail_handle, channelConfigs[1],
                                   &sli_rail_util_on_channel_config_change);
  status = sl_rail_util_protocol_config(rail_handle,
                                       SL_RAIL_UTIL_INIT_PROTOCOL_INST0_DEFAULT);
  if (status != RAIL_STATUS_NO_ERROR) {
    PRINTF("protocol configuration failed: %d\r\n", (int)status);
    return false;
  }
  status = RAIL_ConfigCal(rail_handle, RAIL_CAL_TEMP | RAIL_CAL_ONETIME);
  if (status != RAIL_STATUS_NO_ERROR) {
    PRINTF("calibration configuration failed: %d\r\n", (int)status);
    return false;
  }
  status = RAIL_ConfigEvents(rail_handle, RAIL_EVENTS_ALL, RAIL_EVENTS_TX_COMPLETION);
  if (status != RAIL_STATUS_NO_ERROR) {
    PRINTF("event configuration failed: %d\r\n", (int)status);
    return false;
  }

  RAIL_StateTransitions_t tx_tr = {
    .success = SL_RAIL_UTIL_INIT_TRANSITION_INST0_TX_SUCCESS,
    .error   = SL_RAIL_UTIL_INIT_TRANSITION_INST0_TX_ERROR,
  };
  status = RAIL_SetTxTransitions(rail_handle, &tx_tr);
  if (status != RAIL_STATUS_NO_ERROR) {
    PRINTF("transition configuration failed: %d\r\n", (int)status);
    return false;
  }
  PRINTF("radio configuration complete\r\n");
  return true;
}

/* Test datagram, 868.95 MHz. */
static const uint8_t radio_datagram[] = {
  0x0F, 0x44, 0x34, 0x12, 0x78, 0x56, 0x00, 0x00,
  0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static CC_ALIGN_DATA(16) uint8_t rail_tx_fifo[256];

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nRAIL-TX test: test datagram every 3 s\r\n");

  if (!radio_init()) {
    PRINTF("radio configuration: FAIL\r\n");
    while (true) chThdSleepMilliseconds(1000);
  }

  unsigned n = 0;
  while (true) {
    memcpy(rail_tx_fifo, radio_datagram, sizeof(radio_datagram));
    uint16_t fifo_size = RAIL_SetTxFifo(rail_handle, rail_tx_fifo,
                                       sizeof(radio_datagram), sizeof(rail_tx_fifo));
    tx_events = 0U;
    RAIL_Status_t status = fifo_size >= sizeof(radio_datagram)
                         ? RAIL_StartTx(rail_handle, tx_channel, RAIL_TX_OPTIONS_DEFAULT, NULL)
                         : RAIL_STATUS_INVALID_PARAMETER;
    msg_t done = status == RAIL_STATUS_NO_ERROR
               ? chSemWaitTimeout(&tx_done_sem, TIME_MS2I(1000)) : MSG_RESET;
    bool sent = done == MSG_OK && (tx_events & RAIL_EVENT_TX_PACKET_SENT) != 0U;
    PRINTF("TX #%u: fifo=%u status=%d completion=%d events=%08lx:%08lx %s\r\n",
           ++n, (unsigned)fifo_size, (int)status, (int)done,
           (unsigned long)(tx_events >> 32), (unsigned long)tx_events,
           sent ? "PASS" : "FAIL");
    if (!sent) {
      while (true) chThdSleepMilliseconds(1000);
    }
    chThdSleepMilliseconds(3000);
  }
}
