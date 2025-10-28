
/**
 * sim_at.c
 * Core AT engine implementation for SIMCom modem (ESP-IDF v5.3)
 *
 * NOTE: This implementation focuses on the core requested features. Higher-level
 * modules (SIM, MQTT, HTTP) are separate files (planned later).
 */

#include "sim_at.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "sim_at";

/* Internal configuration copy */
static sim_at_config_t g_cfg;
static bool g_inited = false;
static bool g_debug = false;

/* Parser task */
#define SIM_AT_PARSER_TASK_STACK 4096
#define SIM_AT_PARSER_TASK_PRIO 5
static TaskHandle_t s_parser_task = NULL;

/* UART parse response */
#define SIM_AT_MAX_LINES 5

static char s_responses[SIM_AT_MAX_LINES][SIM_AT_MAX_RESP_LEN];
static int s_resp_head = 0;  // write index
static int s_resp_tail = 0;  // read index
static int s_resp_count = 0; // number of stored responses

static char s_line_buf[SIM_AT_MAX_RESP_LEN];
static int s_line_pos = 0;

/* protects access to s_pending */
static SemaphoreHandle_t s_sync_sem = NULL;

const char *sim_at_err_to_str(sim_at_err_t err)
{
    ESP_LOGI(TAG, "%d", err);
    switch (err)
    {
    // TODO: No usa todos ni ahí, al pedo capaz tantos
    case SIM_AT_OK:
        return "SIM_AT_OK";
    case SIM_AT_ERR_INVALID_ARG:
        return "SIM_AT_ERR_INVALID_ARG";
    case SIM_AT_ERR_NO_MEM:
        return "SIM_AT_ERR_NO_MEM";
    case SIM_AT_ERR_TIMEOUT:
        return "SIM_AT_ERR_TIMEOUT";
    case SIM_AT_ERR_UART:
        return "SIM_AT_ERR_UART";
    case SIM_AT_ERR_BUSY:
        return "SIM_AT_ERR_BUSY";
    case SIM_AT_ERR_INTERNAL:
        return "SIM_AT_ERR_INTERNAL";
    case SIM_AT_ERR_NOT_INIT:
        return "SIM_AT_ERR_NOT_INIT";
    case SIM_AT_ERR_OVERFLOW:
        return "SIM_AT_ERR_OVERFLOW";
    case SIM_AT_ERR_ABORTED:
        return "SIM_AT_ERR_ABORTED";
    default:
        return "INVALID ERR";
    }
    return "INVALID ERR";
}

void get_sim_at_response(char *buf)
{
    strncpy(buf, s_responses[s_resp_tail++], SIM_AT_MAX_RESP_LEN - 1);
    if (s_resp_tail >= SIM_AT_MAX_LINES) s_resp_tail = 0;
}

void ignore_sim_response(void)
{
    s_resp_tail++;
    if (s_resp_tail >= SIM_AT_MAX_LINES) s_resp_tail = 0;
}

/* write raw command to UART (blocking) */
static sim_at_err_t prv_uart_write_cmd(const char *cmd)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;

    int len = strlen(cmd);

    int written = uart_write_bytes(g_cfg.uart_port, cmd, len);
    if (written != len)
        return SIM_AT_ERR_UART;

    if (g_debug)
    {
        char clean_cmd[SIM_AT_MAX_CMD_LEN]; // use a safe upper bound constant
        strncpy(clean_cmd, cmd, sizeof(clean_cmd) - 1);
        clean_cmd[len - 2] = '\0'; // ensure null termination

        ESP_LOGI(TAG, "--> %s", clean_cmd);
    }

    return SIM_AT_OK;
}

