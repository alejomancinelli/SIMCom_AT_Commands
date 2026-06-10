/**
 * sim_at.c
 * Core AT engine implementation for SIMCom modem (ESP-IDF v5.3)
 *
 * NOTE: This implementation focuses on the core requested features. Higher-level
 * modules (SIM, MQTT, HTTP) are separate files (planned later).
 */

#include "at/sim_at.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "sim_at";

/* Internal configuration copy */
static bool g_debug = false;
static simcom_config_t* g_cfg;
static bool g_inited = false;

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

/* Last sent command — used to detect and discard echoed lines */
static char s_last_cmd[SIM_AT_MAX_CMD_LEN];

/* Modem reset flag — set when *ATREADY: 1 is received */
static volatile bool g_modem_reset = false;

/*
 * s_sync_sem: binary semaphore given by the parser ONLY when a response
 * terminator (OK / ERROR / >) is received. simcom_cmd_sync() blocks on it,
 * so it wakes up only once the full response is in the ring buffer.
 * simcom_wait_resp() also uses it for commands that emit async terminators.
 */
static SemaphoreHandle_t s_sync_sem = NULL;

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

void simcom_set_config(simcom_config_t* config)
{
    g_cfg = config;
}

void simcom_set_init_flag(bool init_f)
{
    g_inited = init_f;
}

simcom_err_t simcom_sem_create(void)
{
    s_sync_sem = xSemaphoreCreateBinary();
    if (!s_sync_sem)
        return SIM_AT_ERR_NO_MEM;
    return SIM_AT_OK;
}

void simcom_sem_delete(void)
{
    if (s_sync_sem)
    {
        vSemaphoreDelete(s_sync_sem);
        s_sync_sem = NULL;
    }
}

const char *simcom_err_to_str(simcom_err_t err)
{
    switch (err)
    {
    case SIM_AT_OK:                 return "SIM_AT_OK";
    case SIM_AT_ERR_INVALID_ARG:    return "SIM_AT_ERR_INVALID_ARG";
    case SIM_AT_ERR_NO_MEM:         return "SIM_AT_ERR_NO_MEM";
    case SIMCOM_ERR_TIMEOUT:        return "SIMCOM_ERR_TIMEOUT";
    case SIM_AT_ERR_UART:           return "SIM_AT_ERR_UART";
    case SIM_AT_ERR_BUSY:           return "SIM_AT_ERR_BUSY";
    case SIM_AT_ERR_INTERNAL:       return "SIM_AT_ERR_INTERNAL";
    case SIM_AT_ERR_NOT_INIT:       return "SIM_AT_ERR_NOT_INIT";
    case SIM_AT_ERR_OVERFLOW:       return "SIM_AT_ERR_OVERFLOW";
    case SIM_AT_ERR_ABORTED:        return "SIM_AT_ERR_ABORTED";
    case SIMCOM_ERR_MODEM_RESET:    return "SIMCOM_ERR_MODEM_RESET";
    default:                        return "INVALID ERR";
    }
    return "INVALID ERR";
}

const char* simcom_resp_err_to_str(simcom_responses_err_t err)
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

/* ------------------------------------------------------------------ */
/* Internal debug helpers (unchanged)                                  */
/* ------------------------------------------------------------------ */

