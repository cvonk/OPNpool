// to develop: use vsCode with Live Server plugin

(function() {
    "use strict";

    let ipName;
    const ipName_default =  ""; //"http://esp32-wrover-1.iot.vonk"; // "http://pool.iot.vonk"; //
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
        'readonly': {
            'temp': {
                'air': { 'min': 32, 'max': 100, 'unit': '°F' },
                'solar': { 'min': 32, 'max': 100, 'unit': '°F' },
                'water': { 'min': 32, 'max': 100, 'unit': '°F' },
            },
            'pump': {
                'rpm': {'min': 0, 'max': 3500, 'unit': 'rpm'},
                'pwr': {'min': 0, 'max': 1300, 'unit': 'W'},
            },
            'chlor': {
                'salt': { 'min': 0, 'max': 6500, 'unit': 'ppm'},
                'pct': { 'min': 0, 'max': 100, 'unit': '%'},
            },
        },
        'readwrite' : {
            'thermo': {
                'pool': { 'min': 32, 'max': 100, 'unit': '°F' },
                'spa': { 'min': 32, 'max': 100, 'unit': '°F'},
            },
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
            const hue = blue * (1 - ratio) + red * ratio;
            const rgb = hsv_2o_rgb(hue, 1, 1);
            //thermostat.track_obj.style.backgroundColor = rgb;
            thermostat.track_obj.style.stroke = rgb;
        }
    }

    function update_circuits(json_circuits)
    {
        Object.keys(_ui.circuits).forEach( function(key) {
            const val = json_circuits.active[key.toLowerCase()];
            //this[key].obj.attr('checked', val).trigger('create').checkboxradio('refresh');
            this[key].obj.prop('checked', val).trigger('create').checkboxradio('refresh');;
        }, _ui.circuits);
    }

    function update_readonly(json_temps, json_thermos, json_chlor, json_pump)
    {
        update_round_slider(_ui.readonly.temp.air, json_temps.air);
        update_round_slider(_ui.readonly.temp.solar, json_temps.solar);
        update_round_slider(_ui.readonly.temp.water, json_thermos.pool.temp);

        _ui.readonly.pump.rpm.obj_lbl.text((json_pump.running ? 'Running' : 'Idle' ) + ' in ' + json_pump.mode + ' mode');
        _ui.readonly.pump.pwr.obj_lbl.text((json_pump.running ? 'Running' : 'Idle' ) + ' with status ' + json_pump.status);
        update_round_slider(_ui.readonly.pump.rpm, json_pump.rpm);
        update_round_slider(_ui.readonly.pump.pwr, json_pump.power);

        //if (!jQuery.isEmptyObject(json_chlor)) {
        const salt_status = json_chlor.salt == 0   ? "Sensor malfunction"
                          : json_chlor.salt < 2600 ? "Very low salt level"
                          : json_chlor.salt < 2800 ? "Low salt level"
                          : json_chlor.salt < 4500 ? "Good salt level" : "High salt level";
        _ui.readonly.chlor.salt.obj_lbl.text(salt_status);
        _ui.readonly.chlor.pct.obj_lbl.text('Status ' + json_chlor.status);
        update_round_slider(_ui.readonly.chlor.salt, json_chlor.salt);
        update_round_slider(_ui.readonly.chlor.pct, json_chlor.pct);
    }

    function update_readwrite(json_thermos)
    {
        Object.keys(_ui.readwrite.thermo).forEach( function(key) {
            ['None', 'Heat', 'SolarPref', 'Solar'].forEach( function(src) {
                const sel = '#' + key.toLowerCase() + '_heater_' + src.toLowerCase(src);
                const val = src == json_thermos[key.toLowerCase()].src;
                //$(sel).attr("checked", val).checkboxradio("refresh");
                $(sel).prop('checked', val).trigger('create').checkboxradio('refresh');;

            });
            const json_thermostat = json_thermos[key.toLowerCase()];
            update_round_slider(this[key], json_thermostat.sp);
            this[key].obj_lbl.text((json_thermostat.heating ? 'heating' : 'idle'));

        }, _ui.readwrite.thermo);
    }

    function update_schedules(scheds)
    {
        if ("pool" in scheds) {
            $("#poolStart").val(scheds.pool.start);
            $("#poolStop").val(scheds.pool.stop);
        }
        if ("spa" in scheds) {
            $("#spaStart").val(scheds.spa.start);
            $("#spaStop").val(scheds.spa.stop);
        }
    }

    function update_system(system)
    {
        $("#date").val(system.tod.date);
        $("#time").val(system.tod.time);
    }

    function change_visibility(active, pump)
    {
        const pumpVisibility = pump.running ? "block" : "none";
        $("#chlor_salt_collapsible").css("display", pumpVisibility);
        $("#chlor_pct_collapsible").css("display", pumpVisibility);
        $("#pump_pwr_collapsible").css("display", pumpVisibility);
        $("#pump_rpm_collapsible").css("display", pumpVisibility);

        const poolVisibility = active.pool ? "block" : "none";
        $("#thermo_pool_collapsible").css("display", poolVisibility);

        const spaVisibility = active.spa ? "block" : "none";
        $("#thermo_spa_collapsible").css("display", spaVisibility);
    }

    function update(jsonData)
    {
        if (message_pending) {
            // the message is queued until the sends a status update
            // ignore that status update
            console.log("rx ignoring (msg pending)");
            message_pending = false;
        } else {
            console.log("rx", jsonData);
            update_circuits(jsonData.circuits);
            update_readonly(jsonData.temps, jsonData.thermos, jsonData.chlor, jsonData.pump);
            update_readwrite(jsonData.thermos);
            update_schedules(jsonData.scheds);
            update_system(jsonData.system);
            change_visibility(jsonData.circuits.active, jsonData.pump)
        }
    }

    function initialize()
    {
        console.log("index.js: ready event, updating");

        Object.keys(_ui.circuits).forEach( function(key) {
            this[key].obj = $('#circuit_' + key);
        }, _ui.circuits);

        Object.keys(_ui.readonly).forEach( function(major_key) {
            Object.keys(this[major_key]).forEach( function(minor_key) {
                this[minor_key].obj = $('#' + major_key + '_' + minor_key);
                this[minor_key].obj_lbl = $('#' + major_key + '_' + minor_key + '_lbl');
                this[minor_key].obj.roundSlider({
                    sliderType: "min-range",
                    circleShape: "half-top",
                    handleSize: 0,
                    value: 0,
                    readOnly: true,
                    min: this[minor_key].min,
                    max: this[minor_key].max,
                    //radius: 120, // determines width & height
                    //width: 35,
                    svgMode: true,
                    tooltipFormat: function(args) {
                        return args.value + " " + _ui.readonly[major_key][minor_key].unit;
                    },
                    beforeCreate: function () {
                        this.options.radius = this.control.parent().width() / 2;
                        this["_bind"]($(window), "resize", function () {
                            const radius = this.control.parent().width() / 2;
                            this.option("radius", radius);
                            this.option("width", radius/4);
                        });
                        let collapsable = $('#' + major_key + '_' + minor_key + '_collapsible'); //temp_water_collapsible
                        collapsable.on('collapsibleexpand', function(event, ui) {
                            let obj = $('#' + major_key + '_' + minor_key);
                            const radius = obj.parent().width() / 2;
                            console.log("expand", obj.roundSlider({radius:radius, width:radius/4}));
                        });
                    }
                });
            }, this[major_key]);
        }, _ui.readonly);

        // event.targettemp1 > div > div > div > div

        Object.keys(_ui.readwrite).forEach( function(major_key) {
            Object.keys(this[major_key]).forEach( function(minor_key) {
                this[minor_key].obj = $('#' + major_key + '_' + minor_key);
                this[minor_key].obj_lbl = $('#' + major_key + '_' + minor_key + '_lbl');
                this[minor_key].obj.roundSlider({
                    sliderType: "min-range",
                    circleShape: "half-top",
                    handleShape: "round",
                    value: 0,
                    readOnly: false,
                    min: this[minor_key].min,
                    max: this[minor_key].max,
                    step: 1,
                    radius: 120, // determines width & height
                    width: 35,
                    svgMode: true,
                    animation: false,
                    keyboardAction: true,
                    mouseScrollAction: true,
                    tooltipFormat: function(args) {
                        return args.value + " " + _ui.readwrite[major_key][minor_key].unit;
                    },
                    drag: function(args) {
                        update_round_slider(_ui.readwrite[major_key][minor_key], args.value);
                    },
                    change: function(args) {
                        update_round_slider(_ui.readwrite[major_key][minor_key], args.value);
                        let pair = {};
                        const key1 = 'homeassistant/climate/pool/' + minor_key + '_heater/set_temp';
                        pair[key1] = args.value
                        send_message(pair);
                    },
                    beforeCreate: function () {
                        this.options.radius = this.control.parent().width() / 2;
                        this["_bind"]($(window), "resize", function () {
                            const radius = this.control.parent().width() / 2;
                            this.option("radius", radius);
                            this.option("width", radius/4);
                        });
                        let collapsable = $('#' + major_key + '_' + minor_key + '_collapsible'); //temp_water_collapsible
                        collapsable.on('collapsibleexpand', function(event, ui) {
                            let obj = $('#' + major_key + '_' + minor_key);
                            const radius = obj.parent().width() / 2;
                            console.log("expand", obj.roundSlider({radius:radius, width:radius/4}));
                        });
                    }
                });
                this[minor_key].track_obj = $('#' + major_key + '_' + minor_key + ' > div > div.rs-inner-container > div > svg > path.rs-transition.rs-range')[0]; /* svg mode */
                //this[minor_key].track_obj = $('#' + major_key + '_' + minor_key + ' > div > div.rs-inner-container > div > div')[0];  /* for non-svg mode */
            }, this[major_key]);
        }, _ui.readwrite);

        // register change notifications

        $(".circuit").on("change", function(event) {
            let pair = {};
            pair["homeassistant/switch/pool/" + event.target.value + "/set"] = event.target.checked ? "ON" : "OFF";
            send_message(pair);
        });
        $(".heat_src").on("change", function(event) {
            let pair = {};
            pair["homeassistant/climate/pool/" + event.target.name + "/set_mode"] = event.target.value;
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

    // call `initialize`, once the page DOM is ready for JavaScript code
    $(document).ready(initialize);
})();