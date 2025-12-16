#include "sim_network_at.h"

static const char *TAG = "network_at";

sim_at_err_t sim_at_network_registration(sim_network_registration_stat_t *stat)
{
    // Send command
    sim_at_err_t err = sim_at_cmd_sync("AT+CREG?\r\n", 9000);
    if (err != SIM_AT_OK)
    {
        ESP_LOGE(TAG, "Error with AT+CREG? commands: %s", sim_at_err_to_str(err));
        return err;
    }

    // Reads response
    char resp[SIM_AT_MAX_RESP_LEN];
    char *data;
    sim_at_responses_err_t resp_err = sim_at_read_response_values(resp, "+CREG", &data);
    if (resp_err != SIM_AT_RESPONSE_OK)
    {
        ESP_LOGE(TAG, "Error with AT+CREG? response: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }

    // Get network status
    // TODO: N no se utiliza
    int pN, pStat;
    if (sscanf(data, "%d,%d", &pN, &pStat) != 2)
        return SIM_AT_ERR_RESPONSE;

    *stat = pStat;

    // Read OK responss
    resp_err = sim_at_read_ok(resp);
    if (resp_err != SIM_AT_RESPONSE_COMMAND_OK)
    {
        ESP_LOGE(TAG, "Ok response was not received: %s", sim_at_response_err_to_str(resp_err));
        return SIM_AT_ERR_RESPONSE;
    }

    return SIM_AT_OK;
}

const char *sim_network_status_to_string(sim_network_registration_stat_t stat)
{
    switch (stat)
    {
    case NOT_REGISTERED: return "not registered, ME is not currently searching a new operator to register to";
    case REGISTERED: return "registered, home network";
    case SEARCHING: return "not registered, but ME is currently searching a new operator to register to";
    case REGISTRATION_DENIED: return "registration denied";
    case UNKNOWN: return "unknown";
    case ROAMING: return "registered, roaming";
    case SMS_ONLY: return "registered for \"SMS only\", home network";
    case SMS_ONLY_ROAMING: return "registered for \"SMS only\", roaming";
    case EMERGENCY: return "attached for emergency bearer services only";
    default: return "unknown";
    }
}