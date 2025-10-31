#ifndef SIM_STATUS_CONTROL_AT_H
#define SIM_STATUS_CONTROL_AT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sim_at.h"
#include "string.h"
#include "esp_log.h"

/**
 * [--- List of available commands ---]
 * 
 * [x] AT+CFUN      = Set phone functionality
 * [x] AT+CSQ       = Query signal quality
 * [ ] AT+AUTOCSQ   = Set CSQ report
 * [ ] AT+CSQDELTA  = Set RSSI delta change threshold
 * [x] AT+CPOF      = Power down the module
 * [x] AT+CRESET    = Reset the module
 * [ ] AT+CACM      = Accumulated call meter
 * [ ] AT+CAMM      = Accumulated call meter maximum
 * [ ] AT+CPUC      = Price per unit and currency table
 * [x] AT+CCLK      = Real time clock management
 * [ ] AT+CMEE      = Report mobile equipment error
 * [ ] AT+CPAS      = Phone activity status
 * [ ] AT+SIMEI     = Set IMEI for the module
 * 
 */

 /**
 * ------------------------------------
 * ----- [ Status control codes ] -----
 * ------------------------------------ 
 */

 typedef enum {
    FUN_MINIMUN_FUNCTIONALITY = 0,
    FUN_FULL_FUNCTIONALITY = 1,
    FUN_DISABLE_PHONE = 4,
    FUN_FACTORY_TEST_MODE = 5,
    FUN_RESET = 6,
    FUN_OFFLINE_MODE = 7,
    FUN_DISABLE_SIM = 8,
} sim_status_control_fun_t;

 /**
 * ---------------------------------------
 * ----- [ Status control commands ] -----
 * --------------------------------------- 
 */

/**
 * @brief Returns the current phone functionality
 * 
 * @param fun Phone functinality status code:
 *  - MINIMUN_FUNCTIONALITY - minimum functionality
 *  - FULL_FUNCTIONALITY    - full functionality, online mode
 *  - DISABLE_PHONE         - disable phone both transmit and receive RF circuits
 *  - FACTORY_TEST_MODE     - Factory Test Mode
 *  - RESET                 - Reset
 *  - OFFLINE_MODE          - Offline Mode
 *  - DISABLE_SIM           - Disable SIM
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_get_phone_functionality(sim_status_control_fun_t* fun);

/**
 * @brief Sets the current phone functionality
 * 
 * @param fun Phone functinality status code:
 *  - MINIMUN_FUNCTIONALITY - minimum functionality
 *  - FULL_FUNCTIONALITY    - full functionality, online mode
 *  - DISABLE_PHONE         - disable phone both transmit and receive RF circuits
 *  - FACTORY_TEST_MODE     - Factory Test Mode
 *  - RESET                 - Reset
 *  - OFFLINE_MODE          - Offline Mode
 *  - DISABLE_SIM           - Disable SIM
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_set_phone_functionality(sim_status_control_fun_t fun);

/**
 * @brief Gets the signal strength <rssi> and the channel bit error rate <ber> 
 * rssi:
 *  - 0 -113 dBm or less
 *  - 1 -111 dBm
 *  - 2…30 -109… -53 dBm
 *  - 31 -51 dBm or greater
 *  - 99 not known or not detectable
 * 
 * ber:
 *  - 0 <0.01%
 *  - 1 0.01% --- 0.1%
 *  - 2 0.1% --- 0.5%
 *  - 3 0.5% --- 1.0%
 *  - 4 1.0% --- 2.0%
 *  - 5 2.0% --- 4.0%
 *  - 6 4.0% --- 8.0%
 *  - 7 >=8.0%
 *  - 99 not known or not detectable
 * 
 * @param rssi Signal strength
 * @param ber Bit error rate 
 *      
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_query_signal_quality(int* rssi, int* ber);
void parse_rssi(int rssi); // TODO: Completar
void parse_ber(int ber); // TODO: Completar

sim_at_err_t sim_at_power_down_module(void);

/**
 * @brief Resets sim module
 * 
 * @return SIM_AT_OK if succeded
 */
sim_at_err_t sim_at_reset_module(void);

/**
 * @brief Get current RTC time in the "yy/MM/dd,hh:mm:ss±zz" format
 * 
 * @param rtc_time String where the RTC time will be stored
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_get_rtc_time(char* rtc_time);

#ifdef __cplusplus
}
#endif

#endif /* SIM_STATUS_CONTROL_AT_H*/
