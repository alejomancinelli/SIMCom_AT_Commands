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
 * [ ] AT+CCLK      = Real time clock management
 * [ ] AT+CMEE      = Report mobile equipment error
 * [ ] AT+CPAS      = Phone activity status
 * [ ] AT+SIMEI     = Set IMEI for the module
 * 
 */

typedef enum {
    MINIMUN_FUNCTIONALITY = 0,
    FULL_FUNCTIONALITY,
    DISABLE_PHONE,
    FACTORY_TEST_MODE,
    RESET,
    OFFLINE_MODE,
    DISABLE_SIM,
} sim_status_control_fun_t;

sim_at_err_t get_phone_functionality(sim_status_control_fun_t* fun);
sim_at_err_t set_phone_functionality(sim_status_control_fun_t fun);

sim_at_err_t query_signal_quality(int* rssi, int* ber);
void parse_rssi(int rssi); // TODO: Completar
void parse_ber(int ber); // TODO: Completar

sim_at_err_t power_down_module(void);
sim_at_err_t reset_module(void);

#ifdef __cplusplus
}
#endif

#endif /* SIM_STATUS_CONTROL_ATAT_*/
