# ESP32 - Connect to WiFi Access Point

[![LICENSE](https://img.shields.io/github/license/jvonk/pact)](LICENSE)

## Goal

[The ESP-IDF API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
> provides support for WiFi Station mode (aka STA mode or WiFi client mode). ESP32 connects to an access point.

This component makes this API available as a component.  It connects to a WiFi Access Point, and reconnects when the connection drops.

The user can specify event handlers to respond to connect/disconnect events to e.g. start/stop a HTTP server.

## Example

An example can be found in the [`test`](test) directory.

The `WIFI_CONNECT_SSID` and `WIFI_CONNECT_PASSWORD` can be provisioned using `Kconfig`.  If empty, the device will use credentials from flash memory instead. Once connected to an WiFi Access Point, it starts a web server.

## Usage

The software relies on the master ESP-IDF SDK version v4.3-dev-472-gcf056a7d0 and accompanying tools.

Clone this component in your project's `components` directory, or use it as a git submodule.
```
git submodule add -b master https://github.com/cvonk/ESP32_wifi-connect.git components/wifi_connect
git submodule init
```

Copy the `Kconfig.example` into your `Kconfig` and update the configuration.

First initialize non volatile flash memory (`nvs_flash_init`).  Then create invoke the component as
```
wifi_connect_config_t wifi_connect_config = {
    .onConnect = _wifi_connect_on_connect,
    .onDisconnect = _wifi_connect_on_disconnect,
    .priv = NULL,
};
wifi_connect(&wifi_connect_config);
```
Here `_wifi_connect_on_connect` and `_wifi_connect_on_disconnect` are user supplied callback functions.  The element `priv` is intended to private data used in these callback functions.


## Feedback

We love to hear from you. Please use the usual Github mechanisms to contact me.

## LICENSE

Copyright 2020 Coert Vonk

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.