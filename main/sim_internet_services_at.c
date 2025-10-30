#include "sim_internet_services_at.h"

static const char *TAG = "internet_services_at";

sim_at_err_t get_ntp_config(void)
{
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_err_t err = sim_at_cmd_sync("AT+CNTP?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CNTP? commands: %s", sim_at_err_to_str(err));
        return err;
    }

    // Reads response
    get_sim_at_response(resp);
    if (strstr(resp, "+CNTP") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después

    // Ignores Ok
    ignore_sim_response();  

    return SIM_AT_OK;
}

sim_at_err_t set_ntp_config(const char* host, int timezone)
{
    // Es cada 15 min que considera
    int configTimezone = timezone * 4;

    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CNTP=\"%s\",%d\r\n", host, configTimezone);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CNTP commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads OK
    char resp[SIM_AT_MAX_RESP_LEN];   
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
    return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    return SIM_AT_OK;
}

sim_at_err_t ntp_update_system_time(void)
{
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_err_t err = sim_at_cmd_sync("AT+CNTP\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CNTP commands: %s", sim_at_err_to_str(err));
        return err;
    }

    // Reads OK
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después

    // TODO: Faltaría verificar que no tiró error
    // Ignores response
    ignore_sim_response();  

    return SIM_AT_OK;
}