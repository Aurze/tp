/**
 * \file main.c
 * \brief Prototype 1 - LOG550
 * \author Guillaume Chevalier, Karl Thibault, Nicolas Arsenault
 * \version 0.1
 * \date 02 f�vrier 2017
 *
 * \par Portage de l�application sur le noyau multit�ches et temps r�el FreeRTOS
 *
 * \par Objectifs\n
 * � TODO
 * 
 * \par Les modules ASF suivants sont n�cessaires pour compiler ce code :\n
 * � ADC - Anagloc to Digital Converter\n
 * � GPIO - General-Purpose Input/Output (driver)\n
 * � INTC - Interrupt Controller (driver)\n
 * � PM - Power Manager (driver)\n
 * � TC - Timer/Counter (driver)\n
 * � USART - Universal Synchronous/Asynchronous Receiver/Transmitter\n
 * � Generic board support (driver)\n
 * � System Clock Control (service)
 * 
 * \par Veuillez brancher l'EVK1100 � l'ordinateur via le port USART0.
 */

/**
* \mainpage Prototype 2 - LOG550
*
* �quipe
* ======
*   - Guillaume Chevalier
*   - Karl Thibault
*   - Nicolas Arsenault
*
* \par Application qui permet de faire l'acquisition de donn�es de potentiom�tre et de capteur de lumi�re sur l'EVK1100 en utilisant FreeRTOS.
* L'application envoie les donn�es via l'USART0 pour �tre re�ues par l'application OscilloscopeTest sur l'ordinateur.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Environment header files. */
#include "power_clocks_lib.h"
#include "board.h"
#include "compiler.h"
#include "dip204.h"
#include "intc.h"
#include "gpio.h"
#include "pm.h"
#include "delay.h"
#include "spi.h"
#include "conf_clock.h"
#include "adc.h"
#include "usart.h"

/* Scheduler header files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// - D�but des constantes

/// Vitesse d'horloge s�lectionn�e (12 MHz)
#define FPBA FOSC0

/// Configuration USART
#define USART_BAUDRATE 57600
#define COMM_USART AVR32_USART0
#define USART_CHARLENGTH 8
#define CHAR_START_ACQUISITION 's'
#define CHAR_STOP_ACQUISITION 'x'

// Les d�lais sont en millisecondes car le quantum de temps est mis � 1000 Hz donc chaque quantum = 1 ms

/// D�lai de clignotement des LEDs en millisecondes
#define LED_DELAY_MS 200

/// D�lai avant de poller le USART pour caract�re re�u
#define USART_DELAY_MS 200

/// D�lai avant de demander des valeurs au ADC (= (1/500) * 1000))
#define ADC_DELAY_MS 2

/// D�lai (en ticks = 1 ms, voir plus haut) � attendre avant que la queue ait un message, si la queue n'a pas de message
#define MSG_QUEUE_BLOCK_MS 1

/// Taille de la Message Queue (besoin d'une entr�e pour chaque channel, donc 2)
// Pour tester AlarmMsgQ, mettre � 1 et on voit le d�bordement sur le LED3
#define QUEUE_LENGTH 2

/// Map des GPIO utilis�s par l'application
static const gpio_map_t GPIO_MAP =
{
	{AVR32_USART0_RXD_0_0_PIN, AVR32_USART0_RXD_0_0_FUNCTION},
	{AVR32_USART0_TXD_0_0_PIN, AVR32_USART0_TXD_0_0_FUNCTION},
	{AVR32_ADC_AD_2_PIN ,AVR32_ADC_AD_2_FUNCTION},
	{AVR32_ADC_AD_1_PIN ,AVR32_ADC_AD_1_FUNCTION}
};

/// Map des options USART
static const usart_options_t USART_OPTION =
{
	.baudrate = USART_BAUDRATE,
	.channelmode = USART_NORMAL_CHMODE,
	.charlength = USART_CHARLENGTH,
	.paritytype = USART_NO_PARITY,
	.stopbits = USART_1_STOPBIT
};

// - Fin des constantes

// - D�but des variables globales et leurs s�maphores

/// Pointeur sur le ADC de l'AVR32
static volatile avr32_adc_t *adc = &AVR32_ADC;

/// Pointeur sur le USART de l'AVR32
static volatile avr32_usart_t *usart = &COMM_USART;

/// Handle pour la Message Queue
static xQueueHandle mq_handle = NULL;

/// Fanion activ� lorsqu'en capture de donn�es
static bool acquisition = false;
static xSemaphoreHandle sem_acquisition = NULL;

/// Fanion activ� lorsque le Message Queue d�borde
static bool deborde = false;
static xSemaphoreHandle sem_deborde = NULL;

// S�maphores de synchronisation pour la conversion et l'envoi
static xSemaphoreHandle sem_sync_convert = NULL;
static xSemaphoreHandle sem_sync_send = NULL;

// - Fin des variables globales

// - D�but des fonctions

/**
 *\fn LED_Flash(void *parametres)
 *\brief Clignoter les LEDs au besoin aux 200 ms.
 *\par LED1 clignote toujours d�s que votre microcontr�leur est aliment�.
 *\par LED2 clignote lorsque l�acquisition est en service.
 *\par LED3 s�allume et reste allum� si le � Message Queue � d�borde au moins une fois.
 */
