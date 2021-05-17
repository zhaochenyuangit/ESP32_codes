#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/rtc_i2c_reg.h"
#include "soc/soc_ulp.h"

	.bss
    .global dummy
dummy: 
    .long 0

	.global p1_status
p1_status:
	.long 0

	.global event_counter
event_counter:
	.long 0

	.text
exit:
	halt

wakeup:
	/* try to wakeup SoC, if not => halt */
	STAGE_INC  1
	JUMPS exit, 10, GT

	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	AND r0, r0, 1
	JUMP wakeup, eq
	wake

	halt

    .global entry
entry:
	STAGE_RST
    move r3, dummy
    //i2c_rd 0x80, 7, 0, 1
    st r0, r3, 0
	/* read the RTC pins and store it into global variables ; just read the pin register once and store it into R1 */
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
	move r1, r0

	move r3, p1_status
    rsh r0, r1, 7
	and r0, r0, 1
	st r0, r3, 0
    move r3, p1_status
    ld r2, r3, 0
    and r0, r2, 1
    jump wakeup, eq

	halt
