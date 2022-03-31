    const bodyContent = `
    <div data-role="page" data-theme="b" data-content-theme="b">
    <div data-role="content">
        <div id="colborders" data-role="collapsibleset" data-iconpos="right">
            <div id="circuits_collapsible" data-role="collapsible">
                <h3>Circuits</h3>
                <label id="circuits_lbl"></label>
                <div class="ui-grid-b ui-responsive">
                    <div class="ui-block-a center-wrapper">
                        <form id="circuit">
                            <fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
                                <input id="circuit_pool" type="checkbox" class="circuit" value="pool_circuit"/>
                                <label for="circuit_pool">Pool</label>
                                <input id="circuit_spa" type="checkbox" class="circuit" value="spa_circuit" />
                                <label for="circuit_spa">Spa</label>
                            </fieldset>
                        </form>
                    </div>
                    <div class="ui-block-b center-wrapper">
                        <form id="circuit">
                            <fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
                                <input id="circuit_aux1" type="checkbox" class="circuit" value="aux1_circuit" />
                                <label for="circuit_aux1">Aux1</label>
                                <input id="circuit_aux2" type="checkbox" class="circuit" value="aux2_circuit" />
                                <label for="circuit_aux2">Aux2</label>
                                <input id="circuit_aux3" type="checkbox" class="circuit" value="aux3_circuit" />
                                <label for="circuit_aux3">Aux3</label>
                            </fieldset>
                        </form>
                    </div>
                    <div class="ui-block-c center-wrapper">
                        <form id="circuit">
                            <fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
                                <input id="circuit_ft1" type="checkbox" class="circuit" value="ft1_circuit" />
                                <label for="circuit_ft1">Ft1</label>
                                <input id="circuit_ft2" type="checkbox" class="circuit" value="ft2_circuit" />
                                <label for="circuit_ft2">Ft2</label>
                                <input id="circuit_ft3" type="checkbox" class="circuit" value="ft3_circuit" />
                                <label for="circuit_ft3">Ft3</label>
                                <input id="circuit_ft4" type="checkbox" class="circuit" value="ft4_circuit" />
                                <label for="circuit_ft4">Ft4</label>
                            </fieldset>
                        </form>
                    </div>
                </div>
            </div>
            <!--
            <div id="temp_solar_collapsible" data-role="collapsible">
                <h3>Solar temp.</h3>
                <label id="temp_solar_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-solo center-wrapper">
                        <div id="temp_solar"></div>
                    </div>
                </div>
            </div>
            -->
            <div id="temp_water_collapsible" data-role="collapsible">
                <h3>Water temperature</h3>
                <label id="temp_water_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-solo center-wrapper">
                        <div id="temp_water"></div>
                    </div>
                </div>
            </div>
            <div id="temp_air_collapsible" data-role="collapsible">
                <h3>Air temperature</h3>
                <label id="temp_air_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-solo center-wrapper">
                        <div id="temp_air"></div>
                    </div>
                </div>
            </div>
            <div id="thermo_pool_collapsible" data-role="collapsible">
                <h3>Pool thermostat</h3>
                <label id="thermo_pool_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-a center-wrapper">
                        <div id="thermo_pool"></div>
                    </div>
                    <div class="ui-block-b center-wrapper">
                        <form id="pool_heater">
                            <fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
                                <input id="pool_heater_none" class="heat_src pool_heater" value="none" type="radio" name="pool_heater" />
                                <label for="pool_heater_none">None</label>
                                <input id="pool_heater_heat" class="heat_src pool_heater" value="heat" type="radio" name="pool_heater" />
                                <label for="pool_heater_heat">Heater</label>
                            </fieldset>
                        </form>
                        <form>
                            <fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
                                <input id="pool_heater_solar_pref" class="heat_src pool_heater" value="solar_pref" type="radio" name="pool_heater" />
                                <label for="pool_heater_solar_pref">Solar pref</label>
                                <input id="pool_heater_solar" class="heat_src pool_heater" value="solar" type="radio" name="pool_heater" />
                                <label for="pool_heater_solar">Solar</label>
                            </fieldset>
                        </form>
                    </div>
                </div>
            </div>
            <div id="thermo_spa_collapsible" data-role="collapsible">
                <h3>Spa thermostat</h3>
                <label id="thermo_spa_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-a center-wrapper">
                        <div id="thermo_spa"></div>
                    </div>
                    <div class="ui-block-b center-wrapper">
                        <form id="spa_heater">
                            <fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
                                <input id="spa_heater_none" class="heat_src spa_heater" value="none" type="radio" name="spa_heater" />
                                <label for="spa_heater_none">None</label>
                                <input id="spa_heater_heat" class="heat_src spa_heater" value="heat" type="radio" name="spa_heater" />
                                <label for="spa_heater_heat">Heater</label>
                            </fieldset>
                        </form>
                        <form>
                            <fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
                                <input id="spa_heater_solar_pref" class="heat_src spa_heater" value="solar_pref" type="radio" name="spa_heater" />
                                <label for="spa_heater_solar_pref">Solar pref</label>
                                <input id="spa_heater_solar" class="heat_src spa_heater" value="solar" type="radio" name="spa_heater" />
                                <label for="spa_heater_solar">Solar</label>
                            </fieldset>
                        </form>
                    </div>
                </div>
            </div>
            <div id="pump_rpm_collapsible" data-role="collapsible">
                <h3>Pump speed</h3>
                <label id="pump_rpm_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-solo center-wrapper">
                        <div id="pump_rpm"></div>
                    </div>
                </div>
            </div>
            <!--
            <div id="pump_pwr_collapsible" data-role="collapsible">
                <h3>Pump power</h3>
                <label id="pump_pwr_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-solo center-wrapper">
                        <div id="pump_pwr"></div>
                    </div>
                </div>
            </div>
            -->
            <div id="chlor_salt_collapsible" data-role="collapsible">
                <h3>Salt level</h3>
                <label id="chlor_salt_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-solo center-wrapper">
                        <div id="chlor_salt"></div>
                    </div>
                </div>
            </div>
            <div id="chlor_pct_collapsible" data-role="collapsible">
                <h3>Chlorinator</h3>
                <label id="chlor_pct_lbl"></label>
                <div class="ui-grid-a ui-responsive">
                    <div class="ui-block-solo center-wrapper">
                        <div id="chlor_pct"></div>
                    </div>
                </div>
            </div>
            <div id="schedules_collapsible" data-role="collapsible">
                <h3>Schedules</h3>
                <div class="ui-body ui-grid-b ui-responsive">
                    <div class="ui-block-a">
                        Pool
                    </div>
                    <div class="ui-block-b">
                        <input id="poolStart" type="time" disabled />
                    </div>
                    <div class="ui-block-c">
                        <input id="poolStop" type="time" disabled />
                    </div>
                </div>
                <div class="ui-body ui-grid-b ui-responsive">
                    <div class="ui-block-a">
                        Spa
                    </div>
                    <div class="ui-block-b">
                        <input id="spaStart" type="time" disabled />
                    </div>
                    <div class="ui-block-c">
                        <input id="spaStop" type="time" disabled />
                    </div>
                </div>
            </div>
            <div id="system_collapsible" data-role="collapsible">
                <h3>System</h3>
                <div class="ui-body ui-grid-a ui-responsive">
                    <div class="ui-block-a">
                        <input id="date" type="date" disabled />
                    </div>
                    <div class="ui-block-b">
                        <input id="time" type="time" disabled />
                    </div>
                </div>
            </div>
        </div>
    </div>
    <!-- <div id='screen'></div> -->
    <!-- <div class="ui-block-a rslider" id="slider-salt" style="width:40vw; height:32vw"></div> -->
</div>`;

// replace `BODY`, once the page DOM is ready for JavaScript code
$(document).ready(function() 
{
    console.log("load.js: load event, replacing body and add script");

    const body = document.querySelector("body");
    const newBody = document.createElement('body');
    newBody.innerHTML = bodyContent;
    body.parentNode.replaceChild(newBody, body);

    // needs to be loaded after the HTML BODY is in place
    var script = document.createElement('script');
    script.src = 'https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js';
    document.body.appendChild(script);    

    script = document.createElement('script');
    script.src = 'https://coertvonk.com/cvonk/pool/index.js';
    document.body.appendChild(script);    
});