static void LED_Flash(void *parametres)
{
	for ( ;; )
	{
		// Ouvrir le LED 1 en tout temps
		gpio_clr_gpio_pin(LED0_GPIO);

		// Ouvrir le LED 2 si en acquisition
		xSemaphoreTake(sem_acquisition, portMAX_DELAY);
		if (acquisition)
			gpio_clr_gpio_pin(LED1_GPIO);
		xSemaphoreGive(sem_acquisition);

		// Ouvrir le LED 3 si Message Queue a d�bord�
		xSemaphoreTake(sem_deborde, portMAX_DELAY);
		if (deborde)
			gpio_clr_gpio_pin(LED2_GPIO);
		xSemaphoreGive(sem_deborde);

		// Attendre 200 ms
		vTaskDelay(LED_DELAY_MS);

		// Fermer LEDs 1 et 2 pour les faire clignoter
		gpio_set_gpio_pin(LED0_GPIO);
		gpio_set_gpio_pin(LED1_GPIO);

		// Attendre 200 ms
		vTaskDelay(LED_DELAY_MS);
	}
}

/**
 *\fn UART_Cmd_RX(void *parametres)
 *\brief V�rifie si des commandes sont re�ues par le UART � chaque 200 ms
 */
static void UART_Cmd_RX(void *parametres)
{
	xTaskHandle task_ADC_Cmd_handle = *(xTaskHandle *) parametres;
	bool acquisition_received;
	int char_recu;
	for ( ;; )
	{
		// V�rifier si on a re�u un caract�re
		if ((usart->csr & AVR32_USART_CSR_RXRDY_MASK) != 0)
		{
			// Lire un caract�re
			char_recu = (usart->rhr & AVR32_USART_RHR_RXCHR_MASK) >> AVR32_USART_RHR_RXCHR_OFFSET;

			// Est-ce qu'on a re�u start (true) ou stop (false)?
			acquisition_received = char_recu == CHAR_START_ACQUISITION;

			// Mettre � jour la variable globale acquisition
			xSemaphoreTake(sem_acquisition, portMAX_DELAY);
			acquisition = acquisition_received;
			xSemaphoreGive(sem_acquisition);

			if (acquisition_received)
				vTaskResume(task_ADC_Cmd_handle);
			else
				vTaskSuspend(task_ADC_Cmd_handle);
		}

		vTaskDelay(USART_DELAY_MS);
	}
}

/**
 *\fn UART_SendSample(void *parametres)
 *\brief Vide le Message Queue et envoie les �chantillons UART vers le PC.
 */
