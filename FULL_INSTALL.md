# OPNpool Full Install

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## Functionality

As mentioned in the  [`README.md`](README.md) The OPNpool integrates the traditional Pool Controller into our smart home. It keeps a tab on the status of the controller, the pool pump and the chlorinator. It is not only more convenient than walking over to the pool equipment, but will also tell you about problems with e.g. the chlorinator or pump. 

Features:

  - [x] Visualizes the status of the thermostats, pump, chlorinator, circuits and schedules.
  - [x] Lets you change the thermostats and switch circuits
  - [x] Over-the-air updates
  - [x] One time provisioning from a phone
  - [x] MQTT and Home Assistant integration
  - [x] Web UI
  - [x] IP68 waterproof case and connectors
  - [x] No power adapter required
  - [x] Open source!

OPNpool was tested with the Pentair SunTouch controller running firmware 2.080, connected to an IntelliFlo pump and IntelliChlor salt water chlorinator.

> This open source and hardware project is intended to comply with the October 2016 exemption to the Digital Millennium Copyright Act allowing "good-faith" testing," in a controlled environment designed to avoid any harm to individuals or to the public.

If you only want to take this project for a quick spin, refer to the [`README.md`](README.md) instead. The remainder of this document will walk you through the full installation.

## Hardware

We will build a printed circuit board (PCB) with an ESP32 module, RS-485 adapter and DC/DC converter.

### Schematic

A buck converter provides 5 Volts to the battery connector on the LOLIN D32 daughterboard. Using the battery input, helps prevent problems when it is also powered through the USB connector.

