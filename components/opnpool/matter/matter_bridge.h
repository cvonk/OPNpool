/**
 * @file matter_bridge.h
 * @brief Matter over Thread bridge for OPNpool integration.
 *
 * @details
 * Provides the bridge layer between the OPNpool pool state and Matter device
 * clusters. Initializes Matter endpoints, handles attribute updates bidirectionally,
 * and manages Thread network commissioning.
 *
 * This module requires an ESP32-C6 or ESP32-H2 with Thread (802.15.4) radio support.
 * Matter support is conditionally compiled when USE_MATTER is defined.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#ifdef USE_MATTER

#include <esp_err.h>
#include <esp_matter.h>
#include <esp_matter_core.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "../core/poolstate.h"
#include "../pool_task/network_msg.h"
#include "../utils/enum_helpers.h"

namespace esphome {
namespace opnpool {
namespace matter {

/// @name Configuration Structures
/// @brief Configuration types for Matter commissioning.
/// @{

/**
 * @brief Configuration for Matter commissioning.
 *
 * @details
 * Contains parameters needed for device pairing via QR code or manual entry.
 */
struct matter_config_t {
    uint16_t discriminator;  ///< Device discriminator for pairing (0-4095).
    uint32_t passcode;       ///< Setup passcode for commissioning (1-99999998).
};

/// @}

/// @name Matter Endpoint Indices
/// @brief Constants for indexing Matter endpoints.
/// @{

constexpr uint8_t MATTER_POOL_THERMO_IDX = 0;  ///< Pool thermostat endpoint index.
constexpr uint8_t MATTER_SPA_THERMO_IDX  = 1;  ///< Spa thermostat endpoint index.
constexpr uint8_t MATTER_NUM_THERMOSTATS = 2;  ///< Total thermostat endpoints.
constexpr uint8_t MATTER_NUM_CIRCUITS    = 9;  ///< Total circuit (switch) endpoints.
constexpr uint8_t MATTER_NUM_TEMP_SENSORS = 2; ///< Total temperature sensor endpoints.

/// @}

/// @name Matter Bridge Class
/// @brief Main class for Matter over Thread integration.
/// @{

/**
 * @brief Matter bridge for OPNpool.
 *
 * @details
 * Manages the Matter data model, creates endpoints for pool entities, handles
 * bidirectional attribute synchronization between pool state and Matter clusters,
 * and queues commands from Matter controllers for transmission to the pool.
 */
class MatterBridge {
public:
    MatterBridge() = default;
    ~MatterBridge();

    /**
     * @brief Initialize Matter stack and create endpoints.
     *
     * @param[in] config Pointer to Matter configuration.
     * @return           ESP_OK on success, error code on failure.
     */
    esp_err_t init(matter_config_t const * config);

    /**
     * @brief Update Matter attributes from pool state.
     *
     * @details
     * Called from the main loop to synchronize pool state changes to Matter
     * attribute values. Updates thermostat temperatures, circuit on/off states,
     * and sensor readings.
     *
     * @param[in] state Pointer to current pool state.
     */
    void update_from_poolstate(poolstate_t const * state);

    /**
     * @brief Get pending command from Matter controller.
     *
     * @details
     * Retrieves the next command queued by Matter attribute write callbacks.
     * These commands should be forwarded to the pool controller via IPC.
     *
     * @param[out] msg Pointer to network message to populate.
     * @return         True if a command was retrieved, false if queue empty.
     */
    bool get_pending_command(network_msg_t * msg);

    /**
     * @brief Check if Matter is commissioned and connected.
     *
     * @return True if device is commissioned to a Matter fabric.
     */
    bool is_commissioned() const;

    /**
     * @brief Get QR code payload for commissioning.
     *
     * @param[out] buf      Buffer to write QR code string.
     * @param[in]  buf_size Size of buffer.
     * @return              Length of QR code string, or 0 on error.
     */
    size_t get_qr_code(char * buf, size_t buf_size) const;

