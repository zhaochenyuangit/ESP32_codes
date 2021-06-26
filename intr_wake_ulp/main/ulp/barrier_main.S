#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

.set PIN_1_RTC_NUM, 4 //GPIO34
.set PIN_2_RTC_NUM, 5 //GPIO35

/* Define variables, which go into .bss section (zero-initialized data) */
	.bss
	.global triggered_pin
triggered_pin:	.long 0
	.global edge_direction
edge_direction: .long 0
	.global debug
debug: .long 0
	.global debug2
debug2: .long 0
pin1_status: .long 0
pin2_status: .long 0

/* Code goes into .text section */
	.text
	.global entry
entry:
    /*read input registor of RTC_GPIO 0-15 into r0*/
    READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
	move r1, r0

	move r2, debug2
	st r0, r2, 0 

	move r2, PIN_2_RTC_NUM
	rsh r0, r1, r2
	and r0, r0 ,1
	jumpr wakeup, 1 ,eq
	
	move r2, PIN_1_RTC_NUM
	rsh r0, r1, r2 //pin level now
	move r2, pin1_status 
	ld r3, r2, 0 //pin level last time
	st r0, r2, 0 //update pin level for next loop
	add r0, r0, r3 //xor operation
	and r0, r0, 1 //0+0=0, 1+0=1, 1+1=[1]0
	move r2, PIN_1_RTC_NUM
	jumpr wakeup, 1, eq

	

	move r1, debug
	ld r0, r1, 0
	add r0 ,r0, 1
	st r0, r1, 0
	
exit:
	halt

wakeup:
	move r0, triggered_pin
	st r2, r0, 0
	wake
	/* Stop the wakeup timer so it does not restart ULP */
	//WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
	halt