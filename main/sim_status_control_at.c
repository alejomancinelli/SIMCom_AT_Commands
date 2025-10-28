#include "sim_status_control_at.h"

// TODO: Tira problemas por repetir el nombre?
static const char *TAG = "status_control_at";

sim_at_err_t get_phone_functionality(sim_status_control_fun_t* fun)
{
    char resp[SIM_AT_MAX_RESP_LEN];
    sim_at_err_t err = sim_at_cmd_sync("AT+CFUN?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CFUN? commands: %s", sim_at_err_to_str(err));
        return err;
    }

    // Reads response
    get_sim_at_response(resp);
    if (strstr(resp, "+CFUN") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    const char *p = strchr(resp, ':');
    if (!p) return -1; // invalid format
    while (*p == ':' || *p == ' ' || *p == '\t')
    p++;
    *fun = atoi(p);
    
    // Reads OK
    get_sim_at_response(resp);
    
    return SIM_AT_OK;
}

sim_at_err_t set_phone_functionality(sim_status_control_fun_t fun)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CFUN=%d,1\r\n", fun);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CFUN=%d,1 commands: %s", fun, sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    return SIM_AT_OK;
}