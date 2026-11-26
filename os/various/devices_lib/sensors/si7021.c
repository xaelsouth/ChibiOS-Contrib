/*
    Si7021 for ChibiOS/RT - Copyright (C) 2026 Xael South

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
 * @file    si7021.c
 * @brief   Si7021/Si7020/Si7013 humidity & temperature sensor driver.
 *
 * @addtogroup si7021
 * @{
 */

#define I2C_HELPERS_AUTOMATIC_DRV TRUE

#include "hal.h"
#include "i2c_helpers.h"
#include "si7021.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* The stock i2c_helpers macros use TIME_INFINITE; use the *_timeout()
   variants with the finite timeout from the config instead. */
#define si7021_transmit(txbuf, txbytes, rxbuf, rxbytes)                       \
    _i2c_transmit_timeout(&drv->config->i2c, (txbuf), (txbytes),              \
                          (rxbuf), (rxbytes), drv->config->timeout)
#define si7021_receive(rxbuf, rxbytes)                                        \
    _i2c_receive_timeout(&drv->config->i2c, (rxbuf), (rxbytes),               \
                         drv->config->timeout)

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/* Decode a 14-bit raw value (low 2 bits are status) into °C or %RH. */
static msg_t
_decode_measure(uint16_t raw, float *temperature, float *humidity) {
    raw &= 0xFFFCU;
    if (temperature) {
        *temperature = 175.72f * (float)raw / 65536.0f - 46.85f;
    }
    if (humidity) {
        *humidity = 125.0f * (float)raw / 65536.0f - 6.0f;
    }
    return MSG_OK;
}

/* Trigger a no-hold measurement, wait for the conversion, read 3 bytes
   (MSB, LSB, CRC8) and decode. */
static msg_t
_measure(SI7021_drv *drv, uint8_t cmd, float *temperature, float *humidity) {
    uint8_t rx[3];
    msg_t msg;

    if ((msg = si7021_transmit(&cmd, 1, NULL, 0)) < MSG_OK) {
        drv->state = SENSOR_ERROR;
        return msg;
    }
    osalThreadSleepMilliseconds(drv->delay);
    if ((msg = si7021_receive(rx, sizeof(rx))) < MSG_OK) {
        drv->state = SENSOR_ERROR;
        return msg;
    }
    return _decode_measure((uint16_t)(((uint16_t)rx[0] << 8) | rx[1]),
                           temperature, humidity);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void
SI7021_init(SI7021_drv *drv, SI7021_config *config) {
    drv->config = config;
    if (config->timeout == 0) {
        config->timeout = SI7021_DEFAULT_TIMEOUT;
    }
    drv->delay = SI7021_ACQUIRE_TIME;
    drv->state = SENSOR_INIT;
}

msg_t
SI7021_check(SI7021_drv *drv) {
    static const uint8_t cmd[] = { SI7021_CMD_READ_ID2, SI7021_CMD_READ_ID2_B };
    uint8_t rx[6];
    msg_t msg;

    if ((msg = si7021_transmit(cmd, sizeof(cmd), rx, sizeof(rx))) < MSG_OK) {
        return msg;
    }
    if (rx[0] != SI7021_ID2_SI7013 &&
        rx[0] != SI7021_ID2_SI7020 &&
        rx[0] != SI7021_ID2_SI7021) {
        return SENSOR_NOTFOUND;
    }
    return MSG_OK;
}

msg_t
SI7021_start(SI7021_drv *drv) {
    osalDbgAssert((drv->state == SENSOR_INIT   ) ||
                  (drv->state == SENSOR_ERROR  ) ||
                  (drv->state == SENSOR_STOPPED),
                  "invalid state");
    /* Default user register (0x3A) is fine; no configuration needed. */
    drv->state = SENSOR_STARTED;
    return MSG_OK;
}

msg_t
SI7021_stop(SI7021_drv *drv) {
    drv->state = SENSOR_STOPPED;
    return MSG_OK;
}

msg_t
SI7021_readSerial(SI7021_drv *drv, uint8_t *serial) {
    static const uint8_t cmd[] = { SI7021_CMD_READ_ID1, SI7021_CMD_READ_ID1_B };
    msg_t msg;

    osalDbgAssert(drv->state == SENSOR_STARTED, "invalid state");
    if ((msg = si7021_transmit(cmd, sizeof(cmd), serial, 8)) < MSG_OK) {
        return msg;
    }
    return MSG_OK;
}

msg_t
SI7021_startMeasure(SI7021_drv *drv, bool humidity) {
    uint8_t cmd = humidity ? SI7021_CMD_MEAS_RH_NOHOLD : SI7021_CMD_MEAS_T_NOHOLD;
    msg_t msg;

    osalDbgAssert(drv->state == SENSOR_STARTED, "invalid state");
    if ((msg = si7021_transmit(&cmd, 1, NULL, 0)) < MSG_OK) {
        drv->state = SENSOR_ERROR;
        return msg;
    }
    drv->state = SENSOR_MEASURING;
    return MSG_OK;
}

msg_t
SI7021_readMeasure(SI7021_drv *drv, float *temperature, float *humidity) {
    uint8_t rx[3];
    msg_t msg;

    osalDbgAssert((drv->state == SENSOR_MEASURING) ||
                  (drv->state == SENSOR_READY    ),
                  "invalid state");
    if ((msg = si7021_receive(rx, sizeof(rx))) < MSG_OK) {
        drv->state = SENSOR_ERROR;
        return msg;
    }
    drv->state = SENSOR_STARTED;
    return _decode_measure((uint16_t)(((uint16_t)rx[0] << 8) | rx[1]),
                           temperature, humidity);
}

msg_t
SI7021_readTemperatureHumidity(SI7021_drv *drv, float *temperature,
                               float *humidity) {
    msg_t msg;

    osalDbgAssert(drv->state == SENSOR_STARTED, "invalid state");

    if (temperature != NULL) {
        if ((msg = _measure(drv, SI7021_CMD_MEAS_T_NOHOLD, temperature, NULL))
            < MSG_OK) {
            return msg;
        }
    }
    if (humidity != NULL) {
        if ((msg = _measure(drv, SI7021_CMD_MEAS_RH_NOHOLD, NULL, humidity))
            < MSG_OK) {
            return msg;
        }
    }
    drv->state = SENSOR_STARTED;
    return MSG_OK;
}

/**
 * @}
 */
