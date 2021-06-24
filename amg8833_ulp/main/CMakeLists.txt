idf_component_register(SRCS "main.c" 
                            "helper.c" 
                            "../lib/network_wifi.c" 
                            "../lib/network_mqtt.c" 
                            "../lib/grideye_i2c.c" 
                            "../lib/grideye_api_lv1.c"
                            "../lib/sim_uart.c"
                            "../lib/cv_api.c"
                            "../lib/cv_functions.c"
                            "../lib/cv_grideye.c"
                        INCLUDE_DIRS "." 
                                    "../lib"
                        REQUIRES soc
                                 nvs_flash
                                 ulp
                                 mqtt)

# ULP support additions to component CMakeLists.txt.
#
# 1. The ULP app name must be unique (if multiple components use ULP).
set(ulp_app_name ulp_${COMPONENT_NAME})
#
# 2. Specify all assembly source files.
#    Files should be placed into a separate directory (in this case, ulp/),
#    which should not be added to COMPONENT_SRCS.
set(ulp_s_sources "ulp/grideye_main.S" "ulp/i2c-util.S" "ulp/i2c.S" "ulp/stack.S")
#
# 3. List all the component source files which include automatically
#    generated ULP export file, ${ulp_app_name}.h:
set(ulp_exp_dep_srcs "main.c")
#
# 4. Call function to build ULP binary and embed in project using the argument
#    values above.
ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")