#include "sim_at.h"
#include "esp_log.h"
#include "sim_status_control_at.h"

static const char *TAG = "SIM_AT_TEST";

void app_main(void)
{
    sim_at_config_t cfg = {
        .uart_port = UART_NUM_1,
        .tx_pin = 18,
        .rx_pin = 17,
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
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
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

    // /* --- 2. Try async command --- */
    // ESP_LOGI(TAG, "Sending 'ATI' asynchronously...");
    // err = sim_at_cmd_async("ATI\r\n", async_cb, NULL, 3000);
    // if (err != SIM_AT_OK)
    //     ESP_LOGE(TAG, "ASYNC send failed: %d", err);

    // vTaskDelay(pdMS_TO_TICKS(4000));  // allow async cb to print

    // /* --- 3. Send shutdown command --- */
    
    sim_status_control_fun_t phone_functionality;
    err = get_phone_functionality(&phone_functionality);
    if (phone_functionality == MINIMUN_FUNCTIONALITY)
        ESP_LOGI(TAG, "Phone with minimun functionality!");
    if (phone_functionality == FULL_FUNCTIONALITY)
        ESP_LOGI(TAG, "Phone with full functionality!");
    
    err = set_phone_functionality(FULL_FUNCTIONALITY);
    
    if (err == SIM_AT_OK)
        ESP_LOGI(TAG, "Shutdown command OK, resp='%s'", resp);
    else
        ESP_LOGE(TAG, "Shutdown command failed: %d", err);

    ESP_LOGI(TAG, "Test done.");
}