static void _print_bytes(uint8_t* data, int len)
{
    ESP_LOGI(TAG, "Received %d bytes:", len);
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static void _print_sent_command(const char* cmd, int len)
{
    char clean_cmd[SIM_AT_MAX_CMD_LEN];
    strncpy(clean_cmd, cmd, sizeof(clean_cmd) - 1);
    clean_cmd[len - 2] = '\0';
    ESP_LOGI(TAG, "--> %s", clean_cmd);
}

/* ------------------------------------------------------------------ */
/* Ring buffer                                                          */
/* ------------------------------------------------------------------ */

static void _add_resp_to_buff(const char* data)
{
    if (s_resp_count < SIM_AT_MAX_LINES)
    {
        strncpy(s_responses[s_resp_head], data, SIM_AT_MAX_RESP_LEN - 1);
        s_responses[s_resp_head][SIM_AT_MAX_RESP_LEN - 1] = '\0';
        s_resp_head = (s_resp_head + 1) % SIM_AT_MAX_LINES;
        s_resp_count++;
    }
    else
    {
        /* Buffer full: overwrite oldest entry and advance tail to keep
         * head and tail consistent. */
        strncpy(s_responses[s_resp_head], data, SIM_AT_MAX_RESP_LEN - 1);
        s_responses[s_resp_head][SIM_AT_MAX_RESP_LEN - 1] = '\0';
        s_resp_head = (s_resp_head + 1) % SIM_AT_MAX_LINES;
        s_resp_tail = (s_resp_tail + 1) % SIM_AT_MAX_LINES;
        /* s_resp_count stays at SIM_AT_MAX_LINES */
        ESP_LOGW(TAG, "Response ring buffer full — oldest entry overwritten");
    }

    if (g_debug)
        ESP_LOGI(TAG, "<-- %s", data);
}

static void _reset_line_buff(void)
{
    s_line_pos = 0;
    s_line_buf[0] = '\0';
}

/* ------------------------------------------------------------------ */
/* Echo detection (unchanged)                                           */
/* ------------------------------------------------------------------ */

static bool _line_is_echo(const char *line)
{
    if (s_last_cmd[0] == '\0')
        return false;

    char stripped[SIM_AT_MAX_CMD_LEN];
    strncpy(stripped, s_last_cmd, sizeof(stripped) - 1);
    stripped[sizeof(stripped) - 1] = '\0';

    int len = strlen(stripped);
    while (len > 0 && (stripped[len - 1] == '\r' || stripped[len - 1] == '\n'))
        stripped[--len] = '\0';

    return (strcmp(line, stripped) == 0);
}

/* ------------------------------------------------------------------ */
/* UART write (unchanged)                                               */
/* ------------------------------------------------------------------ */

static simcom_err_t _prv_uart_write_cmd(const char *cmd)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;

    int len = strlen(cmd);

    strncpy(s_last_cmd, cmd, SIM_AT_MAX_CMD_LEN - 1);
    s_last_cmd[SIM_AT_MAX_CMD_LEN - 1] = '\0';

    uart_wait_tx_done(g_cfg->uart_port, pdMS_TO_TICKS(100));
    int written = uart_write_bytes(g_cfg->uart_port, cmd, len);

    if (written != len)
        return SIM_AT_ERR_UART;

    if (g_debug) _print_sent_command(cmd, len);

    return SIM_AT_OK;
}

/* ------------------------------------------------------------------ */
/* Line classification                                                  */
/* ------------------------------------------------------------------ */

static bool _response_is_urc(const char *line)
{
    return (
        strstr(line, "+CGEV:")    != NULL ||
        strstr(line, "SMS")       != NULL ||
        strstr(line, "*ISIMAID")  != NULL ||
        strstr(line, "+SIMCARD:") != NULL ||
        strstr(line, "+MSTK:")    != NULL ||
        strstr(line, "PB DONE")   != NULL
    );
}

static bool _response_is_modem_reset(const char *line)
{
    if (strstr(line, "*ATREADY: 1") != NULL)
    {
        g_modem_reset = true;
        ESP_LOGW(TAG, "Modem reset detected (*ATREADY: 1)");
        return true;
    }
    return false;
}

/**
 * @brief Returns true if this line marks the end of an AT response sequence.
 *
 * The SIMCom module always closes a response with one of:
 *   - "OK"        — command succeeded
 *   - "ERROR"     — command failed (plain or CME/CMS)
 *   - ">"         — module is prompting for more input (e.g. AT+CMGS)
 *
 * These are the ONLY lines that should unblock simcom_cmd_sync().
 * Intermediate data lines (+CSQ, +CREG, etc.) are accumulated silently.
 */
static bool _line_is_terminator(const char *line)
{
    /* Exact match for "OK" and "ERROR" to avoid false positives on
     * payload data that might contain those strings (e.g. a URL). */
    if (strcmp(line, "OK") == 0)
        return true;
    if (strcmp(line, "ERROR") == 0)
        return true;
    /* Extended error responses from AT+CMEE */
    if (strncmp(line, "+CME ERROR:", 11) == 0)
        return true;
    if (strncmp(line, "+CMS ERROR:", 11) == 0)
        return true;
    /* Prompt character — stored as a data line AND treated as a terminator */
    if (strcmp(line, ">") == 0)
        return true;

    return false;
}

/**
 * @brief Returns true if this line is a deferred async result that arrives
 * AFTER the initial OK, and needs to unblock a simcom_wait_resp() caller.
 *
 * These commands have a two-phase response:
 *   Phase 1: OK              — cmd_sync returns, caller calls wait_resp()
 *   Phase 2: +CNTP: <n>      — arrives seconds later, unblocks wait_resp()
 *
 * The line is stored in the ring buffer AND signals the semaphore, so the
 * caller can read it with simcom_get_resp() / simcom_read_resp_values().
 *
 * Add any other deferred-result prefixes here as they're discovered.
 */
static bool _line_is_async_terminator(const char *line)
{
    /* NTP sync result: +CNTP: <err_code> */
    if (strncmp(line, "+CNTP:", 6) == 0)
        return true;
    /* MQTT async events that arrive after the initial OK */
    if (strncmp(line, "+CMQTTCONNECT:", 14) == 0)
        return true;
    if (strncmp(line, "+CMQTTDISC:", 11) == 0)
        return true;
    if (strncmp(line, "+CMQTTPUB:", 10) == 0)
        return true;
    if (strncmp(line, "+CMQTTSUB:", 10) == 0)
        return true;

    return false;
}

