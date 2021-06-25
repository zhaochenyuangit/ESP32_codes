a demo of enable edge interrupt during sleep mode, usuage:
1.  setup wake-up pins in edge_intr_sleep.h, setup simulation trigger pins in commands_corner_cases.h   
    currently they are set to pin 4, 25 and 18, 19 repectively, 
    connecting pin 4 with pin 18, pin 25 with pin 19 will do
2. all the pins are floated without any pull-up/down resistor, additional pull-down sensor is necessary
    to avoid ambiguous reading
3. choose sleep mode in edge_intr_sleep.h file,the board will 
    go light sleep, as long as LIGHT_SLEEP is defined
    go deep sleep, when only DEEP_SLEEP is defined 
4. my_task() is the user interface to add corner case handler, gpio_task() is an alias to my_task() 
    and is used in edge_intr_sleep.c file. By this way the user can change function name as they will
    without modifying source code in edge_intr_sleep.c 