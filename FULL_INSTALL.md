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

If there is enough interest, I can start a project on Tindie or Crowd Supply to get some bulk pricing.

### Schematic

A buck converter provides 5 Volts to the battery connector on the LOLIN D32 daughterboard. Using the battery input, helps prevent problems when it is also powered through the USB connector.

![Schematic1](https://coertvonk.com/wp-content/uploads/opnpool-hardware-schematic-power.svg)

The data path is between the RS-485 connector and the ESP32 on the LOLIN D32 daughterboard.  There is an optional terminator resistor to prevent reflections on the bus. The JTAG header is for debugging as detailed in the Debugging chapter of the design document.

![Schematic2](https://coertvonk.com/wp-content/uploads/opnpool-hardware-schematic-logic.svg)

### Board layout

The schematic fits easily on a two layer PCB. Note the cut out for the RF antenna.

![Board layout](https://coertvonk.com/wp-content/uploads/opnpool-hardware-layout.svg)

### Bill of materials

| Name        | Description                                             | Sugggested mfr/part#       |
|-------------|---------------------------------------------------------|----------------------------|
| PBC r2      | Printed circuit board                                   | [OSHPark nS1z3Duu](https://oshpark.com/shared_projects/nS1z3Duu) [^1] |
| Enclosure   | 158x90x60mm clear plastic project enclosure, IP65       | *white label*              |
| LOLIN D32   | Wemos LOLIN D32, based on ESP-WROOM-32 4MB</a>          | Wemos LOLIN-D32            |
| RS485_CONN  | Plug+socket, male+female, 5-pin, 16mm aviation, IP68    | SD 16                      | 
| MAX3485     | Maxim MAX3485CSA, RS-485/UART interface IC 3.3V, 8-SOIC | Analog-Devices MAX3490ECSA |
| DC1         | DC/DC Converter R-78E5.0-0.5, 7-28V to 5V, 0.5A, 3-SIP  | RECOM-Power R-78E5.0-0.5   |
| D1          | Schottky Diode, 1N5818, DO-41                           | ON-Semiconductor 1N5818RLG |
| LED1        | LED, Green Clear 571nm, 1206                            | Lite-On LTST-C150KGKT      |
| LED2        | LED, Amber Clear 602nm, 1206                            | Lite-On LTST-C150AKT       |
| C1, C2      | Capacitor, 10 µF, 25 V, multi-layer ceramic, 0805       | KEMET C0805C106K3PACTU     |
| C3          | Capacitor, 0.1 µF, 6.3 V, multi-layer ceramic, 0805     | KEMET C0805C104M3RACTU     |
| R1, R2      | Resistor, 68 Ω, 1/8 W, 0805                             | YAGEO RC0805FR-0768RL      |
| R3          | Not stuffed, resistor, 120 Ω, 1/4 W, 0805               | KAO SG73S2ATTD121J         |
| RS485-TERM  | Fixed terminal block, 4-pin, screwless, 5 mm pitch      | Phoenix-Contact 1862291    |
| SW1         | Tactile Switch, 6x6mm, through hole                     | TE-Connectivity 1825910-4  |
| PCB Screws  | Machine screw, #6-32 x x 3/16", panhead                 | Keystone-Electronics 9306  |
| CONN Screws | Machine screw, M2-0.4 x 16 mm, cheese head              | Essentra 50M020040D016     |
| CONN Nuts   | Hex nut, M2-0.4, nylon                                  | Essentra 04M020040HN       |

[^1]: We shared our [PCB order](https://oshpark.com/shared_projects/nS1z3Duu) for your convienence.

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
  * Click on the "Provision" button and grant it access [^2].
  * Click on the name of the OPNpool device one it is detected (`POOL*`).
  * Select the Wi-Fi SSID to connect to and give it the password.
  * If you don't have a MQTT broker press Skip.  Otherwise, specify the broker URL in the format `mqtt://username:passwd@host.domain:1883`.
  * Wait a few minutes for the provisioning to complete.

[^2]: Precise location permission is needed to find and connect to the OPNpool device using Bluetooth LE.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=tHYNv9jL5MY" target="_blank">
 <img src="http://img.youtube.com/vi/tHYNv9jL5MY/mqdefault.jpg" alt="Watch the video" width="960" border="0" />
</a>

The device will appear on your network segment as `opnpool.local`.  You can access its web interface through `http://pool.local`. If MQTT is configured, it will publish MQTT messages. If you also use the [Home Assistant](https://www.home-assistant.io/), entities will appear after a few minutes with  `.opnpool` in their name.

## Connect

> :warning: **THIS PROJECT IS OFFERED AS IS. IF YOU USE IT YOU ASSUME ALL RISKS. NO WARRENTIES. At the very least, turn off the power while you work on your pool equipment. Be careful, THERE IS ALWAYS A RISK OF BREAKING YOUR POOL EQUIPMENT.**

Understanding the above warning .. the RS-485 header can be found on the back of the control board. There are probably already wires connected that go to the devices such as pump and chlorinator.

![Inside of Pool controller](assets/media/opnpool-rs485-inside.jpg)

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

### Web UI

The web UI at `http://opnpool.lcoal/`, will show the pool state and allow you to change the thermostat and circuits.

![Web UI](assets/media/opnpool-web-ui-pool-therm-sml.png)

### Home Assistant

If you are using Home Assistant, the `*.opnpool*` entities will update automatically. The `hassio` directory has some YAML code to use with the Lovelace dashboard.

If you go that route, also remember to install `modcard`, `button-card`, `bar-card`, `simple-thermostat`, `template-entity-row` and `mini-graph-card` available through the Home Assistant Community Store (HACS).

![Hassio UI](assets/media/opnpool-readme-hassio-lovelace.png)

### MQTT publish

When the path to the MQTT broker is provisioned, OPNpool will publish its state to topics

| topic                                                                   | values      |
| ------------------------------------------------------------------------|-------------|
| `homeassistant/switch/opnpool/pool_circuit/state`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/spa_circuit/state`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/aux1_circuit/state`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/aux2_circuit/state`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/aux3_circuit/state`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft1_circuit/state`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft2_circuit/state`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft3_circuit/state`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft4_circuit/state`             | (`ON`\|`OFF`) |
| `homeassistant/climate/opnpool/pool_heater/available`        | (`online`\|`offline`) |
| `homeassistant/climate/opnpool/pool_heater/state`            | *see below* |
| `homeassistant/climate/opnpool/spa_heater/available`         | (`online`\|`offline`) |
| `homeassistant/climate/opnpool/spa_heater/state`             | *see below*         |
| `homeassistant/sensor/opnpool/pool_sched/state`              | hh:mm - hh:mm |
| `homeassistant/sensor/opnpool/spa_sched/state`               | hh:mm - hh:mm |
| `homeassistant/sensor/opnpool/aux1_sched/state`              | hh:mm - hh:mm |
| `homeassistant/sensor/opnpool/aux2_sched/state`              | hh:mm - hh:mm |
| `homeassistant/sensor/opnpool/air_temp/state`                | *integer* |
| `homeassistant/sensor/opnpool/water_temp/state`              | *integer* |
| `homeassistant/sensor/opnpool/system_time/state`             | *integer* |
| `homeassistant/sensor/opnpool/ctrl_version/state`            | *integer* |
| `homeassistant/sensor/opnpool/if_version/state`              | *integer* |
| `homeassistant/sensor/opnpool/pump_mode/state`               | (`FILTER`\|`MAN`\|`BKWASH`\| `EP1`\|`EP2`\|`EP3`\|`EP4`) |
| `homeassistant/sensor/opnpool/pump_status/state`             | *integer* |
| `homeassistant/sensor/opnpool/pump_power/state`              | *integer* |
| `homeassistant/sensor/opnpool/pump_gpm/state`                | *integer* |
| `homeassistant/sensor/opnpool/pump_speed/state`              | *integer* |
| `homeassistant/sensor/opnpool/pump_error/state`              | *integer* |
| `homeassistant/sensor/opnpool/chlor_name/state`              | *string* |
| `homeassistant/sensor/opnpool/chlor_pct/state`               | *integer* |
| `homeassistant/sensor/opnpool/chlor_salt/state`              | *integer* |
| `homeassistant/sensor/opnpool/chlor_status/state`            | (`OK`\|`LOW_FLOW`\|`LOW_SALT`\| `HIGH_SALT`\|`COLD`\|`CLEAN_CELL`\| `OTHER`) |
| `homeassistant/binary_sensor/opnpool/pump_running/state`     | (`ON`\|`OFF`) |
| `homeassistant/binary_sensor/opnpool/mode_service/state`     | (`ON`\|`OFF`) |
| `homeassistant/binary_sensor/opnpool/mode_temp_inc/state`    | (`ON`\|`OFF`) |
| `homeassistant/binary_sensor/opnpool/mode_freeze_prot/state` | (`ON`\|`OFF`) |
| `homeassistant/binary_sensor/opnpool/mode_timeout/state`     | (`ON`\|`OFF`) |

The thermostat state is a JSON string, for example
```json
{
    "mode": "heat",    // static
    "heatsrc": "None", // None/Heater/SolarPref/Solar
    "target_temp": 70,
    "current_temp": 67,
    "action": "off"    // off/heating/idle
}
```

### MQTT subscribe

When the path to the MQTT broker is provisioned, OPNpool will subscribe to the topics listed below

| topic                                                                   | accepts     |
| ------------------------------------------------------------------------|-------------|
| `homeassistant/switch/opnpool/pool_circuit/config`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/spa_circuit/config`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/aux1_circuit/config`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/aux2_circuit/config`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/aux3_circuit/config`            | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft1_circuit/config`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft2_circuit/config`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft3_circuit/config`             | (`ON`\|`OFF`) |
| `homeassistant/switch/opnpool/ft4_circuit/config`             | (`ON`\|`OFF`) |
| `homeassistant/climate/opnpool/pool_heater/set_temp`          | *integer*     |
| `homeassistant/climate/opnpool/pool_heater/set_heatsrc`       | (`None`\|`Heater`\|`SolarPref`\|`Solar`) |

### Hosting the Web UI

To host the Web UI yourself, copy the files from `webui` to your web server. Then reflect this change in the file `interface/main/httpd/httpd_root.c`.

## Design docs

The design documentation for this project is available at
> [https://coertvonk.com/category/sw/embedded/opnpool-design](https://coertvonk.com/category/sw/embedded/opnpool-design)

It includes the chapters

- [Introduction](https://coertvonk.com/sw/embedded/opnpool-design/introduction-2-11554)
- [RS-485 bus](https://coertvonk.com/sw/embedded/opnpool-design/bus-access-31957)
- [Hardware](https://coertvonk.com/sw/embedded/opnpool-design/hardware-3-31959)
- [Tools](https://coertvonk.com/sw/embedded/opnpool-design/tools-31961)
- [Debugging](https://coertvonk.com/sw/embedded/opnpool-design/debugging-31963)
- [Protocol](https://coertvonk.com/sw/embedded/opnpool-design/protocol-31965)
- [Deploying](https://coertvonk.com/sw/embedded/opnpool-design/deploying-31984)
- [Interface](https://coertvonk.com/sw/embedded/opnpool-design/interface-software-31967)
- [Web UI](https://coertvonk.com/sw/embedded/opnpool-design/web-ui-32000)
- [Home Automation](https://coertvonk.com/sw/embedded/opnpool-design/home-automation-32008)
