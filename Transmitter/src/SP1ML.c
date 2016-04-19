/*
 * SP1ML.c
 *
 * Created: 3/5/2016 9:36:47 AM
 *  Author: akr72
 */ 

/* SP1ML Radio driver module */

#include "HAL.h"
#include <asf.h>

void configure_SP1ML(void)
{
	struct system_pinmux_config config_pinmux;
	system_pinmux_get_config_defaults(&config_pinmux);
	config_pinmux.mux_position = SYSTEM_PINMUX_GPIO;
	config_pinmux.direction = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
	config_pinmux.input_pull = SYSTEM_PINMUX_PIN_PULL_DOWN;
// 	
	// Enable
 	system_pinmux_pin_set_config(SP1ML_EN_PIN, &config_pinmux);
 	port_pin_set_output_level(SP1ML_EN_PIN, false);
	
	// Mode 0
	system_pinmux_pin_set_config(SP1ML_MODE_PIN, &config_pinmux);
	port_pin_set_output_level(SP1ML_MODE_PIN, false);

	// SHDN
	system_pinmux_pin_set_config(SP1ML_SHDN_PIN, &config_pinmux);
	port_pin_set_output_level(SP1ML_SHDN_PIN, true);
	
	// reset
	system_pinmux_pin_set_config(SP1ML_RESET_PIN, &config_pinmux);
	port_pin_set_output_level(SP1ML_RESET_PIN, false);
	
	for(int i = 0; i < 65535; i++);
	
	port_pin_set_output_level(SP1ML_RESET_PIN, true);

	usart_get_config_defaults(&config_usart);
	config_usart.generator_source = GCLK_GENERATOR_2;
	config_usart.run_in_standby = false;
	config_usart.baudrate = 115200;
	config_usart.receiver_enable = true;
	config_usart.transmitter_enable = true;
	config_usart.transfer_mode = USART_TRANSFER_ASYNCHRONOUSLY;
	config_usart.parity = USART_PARITY_NONE;
	config_usart.stopbits = USART_STOPBITS_1;
	config_usart.data_order = USART_DATAORDER_LSB;
	config_usart.character_size = USART_CHARACTER_SIZE_8BIT;
	// RXPO PAD03 TXPO PAD02
	config_usart.mux_setting = USART_RX_3_TX_2_XCK_3;
	config_usart.pinmux_pad0 = PINMUX_UNUSED;
	config_usart.pinmux_pad1 = PINMUX_UNUSED;
	config_usart.pinmux_pad2 = PINMUX_PA06D_SERCOM0_PAD2;
	config_usart.pinmux_pad3 = PINMUX_PA07D_SERCOM0_PAD3;
	//REG_SERCOM0_USART_DBGCTRL |= 0x01;
	while ((status = usart_init(&usart_instance, SERCOM0, &config_usart)) != STATUS_OK);
	usart_enable(&usart_instance);
	// Set the shutdown pin low so we save power (even though the module is off)
	port_pin_set_output_level(SP1ML_SHDN_PIN, false);
	
}

uint8_t SP1ML_set_baud(uint32_t rate)
{
	
	// Check for valid rates
	if(rate < 9600 || rate > 921600) return 0;
	uint8_t recv_buff[24];
	uint8_t ucRateStr[13];
	uint8_t ucRadioBaudQuery[7] = {0x41, 0x54, 0x53, 0x30, 0x30, 0x3F, 0x0D};
	
	// Turn the radio on
	port_pin_set_output_level(SP1ML_EN_PIN, true);
	
	for(int i = 0; i < 65535; i++);
	for(int i = 0; i < 24; i++)
	{
		recv_buff[i] = 0;
	}
	
	// 	Put the SP1ML into command mode -- handles waking up
	SP1ML_enter_cmd_mode();
	
	status = usart_write_buffer_wait(&usart_instance, "AT/V\r", 5);
	status = usart_read_buffer_wait(&usart_instance, recv_buff, 24);
	
	sprintf(ucRateStr, "ATS00=%+06d\r", rate);
	
	for(int i = 0; i < 65535; i++);
	
	status = usart_write_buffer_wait(&usart_instance, ucRateStr, 13);
	
	for(int i = 0; i < 65535; i++);
	usart_disable(&usart_instance);
	for(int i = 0; i < 65535; i++);
	config_usart.baudrate = rate;
	while ((status = usart_init(&usart_instance, SERCOM0, &config_usart)) != STATUS_OK);
	usart_enable(&usart_instance);
	for(int i = 0; i < sizeof(recv_buff)/sizeof(recv_buff[0]); i++){
		recv_buff[i] = 0;
	}
	
	status = usart_write_buffer_wait(&usart_instance, ucRadioBaudQuery, 7);
	status = usart_read_buffer_wait(&usart_instance, recv_buff, 24);
	
	// Don't shut down or disable, because we will lose the new setting.
	
	return 1;
}

