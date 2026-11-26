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
 * @file    si7021.h
 * @brief   Si7021/Si7020/Si7013 relative humidity & temperature sensor
 *          interface module header (Silicon Labs, I2C addr 0x40).
 *
 * @details Unlike the other devices_lib sensors this driver uses the
 *          *_timeout() i2c helpers with a finite, configuration-provided
 *          timeout — the stock i2c_send/i2c_receive macros hardcode
 *          TIME_INFINITE and can block forever on a stuck bus.
 *
 * No locking is done.
 *
 * @{
 */

#ifndef _SENSOR_SI7021_H_
#define _SENSOR_SI7021_H_

#include <math.h>
#include <stdbool.h>
#include "i2c_helpers.h"
#include "sensor.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* I2C address (fixed by the chip). */
#define SI7021_I2CADDR              0x40
#define SI7021_I2CADDR_DEFAULT      SI7021_I2CADDR

/* Commands. */
#define SI7021_CMD_MEAS_RH_HOLD     0xE5    /**< Measure RH, hold master   */
#define SI7021_CMD_MEAS_T_HOLD      0xE3    /**< Measure temp, hold master */
#define SI7021_CMD_MEAS_RH_NOHOLD   0xF5    /**< Measure RH, no hold       */
#define SI7021_CMD_MEAS_T_NOHOLD    0xF3    /**< Measure temp, no hold     */
#define SI7021_CMD_READ_ID1         0xFA    /**< + 0x0F: electronic ID (8B) */
#define SI7021_CMD_READ_ID1_B       0x0F
#define SI7021_CMD_READ_ID2         0xFC    /**< + 0xC9: device ID + serial */
#define SI7021_CMD_READ_ID2_B       0xC9
#define SI7021_CMD_RESET            0xFE
#define SI7021_CMD_WRITE_USER_REG   0xE6
#define SI7021_CMD_READ_USER_REG    0xE7

/* First byte of the READ_ID2 response. */
#define SI7021_ID2_SI7013           0x0D
#define SI7021_ID2_SI7020           0x14
#define SI7021_ID2_SI7021           0x15

/**
 * @brief Time in milli-seconds necessary for the sensor to boot after reset.
 */
#define SI7021_BOOTUP_TIME          10

/**
 * @brief Time in milli-seconds for a 14-bit conversion plus safety margin.
 */
#define SI7021_ACQUIRE_TIME         15

/**
 * @brief Default finite I2C transaction timeout (used when config->timeout
 *        is 0).
 */
#define SI7021_DEFAULT_TIMEOUT      TIME_MS2I(200)

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   SI7021 configuration structure.
 */
typedef struct {
    I2CHelper i2c;          /**< @brief I2C driver + address. Keep it first. */
    systime_t timeout;      /**< @brief Finite timeout for every I2C transaction
                                     (0 = #SI7021_DEFAULT_TIMEOUT). Never use
                                     TIME_INFINITE here: a stuck bus must not
                                     block the caller forever. */
} SI7021_config;

/**
 * @brief   SI7021 driver structure.
 */
typedef struct {
    SI7021_config *config;
    sensor_state_t state;
    unsigned int   delay;   /**< @brief Acquisition time in milli-seconds. */
} SI7021_drv;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/**
 * @brief   Initialize the sensor driver.
 */
void SI7021_init(SI7021_drv *drv, SI7021_config *config);

/**
 * @brief   Check that the sensor is really present (reads the electronic ID).
 * @retval  MSG_OK          a Si7013/Si7020/Si7021 is at the configured address
 * @retval  SENSOR_NOTFOUND device ID mismatch
 * @retval  < MSG_OK        I2C error (timeout, NACK, ...)
 */
msg_t SI7021_check(SI7021_drv *drv);

/**
 * @brief   Start the sensor.
 */
msg_t SI7021_start(SI7021_drv *drv);

/**
 * @brief   Stop the sensor (Si7021 has no sleep command; state only).
 */
msg_t SI7021_stop(SI7021_drv *drv);

/**
 * @brief   Read the 8-byte electronic serial (READ_ID1).
 */
msg_t SI7021_readSerial(SI7021_drv *drv, uint8_t *serial);

/**
 * @brief   Trigger a measurement (no-hold command).
 * @param[in] humidity  false = temperature (0xF3), true = humidity (0xF5)
 */
msg_t SI7021_startMeasure(SI7021_drv *drv, bool humidity);

/**
 * @brief   Read the last triggered measurement.
 * @note    Wait SI7021_getAcquisitionTime() between startMeasure and this.
 */
msg_t SI7021_readMeasure(SI7021_drv *drv, float *temperature, float *humidity);

/**
 * @brief   Read temperature and humidity (start measure, wait, read).
 */
msg_t SI7021_readTemperatureHumidity(SI7021_drv *drv, float *temperature,
                                     float *humidity);

/**
 * @brief   Time in milli-seconds necessary for acquiring a new measure.
 */
static inline unsigned int SI7021_getAcquisitionTime(SI7021_drv *drv) {
    return drv->delay;
}

/**
 * @brief   Return the temperature in °C, NAN on failure.
 */
static inline float SI7021_getTemperature(SI7021_drv *drv) {
    float temperature = NAN;
    SI7021_readTemperatureHumidity(drv, &temperature, NULL);
    return temperature;
}

/**
 * @brief   Return the relative humidity in percent, NAN on failure.
 */
static inline float SI7021_getHumidity(SI7021_drv *drv) {
    float humidity = NAN;
    SI7021_readTemperatureHumidity(drv, NULL, &humidity);
    return humidity;
}

#endif /* _SENSOR_SI7021_H_ */

/**
 * @}
 */
