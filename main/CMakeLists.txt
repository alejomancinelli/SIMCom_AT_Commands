set(srcs
	sim_at.c
    sim_status_control_at.c
    sim_network_at.c
    sim_simcard_at.c
    sim_packet_domain_at.c
    sim_mqtt_at.c 
)

idf_component_register(SRCS "sim_status_control_at.c" "sim_internet_services_at.c" "sim_at.c" "sim_mqtt_at.c" "test_sim_at.c" ${srcs}
                    INCLUDE_DIRS ".")
