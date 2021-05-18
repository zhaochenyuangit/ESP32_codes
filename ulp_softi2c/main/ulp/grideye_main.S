/*
 * Demo of I2C ULP routines
 */

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

#include "stack.S"

.set GRIDEYE_ADDR, 0x69
.set PIXEL_START, 0x80
.set THMS_START, 0x0E

/* Define variables, which go into .bss section (zero-initialized data) */
	.bss

	.global temp
temp: .long 0

	.global stack
stack:
	.skip 100
	.global stackEnd
stackEnd:
	.long 0


	/* Code goes into .text section */
	.text
	.global entry
entry:
	move r3,stackEnd

	// GPIO2 LED ON
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + 12, 1, 1)

	// Read 16 bit result
	move r1, GRIDEYE_ADDR
	push r1
	move r1, PIXEL_START
	push r1
	psr
	jump read8
	add r3,r3,2 // remove call parameters from stack
	move r1,r0 // save result
	move r2,temp // store result
	st r1,r2,0
	jumpr waitNext, 80, lt
	//move r2, 50
	//psr
	//jump waitMs 

	// GPIO2 LED OFF
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + 12, 1, 0)

wakeUp:
	/* Wake up the SoC, end program */
	wake
	/* Stop the wakeup timer so it does not restart ULP */
	WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)

waitNext:
	halt

waitMs:
	wait 8000
	sub r2,r2,1
	jump doneWaitMs,eq
	jump waitMs
doneWaitMs:
	ret
