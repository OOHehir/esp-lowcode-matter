# LowCode Components

| Component Name          | Description                                                                                                  |
|-------------------------|--------------------------------------------------------------------------------------------------------------|
| button                  | Button input driver component supporting both HP (High Power) and LP (Low Power) GPIO configurations         |
| display_ssd1306         | OLED display driver component for SSD1306 using I2C interface, supporting text and basic graphics rendering  |
| light                   | PWM and WS2812 light control component supporting various channel combinations (RGB, RGBCW, etc.)            |
| low_code                | Core low code implementation component                                                                       |
| low_code_transport      | Communication transport layer component for data exchange between the cores                                  |
| sw_timer                | Software timer implementation for LP core with support for periodic and one-shot timers                      |
| occupancy_sensor_ld2420 | Occupancy Sensor LD2420 component, uses UART driver for detecting occupancy                                  |
| pressure_sensor_wsen_pdus | Pressure sensor component for the Würth Elektronik WSEN-PDUS using I2C driver, reporting pressure in 0.1 kPa units |
| relay                   | GPIO based relay control driver component                                                                    |
| system                  | System utilities component providing GPIO, timing, and basic system functions for LP core                    |
| temperature_sensor_sht30 | Temperature sensor component for SHT30 using I2C driver for accurate ambient temperature readings           |

## Related Documents

* [LowCode Drivers](../drivers/README.md)
* [Create LowCode Product](../docs/create_product.md)
* [Products](../products/README.md)
* [All Documents](../docs/all_documents.md)
