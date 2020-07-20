# ESP32 - Over The Air (OTA) Update task

[![LICENSE](https://img.shields.io/github/license/jvonk/pact)](LICENSE)

## Goal

[The ESP-IDF API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html) provides a update mechanism that allows a device to update itself based on data received while the normal firmware is running.

This component makes this API available as a component.  It checks for an OTA update on a network server, and if present (and different) it applies the update.  Upon completion, the device resets to activate the downloaded code.

We use the term "update" loosly, because it can also be used to downgrade the firmware.

## Example

An example can be found in the [`test`](test) directory.

The `WIFI_SSID` and `WIFI_PASSWORD` can be provisioned using the `Kconfig`.  If empty, the device will use credentials from flash memory instead.

Initially the `build/ota_update_test.bin` should be flashed using the UART.  From then on, it will automatically update from file server spedified by `OTA_UPDATE_FIRMWARE_URL` in `Kconfig`.

## Usage

The software relies on the master ESP-IDF SDK version v4.3-dev-472-gcf056a7d0 and accompanying tools.

Clone this component in your project's `components` directory, or use it as a git submodule.

Copy the `Kconfig.example` to `Kconfig` and update the configuration.

In your project's `sdkconfig.defaults`, specify  a custom `partition.csv` that includes a `coredump` partition.  An example of `sdkconfig.defaults` is shown below.
```
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
```

An example of `partition.csv` is shown below.

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,      0x09000,  0x004000,
otadata,  data, ota,      0x0d000,  0x002000,
phy_init, data, phy,      0x0f000,  0x001000,
factory,  app,  factory,  0x010000, 0x150000,
ota_0,    app,  ota_0,    0x160000, 0x140000,
ota_1,    app,  ota_1,    0x2A0000, 0x140000,
coredump, data, coredump, 0x3E0000, 128k
```

Before start the component's task, call `nvs_flash_init()`, and establish WiFi connectivity.  Then create the `ota_update_task` using
```
xTaskCreate(&ota_update_task, "ota_update_task", 4096, NULL, 5, NULL);
```

The OTA server should support either HTTP or HTTPS.  If you WiFi name and password are provisioned using `Kconfig` it will be part of the binary, so you should be more comfortable using local server.

To determine if the currently running code is different as the code on the server, it compares the project name, version, date and time.  Note that these are not always updated by the SDK.  The best way to make sure they are updated is by committing your code to Git and building the project from scratch.

## Feedback

We love to hear from you. Please use the usual Github mechanisms to contact me.

## LICENSE

Copyright 2020 Coert Vonk

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.