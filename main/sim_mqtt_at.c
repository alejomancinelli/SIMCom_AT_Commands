#include "sim_mqtt_at.h"

static const char *TAG = "mqtt_at";

const char* sim_mqtt_err_to_string(sim_mqtt_err_codes_t err)
{
    switch (err)
    {
        case SIM_MQTT_OK:                                  return "Operation succeeded";
        case SIM_MQTT_ERR_FAIL:                            return "General failure";
        case SIM_MQTT_ERR_BAD_UTF8_STRING:                 return "Bad UTF-8 string";
        case SIM_MQTT_ERR_SOCK_CONNECT_FAIL:               return "Socket connect failed";
        case SIM_MQTT_ERR_SOCK_CREATE_FAIL:                return "Socket creation failed";
        case SIM_MQTT_ERR_SOCK_CLOSE_FAIL:                 return "Socket close failed";
        case SIM_MQTT_ERR_MSG_RECEIVE_FAIL:                return "Message receive failed";
        case SIM_MQTT_ERR_NETWORK_OPEN_FAIL:               return "Network open failed";
        case SIM_MQTT_ERR_NETWORK_CLOSE_FAIL:              return "Network close failed";
        case SIM_MQTT_ERR_NETWORK_NOT_OPENED:              return "Network not opened";
        case SIM_MQTT_ERR_CLIENT_INDEX:                    return "Invalid client index";
        case SIM_MQTT_ERR_NO_CONNECTION:                   return "No active connection";
        case SIM_MQTT_ERR_INVALID_PARAMETER:               return "Invalid parameter";
        case SIM_MQTT_ERR_NOT_SUPPORTED:                   return "Operation not supported";
        case SIM_MQTT_ERR_CLIENT_BUSY:                     return "Client is busy";
        case SIM_MQTT_ERR_REQUIRE_CONN_FAIL:               return "Require connection failed";
        case SIM_MQTT_ERR_SOCK_SEND_FAIL:                  return "Socket sending failed";
        case SIM_MQTT_ERR_TIMEOUT:                         return "Timeout occurred";
        case SIM_MQTT_ERR_TOPIC_EMPTY:                     return "Topic is empty";
        case SIM_MQTT_ERR_CLIENT_IN_USE:                   return "Client is already used";
        case SIM_MQTT_ERR_CLIENT_NOT_ACQUIRED:             return "Client not acquired";
        case SIM_MQTT_ERR_CLIENT_NOT_RELEASED:             return "Client not released";
        case SIM_MQTT_ERR_LEN_OUT_OF_RANGE:                return "Length out of range";
        case SIM_MQTT_ERR_NETWORK_OPENED:                  return "Network already opened";
        case SIM_MQTT_ERR_PACKET_FAIL:                     return "Packet failure";
        case SIM_MQTT_ERR_DNS:                             return "DNS error";
        case SIM_MQTT_ERR_SOCK_CLOSED_BY_SERVER:           return "Socket closed by server";
        case SIM_MQTT_ERR_CONN_REFUSED_BAD_PROTOCOL:       return "Connection refused: unaccepted protocol version";
        case SIM_MQTT_ERR_CONN_REFUSED_ID_REJECTED:        return "Connection refused: identifier rejected";
        case SIM_MQTT_ERR_CONN_REFUSED_SERVER_UNAVAILABLE: return "Connection refused: server unavailable";
        case SIM_MQTT_ERR_CONN_REFUSED_BAD_CREDENTIALS:    return "Connection refused: bad username or password";
        case SIM_MQTT_ERR_CONN_REFUSED_NOT_AUTHORIZED:     return "Connection refused: not authorized";
        case SIM_MQTT_ERR_HANDSHAKE_FAIL:                  return "Handshake failed";
        case SIM_MQTT_ERR_CERT_NOT_SET:                    return "Certificate not set";
        case SIM_MQTT_ERR_OPEN_SESSION_FAIL:               return "Open session failed";
        case SIM_MQTT_ERR_DISCONNECT_FAIL:                 return "Disconnect from server failed";
        default:                                           return "Unknown MQTT error";
    }
}

