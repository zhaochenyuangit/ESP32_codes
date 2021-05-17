#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "soc/sens_reg.h"
#include "soc/rtc_i2c_reg.h"

/* Define variables, which go into .bss section (zero-initialized data) */
    .bss
    .global dummy
dummy: .long
    .global test
test: .long
/* Codes go to .text section*/
    .text
    .global entry
entry:
    // Main code, store the i2c read into dummy and return the value to main process
    i2c_rd  0x0E, 7, 0, 1
    st      r0, r3, 0
    move    r3, dummy
    READ_RTC_REG(SENS_SAR_SLAVE_ADDR1_REG, 0, 16)
    st r0,r2,0
    move r2,test
    jump    wake_up
    jump    entry

/* end the program */
.global exit
exit:
/* Set the GPIO13 output LOW (clear output) to signal that ULP is now going down (the 14 is because GPIO13 = RTC_GPIO_14) */
//WRITE_RTC_REG(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + 7, 1, 1)
/* Enable hold on GPIO13 output */
//WRITE_RTC_REG(RTC_IO_TOUCH_PAD4_REG, RTC_IO_TOUCH_PAD4_HOLD_S, 1, 1)
halt

.global wake_up
wake_up:
/* try to wakeup SoC, if not => halt */
STAGE_INC  1
JUMPS exit, 10, GT

READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
AND r0, r0, 1
JUMP wake_up, eq
wake    
jump entry

