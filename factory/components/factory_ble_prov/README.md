# ESP32 - WiFi Provisioning over BLE

[![LICENSE](https://img.shields.io/github/license/jvonk/pact)](LICENSE)

## Goal

[The ESP-IDF API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html) provides
> An extensible mechanism to the developers to configure the device with the Wi-Fi credentials and/or other custom configuration using various transports and different security schemes. Depending on the use-case it provides a complete and ready solution for Wi-Fi network provisioning along with example iOS and Android applications. Or developers can extend the device-side and phone-app side implementations to accommodate their requirements for sending additional configuration data.

This component makes this API available as a component using the Bluetooth Low Energy (BLE GATT based) transport scheme. A phone (Android/iOS) is used to communicate with the ESP32 device over BLE.  The phone app can discover and connect to the device without requiring user to go out of the phone app.

## Example

An example can be found in the [`test`](test) directory.

1. Build, and flash the `test/build/factory.bin`.

2. Using the Espressif BLE Provisioning phone app, `scan` and connect to the ESP32.  Then specify the WiFi SSID and password. Depending on the version of the app, you may have to change `_ble_device_name_prefix` to `PROV_` in `test/main/main.c`, and change the `config.service_uuid` in `ble_prov.c`.
Personally, I still use an older customized version of the app.

3. After provisioning the WiFi SSID and password are stored in the `nvs` partition of flash memory.  

## Usage

The software has been tested on the ESP-IDF SDK v4.1-beta2 and v4.3-dev-472-gcf056a7d0 with accompanying tools.

Clone this component in your project's `components` directory, or use it as a git submodule.

Copy the `Kconfig.example` to `Kconfig` and update the configuration.

In your project's `sdkconfig.defaults`, specify  a custom `partition.csv`.  An example of `sdkconfig.defaults` is shown below.
```
# Override some defaults so BT stack is enabled and
CONFIG_BT_ENABLED=y
CONFIG_BTDM_CONTROLLER_MODE_BLE_ONLY=y
CONFIG_BTDM_CONTROLLER_MODE_BR_EDR_ONLY=
CONFIG_BTDM_CONTROLLER_MODE_BTDM=
# factory binary is larger than default size
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=partitions.csv
CONFIG_PARTITION_TABLE_FILENAME=partitions.csv
# need 4 MByte flash for OTA updates
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
```

An example of `partition.csv` that includes `factory` and `nvs` partitions is shown below.

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

Refer to the example in the [`test` directory](test) for details on how to integrate the `ble_prov()` functionality.

To provision the WiFi credentials using a phone app, this `factory` app advertises itself to the phone app.  The Espressif BLE Provisioning app is available as source code or from the app stores:
- [Android](https://play.google.com/store/apps/details?id=com.espressif.provble)
- [iOS](https://apps.apple.com/in/app/esp-ble-provisioning/id1473590141)

The WiFi SSID and password are stored in the `nvs` partition of flash memory.  

## OTA Update

To complete the experience, this provisioning should be combined with an OTA Update mechanism such as my [ESP_OTA-Update](https://github.com/cvonk/ESP32_ota-update-task) repository.  This allows the provisioning to trigger an OTA update of the application code.  

A good place to initiate the OTA update would be after receiving `IP_EVENT_STA_GOT_IP`.

## Feedback

We love to hear from you. Please use the usual Github mechanisms to contact me.

## LICENSE

Copyright 2020 Coert Vonk

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.