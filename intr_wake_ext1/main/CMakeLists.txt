idf_component_register(SRCS 
                    "main.c"    
                    "../src/edge_intr_debounce.c"
                    "../src/deep_sleep.c"
                    "../src/commands_corner_cases.c"
                    "../src/my_wifi.c"
                    "../src/my_mqtt.c"
                    "../src/sntpController.c"
                    "../src/display.c"
                    INCLUDE_DIRS "."
				"../src"
				"../include")