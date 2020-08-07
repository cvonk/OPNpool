// time range slider: http://jsfiddle.net/ezanker/fu26u/204/

// To develop: use vsCode with Live Server plugin
// To debug code on page load run "window.location.reload()" in the JavaScript Console.
(function() {
    "use strict";

    let ipName;
    const ipName_default =  "http://esp32-wrover-1.iot.vonk";
    let message_pending = false;
    let first_status_req = true;
    let delay_cnt = 0;
    const delay_duration = 5;  // [sec]

    //---------still of use---------
    // schedule.pool.start: '08:15'
    // schedule.pool.stop: '13:30'

    let _ui = {
        'circuits': {
            'pool': {},
            'spa': {},
            'aux1': {},
            'aux2': {},
            'aux3': {},
            'ft1': {},
            'ft2': {},
            'ft3': {},
            'ft4': {}
        },
        'temps': {
            'air': { 'min': 32, 'max': 100, 'unit': '°F' },
            'solar': { 'min': 32, 'max': 100, 'unit': '°F' },
            'water': { 'min': 32, 'max': 100, 'unit': '°F' },
        },
        'pump': {
            'rpm': {'min': 0, 'max': 3500, 'unit': 'rpm'},
            'pwr': {'min': 0, 'max': 1300, 'unit': 'W'},
        },
        'thermostats': {
            'pool': { 'min': 32, 'max': 100, 'unit': '°F' },
            'spa': { 'min': 32, 'max': 100, 'unit': '°F'},
        },
        'chlor': {
            'salt': { 'min': 0, 'max': 6500, 'unit': 'ppm'},
            'pct': { 'min': 0, 'max': 100, 'unit': '%'},
        },
    };

    function restart_delay()
    {
        delay_cnt = delay_duration;
    }

    function send_message(pair)
    {
        console.log("tx", pair);
        const url = ipName + "/json";
        const status_req = jQuery.isEmptyObject(pair);
        const set_req = !status_req;
        $.ajax({
            dataType: "jsonp",
            url: url + "?callback=?",
            data: pair,
            success: function(jsonData) {
                if (status_req) {
                    update(jsonData);
                    first_status_req = false;
                } else if (set_req) {
                    message_pending = true;
                    restart_delay();
                }
            },
            beforeSend: function() {
                if (first_status_req) {
                    toast("Loading data");
                } else if (set_req) {
                    toast("Requesting");
                }
            },
            error: function() {
                toast("Retrying");
            }
        });
    }

    function every_sec_cb()
    {
        if (delay_cnt-- == 0) {
            send_message({});
            delay_cnt = delay_duration;
        }
    }

    function in_iframe() {
        try {
            return window.self !== window.top;
        } catch (e) {
            return true;
        }
    }

    function toast(msg)
    {
        $("<div class='ui-loader ui-overlay-shadow ui-body-e ui-corner-all'><div>" + msg + "</div></div>")
            .css({
                display: 'block',
                opacity: 0.90,
                position: 'fixed',
                padding: '7px',
                'text-align': "center",
                width: '270px',
                left: ($(window).width() - 270)/2,
                top: $(window).height()*3/4,
                'background-color': '#54BBE0',
                '-webkit-border-radius': '24px',
                'border-radius': '24px'
            })
            .appendTo( $.mobile.pageContainer ).delay(6000)
            .fadeOut( 200, function(){
                $(this).remove();
            });
    }

/*
    function degrees_f_to_c(f)
    {
        return Math.round(((f - 32) * 5) / 9);
    }

    function capitalize_1st_letter(str)
    {
        if (typeof str !== 'string') {
            return str;
        }
        return str.charAt(0).toUpperCase() + str.toLowerCase().slice(1);
    }
*/

    // https://en.wikipedia.org/wiki/HSL_and_HSV
    function hsv_2o_rgb(h, s, v)
    {
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
            case 0: r = rgb_max; g = rgb_min + rgb_adj; b = rgb_min; break;
            case 1: r = rgb_max - rgb_adj; g = rgb_max; b = rgb_min; break;
            case 2: r = rgb_min; g = rgb_max; b = rgb_min + rgb_adj; break;
            case 3: r = rgb_min; g = rgb_max - rgb_adj; b = rgb_max; break;
            case 4: r = rgb_min + rgb_adj; g = rgb_min; b = rgb_max; break;
            default: r = rgb_max; g = rgb_min; b = rgb_max - rgb_adj;break;
        }
        return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    }

    function update_round_slider(thermostat, v)
    {
        thermostat.obj.roundSlider("setValue", v); // only needed when called directly
        if (thermostat.track_obj) {
            const blue = 240;
            const red = 0;
            const ratio = (v - thermostat.min) / (thermostat.max - thermostat.min);
            console.log(ratio);
            const hue = blue * (1 - ratio) + red * ratio;
            const rgb = hsv_2o_rgb(hue, 1, 1);
            thermostat.track_obj.style.backgroundColor = rgb;
        }
    }

    function update_circuits(json_active)
    {
        Object.keys(_ui.circuits).forEach( function(key) {
            const val = json_active[key.toUpperCase()];
            this[key].obj.attr('checked', val).checkboxradio('refresh');
        }, _ui.circuits);
    }

    function update_temps(json_temps, json_thermostats)
    {
        update_round_slider(_ui.temps.air, json_temps['AIR']);
        update_round_slider(_ui.temps.solar, json_temps['SOLAR']);
        update_round_slider(_ui.temps.water, json_thermostats['POOL'].temp);
    }

    function update_pump(json_pump)
    {
        _ui.pump.rpm.obj_lbl.text((json_pump.running ? 'Running' : 'Idle' ) + ' in ' + json_pump.mode + ' mode');
        _ui.pump.pwr.obj_lbl.text((json_pump.running ? 'Running' : 'Idle' ) + ' with status ' + json_pump.status);
        update_round_slider(_ui.pump.rpm, json_pump.rpm);
        update_round_slider(_ui.pump.pwr, json_pump.pwr);
    }

    function update_chlor(json_chlor)
    {
        //if (!jQuery.isEmptyObject(json_chlor)) {
        const salt_status = json_chlor.salt == 0   ? "Sensor malfunction"
                          : json_chlor.salt < 2600 ? "Very low salt level"
                          : json_chlor.salt < 2800 ? "Low salt level"
                          : json_chlor.salt < 4500 ? "Good salt level" : "High salt level";
        _ui.chlor.salt.obj_lbl.text(salt_status);
        _ui.chlor.pct.obj_lbl.text('Status ' + json_chlor.status);
        update_round_slider(_ui.chlor.salt, json_chlor.salt);
        update_round_slider(_ui.chlor.pct, json_chlor.pct);
    }

    function update_system(system)
    {
        $("#date").val(system.tod.date);
        $("#time").val(system.tod.time);
    }

    function update_thermostats(json_thermostats)
    {
        Object.keys(_ui.thermostats).forEach( function(key) {
            ['off', 'heat', 'solar_pref', 'solar'].forEach( function(src) {
                const sel = '#' + key.toLowerCase() + '_heater_' + src.toLowerCase(src);
                const val = src == json_thermostats[key.toUpperCase()].src;
                $(sel).attr("checked", val).checkboxradio("refresh");
            });
            const json_thermostat = json_thermostats[key.toUpperCase()];
            update_round_slider(this[key], json_thermostat.sp);
            this[key].obj_lbl.text((json_thermostat.heating ? 'heating' : 'idle'));

        }, _ui.thermostats);
    }

    function disable_inputs(value)
    {
        $("input[type='checkbox']").attr("disabled", value);
        $("input[type='radio']").attr("disabled", value);
        $(".my-setpoint").slider(value ? "disable" : "enable");
    }

/*
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
*/

    function update(jsonData)
    {
        if (message_pending) {
            // the message is queued until the sends a status update
            // ignore that status update
            console.log("rx ignoring (msg pending)");
            message_pending = false;
        } else {
            console.log("rx", jsonData);
            update_circuits(jsonData.circuits.active);
            update_temps(jsonData.temps, jsonData.thermostats);
            update_thermostats(jsonData.thermostats);
            update_chlor(jsonData.chlor);
            update_pump(jsonData.pump);
            update_system(jsonData.system);
        }
    }

    function initialize()
    {
        //hideInputElements(true);

        Object.keys(_ui.circuits).forEach( function(key) {
            this[key].obj = $('#circuit_' + key);
        }, _ui.circuits);

        Object.keys(_ui.temps).forEach( function(key) {
            this[key].obj = $('#temp_' + key);
            this[key].obj_lbl = $('temp_' + key + '_lbl');
            this[key].obj.roundSlider({
                sliderType: "min-range",
                circleShape: "half-top",
                handleSize: 0,
                value: 0,
                readOnly: true,
                min: this[key].min,
                max: this[key].max,
                radius: 120, // determines width & height
                width: 35,
                svgMode: false,
                tooltipFormat: function(args) {
                    return args.value + " " + _ui.temps[key].unit;
                },
            });
        }, _ui.temps);

        Object.keys(_ui.pump).forEach( function(key) {
            this[key].obj = $('#pump_' + key);
            this[key].obj_lbl = $('#pump_' + key + '_lbl');
            this[key].obj.roundSlider({
                sliderType: "min-range",
                circleShape: "half-top",
                handleSize: 0,
                value: 0,
                readOnly: true,
                min: this[key].min,
                max: this[key].max,
                radius: 120, // determines width & height
                width: 35,
                svgMode: false,
                tooltipFormat: function(args) {
                    return args.value + " " + _ui.pump[key].unit;
                },
            });
        }, _ui.pump);

        Object.keys(_ui.thermostats).forEach( function(key) {
            this[key].obj = $('#thermostat_' + key);
            this[key].obj_lbl = $('#thermostat_' + key + '_lbl');
            this[key].obj.roundSlider({
                sliderType: "min-range",
                circleShape: "half-top",
                handleShape: "round",
                value: 0,
                readOnly: false,
                min: this[key].min,
                max: this[key].max,
                step: 1,
                radius: 120, // determines width & height
                width: 35,
                svgMode: false,
                animation: true,
                keyboardAction: true,
                mouseScrollAction: true,
                tooltipFormat: function(args) {
                    return args.value + " " + _ui.thermostats[key].unit;
                },
                drag: function(args) {
                    update_round_slider(_ui.thermostats[key], args.value);
                },
                change: function(args) {
                    update_round_slider(_ui.thermostats[key], args.value);
                    let pair = {};
                    const key1 = '?homeassistant/climate/pool/' + key.toLowerCase() + '/set_temp';
                    pair[key1] = args.value
                    send_message(pair);
                }
            });
            this[key].track_obj = $('#thermostat_' + key + ' > div > div.rs-inner-container > div > div')[0];  /* for non-svg mode */
        }, _ui.thermostats);

        Object.keys(_ui.chlor).forEach( function(key) {
            this[key].obj = $('#chlor_' + key);
            this[key].obj_lbl = $('#chlor_' + key + '_lbl');
            this[key].obj.roundSlider({
                sliderType: "min-range",
                circleShape: "half-top",
                handleSize: 0,
                value: 0,
                readOnly: true,
                min: this[key].min,
                max: this[key].max,
                radius: 120, // determines width & height
                width: 35,
                svgMode: false,
                tooltipFormat: function(args) {
                    return args.value + " " + _ui.chlor[key].unit;
                },
            });
        }, _ui.chlor);

/*
        _ui.chlor.salt.obj = new JustGage({
            id: "chlor_salt",
            label: 'Salt level',
            symbol: " ppm",
            value: 0,
            min: 0,
            max: 6500,
            relativeGaugeSize: true,
            valueFontColor: "white",
            levelColors: ['#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#48CCCD', '#48CCCD', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800'],
        });

        _ui.chlor.pct.obj = new JustGage({
            id: "chlor_pct",
            label: 'Percentage',
            symbol: "%",
            value: 0,
            min: 0,
            max: 100,
            relativeGaugeSize: true,
            valueFontColor: "white",
        });
*/

        // register change notifications

        $(".circuit").on("change", function(event) {
            let pair = {};
            pair["homeassistant/switch/pool/" + event.target.value + "_circuit/set"] = event.target.checked ? "ON" : "OFF";
            send_message(pair);
        });
        $(".heat_src").on("change", function(event) {
            let pair = {};
            pair["?homeassistant/climate/pool/" + event.target.name + "/set_mode"] = event.target.value;
            send_message(pair);
        });

        // sending a message to the parent tells it we're loaded,
        // in reply the parent will share its IP name.
        // Next, have every_sec_cb be called every sec
        if (in_iframe()) {
            window.addEventListener('message', event => {
                ipName = event.origin;
                setInterval(every_sec_cb, 1000);
            }, false);
            parent.postMessage('fromIframe', '*');
        } else {
            ipName = ipName_default;
            console.log('not in Iframe, assuming ' + ipName);
            setInterval(every_sec_cb, 1000);
        }
    }
    $(document).ready(initialize);
})();