sim_at_err_t start_mqtt_service(void)
{
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_err_t err = sim_at_cmd_sync("AT+CMQTTSTART\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTSTART commands: %s", sim_at_err_to_str(err));
        return err;
    }

    // Reads OK
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    // Ignores response
    ignore_sim_response();    
    
    // TODO: Faltaría ver los códigos de error y eso

    return SIM_AT_OK;
}

sim_at_err_t stop_mqtt_service(void)
{
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_err_t err = sim_at_cmd_sync("AT+CMQTTSTOP\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTSTOP commands: %s", sim_at_err_to_str(err));
        return err;
    }

    // Reads OK
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    // Ignores response
    ignore_sim_response();    
    
    // TODO: Faltaría ver los códigos de error y eso

    return SIM_AT_OK;
}

sim_at_err_t acquire_mqtt_client(int client_index, const char* client_id)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CMQTTACCQ=%d,\"%s\"\r\n", client_index, client_id);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTACCQ commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads OK
    char resp[SIM_AT_MAX_RESP_LEN];    
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    // TODO: Faltaría ver los códigos de error y eso

    return SIM_AT_OK;
}

sim_at_err_t release_mqtt_client(int client_index)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CMQTTREL=%d\r\n", client_index);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTREL commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads OK
    char resp[SIM_AT_MAX_RESP_LEN];    
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    // TODO: Faltaría ver los códigos de error y eso

    return SIM_AT_OK;
}

sim_at_err_t connect_mqtt_server(int client_index, char* server_addr, int keepalive_time, int clean_session)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CMQTTCONNECT=%d,\"%s\",%d,%d\r\n", client_index, server_addr, keepalive_time, clean_session);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTCONNECT commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads OK
    char resp[SIM_AT_MAX_RESP_LEN];    
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después

    // Ignore rest of the response
    ignore_sim_response();

    // TODO: Faltaría analizar bien la respuesta
    
    return SIM_AT_OK;
}

sim_at_err_t disconnect_mqtt_server(int client_id, int timeout)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CMQTTDISC=%d,%d\r\n", client_id, timeout);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTDISC commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads OK
    char resp[SIM_AT_MAX_RESP_LEN];    
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después

    // Ignore rest of the response
    ignore_sim_response();

    // TODO: Faltaría analizar bien la respuesta
    
    return SIM_AT_OK;
}

sim_at_err_t mqtt_topic(int client_index, const char* topic)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CMQTTTOPIC=%d,%d\r\n", client_index, (int)strlen(topic));
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTTOPIC commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    char resp[SIM_AT_MAX_RESP_LEN];    
    get_sim_at_response(resp);
    if (strstr(resp, ">") == NULL)
       return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    // Send topic
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "%s\r\n", topic);
    err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error sending topic: %s", sim_at_err_to_str(err));
        return err;
    }
    
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    return SIM_AT_OK;
}

sim_at_err_t mqtt_payload(int client_index, const char* payload)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CMQTTPAYLOAD=%d,%d\r\n", client_index, (int)strlen(payload));
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTPAYLOAD commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    char resp[SIM_AT_MAX_RESP_LEN];    
    get_sim_at_response(resp);
    if (strstr(resp, ">") == NULL)
       return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    // Send payload
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "%s\r\n", payload);
    err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error sending payload: %s", sim_at_err_to_str(err));
        return err;
    }
    
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    return SIM_AT_OK;  
}

sim_at_err_t mqtt_publish(int client_index, int qos, int pub_timeout)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CMQTTPUB=%d,%d,%d\r\n", client_index, qos, pub_timeout);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CMQTTPUB commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    char resp[SIM_AT_MAX_RESP_LEN];  
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después

    // Ignore rest of the response
    ignore_sim_response();

    return SIM_AT_OK;  
}