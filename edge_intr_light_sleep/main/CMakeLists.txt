idf_component_register(SRCS 
                    "main.c"    
                    "../lib/edge_intr_debounce.c"
                    "../lib/commands_corner_cases.c"
                    "../lib/my_wifi.c"
                    "../lib/my_mqtt.c"
                    "../lib/sntpController.c"
                    INCLUDE_DIRS "."
                                "../lib"
                                "../include")