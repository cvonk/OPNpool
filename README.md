# OPNpool

The OPNpool integrates the traditional Pool Controller into our smart home. It keeps a tab on the status of the controller, the pool pump and the chlorinator. It is not only more convenient than walking over to the pool equipment, but will also tell you about problems with e.g. the chlorinator or pump. 

Features:

  * Visualizes the status of the thermostats, pump, chlorinator, circuits and schedules.
  * Lets you change the thermostats and toggle circuits
  * Connects to the internet
  * MQTT and Home Assistant integration
  * Over-the-air updates
  * One time provisioning from an Android phone
  * Web app UI
  * IP68 waterproof case and connectors
  * No power adapter required
  * *Open source!

> This open source and hardware project is intended to comply with the October 2016 exemption to the Digital Millennium Copyright Act allowing "good-faith" testing," in a controlled environment designed to avoid any harm to individuals or to the public.

The full fledged project installation method is described in the `FULL_INSTALL.md`. Before you go down that road, you may want to give it a quick spin to see what it can do. The remainder of this README will walk you through this.

## Parts

At the core this project is an ESP32 module and a 3.3 Volt RS-485 adapter. You can breadboard this using:

* Any ESP32 module that has an USB connector and `GPIO#25` to `GPIO#27` available (such as the Wemos LOLIN D32).
* Any "Max485 Module TTL". To make it 3.3 Volt compliant, change the chip to a MAX3485CSA+. While you're at it, you may as well remove the 10 kΩ pullup resistors (`R1` to `R4`).
* A piece of Cat5 ethernet cable to connect to the pool controller.

## Build

Clone the repository and its submodules to a local directory. The `--recursive` flag automatically initializes and updates the submodules in the repository,.

```bash
git clone --recursive https://github.com/cvonk/OPNpool.git
```

From within Microsoft Visual Code (VScode), add the [Microsoft's C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools). Then add the [Espressif IDF extension &ge;4.4](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension). The latter will automatically start its configuration. Answer according to the table below

| Question     | Choice             |
|--------------| ------------------ |
| Mode         | Advanced           |
| ESP-IDF path | `C:/espressif`     |
| Tools        | `C:/espressif/bin` |
| Download     | yes                |

From VScode:

  * Change to the `OPNpool/interface` folder.
  * Connect your ESP32 module, and configure the device and COM port (press the F1-key and select "ESP-IDF: Device configurion")
  * Edit the configuration (press the F1-key, select "ESP-IDF: SDK configuration editor" and scroll down to OPNpool)
      * Select "Use hardcoded Wi-Fi credentials" and specify the SSID and password of your Wi-Fi access point.
      * If you have a MQTT broker set up, select "Use hardcoded MQTT URL" and specify the URL in the format `mqtt://username:passwd@host.domain:1883`
  * Start the build-upload-monitor cycle (press the F1-key and select "ESP-IDF: Build, Flash and start a monitor on your device").

The device will appear on your network segment as `opnpool.local`.  You can access its web UI through `http://pool.local`. If MQTT is configured, it will publish MQTT messages and if you use the Home Assistant for automation, entities will appears with `.opnpool` in the name.

### Connect

> **:warning PROCEED AT YOUR OWN RISK! At the very least make turn off the power while you work on your pool equipment. Be careful, THERE IS ALWAYS A RISK OF BREAKING YOUR POOL EQUIPMENT.**

Having said that .. the RS-485 header is on the back of the control board. There are probably already wires connected that go to the devices such as pump and chlorinator.

To minimize electromagnetic interference, use a twisted pairs from e.g. CAT-5 cable to connect the `A`/`B` pair to the 3.3 volt RS-485 adapter as shown in the table below.

| Controller     | RS-485 adapter | idle state |         
|:---------------|:--------------:|:-----------|
| -DATA (green)  |  A             | negative   |
| +DATA (yellow) |  B             | positive   |

Connect the RS-485 adapter to the ESP32 module.  I also pulled GPIO#27 down with 10 k&ohm;, just to make sure it doesn't transmit while the ESP32 is booting.

| RS-485 adapter | ESP32 module |
|:---------------|:-------------|
| RO             | GPIO#25      |
| DI             | GPIO#26      |
| DE and RE      | GPIO#27      |
| GND            | GND          |

You will now see the decoded messages on the serial monitor such as:

```json
I (16670) poolstate_rx: {CTRL_STATE_BCAST: {
    "state":{"system":{"tod":{"time":"14:01","date":"2022-04-05"},"firmware":"v0.000"},"temps":{"air":69,"solar":80},
    "thermos":{"pool":{"temp":68,"src":"None","heating":false},"spa":{"temp":69,"src":"None","heating":false}},
    "scheds":{"pool":{"start":"08:00","stop":"10:00"}},
    "modes":{"service":false,"UNKOWN_01":false,"tempInc":false,"freezeProt":false,"timeout":false},
    "circuits":{"active":{"spa":false,"aux1":false,"aux2":false,"aux3":false,"ft1":false,"pool":true,"ft2":false,"ft3":false,"ft4":false},"delay":{"spa":false,"aux1":false,"aux2":false,"aux3":false,"ft1":false,"pool":false,"ft2":false,"ft3":false,"ft4":false}}}}
}
```

The web UI, `http://opnpool.local`, will show the pool state and allow you to change the thermostat and circuits.

![Web UI](media/opnpool-web-ui-pool-therm-sml.png)

If you are using Home Assistant, the `*.opnpool*` entities will update automatically. The `hassio` directory has some YAML code to use with the Lovelace dashboard.

If you go that route, also remember to install `modcard`, `button-card`, `bar-card`, `simple-thermostat`, `template-entity-row` and `mini-graph-card` available through the Home Assistant Community Store (HACS).

![Hassio UI](media/opnpool-readme-hassio-lovelace.png)