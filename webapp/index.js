// time range slider: http://jsfiddle.net/ezanker/fu26u/204/

// To develop: use vsCode with Live Server plugin
// To debug code on page load run "window.location.reload()" in the JavaScript Console.
(function() {
    "use strict";

    var ipName; // = "http://esp32-wrover-1.iot.vonk";
    var intervalId = 0;

    // request the parent's origin (sending a message to the parent tells it we're loaded),
    // the parent reply contains its origin.
    if (1) {
        window.addEventListener('message', event => {
            ipName = event.origin;
            console.log(ipName);
            //alert(event.origin);
            scheduledUpdate();
        }, false);
        parent.postMessage('fromIframe', '*');
    } else {
        ipName = "http://esp32-wrover-1.iot.vonk";
        console.log(ipName);
        scheduledUpdate();
    }

    document.addEventListener('deviceready', onDeviceReady.bind(this), false);

    function onDeviceReady() {
        // Handle the Cordova pause and resume events
        document.addEventListener('pause', onPause.bind(), false); // .bind(this)
        document.addEventListener('resume', onResume.bind(), false); // .bind(this)

        // TODO: Cordova has been loaded. Perform any initialization that requires Cordova here.
    }

    function onPause() {
        // TODO: This application has been suspended. Save application state here.
    }

    function onResume() {
        // TODO: This application has been reactivated. Restore application state here.
    }

    //---------still of use---------
    // schedule.pool.start: '08:15'
    // schedule.pool.stop: '13:30'

    var gChlorSalt = {};
    var gChlorPct = {};
    var gTempWater = {};
    var gTempAir = {};
    var gPumpRpm = {};
    var gPumpPwr = {};

    function fahrenheit2centigrade(f) {
        return Math.round(((f - 32) * 5) / 9);
    }

    function capitalizeFirstLetter(string) {
        return string.charAt(0).toUpperCase() + string.slice(1);
    }

    /*
    function updateRoundSlider(v) {
        $("#slider-salt").roundSlider("setValue", v); // only needed when called directly
        var el = $('#slider-salt > div > div.rs-inner-container > div > div')[0];
        el.style.backgroundColor = v < 1000 ? '#2266f7' : v < 3000 ? '#43BC53' : '#CD5342';
    }

    function sliderValueOnKnob(obj) {
        obj.closest(".ui-slider").find(".ui-slider-handle").text(obj.val());
    }
    */

    function circuitChanged(obj) {
        console.log(obj.val() + ' = ' + obj.target.value);
        return obj;
    }

    function updateTime(tod) {
        $("#date").val(tod.date);
        $("#time").val(tod.time);
    }

    function updateCircuits(active) {
        // var value = active.POOL$.inArray('pool', active) > -1;
        var value = active.POOL;
        $("#cCircuitPool").attr("checked", value).checkboxradio("refresh");

        //value = $.inArray('spa', active) > -1;
        value = active.SPA;
        $("#cCircuitSpa").attr("checked", value).checkboxradio("refresh");

        //value = $.inArray('aux1', active) > -1;
        value = active.AUX;
        $("#cCircuitAux1").attr("checked", value).checkboxradio("refresh");
    }

    function updateChlor(chlor) {
        if (!jQuery.isEmptyObject(chlor)) {
            var title = "Chlorinator";
            if (chlor.status !== 'OK') {
                title += chlor.status;
            }
            gChlorSalt.refresh(chlor.salt);
            gChlorPct.refresh(chlor.pct);
            gChlorPct.txtLabel.attr("text", title);
        }
    }

    function updateTemp(active, pool, spa, air_temp) {
        var water_label = "Water temp. ";
        if (active.POOL && pool.heating) {
            water_label += " " + pool.src;
        }
        if (active.SPA && spa.heating) {
            water_label += " " + spa.src;
        }
        gTempWater.refresh(fahrenheit2centigrade(pool.temp));
        gTempWater.txtLabel.attr("text", water_label);
        gTempAir.refresh(fahrenheit2centigrade(air_temp));
    }

    function updateHeating(pool, spa) {
        $("#sPoolSp").val(pool.sp).slider('refresh');
        $("#sSpaSp").val(spa.sp).slider('refresh');

        var selector = "#cPoolHeatSrc" + capitalizeFirstLetter(pool.src);
        $(selector).attr("checked", true).checkboxradio("refresh");

        selector = "#cSpaHeatSrc" + capitalizeFirstLetter(spa.src);
        $(selector).attr("checked", true).checkboxradio("refresh");
    }

    function updatePump(pump) {
        var title = "Pump speed";
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

    function disableInputs(value) {
        $("input[type='checkbox']").attr("disabled", value); //.checkboxradio("refresh");
        $("input[type='radio']").attr("disabled", value); // .checkboxradio("refresh");
        $(".my-setpoint").slider(value ? "disable" : "enable"); //.slider("refresh");
    }

    function overlayPage(value) {
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

    function showSpinner(value) {
        if (value.length) {
            $.mobile.loading('show', {
                text: value,
                textVisible: true
            });
        } else {
            $.mobile.loading('hide');
        }
    }

    function update(jsonData) {
        //updateRoundSlider(jsonData.chlor.salt);
        updateTime(jsonData.system.tod);
        updateCircuits(jsonData.circuits.active);
        updateChlor(jsonData.chlor);
        updateTemp(jsonData.circuits.active, jsonData.thermostats.POOL, jsonData.thermostats.SPA, jsonData.temps.AIR);
        updateHeating(jsonData.thermostats.POOL, jsonData.thermostats.SPA);
        updatePump(jsonData.pump);
    }

    var firstupdate = 1;

    function sendMessage(pair) {
        var url = ipName + "/json";
        var request = !jQuery.isEmptyObject(pair);
        //alert(url);
        $.ajax({
            dataType: "jsonp",
            url: url + "?callback=?",  
            data: pair,
            success: function(jsonData) {
                console.log(jsonData);
                if (request) {
                    clearInterval(intervalId);
                    intervalId = 0;
                } else {
                    update(jsonData);
                    showSpinner("");
                    //console.log("disable inputs");
                    disableInputs(false);
                    //console.log("disable inputs");
                    overlayPage(false);
                    firstupdate = 0;
                }
                if (intervalId === 0) { // schedule after request, or on first invocation
                    intervalId = setInterval(scheduledUpdate, 10000);
                }
            },
            beforeSend: function() {
                if (firstupdate || request) {
                    showSpinner(request ? "Requesting\n(please wait 10 sec)" : "Loading data");
                    disableInputs(true);
                    overlayPage(true);
                }
            },
            error: function() {
                showSpinner("Retry in 10 seconds\n(" + ipName + ")");
            }
        });

    }

    function scheduledUpdate() {
        sendMessage({});
    }

    function initialize() {
        //hideInputElements(true);

        /*    $("#slider-salt").roundSlider({
                sliderType: "min-range",
                circleShape: "half-top",
                handleShape: "round",
                value: 0,
                readOnly: false,
                min: 0,
                max: 5000,
                tooltipFormat: function (args) {
                    return args.value + " ppm";
                },
                drag: function (args) {
                    updateRoundSlider(args.value);
                },
                change: function (args) {
                    updateRoundSlider(args.value);
                }
            });
        */

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
            label: 'Water temp.',
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
            label: 'Air temp.',
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

        // make horizontal sliders more fancy (color grade slider track, print value on knob)
        // (https://jqmtricks.wordpress.com/2014/04/21/fun-with-the-slider-widget/)
        var colorback = '<div class="sliderBackColor bg-blue"></div>';
        colorback += '<div class="sliderBackColor bg-green"></div>';
        colorback += '<div class="sliderBackColor bg-orange"></div>';
        $(".my-slider .ui-slider-track").prepend(colorback);
        /*
        sliderValueOnKnob($("#sPoolSp"));
        sliderValueOnKnob($("#sSpaSp"));
        $(".my-setpoint").on("change", function () {
            sliderValueOnKnob($(this));
        });*/

        // register change notifications

        $(".my-circuit").on("change", function(event) {
            var pair = {};
            pair["homeassistant/switch/esp32-wrover-1/" + event.target.value + "_circuit/set"] = event.target.checked ? "ON" : "OFF";
            console.log(pair);
            sendMessage(pair);
        });
        $(".heatsrc").on("change", function(event) {
            var pair = {};
            pair["?homeassistant/climate/esp32-wrover-1/" + event.target.name + "/set_mode"] = event.target.value;
            console.log(pair);
            sendMessage(pair);
        });
        $(".my-slider").on("slidestop", function(event) {
            var pair = {};
            pair["?homeassistant/climate/esp32-wrover-1/" + event.target.name + "/set_temp"] = event.target.value;
            console.log(pair);
            sendMessage(pair);
        });

    }

    $(document).ready(initialize);

})();
