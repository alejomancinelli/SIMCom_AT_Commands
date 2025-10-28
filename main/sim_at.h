#ifndef SIM_AT_H
#define SIM_AT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/uart.h"

/**
 * SIM AT core library
 *
 * NOTE: This header provides the core AT engine API + URC registration.
 *       Higher-level helpers (SIM, HTTP, MQTT) are declared as placeholders
 *       and will be implemented later.
 */

/**
 * -----------------------
 * Compile-time tunables
 * -----------------------
 * Adjust as needed before building the library. These control static
 * internal buffer sizes and max concurrent pending commands.
 */
#ifndef SIM_AT_MAX_CMD_LEN
#define SIM_AT_MAX_CMD_LEN        256U    /**< max single AT command length (including CR/LF) */
#endif

#ifndef SIM_AT_MAX_RESP_LEN
#define SIM_AT_MAX_RESP_LEN       1024U   /**< default max response length the library will accept per command */
#endif

#ifndef SIM_AT_MAX_URC_PREFIX_LEN
#define SIM_AT_MAX_URC_PREFIX_LEN 32U
#endif

#ifndef SIM_AT_MAX_PENDING_COMMANDS
#define SIM_AT_MAX_PENDING_COMMANDS 4U    /**< number of in-flight commands supported without dynamic alloc */
#endif

/**
 * -----------------------
 * Error / status codes
 * ----------------------- 
 */
typedef enum {
    SIM_AT_OK = 0,
    SIM_AT_ERR_INVALID_ARG = -1,
    SIM_AT_ERR_NO_MEM = -2,    /* reserved—shouldn't be returned by API since no dynamic alloc */
    SIM_AT_ERR_TIMEOUT = -3,
    SIM_AT_ERR_UART = -4,
    SIM_AT_ERR_BUSY = -5,
    SIM_AT_ERR_INTERNAL = -6,
    SIM_AT_ERR_NOT_INIT = -7,
    SIM_AT_ERR_OVERFLOW = -8,  /* response too long for provided buffer */
    SIM_AT_ERR_ABORTED = -9,
} sim_at_err_t;

/**
 * -----------------------
 * Configuration
 * ----------------------- 
 */

/**
 * Pins struct: use -1 for unused pins.
 */
typedef struct {
    int8_t dtr_pin;     /**< DTR pin number (used to wake/sleep modem), -1 if unused */
    int8_t pwrkey_pin;  /**< PWRKEY pin (power on/off sequence), -1 if unused */
    int8_t rst_pin;     /**< RESET pin, -1 if unused */
} sim_at_pins_t;

/**
 * Top-level config for init.
 *
 * Note: user-provided uart config is an esp-idf uart_config_t; pins are provided too.
 */
typedef struct {
    uart_port_t uart_port;          /**< UART port (e.g., UART_NUM_1) */
    int tx_pin;                     /**< TX gpio */
    int rx_pin;                     /**< RX gpio */
    int rts_pin;                    /**< RTS gpio (-1 if unused) */
    int cts_pin;                    /**< CTS gpio (-1 if unused) */
    uart_config_t uart_conf;        /**< baud_rate, data_bits, parity, stop_bits, flow_ctrl... */
    sim_at_pins_t control_pins;     /**< DTR / PWRKEY / RESET wiring */
    uint32_t default_cmd_timeout_ms;/**< default blocking command timeout */
    bool use_hw_flow_control;       /**< enable hardware flow control (RTS/CTS) */
} sim_at_config_t;

/**
 * -----------------------
 * Response / callback types
 * ----------------------- 
 */

/**
 * Response object returned to user callbacks or filled by sync call.
 * The library will not malloc; the caller must provide buffers to receive
 * response data in sync calls. Async callbacks receive a pointer valid only
 * for the duration of the callback (unless user copies contents).
 */
typedef struct {
    const char *cmd;               /**< pointer to the command string that originated this response (const view) */
    const char *raw;               /**< pointer to raw response buffer (NUL-terminated). Valid only during callback lifetime. */
    size_t raw_len;                /**< length of raw response (bytes, excluding NUL) */
    int final_code;                /**< 0 = OK, non-zero = ERROR or module-specific numeric error if parsed */
    void *user_ctx;                /**< user-provided context pointer */
} sim_at_response_t;

/**
 * Async completion callback signature.
 * Runs in library context (not in caller task). Keep short — do not block.
 */
typedef void (*sim_at_cmd_cb_t)(const sim_at_response_t *resp);

/**
 * URC (unsolicited result code) handler callback.
 *
 * - prefix: the prefix used to register (for filtering), e.g. "+CMTI"
 * - urc: view to the whole URC line (NUL-terminated).
 * - user_ctx: user pointer provided at registration time.
 *
 * The URC string is valid only for the duration of the callback; copy if needed.
 */
typedef void (*sim_at_urc_cb_t)(const char *prefix, const char *urc, void *user_ctx);

/**
 * -----------------------
 * Core API: lifecycle
 * ----------------------- 
 */

/**
 * Initialize the SIM AT core library.
 *
 * - cfg pointer must remain valid during sim_at_init (but need not remain after init).
 * - This function configures UART, creates internal static resources and starts the parser task.
 * - No dynamic allocation is performed for public structures; internal static buffers are allocated inside the implementation.
 *
 * Returns SIM_AT_OK on success.
 */
