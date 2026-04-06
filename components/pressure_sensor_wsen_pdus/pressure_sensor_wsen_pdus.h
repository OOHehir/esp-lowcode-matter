/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the WSEN-PDUS differential pressure sensor.
 *
 * Performs a soft reset and verifies communication over the given I2C port.
 * The sensor I2C address is fixed at 0x78.
 *
 * @param[in] i2c_port I2C port number to which the sensor is connected.
 * @return 0 on success, non-zero on failure.
 */
int pressure_sensor_wsen_pdus_init(int i2c_port);

/**
 * @brief Read differential pressure in kPa from the WSEN-PDUS sensor.
 *
 * Reads a single pressure measurement from the sensor. The WSEN-PDUS
 * ±0.1 kPa variant has a full-scale range of ±100 Pa (±0.1 kPa).
 *
 * @param[in]  i2c_port I2C port number to which the sensor is connected.
 * @param[out] pressure_kpa Pointer to a float where the pressure in kPa will be stored.
 * @return 0 on success, non-zero on failure.
 */
int pressure_sensor_wsen_pdus_get_kpa(int i2c_port, float *pressure_kpa);

#ifdef __cplusplus
}
#endif
