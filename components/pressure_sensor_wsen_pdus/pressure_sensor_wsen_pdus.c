// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * Driver for Würth Elektronik WSEN-PDUS differential pressure sensor
 * Part: 2511020213301  (±0.1 kPa / ±100 Pa range)
 * Interface: I2C, fixed address 0x78
 * Datasheet: https://www.we-online.com/en/components/products/WSEN-PDUS
 *
 * Read protocol (no register address required):
 *   Issue an I2C read of 4 bytes from address 0x78.
 *   Byte 0: status[7:6] | pressure_msb[5:0]
 *   Byte 1: pressure_lsb[7:0]
 *   Byte 2: temperature_msb[7:0]
 *   Byte 3: temperature_lsb[7:2] | 00
 *
 *   pressure (14-bit): ((byte0 & 0x3F) << 8) | byte1
 *   temperature (14-bit): (byte2 << 6) | (byte3 >> 2)
 *
 * Pressure conversion for ±0.1 kPa variant:
 *   P_Pa = (raw / 16383.0) * 200.0 - 100.0
 *   Full scale: raw=0 → -100 Pa, raw=8191 → 0 Pa, raw=16383 → +100 Pa
 *
 * Status bits (byte0[7:6]):
 *   00 = normal operation
 *   01 = device in command mode
 *   10 = stale data (same data as last read)
 *   11 = diagnostic condition
 */

#include <esp_err.h>
#include <i2c_master.h>
#include <pressure_sensor_wsen_pdus.h>

static const char *TAG = "pressure_sensor_wsen_pdus";

/* I2C 7-bit address */
#define WSEN_PDUS_I2C_ADDR      0x78

/* Full-scale range for ±0.1 kPa variant in Pa */
#define WSEN_PDUS_FULL_SCALE_PA 200.0f
#define WSEN_PDUS_OFFSET_PA     100.0f
#define WSEN_PDUS_RAW_MAX       16383.0f

/* Status field values (top 2 bits of byte 0) */
#define WSEN_PDUS_STATUS_MASK   0xC0
#define WSEN_PDUS_STATUS_NORMAL 0x00
#define WSEN_PDUS_STATUS_STALE  0x80

int pressure_sensor_wsen_pdus_init(int i2c_port)
{
    /* Verify communication by performing a test read */
    uint8_t data[4] = {0};
    esp_err_t err = i2c_master_read_from_device(i2c_port, WSEN_PDUS_I2C_ADDR, data, sizeof(data), -1);
    if (err != ESP_OK) {
        printf("%s: Failed to communicate with sensor (err=%d)\n", TAG, err);
        return ESP_FAIL;
    }

    uint8_t status = data[0] & WSEN_PDUS_STATUS_MASK;
    if (status == 0xC0) {
        /* 0b11 in status field indicates a diagnostic condition */
        printf("%s: Sensor reported diagnostic condition\n", TAG);
        return ESP_FAIL;
    }

    printf("%s: Sensor initialised (status=0x%02X)\n", TAG, status >> 6);
    return ESP_OK;
}

int pressure_sensor_wsen_pdus_get_kpa(int i2c_port, float *pressure_kpa)
{
    if (!pressure_kpa) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[4] = {0};
    esp_err_t err = i2c_master_read_from_device(i2c_port, WSEN_PDUS_I2C_ADDR, data, sizeof(data), -1);
    if (err != ESP_OK) {
        printf("%s: I2C read failed (err=%d)\n", TAG, err);
        return err;
    }

    uint8_t status = data[0] & WSEN_PDUS_STATUS_MASK;
    if (status == WSEN_PDUS_STATUS_STALE) {
        printf("%s: Stale data returned by sensor\n", TAG);
        /* Return stale data but signal to caller */
        return ESP_ERR_INVALID_STATE;
    }

    /* Reconstruct 14-bit pressure value */
    uint16_t raw_pressure = ((uint16_t)(data[0] & 0x3F) << 8) | data[1];

    /* Convert to Pa: maps [0, 16383] → [-100, +100] Pa */
    float pressure_pa = ((float)raw_pressure / WSEN_PDUS_RAW_MAX) * WSEN_PDUS_FULL_SCALE_PA - WSEN_PDUS_OFFSET_PA;

    *pressure_kpa = pressure_pa / 1000.0f;
    return ESP_OK;
}