uint8_t SP1ML_set_output_power(int8_t power)
{
	uint8_t recv_buff[24];
	uint8_t ucPwrStr[10];
	uint8_t ucTransmitPowerQuery[7] = {0x41, 0x54, 0x53, 0x30, 0x34, 0x3F, 0x0D};
		
	// Make sure its turned on
	port_pin_set_output_level(SP1ML_EN_PIN, true);
	
	// Also handles waking up and correct mode.
	SP1ML_set_baud(9600);
	for(int i = 0; i < 65535; i++);
	
	for(int i = 0; i < 24; i++){
		recv_buff[i] = 0;
	}
	
	sprintf(ucPwrStr, "ATS04=%+03d\r", power);
	
	status = usart_write_buffer_wait(&usart_instance, ucPwrStr, 10);
	status = usart_read_buffer_wait(&usart_instance, recv_buff, 24);
	
	for(int i = 0; i < 65535; i++);
	
	status = usart_write_buffer_wait(&usart_instance, ucTransmitPowerQuery, 7);
	status = usart_read_buffer_wait(&usart_instance, recv_buff, 24);
	
	// Don't shut down or disable, because we will lose the new setting.
	
	return 1;
}

void SP1ML_transmit_data(uint8_t * data, uint16_t length)
{
	uint8_t ucModCMD[8] = {0x41, 0x54, 0x53, 0x30, 0x33, 0x3D, 0x34, 0x0D};
	uint8_t ucDebugData[2] = {0x01};
	uint8_t recv_buff[24];
	
	SP1ML_set_output_power(7);
	for(int i = 0; i < 65535; i++);
	
	for(int i = 0; i < 24; i++){
		recv_buff[i] = 0;
	}
	
	// Enter OOK MOD mode
	status = usart_write_buffer_wait(&usart_instance, ucModCMD, 8);
	status = usart_read_buffer_wait(&usart_instance, recv_buff, 24);
	
	SP1ML_enter_op_mode();
	
	for(int i = 0; i < 1000; i++){
		usart_write_buffer_wait(&usart_instance, data, length);
	}

	
	// Turn the radio off
	port_pin_set_output_level(SP1ML_EN_PIN, false);
	
}

void SP1ML_transmit_debug(void)
{
	uint8_t ucModCMD[8] = {0x41, 0x54, 0x53, 0x30, 0x33, 0x3D, 0x34, 0x0D};
	uint8_t ucDebugData[2] = {0x01};
	uint8_t recv_buff[24];
	
	SP1ML_set_output_power(7);
	for(int i = 0; i < 65535; i++);
	
	for(int i = 0; i < 24; i++){
		recv_buff[i] = 0;
	}
	
	// Enter OOK MOD mode
	status = usart_write_buffer_wait(&usart_instance, ucModCMD, 8);
	status = usart_read_buffer_wait(&usart_instance, recv_buff, 24);
	
	SP1ML_enter_op_mode();
	while(true){
		usart_write_buffer_wait(&usart_instance, ucDebugData, 1);
		//for(int i = 0; i < 4000000; i++);
	}
}

void SP1ML_enter_cmd_mode(void)
{
	// Wake the radio up and leave it awake. The radio must already be enabled.
	// We don't fiddle with the enable or shutdown after mode switch, we assume that is done in the calling functions
	
	// Wake up the radio
	port_pin_set_output_level(SP1ML_SHDN_PIN, true);
	
	status = usart_write_buffer_wait(&usart_instance, "+++", 3);
	
	
}

void SP1ML_enter_op_mode(void)
{
	
	// Wake the radio up and leave it awake. The radio must already be enabled.
	// We don't fiddle with the enable or shutdown after mode switch, we assume that is done in the calling functions
	
	// Wake up the radio
	port_pin_set_output_level(SP1ML_SHDN_PIN, true);
	
	status = usart_write_buffer_wait(&usart_instance, "ATO\r", 4);
	
	
}