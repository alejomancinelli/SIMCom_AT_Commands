#ifndef SIM_PACKET_DOMAIN_AT_H
#define SIM_PACKET_DOMAIN_AT_H

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
 * [ ] AT+CGERG         = Network registration status
 * [x] AT+CEREG         = EPS network registration status
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

/**
 * ------------------------------------------
 * ----- [ Packet Domain status codes ] -----
 * ------------------------------------------ 
 */

typedef enum {
    EPS_NOT_REGISTERED = 0,
    EPS_REGISTERED,
    EPS_SEARCHING,
    EPS_REGISTRATION_DENIED,
    EPS_UNKNOWN,
    EPS_ROAMING,
    EPS_SMS_ONLY,
    EPS_SMS_ONLY_ROAMING,
    EPS_EMERGENCY = 11,
} sim_eps_network_registration_stat_t;

typedef enum {
    PDP_IP = 0,
    PDP_IPV6, 
    PDP_IPV4V6,
} sim_pdp_type_t;

/**
 * --------------------------------------
 * ----- [ Packet Domain commands ] -----
 * -------------------------------------- 
 */

/**
 * @brief Returns the status of result code presentation which shows whether the network has currently 
 * indicated the registration of the MT.
 * EPS (Evolved Packet System) is the core network architecture used in 4G LTE. EPS consists of two main parts: 
 * the E-UTRAN (radio access network, managed by eNodeBs) and the Evolved Packet Core (EPC), which handles 
 * user sessions, authentication, mobility, and internet access. 
 * 
 * @param stat Network registration status code
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_eps_network_registration(sim_eps_network_registration_stat_t* stat);

/**
 * @brief Parse the +CGREG status code to string
 * 
 * @param stat Status code
 * 
 * @return A string with the status code description
 */
const char* sim_at_sim_eps_network_status_to_string(sim_eps_network_registration_stat_t stat);

/**
 * @brief Gets the packet domain service state
 * 
 * @param state Indicates the state of Packet Domain attachment
 *  - 0 detached
 *  - 1 attached
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_get_packet_domain_attach(int* state);

/**
 * @brief Sets the packet domain service state
 * 
 * @param state Indicates the state of Packet Domain attachment
 *  - 0 detached
 *  - 1 attached
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_set_packet_domain_attach(int state);

/**
 * @brief Gets the state of a particular PDP context.
 * A PDP (Packet Data Protocol) context is a data structure in mobile networks that defines a user’s session 
 * for packet-switched data services. It contains parameters such as the user’s IP address, QoS (Quality of Service), 
 * and routing information. When a PDP context is activated, it establishes a logical connection between the user’s 
 * device and the network’s gateway, enabling internet or data access.
 * 
 * @param cid A numeric parameter which specifies a particular PDP context definition
 * @param state Indicates the state of PDP context activation
 *  - 0 deactivated
 *  - 1 activated
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_get_pdp_context_activate(int* cid, int* state);

/**
 * @brief Sets the state of a particular PDP context.
 * A PDP (Packet Data Protocol) context is a data structure in mobile networks that defines a user’s session 
 * for packet-switched data services. It contains parameters such as the user’s IP address, QoS (Quality of Service), 
 * and routing information. When a PDP context is activated, it establishes a logical connection between the user’s 
 * device and the network’s gateway, enabling internet or data access.
 * 
 * @param cid A numeric parameter which specifies a particular PDP context definition
 * @param state Indicates the state of PDP context activation
 *  - 0 deactivated
 *  - 1 activated
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_set_pdp_context_activate(int cid, int state);

/**
 * @brief Get the specified PDP context parameter values for a PDP context identified by the (local)context 
 * identification parameter.
 * Just logs the result in the console.
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_get_pdp_context(void);

/**
 * @brief Specifies PDP context parameter values for a PDP context identified by the (local)context 
 * identification parameter.
 * 
 * @param cid A numeric parameter which specifies a particular PDP context definition
 * @param pdp_type A string parameter which specifies the type of packet data protocol
 * @param apn A string parameter which is a logical name that is used to select the GGSN or the external packet data network.
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed 
 */
sim_at_err_t sim_at_set_pdp_context(int cid, sim_pdp_type_t pdp_type, const char* apn);

/**
 * @brief Parse the PDP type to string
 * 
 * @param pdp_type PDP type
 * 
 * @return A string with the PDP type
 */
const char* sim_pdp_type_to_string(sim_pdp_type_t pdp_type);

/**
 * @brief Returns a list of PDP addresses for the specified context identifiers
 * 
 * @param cid A numeric parameter which specifies a particular PDP context definition
 * @param addr A string that identifies the MT in the address space applicable to the PDP
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed 
 */
sim_at_err_t sim_at_show_pdp_address(int* cid, char* addr);

/**
 * @brief Ping destination address.
 * 
 * @param dest_addr The destination is to be pinged; it can be an IP address or a domain name.
 * 
 * @return SIM_AT_OK if succeded, Error Code if failed 
 */
sim_at_err_t sim_at_ping(const char* dest_addr);

#ifdef __cplusplus
}
#endif

#endif /* SIM_PACKET_DOMAIN_AT_H */
