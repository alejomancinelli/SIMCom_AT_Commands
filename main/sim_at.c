
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
// TODO: Antes de ver las respuesta el s_resp_count se debería reinicializar a 0

static char s_line_buf[SIM_AT_MAX_RESP_LEN];
static int s_line_pos = 0;

/* protects access to s_pending */
static SemaphoreHandle_t s_sync_sem = NULL;

const char *sim_at_err_to_str(sim_at_err_t err)
{
    switch (err)
    {
    case SIM_AT_OK:                 return "SIM_AT_OK";
    case SIM_AT_ERR_INVALID_ARG:    return "SIM_AT_ERR_INVALID_ARG";
    case SIM_AT_ERR_NO_MEM:         return "SIM_AT_ERR_NO_MEM";
    case SIM_AT_ERR_TIMEOUT:        return "SIM_AT_ERR_TIMEOUT";
    case SIM_AT_ERR_UART:           return "SIM_AT_ERR_UART";
    case SIM_AT_ERR_BUSY:           return "SIM_AT_ERR_BUSY";
    case SIM_AT_ERR_INTERNAL:       return "SIM_AT_ERR_INTERNAL";
    case SIM_AT_ERR_NOT_INIT:       return "SIM_AT_ERR_NOT_INIT";
    case SIM_AT_ERR_OVERFLOW:       return "SIM_AT_ERR_OVERFLOW";
    case SIM_AT_ERR_ABORTED:        return "SIM_AT_ERR_ABORTED";
    default:                        return "INVALID ERR";
    }

    return "INVALID ERR";
}

const char* sim_at_response_err_to_str(sim_at_responses_err_t err)
{
    switch (err)
    {
    case SIM_AT_RESPONSE_OK:                    return "SIM_AT_RESPONSE_OK";
    case SIM_AT_RESPONSE_COMMAND_OK:            return "SIM_AT_RESPONSE_COMMAND_OK";
    case SIM_AT_RESPONSE_ERR_INVALID_FORMAT:    return "SIM_AT_RESPONSE_ERR_INVALID_FORMAT";
    case SIM_AT_RESPONSE_ERR_COMMAND_ERROR:     return "SIM_AT_RESPONSE_ERR_COMMAND_ERROR";
    case SIM_AT_RESPONSE_ERR_COMMAND_INVALID:   return "SIM_AT_RESPONSE_ERR_COMMAND_INVALID";
    default:                                    return "INVALID ERR";
    }

    return "INVALID ERR";
}

/**
 * @brief Prints raw bytes in HEX format
 * 
 * @param data String of bytes to print
 * @param len Lenght of the string
 */
static void print_bytes(uint8_t* data, int len)
{
    ESP_LOGI(TAG, "Received %d bytes:", len);
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

/**
 * @brief Print sent command string
 * 
 * @param cmd Command string
 * @param len String lenght
 */
static void print_sent_command(const char* cmd, int len)
{
    char clean_cmd[SIM_AT_MAX_CMD_LEN]; // use a safe upper bound constant
    strncpy(clean_cmd, cmd, sizeof(clean_cmd) - 1);
    clean_cmd[len - 2] = '\0'; // ensure null termination

    ESP_LOGI(TAG, "--> %s", clean_cmd);
}

/**
 * @brief Add UART response to ring buffer
 * 
 * @param data Response string
 */
static void add_response_to_buffer(const char* data)
{
    if (s_resp_head < SIM_AT_MAX_LINES)
    {
        // Write normally
        strncpy(s_responses[s_resp_head], data, SIM_AT_MAX_RESP_LEN - 1);
        s_responses[s_resp_head][SIM_AT_MAX_RESP_LEN - 1] = '\0';
        s_resp_head = (s_resp_head + 1) % SIM_AT_MAX_LINES;
        s_resp_count++;
    }
    else
    {
        // Buffer full: overwrite oldest
        s_resp_head = 0;
        strncpy(s_responses[s_resp_head], data, SIM_AT_MAX_RESP_LEN - 1);
        s_responses[s_resp_head][SIM_AT_MAX_RESP_LEN - 1] = '\0';
    }

    if (g_debug)
        ESP_LOGI(TAG, "<-- %s", data); 
}

/**
 * @brief Resets response line buffer
 */
static void reset_line_buffer(void)
{
    s_line_pos = 0;
    s_line_buf[0] = '\0';
}

/**
 * @brief Write raw command to UART (blocking) 
 * 
 * @param cmd NUL-Terminated AT Command (e.g. "AT+CGSN\r\n"). Must be <= SIM_AT_MAX_CMD_LEN.
 * 
 * @returns
 *  - SIM_AT_OK on success
 *  - SIM_AT_ERR_NOT_INIT is module not initialized
 *  - SIM_AT_ERR_UART if there is a UART error
 *  
 */ 
static sim_at_err_t prv_uart_write_cmd(const char *cmd)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;

    int len = strlen(cmd);

    uart_wait_tx_done(g_cfg.uart_port, pdMS_TO_TICKS(100));
    int written = uart_write_bytes(g_cfg.uart_port, cmd, len);
    
    if (written != len)
        return SIM_AT_ERR_UART;

    if (g_debug) print_sent_command(cmd, len);

    return SIM_AT_OK;
}

