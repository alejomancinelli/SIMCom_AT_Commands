#ifndef SIM_NETWORK_AT_H
#define SIM_NETWORK_AT_H

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
 * [x] AT+CREG          = Network registration
 * [x] AT+COPS          = Operator selection
 * [ ] AT+CUSD          = Unstructured supplementary service data
 * [ ] AT+CSSN          = Supplementary service notifications
 * [ ] AT+CPOL          = Preferred operator list
 * [ ] AT+COPN          = Read operator names
 * [ ] AT+CNMP          = Preferred mode selection
 * [ ] AT+CNBP          = Preferred band selection
 * [ ] AT+CPSI          = Inquiring UE system information
 * [ ] AT+CNSMOD        = Show network system mode
 * [ ] AT+CTZU          = Automatic time and time zone update
 * [ ] AT+CTZR          = Time and time zone reporting
 * 
 */

typedef enum {
    NOT_REGISTERED = 0,
    REGISTERED,
    SEARCHING,
    REGISTRATION_DENIED,
    UNKNOWN,
    ROAMING,
    SMS_ONLY,
    SMS_ONLY_ROAMING,
    EMERGENCY = 11,
} sim_network_registration_stat_t;

sim_at_err_t network_registration(sim_network_registration_stat_t* stat); // TODO: Se ignora n

// TODO: COPS por el momento no se utiliza, pero podría ser necesaria


#ifdef __cplusplus
}
#endif

#endif /* SIM_NETWORK_AT_*/
