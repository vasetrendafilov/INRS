idf_component_register(SRCS "tasks.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bmx280 mpu9250 filter neo6m)
