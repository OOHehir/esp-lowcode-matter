// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include "bp5758_driver.h"
#include "i2c_master.h"
#include "hal/i2c_types.h"

/* BP5758 I2C address */
#define BP5758_I2C_ADDR          0x15

/* BP5758 Register addresses */
#define BP5758_REG_OUTPUT_CTRL   0x01
#define BP5758_REG_OUT1_CURRENT  0x02
#define BP5758_REG_OUT2_CURRENT  0x03
#define BP5758_REG_OUT3_CURRENT  0x04
#define BP5758_REG_OUT4_CURRENT  0x05
#define BP5758_REG_OUT5_CURRENT  0x06
#define BP5758_REG_CURRENT_CTRL  0x07

/* Default maximum current setting (0x1F = 90mA) */
#define BP5758_MAX_CURRENT       0x1F

/* Number of channels */
#define BP5758_CHANNEL_COUNT     5

/* I2C port and pins */
static int s_i2c_port = I2C_NUM_0;
static gpio_num_t s_scl_io = GPIO_NUM_NC;
static gpio_num_t s_sda_io = GPIO_NUM_NC;

/* Channel values buffer */
static uint8_t s_channel_values[BP5758_CHANNEL_COUNT] = {0};

/* Initialized flag */
static bool s_initialized = false;

static const char *TAG = "bp5758_driver";

/**
 * @brief Write data to BP5758 register
 */
static int bp5758_write_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    int ret = i2c_master_write_to_device(s_i2c_port, BP5758_I2C_ADDR, data, 2, -1);
    if (ret != 0) {
        printf("%s: Failed to write register 0x%02x\n", TAG, reg);
        return -1;
    }
    return 0;
}

/**
 * @brief Initialize BP5758 device
 */
static int bp5758_device_init(void)
{
    int ret;

    /* Enable all 5 output channels */
    ret = bp5758_write_register(BP5758_REG_OUTPUT_CTRL, 0x1F);
    if (ret != 0) {
        return ret;
    }

    /* Set maximum current for all channels */
    for (int i = 0; i < BP5758_CHANNEL_COUNT; i++) {
        ret = bp5758_write_register(BP5758_REG_OUT1_CURRENT + i, BP5758_MAX_CURRENT);
        if (ret != 0) {
            return ret;
        }
    }

    /* Set current control register */
    ret = bp5758_write_register(BP5758_REG_CURRENT_CTRL, 0x00);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

int bp5758_driver_init(void)
{
    if (s_initialized) {
        printf("%s: Already initialized\n", TAG);
        return 0;
    }

    if (s_scl_io == GPIO_NUM_NC || s_sda_io == GPIO_NUM_NC) {
        printf("%s: I2C pins not configured\n", TAG);
        return -1;
    }

    /* Initialize I2C master */
    int ret = i2c_master_init(s_i2c_port, s_scl_io, s_sda_io);
    if (ret != 0) {
        printf("%s: Failed to initialize I2C master\n", TAG);
        return -1;
    }

    /* Initialize BP5758 device */
    ret = bp5758_device_init();
    if (ret != 0) {
        printf("%s: Failed to initialize BP5758 device\n", TAG);
        return -1;
    }

    /* Clear all channels */
    memset(s_channel_values, 0, sizeof(s_channel_values));

    s_initialized = true;
    printf("%s: Initialized successfully\n", TAG);
    return 0;
}

void bp5758_driver_deinit(void)
{
    /* Turn off all channels */
    memset(s_channel_values, 0, sizeof(s_channel_values));
    bp5758_driver_update_channels();
    
    s_initialized = false;
    printf("%s: Deinitialized\n", TAG);
}

int bp5758_driver_set_channel(uint8_t channel, uint8_t val)
{
    if (!s_initialized) {
        printf("%s: Driver not initialized\n", TAG);
        return -1;
    }

    if (channel >= BP5758_CHANNEL_COUNT) {
        printf("%s: Invalid channel %d\n", TAG, channel);
        return -1;
    }

    /* Scale from 0-255 to 0-255 (no scaling needed) */
    s_channel_values[channel] = val;
    return 0;
}

int bp5758_driver_get_channel(uint8_t channel, uint8_t *val)
{
    if (!s_initialized) {
        printf("%s: Driver not initialized\n", TAG);
        return -1;
    }

    if (channel >= BP5758_CHANNEL_COUNT || val == NULL) {
        printf("%s: Invalid channel %d or null pointer\n", TAG, channel);
        return -1;
    }

    *val = s_channel_values[channel];
    return 0;
}

int bp5758_driver_update_channels(void)
{
    if (!s_initialized) {
        printf("%s: Driver not initialized\n", TAG);
        return -1;
    }

    /* BP5758 uses grayscale registers to control brightness
     * We need to write all 5 channel values in sequence
     * The format is typically a multi-byte write starting from a specific register
     * For simplicity, we'll write each channel individually to OUT registers
     */
    
    /* Write grayscale data to each channel
     * This is a simplified implementation - actual BP5758 may require
     * a specific command sequence for updating outputs
     */
    for (int i = 0; i < BP5758_CHANNEL_COUNT; i++) {
        /* Map PWM value (0-255) directly to channel output
         * BP5758 typically uses the same register to write PWM data
         * after configuration. This implementation uses a simple approach.
         */
        uint8_t cmd[2];
        cmd[0] = BP5758_REG_OUT1_CURRENT + i;  /* Reusing current registers as PWM registers */
        cmd[1] = s_channel_values[i] * BP5758_MAX_CURRENT / 255;  /* Scale to max current */
        
        int ret = i2c_master_write_to_device(s_i2c_port, BP5758_I2C_ADDR, cmd, 2, -1);
        if (ret != 0) {
            printf("%s: Failed to update channel %d\n", TAG, i);
            return -1;
        }
    }

    return 0;
}

int bp5758_driver_regist_channel(uint8_t channel, gpio_num_t gpio)
{
    /* For BP5758, we use this function to configure I2C pins
     * The channel parameter is overloaded:
     * - When called first time: gpio parameter contains SDA pin
     * - When called second time: gpio parameter contains SCL pin
     * This is a workaround since the light driver interface expects GPIO per channel
     */
    
    if (s_sda_io == GPIO_NUM_NC) {
        s_sda_io = gpio;
        printf("%s: Registered SDA pin: %d\n", TAG, gpio);
    } else if (s_scl_io == GPIO_NUM_NC) {
        s_scl_io = gpio;
        printf("%s: Registered SCL pin: %d\n", TAG, gpio);
    }
    
    return 0;
}
