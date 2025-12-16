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

/**
 * ----------------------------------
 * ----- [ SIM Card Pin codes ] -----
 * ---------------------------------- 
 */

typedef enum {
    SIMCARD_READY = 0,
    SIMCARD_SIM_PIN,
    SIMCARD_SIM_PUK,
    SIMCARD_PH_SIM_PIN,
    SIMCARD_SIM_PIN_2,
    SIMCARD_SIM_PUK_2,
    SIMCARD_PH_NET_PIN,
} sim_simcard_pin_code_t;

/**
 * --------------------------------------
 * ----- [ SIM Card commands ] -----
 * -------------------------------------- 
 */

 /**
  * @brief Get if the SIM Card needs a PIN to be initialized
  * 
  * @param code SIM Card Pin code:
  * - SIMCARD_READY: ME is not pending for any password
  * - SIMCARD_SIM_PIN: ME is waiting SIM PIN to be given
  * - SIMCARD_SIM_PUK: ME is waiting SIM PUK to be given
  * - SIMCARD_PH_SIM_PIN: ME is waiting phone-to-SIM card password to be given
  * - SIMCARD_SIM_PIN_2: ME is waiting SIM PIN2 to be given
  * - SIMCARD_SIM_PUK_2: ME is waiting SIM PUK2 to be given
  * - SIMCARD_PH_NET_PIN: ME is waiting network personalization password to be given
  * 
  * @return SIM_AT_OK if succeded, Error Code if failed
  */
sim_at_err_t sim_at_get_simcard_pin_info(sim_simcard_pin_code_t* code);

#ifdef __cplusplus
}
#endif

#endif /* SIM_SIMCARD_AT_H*/
