set(srcs
	sim_at.c 
)

idf_component_register(SRCS "sim_simcard_at.c" "sim_network_at.c" "sim_status_control_at.c" "sim_at.c" "test_sim_at.c" ${srcs}
                    INCLUDE_DIRS ".")