static void UART_SendSample(void *parametres)
{
	portBASE_TYPE received;
	int recv_value = 0, i = 0;

	for ( ;; )
	{
		// Prendre deux fois le s�maphore d'envoi pour avoir deux �chantillons
		xSemaphoreTake(sem_sync_send, portMAX_DELAY);
		xSemaphoreTake(sem_sync_send, portMAX_DELAY);

		// D�but de la section critique d'envoi USART
		taskENTER_CRITICAL();

		// Traiter deux �chantillons
		for (i = 0; i < 2; i++)
		{
			// Recevoir la valeur dans la variable recv_value
			received = xQueueReceive(mq_handle, &recv_value, MSG_QUEUE_BLOCK_MS);
		
			// Il y a une valeur � envoyer dans la queue
			if (received == pdTRUE) {
				
				// Attendre (activement) que le USART soit pr�t � transmettre (sinon on va aller trop vite)
				while (!(COMM_USART.csr & AVR32_USART_CSR_TXRDY_MASK));

				// Transmettre sur le USART
				COMM_USART.thr = recv_value;
			}
		}

		// Fin de la section critique
		taskEXIT_CRITICAL();

		// Redonner le s�maphore de conversion
		xSemaphoreGive(sem_sync_convert);
	}
}

/**
 *\fn ADC_Cmd(void *parametres)
 *\brief D�marre les conversions, obtient les �chantillons et les place dans la Message Queue
 *\par Si le Message Queue est plein, envoie information � la t�che AlarmMsgQ.
 *\par Le param�tre re�u doit �tre le handle pour la t�che AlarmMsgQ.
 */
static void ADC_Cmd(void *parametres)
{
	xTaskHandle task_AlarmMsgQ_handle = *(xTaskHandle *) parametres;
	int i, adc_val, adc_msg;
	portBASE_TYPE erreur;
	
	vTaskSuspend(NULL);
	for ( ;; )
	{
		// Prendre la s�maphore de conversion
		xSemaphoreTake(sem_sync_convert, portMAX_DELAY);

		// Demander une conversion ADC
		adc_start(adc);
		
		// Effectuer deux fois : 1 = potentiom�tre, 2 = lumi�re (doivent �tre continus, donc forc�ment li� au board EVK1100)
		for (i = ADC_POTENTIOMETER_CHANNEL; i <= ADC_LIGHT_CHANNEL; i++)
		{
			// R�cup�rer la valeur ADC
			adc_val = *((uint32_t *)((&(adc->cdr0)) + i));

			// Convertir en message pour le logiciel PC en ajoutant le canal
			if (i == ADC_POTENTIOMETER_CHANNEL)
				adc_msg = ((adc_val >> 2) & 0xFE) & AVR32_USART_THR_TXCHR_MASK;
			else
				adc_msg = ((adc_val >> 2) | 0x1) & AVR32_USART_THR_TXCHR_MASK;

			// Envoyer dans la queue pour envoi au PC
			erreur = xQueueSend(mq_handle, &adc_msg, MSG_QUEUE_BLOCK_MS);
		
			// Le Message Queue d�borde - indiquer sur le LEDs
			if (erreur == errQUEUE_FULL)
				vTaskResume(task_AlarmMsgQ_handle);

			// Augmenter le s�maphore de synchronisation d'envoi
			xSemaphoreGive(sem_sync_send);
		}
		
		vTaskDelay(ADC_DELAY_MS);
	}
}

/**
 *\fn AlarmMsgQ(void *parametres)
 *\brief R�veill�e lorsque d�bordement de la Message Queue
 */
static void AlarmMsgQ(void *parametres)
{
	for ( ;; )
	{
		// Suspendre la t�che actuelle (elle se fera r�veiller de l'ext�rieur si n�cessaire)
		vTaskSuspend(NULL);
		
		// Indiquer � LED_Flash qu'il faut allumer le LED 3 via la globale "deborde"
		xSemaphoreTake(sem_deborde, portMAX_DELAY);
		deborde = true;
		xSemaphoreGive(sem_deborde);
	}
}

