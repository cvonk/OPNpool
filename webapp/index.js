// time range slider: http://jsfiddle.net/ezanker/fu26u/204/

// For an introduction to the Blank template, see the following documentation:
// http://go.microsoft.com/fwlink/?LinkID=397704
// To debug code on page load in Ripple or on Android devices/emulators: launch your app, set breakpoints, 
// and then run "window.location.reload()" in the JavaScript Console.
(function () {
    "use strict";

    var ipName; // = "http://pool.vonk";
    var intervalId = 0;

    // request the parent's origin (sending a message to the parent tells it we're loaded),
    // the parent reply contains its origin.
    window.addEventListener('message', event => {
        ipName = event.origin;
        //alert(event.origin);
    }, false);
    parent.postMessage('fromIframe', '*');
    
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
        var value = $.inArray('pool', active) > -1;
        $("#cCircuitPool").prop("checked", value).checkboxradio("refresh");

        value = $.inArray('spa', active) > -1;
        $("#cCircuitSpa").prop("checked", value).checkboxradio("refresh");

        value = $.inArray('aux1', active) > -1;
        $("#cCircuitAux1").prop("checked", value).checkboxradio("refresh");
    }

    function updateChlor(chlor) {
        if (!jQuery.isEmptyObject(chlor)) {
            gChlorSalt.refresh(chlor.salt);

            var title = "chlorinator";
            if (chlor.status !== 'ok') {
                title += "\nerror: " + chlor.status;
            }
            gChlorPct.refresh(chlor.pct);
            gChlorPct.txtTitle.attr("text", title);
        }
    }

    function updateTemp(active, pool, spa, air) {
        //var title = active + " temp.";
        //if (($.inArray('pool', active) > -1) && pool.heating) {
        //    title += pool.src;
        //}
        //if (($.inArray('spa', active) > -1) && spa.heating) {
        //    title += spa.src;
        //}
        gTempWater.refresh(fahrenheit2centigrade(pool.temp));
        //gTempWater.txtTitle.attr("text", title);
        gTempAir.refresh(fahrenheit2centigrade(air.temp));
    }

    function updateHeating(pool, spa) {
        $("#sPoolSp").val(pool.sp).slider('refresh');
        $("#sSpaSp").val(spa.sp).slider('refresh');

        var selector = "#cPoolHeatSrc" + capitalizeFirstLetter(pool.src);
        $(selector).prop("checked", true).checkboxradio("refresh");

        selector = "#cSpaHeatSrc" + capitalizeFirstLetter(spa.src);
        $(selector).prop("checked", true).checkboxradio("refresh");
    }

    function updatePump(pump) {
        var title = "pump speed";
        if (pump.running && pump.mode !== "filter") {
            title += "\nmode: " + pump.mode;
        }
        gPumpRpm.refresh(pump.rpm);
        gPumpRpm.txtTitle.attr("text", title);

        title = "pump power";
        if (pump.status !== 0) {
            title += "\nstatus: " + pump.status;
        } else if (pump.err !== 0) {
            title += "\nerror: " + pump.err;
        }
        gPumpPwr.refresh(pump.pwr);
        gPumpPwr.txtTitle.attr("text", title);
    }

    function disableInputs(value) {
        $("input[type='checkbox']").prop("disabled", value); //.checkboxradio("refresh");
        $("input[type='radio']").prop("disabled", value); // .checkboxradio("refresh");
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
        updateTime(jsonData.tod);
        updateCircuits(jsonData.active);
        updateChlor(jsonData.chlor);
        updateTemp(jsonData.active, jsonData.pool, jsonData.spa, jsonData.air);
        updateHeating(jsonData.pool, jsonData.spa);
        updatePump(jsonData.pump);
    }

    var firstupdate = 1;

    function sendMessage(pair) {
        var request = !jQuery.isEmptyObject(pair);
        $.ajax({
            dataType: "json",
            url: "http://" + ipName + "/json" + "?callback=?",
            data: pair,
            success: function (jsonData) {
                console.log(jsonData);
                if (request) {
                    clearInterval(intervalId);
                    intervalId = 0;
                } else {
                    update(jsonData);
                    showSpinner("");
                    console.log("disable inputs");
                    disableInputs(false);
                    console.log("disable inputs");
                    overlayPage(false);
                    firstupdate = 0;
                }
                if (intervalId === 0) { // schedule after request, or on first invocation
                    intervalId = setInterval(scheduledUpdate, 10000);
                }
            },
            beforeSend: function () {
                if (firstupdate || request) {
                    showSpinner(request ? "Requesting\n(please wait 10 sec)" : "Loading data");
                    disableInputs(true);
                    overlayPage(true);
                }
            },
            error: function () {
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

        var gChlorSaltParams = {
            id: "gChlorSalt",
            value: 0,
            min: 0,
            max: 6500,
            relativeGaugeSize: true,
            valueFontColor: "white",
            showMinMax: true,
            title: "salt",
            label: "ppm",
            levelColors: ['#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#0000FF', '#48CCCD', '#48CCCD', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#85A137', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800', '#FF2800'],
            levelColorsGradient: false
        };
        gChlorSalt = new JustGage(gChlorSaltParams);

        gChlorPct = new JustGage({
            id: "gChlorPct",
            value: 0,
            min: 0,
            max: 100,
            relativeGaugeSize: true,
            valueFontColor: "white",
            showMinMax: true,
            title: "chlorinator",
            label: "%"
        });

        gTempWater = new JustGage({
            id: "gTempWater",
            value: 0,
            min: 0,
            max: 50,
            relativeGaugeSize: true,
            valueFontColor: "white",
            showMinMax: true,
            title: "Water",
            label: "°C",
            levelColors: ['#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#C9DB39', '#F5C80D', '#F1933C', '#F1933C', '#F1933C', '#F1933C', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832'],
            levelColorsGradient: false
        });

        gTempAir = new JustGage({
            id: "gTempAir",
            value: 0,
            min: 0,
            max: 50,
            relativeGaugeSize: true,
            valueFontColor: "white",
            showMinMax: true,
            title: "Air",
            label: "°C",
            levelColors: ['#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#147EB2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#0386B2', '#C9DB39', '#F5C80D', '#F1933C', '#F1933C', '#F1933C', '#F1933C', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832', '#DD3832'],
            levelColorsGradient: false
        });

        gPumpRpm = new JustGage({
            id: "gPumpRpm",
            value: 0,
            min: 0,
            max: 3450,
            relativeGaugeSize: true,
            valueFontColor: "white",
            showMinMax: true,
            title: "pump speed",
            label: "rpm"
        });

        gPumpPwr = new JustGage({
            id: "gPumpPwr",
            value: 0,
            min: 0,
            max: 1300,
            relativeGaugeSize: true,
            valueFontColor: "white",
            showMinMax: true,
            title: "pump power",
            label: "watts"
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

        $(".my-circuit").on("change", function (event) {
            var pair = {};
            pair[event.target.value + "-active"] = event.target.checked ? 1 : 0;
            console.log(pair);
            sendMessage(pair);
        });

        $(".heatsrc").on("change", function (event) {
            var pair = {};
            pair[event.target.name + "-src"] = event.target.value;
            console.log(pair);
            sendMessage(pair);
        });

        $(".my-slider").on("slidestop", function (event) {
            var pair = {};
            pair[event.target.name] = event.target.value;
            console.log(pair);
            sendMessage(pair);
        });

        scheduledUpdate();
    }

    $(document).ready(initialize);

})();
