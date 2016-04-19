/*
 * ADT7420.c
 *
 * Created: 3/5/2016 9:36:47 AM
 *  Author: akr72
 */ 

/* ADT7420 Temperature sensor driver functions */
#include <asf.h>
#include "HAL.h"

uint16_t uiTemperature=0;

void configure_ADT7420(void)
{

	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t wr_buffer[2] = {TEMP_SENSOR_CONFIG_ADDR,TEMP_SENSOR_CONFIG_OP_MODE_SHDN};
	
	i2c_packet.address = TEMP_SENSOR_ADDRESS;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer;
	
	struct system_pinmux_config config_pinmux;
	system_pinmux_get_config_defaults(&config_pinmux);
	config_pinmux.mux_position = SYSTEM_PINMUX_GPIO;
	config_pinmux.direction = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
	config_pinmux.input_pull = SYSTEM_PINMUX_PIN_PULL_DOWN;
	
	// Mode 0
	system_pinmux_pin_set_config(ADT7420_EN_PIN, &config_pinmux);
	port_pin_set_output_level(ADT7420_EN_PIN, true);
	
	for(int i = 0; i < 10000; i++);
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
 		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	port_pin_set_output_level(ADT7420_EN_PIN, false);
}


void ADT7420_read_temp(void)
{
	struct i2c_master_packet i2c_packet;
	uint16_t uiTimer = 0;
	uint8_t ucDataBuffer[1] = {0};
	uint8_t wr_buffer[2] = {0,0};
	
	i2c_packet.address = TEMP_SENSOR_ADDRESS;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.hs_master_code = 0;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer;
	wr_buffer[0] = TEMP_SENSOR_CONFIG_ADDR;
	wr_buffer[1] = TEMP_SENSOR_CONFIG_OP_MODE_OS;
	
	port_pin_set_output_level(ADT7420_EN_PIN, true);
	
	// Delay long enough for the chip to come up
	for(int i = 0; i < 10000; i++);
	
	do{
		status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	
	// Wait for 240 ms for conversion to complete
	for(int i = 0; i < 10000; i++);
	
	uiTimer = 0;
	
	// Set reg pointer to upper byte and read it
	i2c_packet.data_length = 1;
	wr_buffer[0] = TEMP_SENSOR_TEMP_REG_MS_ADDR;
	do{
		status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	
	uiTimer = 0;
	
	i2c_packet.data_length = 1;
	i2c_packet.data = ucDataBuffer;
	do{
		status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	uiTemperature = ucDataBuffer[0] << 8;
	
	uiTimer = 0;
	
	// Set reg pointer to lower byte and read it
	i2c_packet.data = wr_buffer;
	i2c_packet.data_length = 1;
	wr_buffer[0] = TEMP_SENSOR_TEMP_REG_LS_ADDR;
	do{
		status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	i2c_packet.data_length = 1;
	i2c_packet.data = ucDataBuffer;
	
	uiTimer = 0;
	
	do{
		status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	uiTemperature = uiTemperature | ucDataBuffer[0];
	uint8_t ucTemp = (uiTemperature/128 & 0xFF);
	SP1ML_transmit_data(&ucTemp, 1);
	
	// Put it back in shutdown.
	port_pin_set_output_level(ADT7420_EN_PIN, false);
	
}