/* ------------------------------------------------------------------ */
/* Parser task                                                          */
/* ------------------------------------------------------------------ */

static void _s_parser_task_fn(void *arg)
{
    const TickType_t rx_wait = pdMS_TO_TICKS(UART_MAX_WAITTIME);
    uint8_t *data = (uint8_t *)malloc(SIM_AT_MAX_RESP_LEN + 1);

    while (1)
    {
        int len = uart_read_bytes(g_cfg->uart_port, data, SIM_AT_MAX_RESP_LEN, rx_wait);
        if (len <= 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (g_debug) _print_bytes(data, len);

        for (int i = 0; i < len; i++)
        {
            char c = (char)data[i];

            /* ---- Accumulate into line buffer ---- */
            if (s_line_pos < SIM_AT_MAX_RESP_LEN - 1)
            {
                s_line_buf[s_line_pos++] = c;
                s_line_buf[s_line_pos]   = '\0';
            }

            /* ---- '>' prompt: treat immediately, don't wait for '\n' ---- */
            if (c == '>')
            {
                /* Store ">" as a data line so the caller can detect it */
                _add_resp_to_buff(">");
                /* ">" is also a terminator — wake up cmd_sync */
                xSemaphoreGive(s_sync_sem);
                _reset_line_buff();
                continue;
            }

            /* ---- End-of-line ---- */
            if (c == '\n')
            {
                /* Strip trailing CR/LF */
                while (s_line_pos > 0 &&
                       (s_line_buf[s_line_pos - 1] == '\r' ||
                        s_line_buf[s_line_pos - 1] == '\n'))
                {
                    s_line_buf[--s_line_pos] = '\0';
                }

                /* Discard blank lines */
                if (s_line_pos == 0)
                {
                    _reset_line_buff();
                    continue;
                }

                /* Discard echo */
                if (_line_is_echo(s_line_buf))
                {
                    if (g_debug)
                        ESP_LOGW(TAG, "Echo discarded: %s", s_line_buf);
                    _reset_line_buff();
                    continue;
                }

                /* Discard modem-reset URC */
                if (_response_is_modem_reset(s_line_buf))
                {
                    _reset_line_buff();
                    continue;
                }

                /* Discard other URCs */
                if (_response_is_urc(s_line_buf))
                {
                    if (g_debug)
                        ESP_LOGW(TAG, "URC discarded: %s", s_line_buf);
                    _reset_line_buff();
                    continue;
                }

                /*
                 * Store the line unconditionally (data lines AND terminators
                 * both go into the ring buffer so the caller can inspect them).
                 */
                _add_resp_to_buff(s_line_buf);

                /*
                 * Signal the semaphore when:
                 *
                 *  a) A standard terminator arrives (OK / ERROR / CME / CMS).
                 *     This unblocks simcom_cmd_sync() after a synchronous exchange.
                 *
                 *  b) A deferred async result arrives (+CNTP:, +CMQTTCONNECT:, ...).
                 *     These come AFTER the initial OK, seconds later.
                 *     simcom_wait_resp() blocks waiting for exactly this signal.
                 *
                 * Plain data lines (+CSQ:, +CREG:, +CGPADDR:, ...) are accumulated
                 * silently -- the caller reads them after cmd_sync returns.
                 */
                if (_line_is_terminator(s_line_buf) ||
                    _line_is_async_terminator(s_line_buf))
                {
                    xSemaphoreGive(s_sync_sem);
                }

                _reset_line_buff();
            }
        }
    }
    free(data);
}

/* ------------------------------------------------------------------ */
/* Public API — signatures unchanged                                    */
/* ------------------------------------------------------------------ */

simcom_err_t simcom_cmd_sync(const char *cmd, uint32_t timeout_ms)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    if (strlen(cmd) >= SIM_AT_MAX_CMD_LEN)
        return SIM_AT_ERR_INVALID_ARG;

    /* Reset the ring buffer so stale responses from a previous command
     * don't bleed into this one. */
    s_resp_count = 0;
    s_resp_tail  = s_resp_head;

    simcom_err_t r = _prv_uart_write_cmd(cmd);
    if (r != SIM_AT_OK)
    {
        ESP_LOGE(TAG, "Error sending UART data");
        return r;
    }

    /* Drain any stale semaphore gives that arrived before this command */
    while (xSemaphoreTake(s_sync_sem, 0) == pdTRUE);

    /*
     * Block until the parser signals a terminator (OK / ERROR / >).
     * All intermediate data lines have already been buffered by then.
     */
    TickType_t wait_ticks = pdMS_TO_TICKS(
        (timeout_ms == 0) ? g_cfg->default_cmd_timeout_ms : timeout_ms);

    if (xSemaphoreTake(s_sync_sem, wait_ticks) == pdFALSE)
        return SIMCOM_ERR_TIMEOUT;

    return SIM_AT_OK;
}