/**
* \fn void hdw_init()
* \brief Initialisation du mat�riel (analog-digital converter et du USART0)
*/
static inline void hdw_init()
{
	// Utiliser le cristal OSC0 12 MHz
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
	
	// �teindre tous les LEDs par d�faut
	LED_Display(0);

	// USART et ADC sur GPIO
	gpio_enable_module(GPIO_MAP, sizeof(GPIO_MAP) / sizeof(GPIO_MAP[0]));
	
	// Initialisation USART
	usart_init_rs232(&COMM_USART, &USART_OPTION, FPBA);
	
	// Initialisation ADC
	adc_configure(adc);

	// Ajuster la valeur de l'horloge ADC (pris de l'exemple Atmel pour ADC sans interruption)
	AVR32_ADC.mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;

	// Activer les canaux ADC du potentiom�tre et du capteur de lumi�re
	// Note : Le fait d'activer les deux canaux fait en sorte que la valeur de la lumi�re est un peu modifi�e selon la valeur du potentiom�tre...
	adc_enable(adc, ADC_POTENTIOMETER_CHANNEL);
	adc_enable(adc, ADC_LIGHT_CHANNEL);
}

/**
 * \fn void sem_init()
 * \brief Initialisation des s�maphores lors du lancement du programme.
 */
 static inline void sem_init()
 {
	// S�maphores de r�gion critique
	sem_acquisition = xSemaphoreCreateCounting(1, 1);
	sem_deborde = xSemaphoreCreateCounting(1, 1);
	
	// S�maphore de synchronisation : permet de d�buter la conversion 
	sem_sync_convert = xSemaphoreCreateCounting(1, 1);

	// S�maphore de synchronisation : permet d'envoyer les �chantillons 
	sem_sync_send = xSemaphoreCreateCounting(2, 0);
 }

 /**
  * \fn void task_init()
  * \brief Initialise les t�ches, la queue et l'ordonnanceur FreeRTOS.
  */
 static inline void task_init()
 {
	// Message Queue
	mq_handle = xQueueCreate(QUEUE_LENGTH, sizeof(int));

	// L'allocation de laqueue a r�ussi
	if (mq_handle != NULL)
	{
		// La t�che AlarmMsgQ doit �tre conserv�e pour qu'elle puisse se faire r�veiller par ADC_Cmd si la queue est pleine
		xTaskHandle task_AlarmMsgQ_handle = NULL;

		// Elle a la priorit� la plus �lev�e pour recevoir du temps d'ex�cution d�s qu'elle est r�veill�e
		xTaskCreate(AlarmMsgQ, (const signed portCHAR *)"AlarmMsgQ", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 5, &task_AlarmMsgQ_handle);

		// La t�che ADC_Cmd doit �tre conserv�e pour que la t�che UART_Cmd_RX puisse la r�veiller et l'arr�ter
		xTaskHandle task_ADC_Cmd_handle = NULL;
		xTaskCreate(ADC_Cmd, (const signed portCHAR *)"ADC_Cmd", configMINIMAL_STACK_SIZE, &task_AlarmMsgQ_handle, tskIDLE_PRIORITY + 1, &task_ADC_Cmd_handle);

		// Initialiser les autres t�ches
		xTaskCreate(LED_Flash, (const signed portCHAR *)"LED_Flash", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 4, NULL);
		xTaskCreate(UART_Cmd_RX, (const signed portCHAR *)"UART_Cmd_RX", configMINIMAL_STACK_SIZE, &task_ADC_Cmd_handle, tskIDLE_PRIORITY + 3, NULL);
		xTaskCreate(UART_SendSample, (const signed portCHAR *)"UART_SendSample", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);

		// D�marrer l'ordonnanceur
		vTaskStartScheduler();
	}
	else
	{
		// Allumer le LED rouge (6) pour indiquer l'erreur d'allocation de m�moire
		gpio_clr_gpio_pin(LED5_GPIO);
	}
 }

int main(void)
{
	// Initialiser le mat�riel
	hdw_init();

	// Initialiser les s�maphores
	sem_init();

	// Initialiser les t�ches et l'ordonnanceur
	task_init();

	// M�moire insuffisante...
	return 0;
}

// - Fin des fonctions