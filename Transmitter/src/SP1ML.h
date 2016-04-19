/*
 * SP1ML.h
 *
 * Created: 4/12/2016 11:59:16 AM
 *  Author: akr72
 */ 


#ifndef SP1ML_H_
#define SP1ML_H_
#include "HAL.h"
#include <asf.h>

/* SP1ML Radio Defines */
#define SP1ML_MODE_PIN	PIN_PA01
#define SP1ML_RESET_PIN	PIN_PA03
#define SP1ML_SHDN_PIN	PIN_PA02
#define SP1ML_EN_PIN	PIN_PA27

/* SP1ML prototype definitions */
void configure_SP1ML(void);
uint8_t SP1ML_set_baud(uint32_t rate);
uint8_t SP1ML_set_output_power(int8_t power);
void SP1ML_enter_op_mode(void);
void SP1ML_enter_cmd_mode(void);
void SP1ML_transmit_debug(void);
void SP1ML_transmit_data(uint8_t * data, uint16_t length);

// UART relate modules
struct usart_config config_usart;
struct usart_module usart_instance;

#endif /* SP1ML_H_ */