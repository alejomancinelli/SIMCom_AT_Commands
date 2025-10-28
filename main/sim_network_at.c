#include "sim_network_at.h"

static const char *TAG = "network_at";

sim_at_err_t network_registration(sim_network_registration_stat_t* stat)
{
    sim_at_err_t err = sim_at_cmd_sync("AT+CREG?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CREG commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "+CREG") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    const char *p = strchr(resp, ':');
    if (!p) return -1; // invalid format
    while (*p == ':' || *p == ' ' || *p == '\t')
        p++;
    // Parse two integers separated by a comma
    int pN, pStat; // TODO: Por el momento no se utiliza para nada, pero se guarda por las dudas
    if (sscanf(p, "%d,%d", &pN, &pStat) != 2)
        return SIM_AT_ERR_INVALID_ARG;
    
    *stat = pStat;

    // Ignore OK
    ignore_sim_response();

    return SIM_AT_OK; 
}