set(srcs
	sim_at.c 
)

idf_component_register(SRCS "test_sim_at.c" ${srcs}
                    INCLUDE_DIRS ".")