/* Parser task: reads bytes from UART, assembles lines, routes them */
static void s_parser_task_fn(void *arg)
{
    const TickType_t rx_wait = pdMS_TO_TICKS(UART_MAX_WAITTIME);
    uint8_t *data = (uint8_t *)malloc(SIM_AT_MAX_RESP_LEN + 1);

    while (1)
    {
        int len = uart_read_bytes(g_cfg.uart_port, data, SIM_AT_MAX_RESP_LEN, rx_wait);
        if (len <= 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // Print received bytes
        if (g_debug) print_bytes(data, len);

        // Form responses
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

                // Check for empty responses
                if (s_line_pos <= 0)
                {
                    reset_line_buffer();
                    continue;
                }

                // Write to circular buffer
                // xSemaphoreTake(s_resp_mutex, portMAX_DELAY); // TODO: Por el momento no pero no es mala

                add_response_to_buffer(s_line_buf);

                // xSemaphoreGive(s_resp_mutex); // TODO: Por el momento no pero no es mala
                xSemaphoreGive(s_sync_sem); // Notify new response available

                reset_line_buffer();
            }
            // Sometimes it responds with '>' to complete with additional data
            if (c == '>')
            {
                add_response_to_buffer(s_line_buf);
                
                // xSemaphoreGive(s_resp_mutex); // TODO: Por el momento no pero no es mala
                xSemaphoreGive(s_sync_sem); // Notify new response available

                reset_line_buffer();
            }
        }
    }
    free(data);
}

/* Public API implementations */

sim_at_err_t sim_at_init(const sim_at_config_t *cfg)
{
    if (!cfg)
        return SIM_AT_ERR_INVALID_ARG;
    if (g_inited)
        return SIM_AT_ERR_ABORTED;

    /* copy config */
    memcpy(&g_cfg, cfg, sizeof(g_cfg));

    /* create locks */
    s_sync_sem = xSemaphoreCreateBinary();
    if (!s_sync_sem)
        return SIM_AT_ERR_NO_MEM;

    /* uart config */
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

    // TODO: Controlar bien esto y hacerlo funcionar
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

    // TODO: Ver de mejorar esto? Capaz tendría que ir aparte
    // Además faltaría checkear que se haya podido sincronizar correctamente
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
        return SIM_AT_ERR_NOT_INIT;
    
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

// TODO: No se si sirve
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

    // Clears previous response count
    // TODO: Ver bien cómo hacer esto
    s_resp_count = 0;
    s_resp_tail = s_resp_head;
    
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

// TODO: No sé si sirve
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

// TODO: No sé si sirve
sim_at_err_t sim_at_uart_flush_rx(void)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    uart_flush_input(g_cfg.uart_port);
    return SIM_AT_OK;
}