![Schematic1](https://coertvonk.com/wp-content/uploads/opnpool-hardware-schematic-power.svg)

The data path is between the RS-485 connector and the ESP32 on the LOLIN D32 daughterboard.  There is an optional terminator resistor to prevent reflections on the bus. The JTAG header is for debugging as detailed in the Debugging chapter of the design document.

![Schematic2](https://coertvonk.com/wp-content/uploads/opnpool-hardware-schematic-logic.svg)

### Board layout

The schematic fits easily on a two layer PCB. Note the cut out for the RF antenna.

![Board layout](https://coertvonk.com/wp-content/uploads/opnpool-hardware-layout.svg)

### Bill of materials

| Name | Description | Price |
|------|-------------|-------|
| LOLIN D32 | [Wemos LOLIN D32, ESP-WROOM-32 4MB](https://wiki.wemos.cc/products:d32:d32") | [$6.50](https://www.aliexpress.com/item/WEMOS-LOLIN32-V1-0-0-wifi-bluetooth-board-based-ESP-32-4MB-FLASH/32808551116.html") |
| MAX3485CSA+ | Maxim MAX3485CSA+, RS-485/UART interface IC 3.3V | [$2.89](http://www.mouser.com/ProductDetail/Maxim-Integrated/MAX3485CSA+/) |
| DC1 | DC/DC Converter R-78E5.0-0.5, 5V, 0.5A | [$2.70](http://www.mouser.com/ProductDetail/RECOM-Power/R-78E50-05/) |
| D2 |  Rectron Schottky Diode, 1N5818 (or MBRS130 1A 0.3V SMB) | [$0.14](https://www.mouser.com/ProductDetail/ON-Semiconductor-Fairchild/SS12) |
| C1, C2 | Capacitor, 10 µF, 25 V, multi-layer ceramic, 0805 | 2* [$0.34](https://www.mouser.com/ProductDetail/TDK/C2012X5R1E106K125AB) |
| C3 | Capacitor, 0.1 µF, 6.3 V, multi-layer ceramic, 0805 | [$0.22](https://www.mouser.com/ProductDetail/KEMET/C0805C104K9RACTU) |
| R1 | Not stuffed, resistor, 120 Ω, 1/4 W, 0805 | [$0.10](https://www.mouser.com/ProductDetail/660-RK73H2ATTD1200F) |
| RS485-TERM-5MM | Fixed terminal block, 4-pin, screwless, 5 mm pitch | [$1.75](https://www.mouser.com/ProductDetail/Phoenix-Contact/1864451)
| RS485_CONN | Plug and socket, 5-pin, SD16-5, IP68 waterproof | [$3.29](https://www.aliexpress.com/item/SD16-16mm-5-Pin-Flange-Waterproof-Aviation-Connector-Plug-Socket-Straight-IP68/32401149735.html) |
| PBC | Printed circuit board | [$9.43](https://oshpark.com/shared_projects/CFBihnoN) |
| Enclosure | 150x85x35mm clear waterproof project enclosure | [$6.99](https://www.dx.com/s/492680) |

I left the link to my PCB order in with the price for your convienence.



At the core this project is an ESP32 module and a 3.3 Volt RS-485 adapter. You can breadboard this using:

* Any ESP32 module that has an USB connector and `GPIO#25` to `GPIO#27` available (such as the Wemos LOLIN D32).
* Any "Max485 Module TTL". To make it 3.3 Volt compliant, change the chip to a MAX3485CSA+. While you're at it, you may as well remove the 10 kΩ pullup resistors (`R1` to `R4`).
* A piece of Cat5 ethernet cable to connect to the pool controller.

## Software

The open software is hosted on GitHub.

> We proudly acklowledge the work of reverse engineering pioneers [Joshua Bloch](https://docs.google.com/document/d/1M0KMfXfvbszKeqzu6MUF_7yM6KDHk8cZ5nrH1_OUcAc/edit), [Michael Russe](http://cocoontech.com/forums/files/file/173-pab014sharezip/), and [George Saw](http://cocoontech.com/forums/topic/27864-download-pitzip/). (Drop me a line if if I forgot you.)

### Git

Clone the repository and its submodules to a local directory. The `--recursive` flag automatically initializes and updates the submodules in the repository.  Start with a fresh clone:

```bash
git clone --recursive https://github.com/cvonk/OPNpool.git
```

or using `ssh`
```bash
git clone --recursive git@github.com:cvonk/OPNpool.git
```

### IDE

From within Microsoft Visual Code (VScode), add the [Microsoft's C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools). Then add the [Espressif IDF extension &ge;4.4](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension). ESP-IDF will automatically start its configuration. Answer according to the table below

| Question     | Choice             |
|--------------| ------------------ |
| Mode         | Advanced           |
| ESP-IDF path | `C:/espressif`     |
| Tools        | `C:/espressif/bin` |
| Download     | yes                |

### Boot steps

As usual, the `bootloader` image does some minimum initializations. If it finds a valid `ota` image, it passes control over to that image. If not, it starts the `factory` image.

  - The `factory` image takes care of provisioning Wi-Fi and MQTT credentials with the help of a phone app. These credentials are stored in the `nvs` partition. It then downloads the `ota` image, and restarts the device.
  - We refer to the `ota` image as the `interface`, as it provides the core of the functionality of the OPNpool device.

### The `interface` image

To host your own `interface` image, you will need to place it on your LAN or on the Web. If you're fine using my logiciel du jour, you can skip this section.

> To use HTTPS, you will need to add the server's public certificate to `interface/components/ota_update_task/CMakelists.txt`, and uncomment some lines in `ota_update_task.c` with `server_cert_pem`.

From VScode:

  * Change to the `OPNpool/interface` folder.
  * Connect your ESP32 module, and once more select the serial port (`^e p`) 
  * Edit the SDK configuration (`^e g`) and scroll down to OPNpool and specify your "Firmware upgrade url endpoint" (e.g. http://host.domain/path/interface.bin).
  * Start the build cycle (`^e b`).
  * Upload `OPNpool/interface/build/interface.bin` to your site.

### The `factory` image

We will build the `factory` image and provision it using an Android phone app.

> If you have an iPhone, or you have problems running the Android app, you can extend `esp_prov.py` to include `mqtt_url` similar to what is shown [here](https://github.com/espressif/esp-idf-provisioning-android/issues/11#issuecomment-586973381). Sorry, I don't have the iOS development environment.

In the last step of provisioning, this `factory` image will download the `interface` image from an external site. If you rather use your own site, you need to build the `interface` image first as shown later in this document.

From VScode:

  * Change to the `OPNpool/factory` folder.
  * Connect your ESP32 module, and select the serial port (`^e p`)
  * If you built and host your own `interface` image, you need to specify the path by editing the SDK configuration (`^e g`) and scroll down to OPNpool and specify your "Firmware upgrade url endpoint" (e.g. http://host.domain/path/interface.bin).
  * Start the build-upload-monitor cycle (`e d`).

Using an Android phone:

  * Install and run the OPNpool app from the [Play Store](https://play.google.com/store/apps/details?id=com.coertvonk.opnpool).
  * Using the overflow menu, select "Provision device".
  * Click on the "Provision" button and grant it access [^1].
  * Click on the name of the OPNpool device one it is detected (`POOL*`).
  * Select the Wi-Fi SSID to connect to and give it the password.
  * If you don't have a MQTT broker press Skip.  Otherwise, specify the broker URL in the format `mqtt://username:passwd@host.domain:1883`.
  * Wait a few minutes for the provisioning to complete.

[^1]: Precise location permission is needed to find and connect to the OPNpool device using Bluetooth LE.

The device will appear on your network segment as `opnpool.local`.  You can access its web interface through `http://pool.local`. If MQTT is configured, it will publish MQTT messages. If you also use the [Home Assistant](https://www.home-assistant.io/), entities will appear after a few minutes with  `.opnpool` in their name.

## Connect

> :warning: **PROCEED AT YOUR OWN RISK! At the very least, turn off the power while you work on your pool equipment. Be careful, THERE IS ALWAYS A RISK OF BREAKING YOUR POOL EQUIPMENT.**

Understanding the above warning .. the RS-485 header can be found on the back of the control board. There are probably already wires connected that go to the devices such as pump and chlorinator.

To minimize electromagnetic interference, use a twisted pairs from e.g. CAT-5 cable to connect the `A`/`B` pair to the RS-485 adapter as shown in the table below.

| Controller       | PCB  | idle state |         
|:-----------------|:----:|:-----------|
| `-DATA` (green)  |  `A` | negative   |
| `+DATA` (yellow) |  `B` | positive   |

In VSCode, the serial monitor will show decoded messages such as:

```json
I (16670) poolstate_rx: {CTRL_STATE_BCAST: {
    "state":{"system":{"tod":{"time":"14:01","date":"2022-04-05"},"firmware":"v0.000"},"temps":{"air":69,"solar":80},
    "thermos":{"pool":{"temp":68,"src":"None","heating":false},"spa":{"temp":69,"src":"None","heating":false}},
    "scheds":{"pool":{"start":"08:00","stop":"10:00"}},
    "modes":{"service":false,"UNKOWN_01":false,"tempInc":false,"freezeProt":false,"timeout":false},
    "circuits":{"active":{"spa":false,"aux1":false,"aux2":false,"aux3":false,"ft1":false,"pool":true,"ft2":false,"ft3":false,"ft4":false},"delay":{"spa":false,"aux1":false,"aux2":false,"aux3":false,"ft1":false,"pool":false,"ft2":false,"ft3":false,"ft4":false}}}}
}
```

The web UI, will show the pool state and allow you to change the thermostat and circuits.

![Web UI](media/opnpool-web-ui-pool-therm-sml.png)

If you are using Home Assistant, the `*.opnpool*` entities will update automatically. The `hassio` directory has some YAML code to use with the Lovelace dashboard.

If you go that route, also remember to install `modcard`, `button-card`, `bar-card`, `simple-thermostat`, `template-entity-row` and `mini-graph-card` available through the Home Assistant Community Store (HACS).

![Hassio UI](media/opnpool-readme-hassio-lovelace.png)