#ifndef SIM_SIMCARD_AT_H
#define SIM_SIMCARD_AT_H

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
 * [ ] AT+CICCID                = Read ICCID from SIM card
 * [x] AT+CPIN                  = Enter PIN
 * [ ] AT+CLCK                  = Facility lock
 * [ ] AT+CPWD                  = Change password
 * [ ] AT+CIMI                  = Request international mobile subscriber identity
 * [ ] AT+CSIM                  = Generic SIM access
 * [ ] AT+CRSM                  = Restricted SIM access
 * [ ] AT+SPIC                  = Times remain to input SIM PIN/PUK
 * [ ] AT+CSPN                  = Get service provider name from SIM
 * [ ] AT+UIMHOTSWAPON          = Set UIM hotswap function on
 * [ ] AT+UIMHOTSWAPLEVEL       = Set UIM card detection level
 * [ ] AT+SWITCHSIM             = Switch master SIM
 * [ ] AT+DUALSIM               = Set dual-sim mode
 * [ ] AT+BINDSIM               = Bind ATP to SIM1 or SIM2
 * [ ] AT+DUALSIMURC            = Dual card reporting control
 * 
 */

typedef enum {
    READY = 0,
    SIM_PIN,
    SIM_PUK,
    PH_SIM_PIN,
    SIM_PIN_2,
    SIM_PUK_2,
    PH_NET_PIN,
} sim_simcard_pin_code_t;

sim_at_err_t get_simcard_pin_info(sim_simcard_pin_code_t* code);

#ifdef __cplusplus
}
#endif

#endif /* SIM_SIMCARD_AT_H*/
