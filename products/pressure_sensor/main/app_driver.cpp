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

/*
 * Application driver for WSEN-PDUS ±0.1 kPa differential pressure sensor
 * on the ESP32-C6 evaluation board.
 *
 * Hardware connections (WSEN-PDUS EVB → ESP32-C6):
 *   VDD  → 3.3V
 *   GND  → GND
 *   SCL  → GPIO1
 *   SDA  → GPIO2
 *   (I2C address fixed at 0x78 on the WSEN-PDUS EVB)
 *
 * Matter reporting:
 *   Cluster: Pressure Measurement (0x0403)
 *   Attribute: MeasuredValue (0x0000), unit = 0.1 kPa
 *   Range for ±0.1 kPa sensor: MeasuredValue ∈ {-1, 0, 1}
 *
 * Reporting interval: every 5 seconds.
 */

#include <stdio.h>

#include <low_code.h>
#include <system.h>
#include <i2c_master.h>
#include <pressure_sensor_wsen_pdus.h>
#include <button_driver.h>
#include <light_driver.h>

#include "app_priv.h"

#define BUTTON_GPIO_NUM  (gpio_num_t)9
#define LED_GPIO_NUM     (gpio_num_t)8

#define I2C_PORT         I2C_NUM_0
#define I2C_SCL_IO       (gpio_num_t)1
#define I2C_SDA_IO       (gpio_num_t)2

/* Matter Pressure Measurement cluster: MeasuredValue unit is 0.1 kPa */
#define KPA_TO_MATTER(kpa)  ((int16_t)((kpa) * 10.0f))

static const char *TAG = "app_driver";

static void app_driver_trigger_factory_reset_button_callback(void *arg, void *data)
{
    low_code_event_t event = {
        .event_type = LOW_CODE_EVENT_FACTORY_RESET
    };
    low_code_event_to_system(&event);
    printf("%s: Factory reset triggered\n", TAG);
}

static void app_driver_report_pressure(float pressure_kpa)
{
    /*
     * Matter Pressure Measurement MeasuredValue is int16s in units of 0.1 kPa.
     * For the ±0.1 kPa sensor this yields values in the range [-1, 1].
     * The raw float is preserved for console diagnostics.
     */
    int16_t matter_value = KPA_TO_MATTER(pressure_kpa);

    printf("%s: Pressure = %.4f kPa  (%.2f Pa)  Matter MeasuredValue = %d\n",
           TAG, pressure_kpa, pressure_kpa * 1000.0f, matter_value);

    low_code_feature_data_t update_data = {
        .details = {
            .endpoint_id = 1,
            .feature_id  = LOW_CODE_FEATURE_ID_PRESSURE_SENSOR_VALUE
        },
        .value = {
            .type      = LOW_CODE_VALUE_TYPE_INTEGER,
            .value_len = sizeof(int16_t),
            .value     = (uint8_t *)&matter_value,
        },
    };

    low_code_feature_update_to_system(&update_data);
}

/*
 * Periodic callback: read pressure from the WSEN-PDUS and report it to the
 * Matter stack via the LP→HP transport.
 *
 * The function signature must match system_timer_handle_t callback format.
 */
void app_driver_read_and_report_feature(system_timer_handle_t timer_handle, void *user_data)
{
    float pressure_kpa = 0.0f;

    int ret = pressure_sensor_wsen_pdus_get_kpa(I2C_PORT, &pressure_kpa);
    if (ret != 0) {
        printf("%s: Failed to read pressure sensor (err=%d)\n", TAG, ret);
        return;
    }

    app_driver_report_pressure(pressure_kpa);

    /* Brief LED pulse to confirm a reading was taken and reported */
    light_driver_set_power(true);
    system_delay_ms(50);
    light_driver_set_power(false);
}