/* Parser task: reads bytes from UART, assembles lines, routes them */
static void s_parser_task_fn(void *arg)
{
    const TickType_t rx_wait = pdMS_TO_TICKS(50);
    uint8_t *data = (uint8_t *)malloc(SIM_AT_MAX_RESP_LEN + 1);

    while (1)
    {
        int len = uart_read_bytes(g_cfg.uart_port, data, SIM_AT_MAX_RESP_LEN, rx_wait);
        if (len <= 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        for (int i = 0; i < len; i++)
        {
            char c = (char)data[i];

            // Append to line buffer
            if (s_line_pos < SIM_AT_MAX_RESP_LEN - 1)
            {
                s_line_buf[s_line_pos++] = c;
                s_line_buf[s_line_pos] = '\0';
            }

            // Detect end of line (CRLF or LF)
            if (c == '\n')
            {
                // Trim CR/LF
                while (s_line_pos > 0 &&
                       (s_line_buf[s_line_pos - 1] == '\r' || s_line_buf[s_line_pos - 1] == '\n'))
                {
                    s_line_buf[--s_line_pos] = '\0';
                }

                if (s_line_pos <= 0)
                {
                    // Reset line buffer
                    s_line_pos = 0;
                    s_line_buf[0] = '\0';
                    continue;
                }

                // Write to circular buffer
                // xSemaphoreTake(s_resp_mutex, portMAX_DELAY); // TODO: Por el momento no pero no es mala

                // TODO: Esto parte del buffer se podría mejorar un poco seguramente
                if (s_resp_head < SIM_AT_MAX_LINES)
                {
                    // Write normally
                    strncpy(s_responses[s_resp_head], s_line_buf, SIM_AT_MAX_RESP_LEN - 1);
                    s_responses[s_resp_head][SIM_AT_MAX_RESP_LEN - 1] = '\0';
                    s_resp_head = (s_resp_head + 1) % SIM_AT_MAX_LINES;
                    s_resp_count++; // TODO: Sirve, pero ver bien cómo hacer después con esto
                }
                else
                {
                    // Buffer full: overwrite oldest
                    s_resp_head = 0;
                    strncpy(s_responses[s_resp_head], s_line_buf, SIM_AT_MAX_RESP_LEN - 1);
                    s_responses[s_resp_head][SIM_AT_MAX_RESP_LEN - 1] = '\0';
                }

                if (g_debug)
                    ESP_LOGI(TAG, "<-- %s", s_line_buf);

                // xSemaphoreGive(s_resp_mutex); // TODO: Por el momento no pero no es mala
                xSemaphoreGive(s_sync_sem); // Notify new response available

                // Reset line buffer
                s_line_pos = 0;
                s_line_buf[0] = '\0';
            }
        }
    }
    // free(data);
}

/* Public API implementations */

sim_at_err_t sim_at_init(const sim_at_config_t *cfg)
{
    if (!cfg)
        return SIM_AT_ERR_INVALID_ARG;
    if (g_inited)
        return SIM_AT_OK; /* idempotent */

    /* copy config */
    memcpy(&g_cfg, cfg, sizeof(g_cfg));

    /* create locks */
    s_sync_sem = xSemaphoreCreateBinary();
    if (!s_sync_sem)
        return SIM_AT_ERR_NO_MEM;

    /* uart config: user provides uart_conf; we just install driver */
    esp_err_t e;
    e = uart_driver_install(g_cfg.uart_port, SIM_AT_MAX_RESP_LEN * 2, 0, 0, NULL, 0);
    if (e != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install failed: %d", e);
        return SIM_AT_ERR_UART;
    }
    e = uart_param_config(g_cfg.uart_port, &g_cfg.uart_conf);
    if (e != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_param_config failed: %d", e);
        uart_driver_delete(g_cfg.uart_port);
        return SIM_AT_ERR_UART;
    }
    e = uart_set_pin(g_cfg.uart_port, g_cfg.tx_pin, g_cfg.rx_pin, g_cfg.rts_pin, g_cfg.cts_pin);
    if (e != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_set_pin failed: %d", e);
        uart_driver_delete(g_cfg.uart_port);
        return SIM_AT_ERR_UART;
    }
    ESP_LOGI(TAG, "UART port %d initialized on TX=%d, RX=%d", g_cfg.uart_port, g_cfg.tx_pin, g_cfg.rx_pin);

    // /* configure control pins as outputs if set */
    // if (g_cfg.control_pins.dtr_pin >= 0) {
    //     gpio_set_direction(g_cfg.control_pins.dtr_pin, GPIO_MODE_OUTPUT);
    //     gpio_set_level(g_cfg.control_pins.dtr_pin, 0);
    // }
    // if (g_cfg.control_pins.pwrkey_pin >= 0) {
    //     gpio_set_direction(g_cfg.control_pins.pwrkey_pin, GPIO_MODE_OUTPUT);
    //     gpio_set_level(g_cfg.control_pins.pwrkey_pin, 0);
    // }
    // if (g_cfg.control_pins.rst_pin >= 0) {
    //     gpio_set_direction(g_cfg.control_pins.rst_pin, GPIO_MODE_OUTPUT);
    //     gpio_set_level(g_cfg.control_pins.rst_pin, 1); /* assume active-low reset */
    // }

    /* create parser task */
    BaseType_t ok = xTaskCreate(s_parser_task_fn, "sim_at_parser", SIM_AT_PARSER_TASK_STACK, NULL, SIM_AT_PARSER_TASK_PRIO, &s_parser_task);
    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create parser task");
        uart_driver_delete(g_cfg.uart_port);
        vSemaphoreDelete(s_sync_sem);
        s_sync_sem = NULL;
        return SIM_AT_ERR_INTERNAL;
    }

    g_inited = true;

    /* disable echo by default to simplify parsing (try but tolerate failure) */
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_cmd_sync("AT\r\n", 2000);
    ignore_sim_response();
    sim_at_cmd_sync("ATE0\r\n", 2000);
    ignore_sim_response();

    ESP_LOGI(TAG, "sim_at initialized");
    return SIM_AT_OK;
}

