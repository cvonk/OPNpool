# ESP32 - Factory Reset WiFi Credentials task

[![LICENSE](https://img.shields.io/github/license/jvonk/pact)](LICENSE)

## Goal

This component is a supplement to:
  - the [BLE Provisioning[(https://github.com/cvonk/ESP32_ble-prov) component, and
  - the [OTA Update Task](https://github.com/cvonk/ESP32_ota-update-task) component.

After pressing the RESET/BOOT button for 3 seconds, it clears the provisioned WiFi credentials and OTA Updates from flash memory, .  This allows the device to start with code from the factory partition so it can be provisioned for a different WiFi SSID or password.

## Example

An example can be found in the [`test`](test) directory.

1. Compile this `test` application, and upload the `.bin` to your OTA Update server.

2. For the `factory` application, start with the example for [BLE Provisioning[(https://github.com/cvonk/ESP32_ble-prov) component and add the [OTA Update Task](https://github.com/cvonk/ESP32_ota-update-task) component.  Follow the steps in those examples.  This will leave the WiFi SSID and password stored in the `nvs` partition of flash memory.  As the last step it will download this example's `.bin` as an OTA Update.

2. Press the RESET/BOOT button for 3 seconds, to clears the provisioned WiFi credentials and OTA Updates from flash memory, .  This allows the device to start with code from the factory partition so it can be provisioned for a different WiFi SSID or password.

## Usage

The software relies on the master ESP-IDF SDK version v4.3-dev-472-gcf056a7d0 and accompanying tools.

Clone this component in your project's `components` directory, or use it as a git submodule.

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

Before start the component's task, call `nvs_flash_init()`.  Then create the `factory_reset_task` using
```
xTaskCreate(&factory_reset_task, "factory_reset_task", 4096, NULL, 5, NULL);
```

## Feedback

We love to hear from you. Please use the usual Github mechanisms to contact me.

## LICENSE

Copyright 2020 Coert Vonk

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.