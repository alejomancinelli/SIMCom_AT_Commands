set(srcs
	srcs/sim_at.c
    srcs/sim_status_control_at.c
    srcs/sim_network_at.c
    srcs/sim_simcard_at.c
    srcs/sim_packet_domain_at.c
    srcs/sim_mqtt_at.c 
)

idf_component_register(
    SRCS ${srcs} 
    INCLUDE_DIRS "include"
)