sim_at_err_t sim_at_deinit(void)
{
    if (!g_inited)
        return SIM_AT_OK;
    /* stop parser task */
    if (s_parser_task)
    {
        vTaskDelete(s_parser_task);
        s_parser_task = NULL;
    }
    /* delete uart driver */
    uart_driver_delete(g_cfg.uart_port);
    /* delete semaphores */
    if (s_sync_sem)
    {
        vSemaphoreDelete(s_sync_sem);
        s_sync_sem = NULL;
    }
    g_inited = false;
    ESP_LOGI(TAG, "sim_at deinitialized");
    return SIM_AT_OK;
}

// sim_at_err_t sim_at_cmd_async(const char *cmd, sim_at_cmd_cb_t cb, void *user_ctx, uint32_t timeout_ms) {
//     if (!g_inited) return SIM_AT_ERR_NOT_INIT;
//     if (!cmd) return SIM_AT_ERR_INVALID_ARG;
//     if (strlen(cmd) >= SIM_AT_MAX_CMD_LEN) return SIM_AT_ERR_INVALID_ARG;

//     xSemaphoreTake(s_sync_sem, portMAX_DELAY);
//     int idx = prv_find_free_slot_idx();
//     if (idx < 0) {
//         xSemaphoreGive(s_sync_sem);
//         return SIM_AT_ERR_BUSY;
//     }

//     sim_at_err_t r = prv_uart_write_cmd(cmd);
//     if (r != SIM_AT_OK) {
//         /* mark done with uart error */
//         xSemaphoreTake(s_sync_sem, portMAX_DELAY);
//         prv_complete_slot(idx, SIM_AT_ERR_UART, -1);
//         xSemaphoreGive(s_sync_sem);
//         return r;
//     }
//     return SIM_AT_OK;
// }

sim_at_err_t sim_at_cmd_sync(const char *cmd, uint32_t timeout_ms)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    if (strlen(cmd) >= SIM_AT_MAX_CMD_LEN)
        return SIM_AT_ERR_INVALID_ARG;

    sim_at_err_t r = prv_uart_write_cmd(cmd);
    if (r != SIM_AT_OK)
    {
        ESP_LOGE(TAG, "Error sending UART data");
        return r;
    }

    /* wait for completion */
    TickType_t wait_ticks = pdMS_TO_TICKS((timeout_ms == 0) ? g_cfg.default_cmd_timeout_ms : timeout_ms);
    if (xSemaphoreTake(s_sync_sem, wait_ticks) == pdFALSE)
    {
        return SIM_AT_ERR_TIMEOUT;
    }

    return SIM_AT_OK;
}

sim_at_err_t sim_at_cmd_sync_ignore_response(const char *cmd, uint32_t timeout_ms, uint8_t num_responses)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    if (strlen(cmd) >= SIM_AT_MAX_CMD_LEN)
        return SIM_AT_ERR_INVALID_ARG;

    sim_at_err_t r = prv_uart_write_cmd(cmd);
    if (r != SIM_AT_OK)
    {
        ESP_LOGE(TAG, "Error sending UART data");
        return r;
    }

    /* wait for completion */
    TickType_t wait_ticks = pdMS_TO_TICKS((timeout_ms == 0) ? g_cfg.default_cmd_timeout_ms : timeout_ms);
    if (xSemaphoreTake(s_sync_sem, wait_ticks) == pdFALSE)
    {
        return SIM_AT_ERR_TIMEOUT;
    }

    for (int i=0; i<num_responses; i++)
        ignore_sim_response();

    return SIM_AT_OK;
}

sim_at_err_t sim_at_uart_flush_rx(void)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    uart_flush_input(g_cfg.uart_port);
    return SIM_AT_OK;
}

sim_at_err_t sim_at_control_dtr(bool state)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    if (g_cfg.control_pins.dtr_pin < 0)
        return SIM_AT_ERR_INVALID_ARG;
    gpio_set_level(g_cfg.control_pins.dtr_pin, state ? 1 : 0);
    return SIM_AT_OK;
}

sim_at_err_t sim_at_control_pwrkey(bool state)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    if (g_cfg.control_pins.pwrkey_pin < 0)
        return SIM_AT_ERR_INVALID_ARG;
    gpio_set_level(g_cfg.control_pins.pwrkey_pin, state ? 1 : 0);
    return SIM_AT_OK;
}

sim_at_err_t sim_at_control_reset(bool state)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    if (g_cfg.control_pins.rst_pin < 0)
        return SIM_AT_ERR_INVALID_ARG;
    gpio_set_level(g_cfg.control_pins.rst_pin, state ? 1 : 0);
    return SIM_AT_OK;
}

sim_at_err_t sim_at_enable_debug(bool en)
{
    g_debug = en;
    ESP_LOGI(TAG, "Debug %s", en ? "enable" : "disable");
    return SIM_AT_OK;
}

/* placeholders remain declared in header for higher-level modules */

/* End of file */
