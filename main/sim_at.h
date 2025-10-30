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
#include "driver/gpio.h"

/**
 * -------------------------------------
 * ----- [ Compile-time tunables ] -----
 * -------------------------------------
 */

// max single AT command length (including CR/LF)
#ifndef SIM_AT_MAX_CMD_LEN
#define SIM_AT_MAX_CMD_LEN        256U 
#endif

// default max response length the library will accept per command
#ifndef SIM_AT_MAX_RESP_LEN
#define SIM_AT_MAX_RESP_LEN       1024U   
#endif

// number of in-flight commands supported without dynamic alloc
#ifndef SIM_AT_MAX_PENDING_COMMANDS
#define SIM_AT_MAX_PENDING_COMMANDS 4U    
#endif

#define UART_MAX_WAITTIME 50 // [ms]

/**
 * ------------------------------------
 * ----- [ Error / status codes ] -----
 * ------------------------------------ 
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
// TODO: Completar con los errores que faltan capaz?

/**
 * -----------------------------
 * ----- [ Configuration ] -----
 * ----------------------------- 
 */

/**
 * Pins struct: use -1 for unused pins.
 */
typedef struct {
    gpio_num_t dtr_pin;     // DTR pin number (used to wake/sleep modem)
    gpio_num_t pwrkey_pin;  // PWRKEY pin (power on/off sequence)
    gpio_num_t rst_pin;     // RESET pin
} sim_at_pins_t;

/**
 * Top-level config for init.
 *
 * Note: user-provided uart config is an esp-idf uart_config_t; pins are provided too.
 */
typedef struct {
    gpio_num_t tx_pin;                  // TX gpio
    gpio_num_t rx_pin;                  // RX gpio
    uart_port_t uart_port;              // UART port (e.g., UART_NUM_1)
    uart_config_t uart_conf;            // baud_rate, data_bits, parity, stop_bits, flow_ctrl...
    
    sim_at_pins_t control_pins;         // DTR / PWRKEY / RESET wiring
    
    uint32_t default_cmd_timeout_ms;    // default blocking command timeout
    
    bool use_hw_flow_control;           // enable hardware flow control (RTS/CTS)
    gpio_num_t rts_pin;                 // RTS gpio (-1 if unused)
    gpio_num_t cts_pin;                 // CTS gpio (-1 if unused)
} sim_at_config_t;

/**
 * -----------------------------------------
 * ----- [ Response / callback types ] -----
 * ----------------------------------------- 
 */

 // TODO: Revisar descripción
 // TODO: Capaz todo esto no sirve porque es para lo de async que medio medio
/**
 * Response object returned to user callbacks or filled by sync call.
 * The library will not malloc; the caller must provide buffers to receive
 * response data in sync calls. Async callbacks receive a pointer valid only
 * for the duration of the callback (unless user copies contents).
 */
// typedef struct {
//     const char *cmd;    // pointer to the command string that originated this response (const view)
//     const char *raw;    // pointer to raw response buffer (NUL-terminated). Valid only during callback lifetime.
//     size_t raw_len;     // length of raw response (bytes, excluding NUL)
//     int final_code;     // 0 = OK, non-zero = ERROR or module-specific numeric error if parsed
//     void *user_ctx;     // user-provided context pointer
// } sim_at_response_t;

// /**
//  * Async completion callback signature.
//  * Runs in library context (not in caller task). Keep short — do not block.
//  */
// typedef void (*sim_at_cmd_cb_t)(const sim_at_response_t *resp);

/**
 * -----------------------------------
 * ----- [ Core API: lifecycle ] -----
 * ----------------------------------- 
 */

/**
 * @brief Initialize the SIM AT core library.
 * - This function configures UART, creates internal static resources and starts the parser task.
 * - No dynamic allocation is performed for public structures; internal static buffers are allocated inside the implementation.
 *
 * @param cfg Configuration struct pointer
 *
 * @return 
 *  - SIM_AT_OK on success.
 *  - SIM_AT_ERR_ABORTED API already initialized
 *  - SIM_AT_ERR_INVALID_ARG on empty config 
 *  - SIM_AT_ERR_NO_MEM no memory available
 *  - SIM_AT_ERR_UART error initializing UART
 *  - SIM_AT_ERR_INTERNAL error creating parser task
 */
sim_at_err_t sim_at_init(const sim_at_config_t *cfg);

/**
 * @brief Deinitialize library. 
 * Stops parser task and release internal resources.
 * Safe to call multiple times. After this, library functions return SIM_AT_ERR_NOT_INIT.
 * 
 * @return 
 *  - SIM_AT_OK on success.
 *  - SIM_AT_ERR_NOT_INIT if the API is not initialized
 */
sim_at_err_t sim_at_deinit(void);

/**
 * ------------------------------------------
 * ----- [ Core API: issuing commands ] -----
 * ------------------------------------------ 
 */

/**
 * @brief Send an AT command synchronously (blocking - do not call from ISR.).
 *
 * @param cmd NUL-terminated AT command (e.g. "AT+CGSN\r\n"). Must be <= SIM_AT_MAX_CMD_LEN.
 * @param timeout_ms how long to wait for final response (OK/ERROR). If zero, uses default configured timeout.
 *
 * @return 
 *  - SIM_AT_OK on success
 *  - SIM_AT_ERR_NOT_INIT.
 *  - SIM_AT_ERR_TIMEOUT
 *  - SIM_AT_ERR_UART
 *
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
// sim_at_err_t sim_at_cmd_async(const char *cmd, sim_at_cmd_cb_t cb, void *user_ctx, uint32_t timeout_ms);

/**
 * ---------------------------------------------
 * ----- [ Core API: requesting commands ] -----
 * --------------------------------------------- 
 */

/**
 * @brief Get next response from ring buffer
 * 
 * @param buf Response buffer
 * 
 * @return False is there is no new responses, True otherwise
 */
bool get_sim_at_response(char* buf);

/**
 * @brief Ignore next response from ring buffer
 */
void ignore_sim_response(void);

/**
 * -----------------------
 * Utility helpers
 * ----------------------- 
 */

/**
 * @brief Flush UART RX buffer inside library context.
 */
sim_at_err_t sim_at_uart_flush_rx(void);

// TODO: Completar
/**
 * Lower-level control: toggle DTR/PWRKEY/RST lines if configured.
 * - state: true = asserted (active), false = deasserted.
 * Library will validate configured pins; if pin == -1 returns SIM_AT_ERR_INVALID_ARG.
 * These functions are provided to allow app-managed power sequences.
 */
// sim_at_err_t sim_at_control_dtr(bool state);
// sim_at_err_t sim_at_control_pwrkey(bool state);
// sim_at_err_t sim_at_control_reset(bool state);

/**
 * -----------------------------------
 * ----- [ Diagnostics / debug ] -----
 * ----------------------------------- 
 */

/**
 * @brief Enable library internal debug logging over ESP_LOG (calls to ESP_LOGI/W).
 * 
 * @param en bool - Enable/Disable sim at debug
 * 
 * @return SIM_AT_OK
 */
sim_at_err_t sim_at_enable_debug(bool en);

/**
 * @brief Returns the corresponding description for a sim error
 * 
 * @param err Error code
 * 
 * @return Error string
 */
const char* sim_at_err_to_str(sim_at_err_t err);

#ifdef __cplusplus
}
#endif

#endif /* SIM_AT_H */
