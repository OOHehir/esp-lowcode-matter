# Pressure Sensor (I2C Driver) | WSEN-PDUS Sensor

## Description

A differential pressure sensing product using the Würth Elektronik WSEN-PDUS ±0.1 kPa sensor with periodic reporting, factory reset via button, and device state indication through a PWM LED.

* **Periodic Pressure Monitoring**: Measures differential pressure using the I2C WSEN-PDUS sensor and reports the data to the system every 5 seconds.
* **Factory Reset via Button**: Holding the user-configurable button for more than 5 seconds and then releasing it triggers a factory reset event.
* **Device State Indication**: A single-channel PWM LED visually indicates the device's current state — for example, a blinking LED signifies setup mode, and a brief pulse confirms each successful sensor reading.
* **Matter Data Model Specification**:
  * **Device Type**: `Pressure Sensor`
  * **Cluster**: Pressure Measurement (0x0403)
  * **Attribute**: MeasuredValue (0x0000), unit = 0.1 kPa

## Hardware Configuration

The following components are used for this product:

* **MCU**: ESP32-C6 (ESP32-C6-MINI-1 module)
* **Pressure Sensor**: [Würth Elektronik WSEN-PDUS](https://www.we-online.com/en/components/products/WSEN-PDUS) ±0.1 kPa differential pressure sensor (I2C, fixed address 0x78)
* **LED Indicator**: Single-channel PWM LED (cold white)
* **Button**: Onboard boot button or external button

You can use any **ESP32-C6 DevKit** as long as the pin connections match the specified GPIO assignments.

### Pin Assignment

**Note:** The following pin assignments are used by default.

| Peripheral            | Signal | ESP32-C6 GPIO |
|-----------------------|--------|---------------|
| **I2C - WSEN-PDUS**  | SCL    | GPIO1         |
| **I2C - WSEN-PDUS**  | SDA    | GPIO2         |
| **PWM LED**           | Cold   | GPIO8         |
| **Button**            | Input  | GPIO9         |

> **Note**: These GPIOs can be reconfigured by updating the macro definitions in **app_driver.cpp**: `I2C_SCL_IO`, `I2C_SDA_IO`, `LED_GPIO_NUM`, `BUTTON_GPIO_NUM`

## Understanding Code

### Initialization Sequence

The `app_driver_init()` function (called from `setup()` in `app_main.cpp`) handles:

* Configuring the button with debouncing for factory reset on long press
* Setting up the single-channel PWM LED
* Initializing the I2C master bus for the WSEN-PDUS sensor
* Initializing the WSEN-PDUS sensor (soft reset + communication verification)
* Creating a system timer that triggers every 5 seconds and calls `app_driver_read_and_report_feature()`

### Core Functions

Every 5 seconds, the timer callback:

* Reads pressure data from the WSEN-PDUS sensor via `pressure_sensor_wsen_pdus_get_kpa()`
* Converts the reading to a Matter MeasuredValue (int16, units of 0.1 kPa)
* Reports it to the system via `low_code_feature_update_to_system()`
* Briefly pulses the LED to confirm the reading was taken

### Pressure Value Conversion

The raw 14-bit ADC output from the sensor is mapped to pressure in Pa using:

```
P_Pa = (raw / 16383.0) × 200.0 − 100.0
```

This yields a range of ±100 Pa (±0.1 kPa). The value reported to the Matter stack is in units of 0.1 kPa, so the range is [-1, 0, 1].

### Extending with Other I2C Sensors

To add support for any other I2C sensor, you can follow these steps:

* Initialize it in `app_driver_init()`
* Add reading logic in `app_driver_read_and_report_feature()`
* Implement a function to report its data

## Related Documents

* [Temperature Sensor (I2C Driver) | SHT30 Sensor](../temperature_sensor/README.md)
* [Programmer's Model](../../docs/programmer_model.md)
* [Components](../../components/README.md)
* [Products](../README.md)
