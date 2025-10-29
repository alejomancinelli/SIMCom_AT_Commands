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

sim_at_err_t start_mqtt_service(void);
sim_at_err_t stop_mqtt_service(void);

sim_at_err_t acquire_mqtt_client(int client_index, int client_id);
sim_at_err_t release_mqtt_client(int client_index);

sim_at_err_t connect_mqtt_server(int client_index, char* server_addr, int keepalive_time, int clean_session);
sim_at_err_t disconnect_mqtt_server(int clinet_id, int timeout);

sim_at_err_t mqtt_topic(int client_index, const char* topic);
sim_at_err_t mqtt_payload(int client_index, const char* payload);
sim_at_err_t mqtt_publish(int client_index, int qos, int pub_timeout);

#ifdef __cplusplus
}
#endif

#endif /* SIM_MQTT_AT_H */
