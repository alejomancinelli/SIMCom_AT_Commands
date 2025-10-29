#ifndef SIM_PATCKET_DOMAIN_AT_H
#define SIM_PATCKET_DOMAIN_AT_H

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
 * [x] AT+CGERG         = Network registration status
 * [ ] AT+CEREG         = EPS network registration status
 * [x] AT+CGATT         = Packet domain attach or detach
 * [x] AT+CGACT         = PDP context activate or deactivate
 * [x] AT+CGDCONT       = Define PDP context
 * [ ] AT+CGDSCONT      = Define Secondary PDP Context
 * [ ] AT+CGTFT         = Traffic Flow Template
 * [ ] AT+CGQREQ        = Quality of service profile (requested)
 * [ ] AT+CGEQREQ       = 3G quality of service profile (requested)
 * [ ] AT+CGQMIN        = Quality of service profile (minimum acceptable)
 * [ ] AT+CGEQMIN       = 3G quality of service profile (minimum acceptable)
 * [ ] AT+CGDATA        = Enter data state
 * [x] AT+CGPADDR       = Show PDP address
 * [ ] AT+CGCLASS       = GPRS mobile station class
 * [ ] AT+CGEREP        = GPRS event reporting
 * [ ] AT+CGAUTH        = Set type of authentication for PDP-IP connections of GPRS
 * [x] AT+CPING         = Ping destination address
 * 
 */

typedef enum {
    GPRS_NOT_REGISTERED = 0,
    GPRS_REGISTERED,
    GPRS_SEARCHING,
    GPRS_REGISTRATION_DENIED,
    GPRS_UNKNOWN,
    GPRS_ROAMING,
    GPRS_SMS_ONLY,
    GPRS_SMS_ONLY_ROAMING,
    GPRS_EMERGENCY = 11,
} sim_gprs_network_registration_stat_t;

typedef enum {
    PDP_IP = 0,
    PDP_IPV6, 
    PDP_IPV4V6,
} sim_pdp_type_t;

sim_at_err_t gprs_network_registration(sim_gprs_network_registration_stat_t* stat);

sim_at_err_t get_packet_domain_attach(int* state);
sim_at_err_t set_packet_domain_attach(int state);

sim_at_err_t get_pdp_context_activate(int* cid, int* state);
sim_at_err_t set_pdp_context_activate(int cid, int state);

sim_at_err_t get_pdp_context();
sim_at_err_t set_pdp_context(int cid, sim_pdp_type_t pdp_type, char* apn);

sim_at_err_t show_pdp_address(int* cid, char* addr);

sim_at_err_t ping(char* dest_addr);

#ifdef __cplusplus
}
#endif

#endif /* SIM_PATCKET_DOMAIN_AT_H */