sim_at_err_t sim_at_init(const sim_at_config_t *cfg);

/**
 * Deinitialize library; stop parser task and release internal resources.
 * Safe to call multiple times. After this, library functions return SIM_AT_ERR_NOT_INIT.
 */
sim_at_err_t sim_at_deinit(void);

/**
 * -----------------------
 * Core API: issuing commands
 * ----------------------- 
 */

/**
 * Send an AT command synchronously (blocking).
 *
 * Parameters:
 * - cmd: NUL-terminated AT command (e.g. "AT+CGSN\r\n"). Must be <= SIM_AT_MAX_CMD_LEN.
 * - resp_buf: buffer provided by caller to receive the response text (NUL-terminated). The library may write up to resp_buf_len bytes (including trailing NUL).
 * - resp_buf_len: length of resp_buf in bytes.
 * - timeout_ms: how long to wait for final response (OK/ERROR). If zero, uses default configured timeout.
 *
 * Returns: SIM_AT_OK on success (final response OK). On success, resp_buf contains the modem's response (excluding echoed command if echo is disabled).
 * Errors: SIM_AT_ERR_TIMEOUT, SIM_AT_ERR_OVERFLOW (if resp_buf too small), SIM_AT_ERR_UART, SIM_AT_ERR_NOT_INIT.
 *
 * Blocking call — do not call from ISR.
 */
sim_at_err_t sim_at_cmd_sync(const char *cmd, uint32_t timeout_ms);

/**
 * Send an AT command asynchronously.
 *
 * Parameters:
 * - cmd: NUL-terminated AT command; the library will copy the command into an internal static slot (bounded by SIM_AT_MAX_CMD_LEN).
 * - cb: optional callback invoked on completion. If NULL, command is fire-and-forget.
 * - user_ctx: user pointer forwarded into response.user_ctx.
 * - timeout_ms: per-command timeout (0 = default).
 *
 * Note: to avoid dynamic allocation, only SIM_AT_MAX_PENDING_COMMANDS may be queued/in-flight.
 * If queue is full, function returns SIM_AT_ERR_BUSY.
 *
 * The callback executes in the library context; keep it short. The sim_at_response_t::raw pointer passed to cb is only valid during the callback.
 */
sim_at_err_t sim_at_cmd_async(const char *cmd, sim_at_cmd_cb_t cb, void *user_ctx, uint32_t timeout_ms);

/**
 * -----------------------
 * Utility helpers
 * ----------------------- 
 */

/**
 * Flush UART RX buffer inside library context.
 */
sim_at_err_t sim_at_uart_flush_rx(void);

/**
 * Lower-level control: toggle DTR/PWRKEY/RST lines if configured.
 * - state: true = asserted (active), false = deasserted.
 * Library will validate configured pins; if pin == -1 returns SIM_AT_ERR_INVALID_ARG.
 * These functions are provided to allow app-managed power sequences.
 */
sim_at_err_t sim_at_control_dtr(bool state);
sim_at_err_t sim_at_control_pwrkey(bool state);
sim_at_err_t sim_at_control_reset(bool state);

// TODO: Todo esto capaz yo lo haría en otros archivos específicos para cada función del modem
/**
 *  -----------------------
 * Placeholders for higher-level API (SIM, PDP, HTTP, MQTT)
 * (signatures shown for planning; will implement later)
 * ----------------------- 
 */

/* Example SIM functions (to be implemented later) */
sim_at_err_t sim_get_iccid(char *buf, size_t buf_len, uint32_t timeout_ms);
sim_at_err_t sim_get_imsi(char *buf, size_t buf_len, uint32_t timeout_ms);

/* Example network / PDP */
sim_at_err_t net_set_apn(const char *apn, const char *user, const char *pass);
sim_at_err_t net_activate_pdp(uint32_t timeout_ms);
sim_at_err_t net_deactivate_pdp(void);
sim_at_err_t net_get_ip(char *ip_buf, size_t ip_buf_len);

/* Example MQTT (placeholders) */
typedef struct {
    const char *host;
    uint16_t port;
    const char *client_id;
    const char *user;
    const char *pass;
    uint32_t keepalive_s;
} sim_mqtt_cfg_t;

sim_at_err_t mqtt_init(const sim_mqtt_cfg_t *cfg);
sim_at_err_t mqtt_connect(uint32_t timeout_ms);
sim_at_err_t mqtt_disconnect(void);
sim_at_err_t mqtt_publish(const char *topic, const uint8_t *payload, size_t payload_len, uint8_t qos, bool retain, uint32_t timeout_ms);
sim_at_err_t mqtt_subscribe(const char *topic, uint8_t qos, void (*msg_cb)(const char *topic, const uint8_t *payload, size_t payload_len, void *ctx), void *ctx);

/**
 * -----------------------
 * Diagnostics / debug
 * ----------------------- 
 */

/**
 * Enable library internal debug logging over ESP_LOG (calls to ESP_LOGI/W).
 * This is a runtime toggle in case you want to reduce logs for low-power testing.
 */
sim_at_err_t sim_at_enable_debug(bool en);

const char* sim_at_err_to_str(sim_at_err_t err);

void get_sim_at_response(char* buf);
void ignore_sim_response(void);

#ifdef __cplusplus
}
#endif

#endif /* SIM_AT_H */