simcom_err_t simcom_wait_resp(uint32_t timeout_ms)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;

    TickType_t wait_ticks = pdMS_TO_TICKS(
        (timeout_ms == 0) ? g_cfg->default_cmd_timeout_ms : timeout_ms);

    if (xSemaphoreTake(s_sync_sem, wait_ticks) == pdFALSE)
        return SIMCOM_ERR_TIMEOUT;

    return SIM_AT_OK;
}

simcom_err_t simcom_cmd_sync_ignore_resp(const char *cmd, uint32_t timeout_ms, uint8_t num_responses)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    if (strlen(cmd) >= SIM_AT_MAX_CMD_LEN)
        return SIM_AT_ERR_INVALID_ARG;

    simcom_err_t r = _prv_uart_write_cmd(cmd);
    if (r != SIM_AT_OK)
    {
        ESP_LOGE(TAG, "Error sending UART data");
        return r;
    }

    TickType_t wait_ticks = pdMS_TO_TICKS(
        (timeout_ms == 0) ? g_cfg->default_cmd_timeout_ms : timeout_ms);

    if (xSemaphoreTake(s_sync_sem, wait_ticks) == pdFALSE)
        return SIMCOM_ERR_TIMEOUT;

    for (int i = 0; i < num_responses; i++)
        simcom_ignore_resp();

    return SIM_AT_OK;
}

simcom_err_t simcom_uart_flush_rx(void)
{
    if (!g_inited)
        return SIM_AT_ERR_NOT_INIT;
    uart_flush_input(g_cfg->uart_port);
    return SIM_AT_OK;
}

bool simcom_get_resp(char *buf)
{
    if (s_resp_count == 0)
        return false;

    strncpy(buf, s_responses[s_resp_tail], SIM_AT_MAX_RESP_LEN - 1);
    buf[SIM_AT_MAX_RESP_LEN - 1] = '\0';

    s_resp_tail = (s_resp_tail + 1) % SIM_AT_MAX_LINES;
    s_resp_count--;

    return true;
}

void simcom_ignore_resp(void)
{
    if (s_resp_count == 0)
        return;

    s_resp_tail = (s_resp_tail + 1) % SIM_AT_MAX_LINES;
    s_resp_count--;
}

simcom_err_t simcom_enable_debug(bool en)
{
    g_debug = en;
    ESP_LOGI(TAG, "Debug %s", en ? "enable" : "disable");
    return SIM_AT_OK;
}

simcom_responses_err_t simcom_read_resp_values(char* resp, const char* key_word, char** index)
{
    simcom_get_resp(resp);

    if (strstr(resp, "ERROR") != NULL)
        return SIM_AT_RESPONSE_ERR_COMMAND_ERROR;

    if (strstr(resp, "OK") != NULL)
        return SIM_AT_RESPONSE_COMMAND_OK;

    if (strstr(resp, key_word) == NULL)
        return SIM_AT_RESPONSE_ERR_COMMAND_INVALID;

    char *p = strchr(resp, ':');
    if (!p) return SIM_AT_RESPONSE_ERR_INVALID_FORMAT;
    while (*p == ':' || *p == ' ' || *p == '\t')
        p++;
    *index = p;

    return SIM_AT_RESPONSE_OK;
}

simcom_responses_err_t simcom_resp_read_ok(char* resp)
{
    simcom_get_resp(resp);

    if (strstr(resp, "OK") != NULL)
        return SIM_AT_RESPONSE_COMMAND_OK;
    if (strstr(resp, "ERROR") != NULL)
        return SIM_AT_RESPONSE_ERR_COMMAND_ERROR;

    return SIM_AT_RESPONSE_ERR_COMMAND_INVALID;
}

bool simcom_was_reset(void)
{
    return g_modem_reset;
}

void simcom_clear_reset(void)
{
    g_modem_reset = false;
}

BaseType_t simcom_parser_task_create(void)
{
    BaseType_t ret = xTaskCreate(
        _s_parser_task_fn, "sim_at_parser",
        SIM_AT_PARSER_TASK_STACK, NULL,
        SIM_AT_PARSER_TASK_PRIO, &s_parser_task);
    return ret;
}

void simcom_parser_task_delete(void)
{
    if (s_parser_task)
    {
        vTaskDelete(s_parser_task);
        s_parser_task = NULL;
    }
}

/* End of file */