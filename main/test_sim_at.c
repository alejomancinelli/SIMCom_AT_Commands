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
    
    /* --- 1. Send basic AT --- */
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
    err = get_phone_functionality(&phone_functionality);
    if (phone_functionality == MINIMUN_FUNCTIONALITY)
        ESP_LOGI(TAG, "Phone with minimun functionality!");
    if (phone_functionality == FULL_FUNCTIONALITY)
        ESP_LOGI(TAG, "Phone with full functionality!");
    
    int rssi = 99, ber = 99; 
    while (rssi == 99)
    {
        query_signal_quality(&rssi, &ber);
        ESP_LOGW(TAG, "No signal! Rssi: %d. Waiting connection...", rssi);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    sim_simcard_pin_code_t simcard_code;
    get_simcard_pin_info(&simcard_code);
    ESP_LOGI(TAG, "SIM Card code: %d", simcard_code);

    sim_network_registration_stat_t stat;
    network_registration(&stat);

    sim_gprs_network_registration_stat_t gprs_stat;
    gprs_network_registration(&gprs_stat);

    int attach;
    get_packet_domain_attach(&attach);

    int cid, state;
    get_pdp_context_activate(&cid, &state);

    get_pdp_context();

    char addr[32];
    show_pdp_address(&cid, addr);

    // ping("www.google.com.ar");

    // NTP
    get_ntp_config();
    set_ntp_config("pool.ntp.org", -3);
    ntp_update_system_time();
    char current_time[128];
    get_rtc_time(current_time);

    // --- MQTT ---
    start_mqtt_service();
    acquire_mqtt_client(0, "Bemakoha4G");
    connect_mqtt_server(0, "tcp://app.bemakoha.com:1883", 20, 1);
    vTaskDelay(pdMS_TO_TICKS(5000));
    mqtt_topic(0, "4gTest");
    mqtt_payload(0, "Hola!");
    mqtt_publish(0, 0, 60);
    vTaskDelay(pdMS_TO_TICKS(5000));
    disconnect_mqtt_server(0, 60);
    release_mqtt_client(0);
    stop_mqtt_service();

    ESP_LOGI(TAG, "Shutting down SIMCom module");
    // power_down_module();
    
    ESP_LOGI(TAG, "Test done.");
}
