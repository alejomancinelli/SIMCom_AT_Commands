#ifndef SIM_MQTT_AT_H
#define SIM_MQTT_AT_H

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
 * [x] AT+CMQTTSTART         = Start MQTT service
 * [x] AT+CMQTTSTOP          = Stop MQTT service
 * [x] AT+CMQTTACCQ          = Acquire a client
 * [x] AT+CMQTTREL           = Release a client
 * [ ] AT+CMQTTSSLCFG        = Set the SSL context (only for SSL/TLS MQTT)
 * [ ] AT+CMQTTWILLTOPIC     = Input the topic of will message
 * [ ] AT+CMQTTWILLMSG       = Input the will message
 * [x] AT+CMQTTCONNECT       = Connect to MQTT server
 * [x] AT+CMQTTDISC          = Disconnect from server
 * [x] AT+CMQTTTOPIC         = Input the topic of publish message
 * [x] AT+CMQTTPAYLOAD       = Input the publish message
 * [x] AT+CMQTTPUB           = Publish a message to server
 * [ ] AT+CMQTTSUBTOPIC      = Input the topic of subscribe message
 * [ ] AT+CMQTTSUB           = Subscribe a message to server
 * [ ] AT+CMQTTUNSUBTOPIC    = Input the topic of unsubscribe message
 * [ ] AT+CMQTTUNSUB         = Unsubscribe a message to server
 * [ ] AT+CMQTTCFG           = Configure the MQTT Context 
 * 
 */

typedef enum {
    SIM_MQTT_OK                                 = 0,  // Operation succeeded
    SIM_MQTT_ERR_FAIL                           = 1,  // General failure
    SIM_MQTT_ERR_BAD_UTF8_STRING                = 2,  // Bad UTF-8 string
    SIM_MQTT_ERR_SOCK_CONNECT_FAIL              = 3,  // Socket connect failed
    SIM_MQTT_ERR_SOCK_CREATE_FAIL               = 4,  // Socket creation failed
    SIM_MQTT_ERR_SOCK_CLOSE_FAIL                = 5,  // Socket close failed
    SIM_MQTT_ERR_MSG_RECEIVE_FAIL               = 6,  // Message receive failed
    SIM_MQTT_ERR_NETWORK_OPEN_FAIL              = 7,  // Network open failed
    SIM_MQTT_ERR_NETWORK_CLOSE_FAIL             = 8,  // Network close failed
    SIM_MQTT_ERR_NETWORK_NOT_OPENED             = 9,  // Network not opened
    SIM_MQTT_ERR_CLIENT_INDEX                   = 10, // Invalid client index
    SIM_MQTT_ERR_NO_CONNECTION                  = 11, // No active connection
    SIM_MQTT_ERR_INVALID_PARAMETER              = 12, // Invalid parameter
    SIM_MQTT_ERR_NOT_SUPPORTED                  = 13, // Operation not supported
    SIM_MQTT_ERR_CLIENT_BUSY                    = 14, // Client is busy
    SIM_MQTT_ERR_REQUIRE_CONN_FAIL              = 15, // Require connection failed
    SIM_MQTT_ERR_SOCK_SEND_FAIL                 = 16, // Socket sending failed
    SIM_MQTT_ERR_TIMEOUT                        = 17, // Timeout occurred
    SIM_MQTT_ERR_TOPIC_EMPTY                    = 18, // Topic is empty
    SIM_MQTT_ERR_CLIENT_IN_USE                  = 19, // Client is already used
    SIM_MQTT_ERR_CLIENT_NOT_ACQUIRED            = 20, // Client not acquired
    SIM_MQTT_ERR_CLIENT_NOT_RELEASED            = 21, // Client not released
    SIM_MQTT_ERR_LEN_OUT_OF_RANGE               = 22, // Length out of range
    SIM_MQTT_ERR_NETWORK_OPENED                 = 23, // Network already opened
    SIM_MQTT_ERR_PACKET_FAIL                    = 24, // Packet failure
    SIM_MQTT_ERR_DNS                            = 25, // DNS error
    SIM_MQTT_ERR_SOCK_CLOSED_BY_SERVER          = 26, // Socket closed by server
    SIM_MQTT_ERR_CONN_REFUSED_BAD_PROTOCOL      = 27, // Connection refused: unaccepted protocol version
    SIM_MQTT_ERR_CONN_REFUSED_ID_REJECTED       = 28, // Connection refused: identifier rejected
    SIM_MQTT_ERR_CONN_REFUSED_SERVER_UNAVAILABLE= 29, // Connection refused: server unavailable
    SIM_MQTT_ERR_CONN_REFUSED_BAD_CREDENTIALS   = 30, // Connection refused: bad user name or password
    SIM_MQTT_ERR_CONN_REFUSED_NOT_AUTHORIZED    = 31, // Connection refused: not authorized
    SIM_MQTT_ERR_HANDSHAKE_FAIL                 = 32, // Handshake failed
    SIM_MQTT_ERR_CERT_NOT_SET                   = 33, // Certificate not set
    SIM_MQTT_ERR_OPEN_SESSION_FAIL              = 34, // Open session failed
    SIM_MQTT_ERR_DISCONNECT_FAIL                = 35  // Disconnect from server failed
} sim_mqtt_err_codes_t;

char* sim_mqtt_err_to_string(sim_mqtt_err_codes_t err);

sim_at_err_t start_mqtt_service(void);
sim_at_err_t stop_mqtt_service(void);

sim_at_err_t acquire_mqtt_client(int client_index, const char* client_id);
sim_at_err_t release_mqtt_client(int client_index);

sim_at_err_t connect_mqtt_server(int client_index, char* server_addr, int keepalive_time, int clean_session);
sim_at_err_t disconnect_mqtt_server(int client_id, int timeout);

sim_at_err_t mqtt_topic(int client_index, const char* topic);
sim_at_err_t mqtt_payload(int client_index, const char* payload);
sim_at_err_t mqtt_publish(int client_index, int qos, int pub_timeout);

#ifdef __cplusplus
}
#endif

#endif /* SIM_MQTT_AT_H */
