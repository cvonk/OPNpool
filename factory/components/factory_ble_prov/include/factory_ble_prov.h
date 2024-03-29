#pragma once

#include <protocomm_security.h>
#include <wifi_provisioning/wifi_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Get state of WiFi Station during provisioning
 *
 * @note    WiFi is initially configured as AP, when
 *          provisioning starts. After provisioning data
 *          is provided by user, the WiFi is reconfigured
 *          to run as both AP and Station.
 *
 * @param[out] state    Pointer to wifi_prov_sta_state_t variable to be filled
 *
 * @return
 *  - ESP_OK    : Successfully retrieved wifi state
 *  - ESP_FAIL  : Provisioning app not running
 */
esp_err_t ble_prov_get_wifi_state(wifi_prov_sta_state_t* state);

/**
 * @brief   Get reason code in case of WiFi station
 *          disconnection during provisioning
 *
* @param[out] reason    Pointer to wifi_prov_sta_fail_reason_t variable to be filled
 *
 * @return
 *  - ESP_OK    : Successfully retrieved wifi disconnect reason
 *  - ESP_FAIL  : Provisioning app not running
 */
esp_err_t ble_prov_get_wifi_disconnect_reason(wifi_prov_sta_fail_reason_t* reason);

/**
 * @brief   Checks if device is provisioned
 * *
 * @param[out] provisioned  True if provisioned, else false
 *
 * @return
 *  - ESP_OK      : Retrieved provision state successfully
 *  - ESP_FAIL    : Failed to retrieve provision state
 */
esp_err_t ble_prov_is_provisioned(bool *provisioned);

/**
 * @brief   Runs WiFi as Station
 *
 * Configures the WiFi station mode to connect to the
 * SSID and password specified in config structure,
 * and starts WiFi to run as station
 *
 * @param[in] wifi_cfg  Pointer to WiFi cofiguration structure
 *
 * @return
 *  - ESP_OK      : WiFi configured and started successfully
 *  - ESP_FAIL    : Failed to set configuration
 */
esp_err_t ble_prov_configure_sta(wifi_config_t * const wifi_cfg);

/**
 * @brief   Start provisioning via Bluetooth
 *
 * @param[in] security  Security mode
 * @param[in] pop       Pointer to proof of possession (NULL if not present)
 *
 * @return
 *  - ESP_OK      : Provisioning started successfully
 *  - ESP_FAIL    : Failed to start
 */
esp_err_t ble_prov_start_provisioning(const char *ble_device_name_prefix, int security, char const * const pop);

#ifdef __cplusplus
}
#endif
