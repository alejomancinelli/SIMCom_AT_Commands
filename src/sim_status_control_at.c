#include "sim_status_control_at.h"

static const char *TAG = "status_control_at";

sim_at_err_t sim_at_get_phone_functionality(sim_status_control_fun_t* fun)
{   
    // Send command
    sim_at_err_t err = sim_at_cmd_sync("AT+CFUN?\r\n", 9000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error sending AT+CFUN? command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    char *data;
    sim_at_responses_err_t resp_err = sim_at_read_response_values(resp, "+CFUN", &data);
    if (resp_err != SIM_AT_RESPONSE_OK)
    {
        ESP_LOGE(TAG, "Error with AT+CFUN? response: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }
    
    // Get FUN status code
    *fun = atoi(data);
    
    // Ignores OK
    resp_err = sim_at_read_ok(resp);
    if (resp_err != SIM_AT_RESPONSE_COMMAND_OK)
    {
        ESP_LOGE(TAG, "Ok response was not received: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }

    return SIM_AT_OK;
}

sim_at_err_t sim_at_set_phone_functionality(sim_status_control_fun_t fun)
{
    // <rst> is always 1:
    // Reset the ME before setting it to <fun> power level. This value only takes effect when <fun> equals 1.
    
    // Command
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CFUN=%d,1\r\n", fun); 
    
    // Send command
    sim_at_err_t err = sim_at_cmd_sync(cmd, 9000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CFUN=%d,1 commands: %s", fun, sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_responses_err_t resp_err = sim_at_read_ok(resp);
    if (resp_err != SIM_AT_RESPONSE_COMMAND_OK)
    {
        ESP_LOGE(TAG, "Ok response was not received: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }
    
    return SIM_AT_OK;
}

sim_at_err_t sim_at_query_signal_quality(int* rssi, int* ber)
{
    // Send command
    sim_at_err_t err = sim_at_cmd_sync("AT+CSQ\r\n", 9000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CSQ commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    char *data;
    sim_at_responses_err_t resp_err = sim_at_read_response_values(resp, "+CSQ", &data);
    if (resp_err != SIM_AT_RESPONSE_OK)
    {
        ESP_LOGE(TAG, "Error with AT+CSQ response: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }
    
    // Parse two integers separated by a comma
    if (sscanf(data, "%d,%d", rssi, ber) != 2)
        return SIM_AT_ERR_RESPONSE;

    // Read OK responss
    resp_err = sim_at_read_ok(resp);
    if (resp_err != SIM_AT_RESPONSE_COMMAND_OK)
    {
        ESP_LOGE(TAG, "Ok response was not received: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }

    return SIM_AT_OK; 
}

int sim_rssi_to_dbm(int rssi)
{
    if (rssi == 99) return -999; // Unknown or not detectable
    if (rssi <= 0)  return -113;
    if (rssi == 1)  return -111;
    if (rssi >= 2 && rssi <= 30)
        return -113 + 2 * rssi;
    if (rssi >= 31)
        return -51;
    return -999; // fallback
}

const char* sim_ber_to_string(int ber)
{
    switch (ber)
    {
        case 0:  return "<0.01%";
        case 1:  return "0.01%–0.1%";
        case 2:  return "0.1%–0.5%";
        case 3:  return "0.5%–1.0%";
        case 4:  return "1.0%–2.0%";
        case 5:  return "2.0%–4.0%";
        case 6:  return "4.0%–8.0%";
        case 7:  return ">=8.0%";
        case 99: return "Not known or not detectable";
        default: return "Invalid value";
    }
}

sim_at_err_t sim_at_power_down_module(void)
{
    // Send command
    sim_at_err_t err = sim_at_cmd_sync("AT+CPOF\r\n", 9000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CPOF command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Read OK responss
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_responses_err_t resp_err = sim_at_read_ok(resp);
    if (resp_err != SIM_AT_RESPONSE_COMMAND_OK)
    {
        ESP_LOGE(TAG, "Ok response was not received: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }

    return SIM_AT_OK; 
}

sim_at_err_t sim_at_reset_module(void)
{
    // Send command
    sim_at_err_t err = sim_at_cmd_sync("AT+CRESET\r\n", 9000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CRESET command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Read OK responss
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_responses_err_t resp_err = sim_at_read_ok(resp);
    if (resp_err != SIM_AT_RESPONSE_COMMAND_OK)
    {
        ESP_LOGE(TAG, "Ok response was not received: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }

    return SIM_AT_OK; 
}

sim_at_err_t sim_at_get_rtc_time(char* rtc_time)
{
    // Send command
    sim_at_err_t err = sim_at_cmd_sync("AT+CCLK?\r\n", 9000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CCLK? command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    char *data;
    sim_at_responses_err_t resp_err = sim_at_read_response_values(resp, "+CCLK", &data);
    if (resp_err != SIM_AT_RESPONSE_OK)
    {
        ESP_LOGE(TAG, "Error with AT+CCLK? response: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }
    
    // Parse response
    if (sscanf(data, "%s", rtc_time) != 1)
        return SIM_AT_ERR_RESPONSE;

    // Read OK response
    resp_err = sim_at_read_ok(resp);
    if (resp_err != SIM_AT_RESPONSE_COMMAND_OK)
    {
        ESP_LOGE(TAG, "Ok response was not received: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }

    return SIM_AT_OK;
}