    /**
     * @brief Matter attribute update callback (static).
     *
     * @details
     * Called by the Matter SDK when an attribute is read or written.
     * Handles PRE_UPDATE for writes (to validate and queue commands).
     *
     * @param[in] type         Callback type (PRE_UPDATE, POST_UPDATE, READ).
     * @param[in] endpoint_id  The endpoint being accessed.
     * @param[in] cluster_id   The cluster being accessed.
     * @param[in] attribute_id The attribute being accessed.
     * @param[in] val          Pointer to attribute value.
     * @param[in] priv_data    Private data (MatterBridge pointer).
     * @return                 ESP_OK to allow, error to reject.
     */
    static esp_err_t attribute_update_callback(
        esp_matter::attribute::callback_type_t type,
        uint16_t endpoint_id,
        uint32_t cluster_id,
        uint32_t attribute_id,
        esp_matter_attr_val_t * val,
        void * priv_data
    );

    /**
     * @brief Matter identification callback (static).
     *
     * @details
     * Called when a Matter controller requests device identification
     * (e.g., blink LED, beep). Used during commissioning.
     *
     * @param[in] type        Identification type.
     * @param[in] endpoint_id The endpoint to identify.
     * @param[in] effect_id   The effect to display.
     * @param[in] effect_variant Effect variant.
     * @param[in] priv_data   Private data (MatterBridge pointer).
     * @return                ESP_OK on success.
     */
    static esp_err_t identification_callback(
        esp_matter::identification::callback_type_t type,
        uint16_t endpoint_id,
        uint8_t effect_id,
        uint8_t effect_variant,
        void * priv_data
    );

private:
    /// @brief Create thermostat endpoints for pool and spa.
    esp_err_t create_thermostat_endpoints();

    /// @brief Create OnOff endpoints for pool circuits.
    esp_err_t create_circuit_endpoints();

    /// @brief Create temperature sensor endpoints.
    esp_err_t create_temperature_sensor_endpoints();

    /// @brief Find circuit index from endpoint ID.
    int find_circuit_index(uint16_t endpoint_id) const;

    /// @brief Find thermostat index from endpoint ID.
    int find_thermostat_index(uint16_t endpoint_id) const;

    /// @brief Handle OnOff cluster write.
    esp_err_t handle_onoff_write(uint16_t endpoint_id, bool value);

    /// @brief Handle Thermostat setpoint write.
    esp_err_t handle_thermostat_setpoint_write(uint16_t endpoint_id, int16_t temp_centi_c);

    /// @brief Handle Thermostat mode write.
    esp_err_t handle_thermostat_mode_write(uint16_t endpoint_id, uint8_t mode);

    esp_matter::node_t * node_ = nullptr;  ///< Matter node handle.

    /// Endpoint IDs for thermostats (pool, spa).
    uint16_t thermostat_endpoints_[MATTER_NUM_THERMOSTATS] = {0};

    /// Endpoint IDs for circuits (switches).
    uint16_t circuit_endpoints_[MATTER_NUM_CIRCUITS] = {0};

    /// Endpoint IDs for temperature sensors (water, air).
    uint16_t temp_sensor_endpoints_[MATTER_NUM_TEMP_SENSORS] = {0};

    /// Pending commands queue (from Matter → Pool).
    QueueHandle_t pending_commands_q_ = nullptr;

    /// Configuration copy.
    matter_config_t config_ = {};

    /// Commissioning status.
    bool commissioned_ = false;
};

/// @}

/**
 * @brief Convert Fahrenheit to Celsius.
 *
 * @param[in] f Temperature in Fahrenheit.
 * @return      Temperature in Celsius.
 */
[[nodiscard]] inline float
matter_fahrenheit_to_celsius(float f)
{
    return (f - 32.0f) * 5.0f / 9.0f;
}

/**
 * @brief Convert Celsius to Fahrenheit.
 *
 * @param[in] c Temperature in Celsius.
 * @return      Temperature in Fahrenheit.
 */
[[nodiscard]] inline float
matter_celsius_to_fahrenheit(float c)
{
    return c * 9.0f / 5.0f + 32.0f;
}

}  // namespace matter
}  // namespace opnpool
}  // namespace esphome

#endif  // USE_MATTER
