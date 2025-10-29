#include "sim_packet_domain_at.h"

static const char *TAG = "packet_domain_at";

sim_at_err_t gprs_network_registration(sim_gprs_network_registration_stat_t* stat)
{
    sim_at_err_t err = sim_at_cmd_sync("AT+CGREG?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CGREG command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "+GREG") == NULL)
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

    // Ignores OK
    ignore_sim_response();

    return SIM_AT_OK; 
}

sim_at_err_t get_packet_domain_attach(int* state)
{
    sim_at_err_t err = sim_at_cmd_sync("AT+CGATT?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CGATT command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "+CGATT") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    const char *p = strchr(resp, ':');
    if (!p) return -1; // invalid format
    while (*p == ':' || *p == ' ' || *p == '\t')
        p++;
    // Parse two integers separated by a comma
    if (sscanf(p, "%d", state) != 1)
        return SIM_AT_ERR_INVALID_ARG;
    
    // Ignores OK
    ignore_sim_response();

    return SIM_AT_OK; 
}

// TODO: Completar
sim_at_err_t set_packet_domain_attach(int state)
{
    return SIM_AT_OK; 
}

sim_at_err_t get_pdp_context_activate(int* cid, int* state)
{
    sim_at_err_t err = sim_at_cmd_sync("AT+CGACT?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CGACT command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "+CGACT") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    const char *p = strchr(resp, ':');
    if (!p) return -1; // invalid format
    while (*p == ':' || *p == ' ' || *p == '\t')
        p++;
    // Parse two integers separated by a comma
    if (sscanf(p, "%d,%d", cid, state) != 2)
        return SIM_AT_ERR_INVALID_ARG;
    
    // TODO: En caso que haya muchos contextos de PDP podría haber problemas al leer las respuestas
    // Ver bien como hacer eso

    // Ignores OK
    ignore_sim_response();

    return SIM_AT_OK; 
}

sim_at_err_t set_pdp_context_activate(int cid, int state)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CGACT=%d,%d\r\n", state, cid);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CGACT=%d,%d command: %s", state, cid, sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "OK") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    return SIM_AT_OK; 
}

sim_at_err_t get_pdp_context()
{
    sim_at_err_t err = sim_at_cmd_sync("AT+CGDCONT?\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CGACT command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    // char resp[SIM_AT_MAX_RESP_LEN];
    // get_sim_at_response(resp);
    // if (strstr(resp, "+CGACT") == NULL)
    //     return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    // const char *p = strchr(resp, ':');
    // if (!p) return -1; // invalid format
    // while (*p == ':' || *p == ' ' || *p == '\t')
    //     p++;
    // // Parse two integers separated by a comma
    // if (sscanf(p, "%d,%d", &cid, &state) != 2)
    //     return SIM_AT_ERR_INVALID_ARG;
    
    // TODO: En caso que haya muchos contextos de PDP podría haber problemas al leer las respuestas
    // Ver bien como hacer eso

    // Ignores resonses OK
    ignore_sim_response();
    ignore_sim_response();

    return SIM_AT_OK; 
}

sim_at_err_t set_pdp_context(int cid, sim_pdp_type_t pdp_type, char* apn)
{
    return SIM_AT_OK; 
}

sim_at_err_t show_pdp_address(int* cid, char* addr)
{
    sim_at_err_t err = sim_at_cmd_sync("AT+CGPADDR\r\n", 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CGPADDR command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    get_sim_at_response(resp);
    if (strstr(resp, "+CGPADDR") == NULL)
        return SIM_AT_ERR_INVALID_ARG; // TODO: Poner otro, o analizar el error después
    
    const char *p = strchr(resp, ':');
    if (!p) return -1; // invalid format
    while (*p == ':' || *p == ' ' || *p == '\t')
        p++;
    // Parse two integers separated by a comma
    if (sscanf(p, "%d,%s", cid, addr) != 2)
        return SIM_AT_ERR_INVALID_ARG;
    
    // TODO: En caso que haya muchos contextos de PDP podría haber problemas al leer las respuestas
    // Ver bien como hacer eso

    // Ignores OK
    ignore_sim_response();

    return SIM_AT_OK; 
}

sim_at_err_t ping(char* dest_addr)
{
    char cmd[SIM_AT_MAX_CMD_LEN];
    snprintf(cmd, SIM_AT_MAX_CMD_LEN, "AT+CPING=\"%s\",1\r\n", dest_addr);
    sim_at_err_t err = sim_at_cmd_sync(cmd, 2000);
    if (err != SIM_AT_OK)
    {   
        ESP_LOGE(TAG, "Error with AT+CGPADDR command: %s", sim_at_err_to_str(err));
        return err;
    }
    
    // Ignores OK
    ignore_sim_response(); // TODO: Acá lo mismo, porque serían varias respuestas en realidad

    return SIM_AT_OK; 
}