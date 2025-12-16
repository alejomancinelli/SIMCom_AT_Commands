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

/**
 * --------------------------------
 * ----- [ MQTT error codes ] -----
 * -------------------------------- 
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

/**
 * -----------------------------
 * ----- [ MQTT commands ] -----
 * ----------------------------- 
 */

/**
 * @brief Parse the mqtt errors codes to string
 * 
 * @param err mqtt error code
 * 
 * @return A string with the status code description
 */
 const char* sim_mqtt_err_to_string(sim_mqtt_err_codes_t err);

 /**
  * @brief Start MQTT service by activating PDP context. This command must be executed before any other 
  * MQTT related operations.
  * 
  * @returns SIM_AT_OK if succeded, Error Code if failed
  */
 sim_at_err_t sim_at_start_mqtt_service(void);

 /**
  * @brief Stops MQTT service
  * 
  * @returns SIM_AT_OK if succeded, Error Code if failed
  */
sim_at_err_t sim_at_stop_mqtt_service(void);


/**
 * @brief Acquire a MQTT client. It must be called before all commands about MQTT connect and after AT+CMQTTSTART.
 * 
 * @param client_index A numeric parameter that identifies a client. The range of permitted values is 0 to 1.
 * @param client_id It specifies a unique identifier for the client. The string length is from 1 to 128 bytes.
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_acquire_mqtt_client(int client_index, const char* client_id);

/**
 * @brief Release a MQTT client. It must be called after AT+CMQTTDISC and before AT+CMQTTSTOP.
 * 
 * @param client_index A numeric parameter that identifies a client. The range of permitted values is 0 to 1.
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_release_mqtt_client(int client_index);


/**
 * @brief Connect to a MQTT server.
 * 
 * @param client_index A numeric parameter that identifies a client. The range of permitted values is 0 to 1.
 * @param server_addr The string that described the server address and port. The range of the string length 
 * is 9 to 256 bytes. The string should be like this "tcp://116.247.119.165:5141", must begin with "tcp://".
 * @param keepalive_time The time interval between two messages received from a client. The client will send 
 * a keep-alive packet when there is no message sent to server after song long time. The range is from 1s to 64800s.
 * @param clean_session The clean session flag. The value range is from 0 to 1, and default value is 0.
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_connect_mqtt_server(int client_index, const char* server_addr, int keepalive_time, int clean_session);

/**
 * @brief Disconnects from the server.
 * 
 * @param client_index A numeric parameter that identifies a client. The range of permitted values is 0 to 1.
 * @param timeout The timeout value for disconnection. The unit is second. The range is 1s to 180s. The default 
 * value is 0s (not set the timeout value).
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_disconnect_mqtt_server(int client_index, int timeout);


/**
 * @brief Input the topic of a publish message.
 * 
 * @param client_index A numeric parameter that identifies a client. The range of permitted values is 0 to 1.
 * @param topic The length of input topic data. The publish message topic should be UTF-encoded string. 
 * The range is from 1 to 1024 bytes.
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_mqtt_topic(int client_index, const char* topic);

/**
 * @brief Input the message body of a publish message.
 * 
 * @param client_index A numeric parameter that identifies a client. The range of permitted values is 0 to 1.
 * @param payload The length of input message data. The publish message should be UTF-encoded string. 
 * The range is from 1 to 10240 bytes.
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_mqtt_payload(int client_index, const char* payload);

/**
 * @brief Publish a message to MQTT server.
 * 
 * @param client_index A numeric parameter that identifies a client. The range of permitted values is 0 to 1.
 * @param qos The publish message’s qos. The range is from 0 to 2.
 *  - 0 at most once
 *  - 1 at least once
 *  - 2 exactly once 
 * @param pub_timeout The publishing timeout interval value. The range is from 1s to 180s.
 * 
 * @returns SIM_AT_OK if succeded, Error Code if failed
 */
sim_at_err_t sim_at_mqtt_publish(int client_index, int qos, int pub_timeout);

#ifdef __cplusplus
}
#endif

#endif /* SIM_MQTT_AT_H */
