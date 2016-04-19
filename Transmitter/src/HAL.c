/*
 * HAL.c
 *
 * Created: 3/4/2016 11:58:39 AM
 *  Author: akr72
 */ 

#include "HAL.h"
#include <asf.h>

void configure_sleepmode(void)
{
	uint8_t ucIndex;
	struct system_standby_config stby_config;
	system_standby_get_config_defaults(&stby_config);
	system_set_sleepmode(SYSTEM_SLEEPMODE_STANDBY);
	// Force buck mode on the internal regulator
	SUPC->VREG.bit.SEL = 1;
	SUPC->VREG.bit.RUNSTDBY = 1;
	SUPC->VREF.bit.ONDEMAND = 1;
	// Enable dynamic powergating for domain 0 and 1
	PM->STDBYCFG.bit.DPGPD0 = 1;
	PM->STDBYCFG.bit.DPGPD1 = 1;
	
	system_flash_set_waitstates(1);
}

void configure_i2c(void)
{
	
	/* Initialize config structure and software module */
	struct i2c_master_config config_i2c_master;
	
	i2c_master_get_config_defaults(&config_i2c_master);
	/* Change buffer timeout to something longer */
	config_i2c_master.buffer_timeout = 65535;
	config_i2c_master.run_in_standby = true;
	config_i2c_master.baud_rate = I2C_MASTER_BAUD_RATE_100KHZ;
	config_i2c_master.pinmux_pad0 =  PINMUX_PA22C_SERCOM3_PAD0;
	config_i2c_master.pinmux_pad1 =  PINMUX_PA23C_SERCOM3_PAD1;
	
	/* Initialize and enable device with config */
	i2c_master_init(&i2c_master_instance, SERCOM3, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
	
	
}


void configure_mag_sw_int(void (*callback)(void))
{

	struct system_pinmux_config config_pinmux;
	system_pinmux_get_config_defaults(&config_pinmux);
	config_pinmux.mux_position = PINMUX_PA17A_EIC_EXTINT1;
	config_pinmux.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
	config_pinmux.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE;
	
	system_pinmux_pin_set_config(PIN_PA17, &config_pinmux);
	system_pinmux_pin_set_input_sample_mode(PIN_PA17, SYSTEM_PINMUX_PIN_SAMPLE_CONTINUOUS);
	
	// Disable the EIC so we can write regs
	REG_EIC_CTRLA = 0;
	// Wait for the sync to complete
	while(REG_EIC_SYNCBUSY & 0x01);
	
	// Set up clock source for ulp32k
	//REG_EIC_CTRLA |= 0x10;
	
	// Enable interrupts on EXTINT[1]
	REG_EIC_INTENSET |= 0x02;
	if(!(REG_EIC_INTENSET & 0x02)) return;
	
	// Turn filtering off and set detection for falling edge for EXTINT[1]
	REG_EIC_CONFIG0 &= ~0x80;
	REG_EIC_CONFIG0 |= 0x20;
	if(!(REG_EIC_CONFIG0 & 0x20) && (REG_EIC_CONFIG0 & 0x80)) return;
	
	// Enable asynchronous interrupts for EXTINT[1]
	REG_EIC_ASYNCH |= 0x00000002;
	if(!(REG_EIC_ASYNCH & 0x02)) return;
	
	// Enable the EIC
	REG_EIC_CTRLA = 0x02;
	// Wait for the sync to complete
	while(REG_EIC_SYNCBUSY & 0x02);
	if(!(REG_EIC_CTRLA & 0x02)) return;
	
	if(!(extint_register_callback(callback, 1, EXTINT_CALLBACK_TYPE_DETECT) == STATUS_OK)) return;

}