int app_driver_init()
{
    printf("%s: Initializing driver\n", TAG);

    /* --- Button (factory reset) --- */
    button_config_t btn_cfg = {
        .gpio_num    = BUTTON_GPIO_NUM,
        .pullup_en   = 1,
        .active_level = 0,
    };
    button_handle_t btn_handle = button_driver_create(&btn_cfg);
    if (!btn_handle) {
        printf("%s: Failed to create button\n", TAG);
        return -1;
    }
    button_driver_register_cb(btn_handle, BUTTON_LONG_PRESS_UP,
                              app_driver_trigger_factory_reset_button_callback, NULL);

    /* --- Single-channel PWM LED (status indicator) --- */
    printf("%s: Initializing light driver\n", TAG);
    light_driver_config_t cfg = {
        .device_type    = LIGHT_DEVICE_TYPE_LED,
        .channel_comb   = LIGHT_CHANNEL_COMB_1CH_C,
        .io_conf = {
            .led_io = {
                .cold = LED_GPIO_NUM,
            },
        },
        .min_brightness = 0,
        .max_brightness = 100,
    };
    light_driver_init(&cfg);

    /* --- I2C master --- */
    int ret = i2c_master_init(I2C_PORT, I2C_SCL_IO, I2C_SDA_IO);
    if (ret != 0) {
        printf("%s: Failed to initialise I2C master\n", TAG);
        return -1;
    }

    /* --- WSEN-PDUS sensor --- */
    ret = pressure_sensor_wsen_pdus_init(I2C_PORT);
    if (ret != 0) {
        printf("%s: Failed to initialise WSEN-PDUS pressure sensor\n", TAG);
        return -1;
    }

    /* --- Periodic measurement timer (every 5 seconds) --- */
    system_timer_handle_t timer = system_timer_create(app_driver_read_and_report_feature,
                                                       NULL, 5000, true);
    if (!timer) {
        printf("%s: Failed to create measurement timer\n", TAG);
        return -1;
    }
    system_timer_start(timer);

    return 0;
}

int app_driver_feature_update(low_code_feature_data_t *data)
{
    if (data->details.endpoint_id == 2 &&
        data->details.feature_id == LOW_CODE_FEATURE_ID_POWER) {
        bool state = *(bool *)data->value.value;
        printf("%s: LED set to %s\n", TAG, state ? "ON" : "OFF");
        light_driver_set_power(state);
    }
    return 0;
}

int app_driver_event_handler(low_code_event_t *event)
{
    printf("%s: Received event: %d\n", TAG, event->event_type);

    light_effect_config_t effect_config = {
        .type           = LIGHT_EFFECT_INVALID,
        .mode           = LIGHT_WORK_MODE_WHITE,
        .max_brightness = 100,
        .min_brightness = 10
    };

    switch (event->event_type) {
        case LOW_CODE_EVENT_SETUP_MODE_START:
            printf("%s: Setup mode started\n", TAG);
            effect_config.type = LIGHT_EFFECT_BLINK;
            light_driver_effect_start(&effect_config, 2000, 120000);
            break;
        case LOW_CODE_EVENT_SETUP_MODE_END:
            printf("%s: Setup mode ended\n", TAG);
            light_driver_effect_stop();
            break;
        case LOW_CODE_EVENT_SETUP_DEVICE_CONNECTED:
            printf("%s: Device connected during setup\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_STARTED:
            printf("%s: Setup process started\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_SUCCESSFUL:
            printf("%s: Setup process successful\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_FAILED:
            printf("%s: Setup process failed\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_CONNECTED:
            printf("%s: Network connected\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_DISCONNECTED:
            printf("%s: Network disconnected\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STARTED:
            printf("%s: OTA update started\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STOPPED:
            printf("%s: OTA update stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_READY:
            printf("%s: Device is ready\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_START:
            printf("%s: Identification started\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_STOP:
            printf("%s: Identification stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_LOW_CODE:
            printf("%s: Low code test mode triggered, subtype: %d\n", TAG,
                   (int)*((int *)(event->event_data)));
            break;
        case LOW_CODE_EVENT_TEST_MODE_COMMON:
            printf("%s: Common test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_BLE:
            printf("%s: BLE test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_SNIFFER:
            printf("%s: Sniffer test mode triggered\n", TAG);
            break;
        default:
            printf("%s: Unhandled event type: %d\n", TAG, event->event_type);
            break;
    }

    return 0;
}
