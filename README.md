# OPNpool

[![GitHub Discussions](https://img.shields.io/github/discussions/cvonk/OPNpool)](https://github.com/cvonk/OPNpool/discussions)
![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/cvonk/OPNpool?include_prereleases&logo=DocuSign&logoColor=%23fff)
![GitHub package.json dependency version (prod)](https://img.shields.io/github/package-json/dependency-version/cvonk/OPNpool/esp-idf)
![GitHub](https://img.shields.io/github/license/cvonk/OPNpool)

The OPNpool integrates the functionality of a traditional Pool Controller into the modern smart home. It keeps tabs on the status of the connected controller, pool pump and chlorinator. This provides not only a more convenient solution than physically interacting with the pool equipment, but the ability to create automations that runs the pump for a duration depending on the temperature.

Features:

  - [x] Visualizes the status of the thermostats, pump, chlorinator, circuits, and schedules.
  - [x] Allows you adjust the thermostats and toggle circuits
  - [x] No physical connection to your LAN
  - [x] Supports over-the-air updates [^1]
  - [x] Easily one-time provisioning from an Android phone [^1]
  - [x] Integrates with MQTT and Home Assistant
  - [x] Accessible as a webapp
  - [x] Protected with IP68 waterproof case and connectors [^1]
  - [x] Does not require a power adapter [^1]
  - [x] Open source!

[^1]: Available with the full install as described in [`FULL_INSTALL.md`](FULL_INSTALL.md)

This device was tested with the Pentair SunTouch controller with firmware 2.080, connected to an IntelliFlo pump and IntelliChlor salt water chlorinator.

> This open source and hardware project is intended to comply with the October 2016 exemption to the Digital Millennium Copyright Act allowing "good-faith" testing," in a controlled environment designed to avoid any harm to individuals or to the public.

The full fledged project installation method is described in the [`FULL_INSTALL.md`](FULL_INSTALL.md). Before you go down that road, you may want to give it a quick spin to see what it can do. The remainder of this README will walk you through this.

## Parts

At the core this project is an ESP32 module and a 3.3 Volt RS-485 adapter. You can breadboard this using:

* Any ESP32 module that has an USB connector and `GPIO#25` to `GPIO#27` available (such as the Wemos LOLIN D32).
* Any "Max485 Module TTL". To make it 3.3 Volt compliant, change the chip to a MAX3485CSA+. While you're at it, you may as well remove the 10 kΩ pullup resistors (`R1` to `R4`).
* A piece of Cat5 ethernet cable to connect to the pool controller.

## Build

> We proudly acknowledge the work of reverse engineering pioneers [Joshua Bloch](https://docs.google.com/document/d/1M0KMfXfvbszKeqzu6MUF_7yM6KDHk8cZ5nrH1_OUcAc/edit), [Michael Russe](http://cocoontech.com/forums/files/file/173-pab014sharezip/), and [George Saw](http://cocoontech.com/forums/topic/27864-download-pitzip/). (Drop me a line if if I forgot you.)

Clone the repository and its submodules to a local directory. The `--recursive` flag automatically initializes and updates the submodules in the repository.

```bash
git clone --recursive https://github.com/cvonk/OPNpool.git
```

or using `ssh`
```bash
git clone --recursive git@github.com:cvonk/OPNpool.git
```

From within Microsoft Visual Code (VScode), add the [Microsoft's C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools). Then add the [Espressif IDF extension &ge;4.4](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension). ESP-IDF will automatically start its configuration. Answer according to the table below

| Question     | Choice             |
|--------------| ------------------ |
| Mode         | Advanced           |
| ESP-IDF path | `C:/espressif`     |
| Tools        | `C:/espressif/bin` |
| Download     | yes                |

From VScode:

  * Change to the `OPNpool/interface` folder.
  * Connect your ESP32 module, and configure the device and COM port (press the F1-key and select "ESP-IDF: Device configuration")
  * If you are using ESP-IDF version 5.0 or greater you need to add the mDNS component by running ```idf.py add-dependency espressif/mdns``` in the terminal.
  * Edit the configuration (press the F1-key, select "ESP-IDF: SDK configuration editor" and scroll down to OPNpool)
      * Select "Use hardcoded Wi-Fi credentials" and specify the SSID and password of your Wi-Fi access point.
      * If you have a MQTT broker set up, select "Use hardcoded MQTT URL" and specify the URL in the format `mqtt://username:passwd@host.domain:1883`
  * Start the build-upload-monitor cycle (press the F1-key and select "ESP-IDF: Build, Flash and start a monitor on your device").

The device will appear on your network segment as `opnpool.local`.  You can access its web interface through `http://pool.local`. If MQTT is configured, it will publish MQTT messages. If you also use the [Home Assistant](https://www.home-assistant.io/), entities will appear after a few minutes with  `.opnpool` in their name.

### Connect

> :warning: **THIS PROJECT IS OFFERED AS IS. IF YOU USE IT YOU ASSUME ALL RISKS. NO WARRENTIES. At the very least, turn off the power while you work on your pool equipment. Be careful, THERE IS ALWAYS A RISK OF BREAKING YOUR POOL EQUIPMENT.**

Understanding the above warning .. the RS-485 header can be found on the back of the control board. There are probably already wires connected that go to the devices such as pump and chlorinator.

![Inside of Pool controller](assets/media/opnpool-rs485-inside.jpg)

To minimize electromagnetic interference, use a twisted pairs from e.g. CAT-5 cable to connect the `A`/`B` pair to the RS-485 adapter as shown in the table below.

| Controller       | RS-485 adapter | idle state |         
|:-----------------|:--------------:|:-----------|
| `-DATA` (green)  |  `A`           | negative   |
| `+DATA` (yellow) |  `B`           | positive   |

Connect the RS-485 adapter to the ESP32 module.  I also pulled `GPIO#27` down with a 10 k&ohm; resistor, to keep it from transmiting while the ESP32 is booting.

| RS-485 adapter | ESP32 module |
|:---------------|:-------------|
| RO             | `GPIO#25`    |
| DI             | `GPIO#26`    |
| DE and RE      | `GPIO#27`    |
| GND            | `GND`        |

The serial monitor will show decoded messages such as:

```json
{
    "state":{"system":{"tod":{"time":"14:01","date":"2022-04-05"},"firmware":"v0.000"},"temps":{"air":69,"solar":80},
    "thermos":{"pool":{"temp":68,"src":"None","heating":false},"spa":{"temp":69,"src":"None","heating":false}},
    "scheds":{"pool":{"start":"08:00","stop":"10:00"}},
    "modes":{"service":false,"UNKOWN_01":false,"tempInc":false,"freezeProt":false,"timeout":false},
    "circuits":{"active":{"spa":false,"aux1":false,"aux2":false,"aux3":false,"ft1":false,"pool":true,"ft2":false,"ft3":false,"ft4":false},"delay":{"spa":false,"aux1":false,"aux2":false,"aux3":false,"ft1":false,"pool":false,"ft2":false,"ft3":false,"ft4":false}}}}
}
```

The web UI, will show the pool state and allow you to change the thermostat and circuits.

![Web UI](assets/media/opnpool-web-ui-pool-therm-sml.png)

If you are using Home Assistant, the `*.opnpool*` entities will update automatically. The `hassio` directory has some YAML code to use with the Lovelace dashboard.

If you go that route, also remember to install `modcard`, `button-card`, `bar-card`, `simple-thermostat`, `template-entity-row` and `mini-graph-card` available through the Home Assistant Community Store (HACS).

![Hassio UI](assets/media/opnpool-readme-hassio-lovelace.png)

## Design documentation

The design documentation for this project is available at
[https://coertvonk.com/category/sw/embedded/opnpool-design](https://coertvonk.com/category/sw/embedded/opnpool-design). It includes the chapters

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
