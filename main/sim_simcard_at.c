#include "sim_simcard_at.h"

static const char *TAG = "simcard_at";

sim_at_err_t get_simcard_pin_info(sim_simcard_pin_code_t* code)
{
    sim_at_err_t err = sim_at_cmd_sync("AT+CPIN?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CPIN? commands: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "+CPIN") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    const char *p = strchr(resp, ':');
    if (!p) return -1; // invalid format
    while (*p == ':' || *p == ' ' || *p == '\t')
        p++;
    // Parse two integers separated by a comma
    char code_str[16];
    if (sscanf(p, "%s", code_str) != 1)
        return SIM_AT_ERR_INVALID_ARG;
    
    if (strcmp(code_str, "READY") == 0)
        *code = READY;
    else if (strcmp(code_str, "SIM PIN") == 0)
        *code = SIM_PIN;
    else if (strcmp(code_str, "SIM PUK") == 0)
        *code = SIM_PUK;
    else if (strcmp(code_str, "PH-SIM PIN") == 0)
        *code = PH_SIM_PIN;
    else if (strcmp(code_str, "SIM PIN2") == 0)
        *code = SIM_PIN_2;
    else if (strcmp(code_str, "SIM PUK2") == 0)
        *code = SIM_PUK_2;
    else if (strcmp(code_str, "PH-NET PIN") == 0)
        *code = PH_NET_PIN;
    else
        ESP_LOGE(TAG, "The SIM Card code was not recognized: %s", code_str);

    // Ignore OK
    ignore_sim_response();

    return SIM_AT_OK; 
}