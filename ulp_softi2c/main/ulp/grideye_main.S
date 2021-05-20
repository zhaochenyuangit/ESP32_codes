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
	.global thms
thms: .long 0
	.global debug
debug: .long 0
pixel_reg: .long 0
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
	// Read 16 bit thermistor
	move r1, GRIDEYE_ADDR
	push r1
	move r1, THMS_START
	push r1
	psr
	jump read16_inverse
	add r3, r3, 2
	move r1, thms
	st r0, r1, 0
	/* for loop start at PIXEL_START*/
	move r0, PIXEL_START
	move r1, pixel_reg
	st r0, r1, 0
next_pixel:
	// Read 16 bit result
	move r1, GRIDEYE_ADDR
	push r1
	move r1, pixel_reg
	ld r1, r1, 0
	push r1
	psr
	jump read16_inverse
	add r3,r3,2
	/*pixel value has a coarser resolution*/ 
	lsh r0, r0, 2
	move r2, r0
	/* compare pixel reading with thermistor*/
	move r1, thms
	ld r1, r1, 0
	sub r0, r1, r0
	move r1, debug
	st r0, r1, 0
	jump wakeUp, ov

	move r1, pixel_reg
	ld r0, r1, 0
	add r0, r0, 2
	st r0, r1, 0 
	jumpr next_pixel, 0xfe, lt

exit:
	halt

wakeUp:
	move r1, temp
	st r2, r1, 0
	move r2, 50
	psr
	jump waitMs
	// GPIO2 LED OFF
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + 12, 1, 0)
	/* Wake up the SoC, end program */
	wake
	/* Stop the wakeup timer so it does not restart ULP */
	WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
	halt


waitMs:
	wait 8000 //wait 1ms
	sub r2,r2,1
	jump doneWaitMs,eq
	jump waitMs
doneWaitMs:
	ret
