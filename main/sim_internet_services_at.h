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

sim_at_err_t get_ntp_config(void);

sim_at_err_t set_ntp_config(const char* host, int timezone);

sim_at_err_t ntp_update_system_time(void);

#ifdef __cplusplus
}
#endif

#endif /* SIM_INTERNET_SERVICES_H */