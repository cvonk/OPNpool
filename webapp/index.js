// time range slider: http://jsfiddle.net/ezanker/fu26u/204/

// To develop: use vsCode with Live Server plugin
// To debug code on page load run "window.location.reload()" in the JavaScript Console.
(function() {
    "use strict";

    let ipName;
    let intervalId = 0;

    // request the parent's origin (sending a message to the parent tells it we're loaded),
    // the parent reply contains its origin.
    if (in_iframe()) {
        window.addEventListener('message', event => {
            ipName = event.origin;
            //console.log(ipName);
            //alert(event.origin);
            scheduled_update();
        }, false);
        parent.postMessage('fromIframe', '*');
    } else {
        ipName = "http://pool.iot.vonk";
        console.log('not in Iframe, guessing (' + ipName + ')');
        scheduled_update();
    }

    document.addEventListener('deviceready', on_device_ready.bind(this), false);

    function on_device_ready() {}

    function in_iframe() {
        try {
            return window.self !== window.top;
        } catch (e) {
            return true;
        }
    }

    //---------still of use---------
    // schedule.pool.start: '08:15'
    // schedule.pool.stop: '13:30'

    let gThermostats = [{'name': 'pool'}, {'name': 'spa'}];
    let gChlorSalt = {};
    let gChlorPct = {};
    let gTempWater = {};
    let gTempAir = {};
    let gPumpRpm = {};
    let gPumpPwr = {};

    function degrees_f_to_c(f) {
        return Math.round(((f - 32) * 5) / 9);
    }

    function capitalize_1st_letter(string) {
        return string.charAt(0).toUpperCase() + string.toLowerCase().slice(1);
    }

    // algorithm from https://en.wikipedia.org/wiki/HSL_and_HSV
    function hsv_2o_rgb(h, s, v) {
        h %= 360;
        s = Math.max(0, Math.min(s, 1));
        v = Math.max(0, Math.min(v, 1));
        const rgb_max = Math.floor(v * 255);
        const rgb_min = Math.floor(rgb_max * (1 - s));
        const i = Math.floor(h / 60);
        const diff = h % 60;
        const rgb_adj = Math.floor((rgb_max - rgb_min) * diff / 60); // RGB adjustment amount by hue
        let r, g, b;
        switch (i) {
            case 0:
                r = rgb_max;
                g = rgb_min + rgb_adj;
                b = rgb_min;
                break;
            case 1:
                r = rgb_max - rgb_adj;
                g = rgb_max;
                b = rgb_min;
                break;
            case 2:
                r = rgb_min;
                g = rgb_max;
                b = rgb_min + rgb_adj;
                break;
            case 3:
                r = rgb_min;
                g = rgb_max - rgb_adj;
                b = rgb_max;
                break;
            case 4:
                r = rgb_min + rgb_adj;
                g = rgb_min;
                b = rgb_max;
                break;
            default:
                r = rgb_max;
                g = rgb_min;
                b = rgb_max - rgb_adj;
                break;
        }
        return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    }

    function update_round_slider(thermostat, v) {
        $(thermostat.id).roundSlider("setValue", v); // only needed when called directly
        const blue = 350;
        const rgb = hsv_2o_rgb(blue * (1 - (v - thermostat.min) / thermostat.max), 1, 1);
        /* for non-svg mode */
        const el = $(thermostat.id + ' > div > div.rs-inner-container > div > div')[0];
        el.style.backgroundColor = rgb;
    }

    function circuitChanged(obj)
    {
        console.log(obj.val() + ' = ' + obj.target.value);
        return obj;
    }

    function update_system(tod)
    {
        $("#date").val(tod.date);
        $("#time").val(tod.time);
    }

    function update_circuits(active_arr)
    {
        Object.keys(active_arr).forEach( function(key, value) {
            const id = '#cCircuit' + capitalize_1st_letter(key);
            $(id).attr("checked", this[key]).checkboxradio("refresh");
        }, active_arr);
    }

    function update_chlor(chlor)
    {
        if (!jQuery.isEmptyObject(chlor)) {
            let title = "Chlorinator";
            if (chlor.status !== 'OK') {
                title += chlor.status;
            }
            gChlorSalt.refresh(chlor.salt);
            gChlorPct.refresh(chlor.pct);
            gChlorPct.txtLabel.attr("text", title);
        }
    }

    function update_temp(active, pool, spa, air_temp)
    {
        let water_label = "Water";
        if (active.POOL && pool.heating) {
            water_label += " " + pool.src;
        }
        if (active.SPA && spa.heating) {
            water_label += " " + spa.src;
        }
        gTempWater.refresh(degrees_f_to_c(pool.temp));
        gTempWater.txtLabel.attr("text", water_label);
        gTempAir.refresh(degrees_f_to_c(air_temp));
    }

    function update_thermostats(set_points)
    {
        gThermostats.forEach(function (thermostat) {
            update_round_slider(thermostat, set_points[thermostat.name]);
        });
    }

    function update_heat_src(pool, spa) {
        $("#sPoolSp").val(pool.sp).slider('refresh');
        $("#sSpaSp").val(spa.sp).slider('refresh');

        let selector = "#cPoolHeatSrc" + capitalize_1st_letter(pool.src);
        $(selector).attr("checked", true).checkboxradio("refresh");

        selector = "#cSpaHeatSrc" + capitalize_1st_letter(spa.src);
        $(selector).attr("checked", true).checkboxradio("refresh");

    }

    function update_pump(pump) {
        let title = "Pump speed";
        if (pump.running && pump.mode !== "FILTER") {
            title += "\nmode: " + pump.mode;
        }
        gPumpRpm.refresh(pump.rpm);
        gPumpRpm.txtLabel.attr("text", title);

        title = "Pump power";
        if (pump.status !== 0) {
            title += "\nstatus: " + pump.status;
        } else if (pump.err !== 0) {
            title += "\nerror: " + pump.err;
        }
        gPumpPwr.refresh(pump.pwr);
        gPumpPwr.txtLabel.attr("text", title);
    }

    function disable_inputs(value)
    {
        $("input[type='checkbox']").attr("disabled", value); //.checkboxradio("refresh");
        $("input[type='radio']").attr("disabled", value); // .checkboxradio("refresh");
        $(".my-setpoint").slider(value ? "disable" : "enable"); //.slider("refresh");
    }

    function overlay_page(value)
    {
        if (value) {
            $('#pageone').css({
                opacity: 0.7
            });
            $('body').css({
                'overflow': 'hidden'
            });
        } else {
            $("#pageone").css('opacity', "");
        }
    }

    function show_spinner(value)
    {
        if (value.length) {
            $.mobile.loading('show', {
                text: value,
                textVisible: true
            });
        } else {
            $.mobile.loading('hide');
        }
    }

    function update(jsonData)
    {
        update_circuits(jsonData.circuits.active);
        update_temp(jsonData.circuits.active, jsonData.thermostats.POOL, jsonData.thermostats.SPA, jsonData.temps.AIR);
        update_thermostats({'pool': jsonData.thermostats.POOL.sp, 'spa': jsonData.thermostats.SPA.sp});
        update_heat_src(jsonData.thermostats.POOL, jsonData.thermostats.SPA);
        update_chlor(jsonData.chlor);
        update_pump(jsonData.pump);
        update_system(jsonData.system.tod);
    }

    var first_update = 1;

    function send_message(pair)
    {
        const url = ipName + "/json";
        const request = !jQuery.isEmptyObject(pair);
        $.ajax({
            dataType: "jsonp",
            url: url + "?callback=?",
            data: pair,
            success: function(jsonData) {
                //console.log(jsonData);
                if (request) {
                    clearInterval(intervalId);
                    intervalId = 0;
                } else {
                    update(jsonData);
                    show_spinner("");
                    //console.log("disable inputs");
                    disable_inputs(false);
                    //console.log("disable inputs");
                    overlay_page(false);
                    first_update = 0;
                }
                if (intervalId === 0) { // schedule after request, or on first invocation
                    intervalId = setInterval(scheduled_update, 10000);
                }
            },
            beforeSend: function() {
                if (first_update || request) {
                    show_spinner(request ? "Requesting\n(please wait 10 sec)" : "Loading data");
                    disable_inputs(true);
                    overlay_page(true);
                }
            },
            error: function() {
                show_spinner("Retry in 10 seconds\n(" + ipName + ")");
            }
        });

    }

    function scheduled_update()
    {
        send_message({});
    }

    function initialize()
    {
        //hideInputElements(true);

        gThermostats.forEach(function (thermostat) {
            thermostat.min = 0;
            thermostat.max = 100;
            thermostat.id = '#gThermostat' + capitalize_1st_letter(thermostat.name);
            $(thermostat.id).roundSlider({
                sliderType: "min-range",
                circleShape: "half-top",
                handleShape: "dot",
                value: 0,
                readOnly: false,
                min: 32,
                max: 100,
                step: 1,
                radius: 120, // determines width & height
                width: 35,
                svgMode: false,
                animation: true,
                keyboardAction: true,
                mouseScrollAction: true,
                tooltipFormat: function(args) {
                    return args.value + " °F";
                },
                drag: function(args) {
                    update_round_slider(thermostat, args.value);
                },
                change: function(args) {
                    update_round_slider(thermostat, args.value);
                    const pair = { "?homeassistant/climate/esp32-wrover-1/' + thermostat.name + '/set_temp": args.value };
                    send_message(pair);
                }
            });

        });

        gChlorSalt = new JustGage({
            id: "gChlorSalt",
            label: 'Salt level',
            symbol: " ppm",
            value: 0,
            min: 0,
            max: 6500,
            relativeGaugeSize: true,
            valueFontColor: "white",
            levelColors: ['#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#48CCCD', '#48CCCD', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800'],
        });

        gChlorPct = new JustGage({
            id: "gChlorPct",
            label: 'Percentage',
            symbol: "%",
            value: 0,
            min: 0,
            max: 100,
            relativeGaugeSize: true,
            valueFontColor: "white",
        });

        gTempWater = new JustGage({
            id: "gTempWater",
            label: 'Water',
            symbol: " °C",
            value: 0,
            min: 0,
            max: 50,
            height: 200,
            relativeGaugeSize: true,
            valueFontColor: "white",
            levelColors: ['#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#C9DB39', '#F5C80D', '#F1933C', '#F1933C', '#F1933C', '#F1933C', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832'],
        });

        gTempAir = new JustGage({
            id: "gTempAir",
            label: 'Air',
            symbol: " °C",
            value: 0,
            min: 0,
            max: 50,
            height: 100,
            relativeGaugeSize: true,
            valueFontColor: "white",
            levelColors: ['#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#C9DB39', '#F5C80D', '#F1933C', '#F1933C', '#F1933C', '#F1933C', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832'],
        });

        gPumpRpm = new JustGage({
            id: "gPumpRpm",
            label: 'Pump speed',
            symbol: " rpm",
            value: 0,
            min: 0,
            max: 3450,
            relativeGaugeSize: true,
            valueFontColor: "white",
        });

        gPumpPwr = new JustGage({
            id: "gPumpPwr",
            label: 'Pump power',
            symbol: " W",
            value: 0,
            min: 0,
            max: 1300,
            relativeGaugeSize: true,
            valueFontColor: "white",
        });

        // register change notifications

        $(".circuit").on("change", function(event) {
            let pair = {};
            pair["homeassistant/switch/esp32-wrover-1/" + event.target.value + "_circuit/set"] = event.target.checked ? "ON" : "OFF";
            console.log(pair);
            send_message(pair);
        });
        $(".heatsrc").on("change", function(event) {
            let pair = {};
            pair["?homeassistant/climate/esp32-wrover-1/" + event.target.name + "/set_mode"] = event.target.value;
            console.log(pair);
            send_message(pair);
        });
        $(".my-slider").on("slidestop", function(event) {
            let pair = {};
            pair["?homeassistant/climate/esp32-wrover-1/" + event.target.name + "/set_temp"] = event.target.value;
            console.log(pair);
            send_message(pair);
        });

    }
    $(document).ready(initialize);
})();