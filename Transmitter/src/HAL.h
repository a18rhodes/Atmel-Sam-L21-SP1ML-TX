/* Generic HAL Defines */

#ifndef HAL_H_
#define HAL_H_

#include "ADT7420.h"
#include "SP1ML.h"

void configure_i2c(void);
void configure_mag_sw_int(void (*callback)(void));
void configure_sleepmode(void);


// General status return value
volatile enum status_code status;

// I2C module instance
struct i2c_master_module i2c_master_instance;

#endif