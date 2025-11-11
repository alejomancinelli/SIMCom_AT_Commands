#include "sim_at.h"
#include "esp_log.h"
#include "sim_status_control_at.h"
#include "sim_network_at.h"
#include "sim_simcard_at.h"
#include "sim_packet_domain_at.h"
#include "sim_mqtt_at.h"
#include "sim_internet_services_at.h"
#include "driver/gpio.h"

static const char *TAG = "SIM_AT_TEST";

void app_main(void)
{
    sim_at_config_t cfg = {
        .uart_port = UART_NUM_1,
        .tx_pin = GPIO_NUM_18,
        .rx_pin = GPIO_NUM_17,
        .rts_pin = -1,
        .cts_pin = -1,
        .use_hw_flow_control = false,
        .default_cmd_timeout_ms = 1000,
        .uart_conf = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        },
        .control_pins = {
            .dtr_pin = -1,     // change to your actual wiring or -1
            .pwrkey_pin = -1,  // change to your actual wiring or -1
            .rst_pin = -1,
        },
    };

    sim_at_enable_debug(true);
    ESP_ERROR_CHECK(sim_at_init(&cfg));

    sim_at_err_t err;
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    /* --- Send basic AT --- */
    char resp[SIM_AT_MAX_RESP_LEN];
    for (int i=0; i<5; i++)
    {
        ESP_LOGI(TAG, "Sending 'AT'...");
        err = sim_at_cmd_sync("AT\r\n", 1000);
        ignore_sim_response();
        // if (err == SIM_AT_OK)
        //     // ESP_LOGI(TAG, "SYNC OK: resp='%s'", resp);
        // else
        //     ESP_LOGE(TAG, "SYNC FAILED: %d", err);
    }
    
    sim_status_control_fun_t phone_functionality;
    err = sim_at_get_phone_functionality(&phone_functionality);
    if (phone_functionality == FUN_MINIMUN_FUNCTIONALITY)
    ESP_LOGI(TAG, "Phone with minimun functionality!");
    if (phone_functionality == FUN_FULL_FUNCTIONALITY)
    ESP_LOGI(TAG, "Phone with full functionality!");
    
    /* --- Status Control AT --- */
    int rssi = 99, ber = 99; 
    sim_at_query_signal_quality(&rssi, &ber);
    while (rssi == 99)
    {
        ESP_LOGW(TAG, "No signal! Rssi: %d. Waiting connection...", sim_rssi_to_dbm(rssi));
        vTaskDelay(pdMS_TO_TICKS(2000));
        sim_at_query_signal_quality(&rssi, &ber);
    }
    ESP_LOGI(TAG, "Module connected! Rssi: %d", sim_rssi_to_dbm(rssi));
    
    /* --- SIM Card AT --- */
    sim_simcard_pin_code_t simcard_code;
    sim_at_get_simcard_pin_info(&simcard_code);
    ESP_LOGI(TAG, "SIM Card code: %d", simcard_code);
    
    /* --- Network AT --- */
    sim_network_registration_stat_t stat;
    sim_at_network_registration(&stat);
    ESP_LOGI(TAG, "Network registration code: %s", sim_network_status_to_string(stat));
    
    /* --- Packet Domain AT --- */
    sim_eps_network_registration_stat_t gprs_stat;
    sim_at_eps_network_registration(&gprs_stat);
    ESP_LOGI(TAG, "EPS Network registration code: %s", sim_at_sim_eps_network_status_to_string(gprs_stat));
    
    int attach;
    sim_at_get_packet_domain_attach(&attach);
    ESP_LOGI(TAG, "Domain service state: %s", (attach > 0 ? "attach" : "detached"));
    // TODO: Falta probar el set
    
    int cid, state;
    sim_at_get_pdp_context_activate(&cid, &state);
    ESP_LOGI(TAG, "State of PDP context: %d --> %s", cid, (state > 0 ? "actived" : "deactivated"));
    
    sim_at_get_pdp_context();
    
    char addr[32];
    sim_at_show_pdp_address(&cid, addr);
    ESP_LOGI(TAG, "Show PDP address: %d --> %s", cid, addr);
    
    sim_at_ping("www.google.com.ar");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    /* --- NTP --- */
    sim_at_get_ntp_config();
    sim_at_set_ntp_config("pool.ntp.org", -3);
    sim_at_ntp_err_code_t ntp_err;
    sim_at_ntp_update_system_time(&ntp_err);
    if (ntp_err != NTP_OPERATION_SUCCEEDED)
        ESP_LOGE(TAG, "NTP update error: %d", (int)ntp_err);
    char current_time[128];                  
    sim_at_get_rtc_time(current_time);

    // // --- MQTT ---
    // start_mqtt_service();
    // acquire_mqtt_client(0, "Bemakoha4G");
    // connect_mqtt_server(0, "tcp://app.bemakoha.com:1883", 20, 1);
    // vTaskDelay(pdMS_TO_TICKS(5000));
    // mqtt_topic(0, "4gTest");
    // mqtt_payload(0, "Hola!");
    // mqtt_publish(0, 0, 60);
    // vTaskDelay(pdMS_TO_TICKS(5000));
    // disconnect_mqtt_server(0, 60);
    // release_mqtt_client(0);
    // stop_mqtt_service();

    ESP_LOGI(TAG, "Shutting down SIMCom module");
    // sim_at_power_down_module();
    
    ESP_LOGI(TAG, "Test done.");
}
