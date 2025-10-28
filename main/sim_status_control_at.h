#ifndef SIM_STATUS_CONTROL_AT_H
#define SIM_STATUS_CONTROL_AT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sim_at.h"
#include "string.h"
#include "esp_log.h"

typedef enum {
    MINIMUN_FUNCTIONALITY = 0,
    FULL_FUNCTIONALITY,
    DISABLE_PHONE,
    FACTORY_TEST_MODE,
    RESET,
    OFFLINE_MODE,
    DISABLE_SIM,
} sim_status_control_fun_t;

sim_at_err_t get_phone_functionality(sim_status_control_fun_t* fun);
sim_at_err_t set_phone_functionality(sim_status_control_fun_t fun);

#ifdef __cplusplus
}
#endif

#endif /* SIM_STATUS_CONTROL_ATAT_*/
