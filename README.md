# ESP LowCode Matter: Pressure Sensor

A Matter-compatible differential pressure sensor built on [Espressif's ESP LowCode](https://github.com/espressif/esp-lowcode-matter) framework. This fork adds a **Würth Elektronik WSEN-PDUS** (±0.1 kPa) pressure sensing product to the LowCode product catalogue.

## What This Project Does

- Reads differential pressure from a WSEN-PDUS sensor over I2C every 5 seconds
- Reports measurements to the Matter fabric via the Pressure Measurement cluster (0x0403)
- Runs on an ESP32-C6 with the LowCode runtime (no full ESP-IDF application needed)
- Supports commissioning into Apple Home, Google Home, Amazon Alexa, Home Assistant, Samsung SmartThings

## Hardware

| Component  | Detail                                                                                              |
|------------|-----------------------------------------------------------------------------------------------------|
| **MCU**    | ESP32-C6 (ESP32-C6-MINI-1 module)                                                                   |
| **Sensor** | [WSEN-PDUS](https://www.we-online.com/en/components/products/WSEN-PDUS) 2511020213301, ±0.1 kPa     |
| **LED**    | Single-channel PWM (cold white) for status indication                                               |
| **Button** | Boot button for factory reset (long press >5 s)                                                     |

### Pin Assignment

| Peripheral       | Signal | GPIO  |
|------------------|--------|-------|
| I2C (WSEN-PDUS)  | SCL    | GPIO1 |
| I2C (WSEN-PDUS)  | SDA    | GPIO2 |
| PWM LED          | Cold   | GPIO8 |
| Button           | Input  | GPIO9 |

> GPIOs can be changed via macros in [app_driver.cpp](products/pressure_sensor/main/app_driver.cpp).

## Getting Started

This project uses the same LowCode development workflow as upstream. You can develop in the browser via GitHub Codespaces or locally.

### Codespace (Browser)

> Requires a Chromium-based browser (Chrome, Edge, etc.)

1. Go to <https://github.com/OOHehir/esp-lowcode-matter/> and sign in to GitHub
2. Click **Code** -> **Codespaces** -> **Create Codespace on Main (+)**
3. Wait ~5 minutes for the environment to set up (it will restart a few times)
4. Wait for the `LowCode is Ready` message in the terminal

### Local Development

- [Terminal setup](./docs/getting_started_terminal.md)
- [VS Code setup](./docs/getting_started_vscode.md)

### Build & Flash

Use the status bar buttons (left to right) or the command palette (`Ctrl/Cmd + Shift + P`, prefix `Lowcode:`):

1. **Select Product** -> choose `pressure_sensor`
2. **Select Chip** -> ESP32-C6 + network type (WiFi or Thread)
3. **Select Port** -> connect your board via USB
4. **Prepare Device** -> erases flash and loads LowCode runtime
5. **Upload Configuration** -> generates certificates and QR code
6. **Upload Code** -> builds, flashes, and runs

Your device is now a commissioned Matter pressure sensor.

## How It Works

### Matter Data Model

| Field       | Value                         |
|-------------|-------------------------------|
| Device Type | Pressure Sensor (0x0305)      |
| Cluster     | Pressure Measurement (0x0403) |
| Attribute   | MeasuredValue (0x0000)        |
| Unit        | 0.1 kPa (int16)               |

### Pressure Conversion

The raw 14-bit ADC output maps to pressure as:

```text
P_Pa = (raw / 16383.0) x 200.0 - 100.0
```

This yields ±100 Pa (±0.1 kPa). The Matter MeasuredValue is in 0.1 kPa units, so the reported range is [-1, 0, 1].

### Code Structure

```text
products/pressure_sensor/
  main/
    app_main.cpp          # setup/loop entry point, callback registration
    app_driver.cpp        # sensor init, periodic read, Matter reporting
    app_priv.h            # shared declarations
  configuration/          # ZAP data models (WiFi & Thread), certs
components/
  pressure_sensor_wsen_pdus/  # I2C driver for the WSEN-PDUS sensor
```

See the detailed [pressure sensor product README](products/pressure_sensor/README.md) for the full initialization sequence and extension guide.

## Upstream

This is a fork of [espressif/esp-lowcode-matter](https://github.com/espressif/esp-lowcode-matter). The upstream project includes additional products (lights, sockets, temperature/occupancy sensors) and the full LowCode documentation:

- [Create a Product](./docs/create_product.md)
- [Product Configuration](./docs/product_configuration.md)
- [Programmer's Model](./docs/programmer_model.md)
- [Debugging](./docs/debugging.md)
- [Device Setup & Ecosystems](./docs/device_setup.md)
- [Matter Solutions Comparison](./docs/matter_solutions.md)
- [All Documents](./docs/all_documents.md)

---

Built by Owen O'Hehir — embedded Linux, IoT, Matter & Rust consulting at [electronicsconsult.com](https://electronicsconsult.com). Available for contract and consulting work.