// TODO: Falta hacer y probar todas estas
// sim_at_err_t sim_at_control_dtr(bool state)
// {
//     if (!g_inited)
//         return SIM_AT_ERR_NOT_INIT;
//     if (g_cfg.control_pins.dtr_pin < 0)
//         return SIM_AT_ERR_INVALID_ARG;
//     gpio_set_level(g_cfg.control_pins.dtr_pin, state ? 1 : 0);
//     return SIM_AT_OK;
// }

// sim_at_err_t sim_at_control_pwrkey(bool state)
// {
//     if (!g_inited)
//         return SIM_AT_ERR_NOT_INIT;
//     if (g_cfg.control_pins.pwrkey_pin < 0)
//         return SIM_AT_ERR_INVALID_ARG;
//     gpio_set_level(g_cfg.control_pins.pwrkey_pin, state ? 1 : 0);
//     return SIM_AT_OK;
// }

// sim_at_err_t sim_at_control_reset(bool state)
// {
//     if (!g_inited)
//         return SIM_AT_ERR_NOT_INIT;
//     if (g_cfg.control_pins.rst_pin < 0)
//         return SIM_AT_ERR_INVALID_ARG;
//     gpio_set_level(g_cfg.control_pins.rst_pin, state ? 1 : 0);
//     return SIM_AT_OK;
// }

// TODO: Falta analizar los casos en que el módulo envía respuestas a eventos que no fueron requeridos,
// como SMSs o qsy
// I (3278) sim_at: <-- QCRDY
// I (3498) sim_at: <-- +CPIN: NOT INSERTED
bool get_sim_at_response(char *buf)
{
    if (s_resp_count == 0)
        return false; // no new responses

    strncpy(buf, s_responses[s_resp_tail], SIM_AT_MAX_RESP_LEN - 1);
    buf[SIM_AT_MAX_RESP_LEN - 1] = '\0'; // ensure null-terminated

    s_resp_tail = (s_resp_tail + 1) % SIM_AT_MAX_LINES;
    s_resp_count--; // maintain count
    
    return true;
}

void ignore_sim_response(void)
{
    if (s_resp_count == 0)
    return; // nothing to ignore

    s_resp_tail = (s_resp_tail + 1) % SIM_AT_MAX_LINES;
    s_resp_count--;
}

sim_at_err_t sim_at_enable_debug(bool en)
{
    g_debug = en;
    ESP_LOGI(TAG, "Debug %s", en ? "enable" : "disable");
    return SIM_AT_OK;
}

sim_at_responses_err_t sim_at_read_response_values(char* resp, const char* key_word, char** index)
{
    // Get responses
    get_sim_at_response(resp);

    if (strstr(resp, "ERROR") != NULL)
        return SIM_AT_RESPONSE_ERR_COMMAND_ERROR;
    
    if (strstr(resp, "OK") != NULL)
        return SIM_AT_RESPONSE_COMMAND_OK;
    
    if (strstr(resp, key_word) == NULL)
        return SIM_AT_RESPONSE_ERR_COMMAND_INVALID;
    
    char *p = strchr(resp, ':');
    if (!p) return SIM_AT_RESPONSE_ERR_INVALID_FORMAT; // invalid format
    while (*p == ':' || *p == ' ' || *p == '\t')
        p++;
    *index = p;

    return SIM_AT_RESPONSE_OK;
}

sim_at_responses_err_t sim_at_read_ok(char* resp)
{
    // Get responses
    get_sim_at_response(resp);

    if (strstr(resp, "OK") != NULL)
        return SIM_AT_RESPONSE_COMMAND_OK;
    if (strstr(resp, "ERROR") != NULL)
        return SIM_AT_RESPONSE_ERR_COMMAND_ERROR;
    
    return SIM_AT_RESPONSE_ERR_COMMAND_INVALID;
}


/* End of file */
