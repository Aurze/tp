/**
 * \file functions.h
 *
 * \brief En-têtes (headers) des fonctions dans le fichier main.c
 */ 

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

inline void send_pot_val(void);
inline void send_light_val(void);
inline void get_received_char(void);
inline void toggle_leds(void);
static void gpio_init(void);
static void usart_init(void);
static void adc_init(void);
static void tc_init(void);
static void adc_timer_irq(void);
static void button_irq(void);
static void led_timer_irq(void);
static void usart_irq(void);
static void adc_irq(void);

#endif /* FUNCTIONS_H_ */
