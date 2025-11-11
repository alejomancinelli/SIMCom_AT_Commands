#ifndef SIM_INTERNET_SERVICES_H
#define SIM_INTERNET_SERVICES_H

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
 * [ ] AT+CHTPSERV          = Set HTP server information
 * [ ] AT+CHTPUPDATE        = Updating date time using HTP protocol
 * [x] AT+CNTP              = Update system time
 * 
 */

/**
 * -------------------------------
 * ----- [ NTP error codes ] -----
 * ------------------------------- 
 */

typedef enum {
    NTP_OPERATION_SUCCEEDED = 0,
    NTP_UNKOWNW_ERROR = 1,
    NTP_WRONG_PARAMETER = 2,
    NTP_WRONG_DATE_AND_TIME_CALCULATED = 3,
    NTP_NETWORK_ERROR = 4,
    NTP_TIMEZONE_ERROR = 5,
    NTP_TIMEOUT_ERROR = 6,
} sim_at_ntp_err_code_t;

/**
 * -------------------------------------------
 * ----- [ Internet Servicies commands ] -----
 * ------------------------------------------- 
 */

/**
 * @brief Logs the current NTP config
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_get_ntp_config(void);

/**
 * @brief Configure the NTP config
 *  
 * @param host NTP server address
 * @param timezone GTM local time zone (-3 for Argentina)
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_set_ntp_config(const char* host, int timezone);

/**
 * @brief Updates the local system time with the configured NTP server configuration
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_ntp_update_system_time(sim_at_ntp_err_code_t* ntp_err);

#ifdef __cplusplus
}
#endif

#endif /* SIM_INTERNET_SERVICES_H */