/**
 * \file main.c
 * \brief Prototype 1 - LOG550
 * \author Guillaume Chevalier, Karl Thibault, Nicolas Arsenault
 * \version 0.1
 * \date 02 février 2017
 *
 * \par Portage de l’application sur le noyau multitâches et temps réel FreeRTOS
 *
 * \par Objectifs\n
 * • TODO
 * 
 * \par Les modules ASF suivants sont nécessaires pour compiler ce code :\n
 * • ADC - Anagloc to Digital Converter\n
 * • GPIO - General-Purpose Input/Output (driver)\n
 * • INTC - Interrupt Controller (driver)\n
 * • PM - Power Manager (driver)\n
 * • TC - Timer/Counter (driver)\n
 * • USART - Universal Synchronous/Asynchronous Receiver/Transmitter\n
 * • Generic board support (driver)\n
 * • System Clock Control (service)
 * 
 * \par Veuillez brancher l'EVK1100 à l'ordinateur via le port USART0.
 */

/**
* \mainpage Prototype 2 - LOG550
*
* Équipe
* ======
*   - Guillaume Chevalier
*   - Karl Thibault
*   - Nicolas Arsenault
*
* \par Application qui permet de faire l'acquisition de données de potentiomètre et de capteur de lumière sur l'EVK1100 en utilisant FreeRTOS.
* L'application envoie les données via l'USART0 pour être reçues par l'application OscilloscopeTest sur l'ordinateur.
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

// - Début des constantes

/// Vitesse d'horloge sélectionnée (12 MHz)
#define FPBA FOSC0

/// Configuration USART
#define USART_BAUDRATE 57600
#define COMM_USART AVR32_USART0
#define USART_CHARLENGTH 8
#define CHAR_START_ACQUISITION 's'
#define CHAR_STOP_ACQUISITION 'x'

// Les délais sont en millisecondes car le quantum de temps est mis à 1000 Hz donc chaque quantum = 1 ms

/// Délai de clignotement des LEDs en millisecondes
#define LED_DELAY_MS 200

/// Délai avant de poller le USART pour caractère reçu
#define USART_DELAY_MS 200

/// Délai avant de demander des valeurs au ADC (= (1/500) * 1000))
#define ADC_DELAY_MS 2

/// Délai (en ticks = 1 ms, voir plus haut) à attendre avant que la queue ait un message, si la queue n'a pas de message
#define MSG_QUEUE_BLOCK_MS 1

/// Taille de la Message Queue (besoin d'une entrée pour chaque channel, donc 2)
// Pour tester AlarmMsgQ, mettre à 1 et on voit le débordement sur le LED3
#define QUEUE_LENGTH 2

/// Map des GPIO utilisés par l'application
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

// - Début des variables globales et leurs sémaphores

/// Pointeur sur le ADC de l'AVR32
static volatile avr32_adc_t *adc = &AVR32_ADC;

/// Pointeur sur le USART de l'AVR32
static volatile avr32_usart_t *usart = &COMM_USART;

/// Handle pour la Message Queue
static xQueueHandle mq_handle = NULL;

/// Fanion activé lorsqu'en capture de données
static bool acquisition = false;
static xSemaphoreHandle sem_acquisition = NULL;

/// Fanion activé lorsque le Message Queue déborde
static bool deborde = false;
static xSemaphoreHandle sem_deborde = NULL;

// Sémaphores de synchronisation pour la conversion et l'envoi
static xSemaphoreHandle sem_sync_convert = NULL;
static xSemaphoreHandle sem_sync_send = NULL;

// - Fin des variables globales

// - Début des fonctions

/**
 *\fn LED_Flash(void *parametres)
 *\brief Clignoter les LEDs au besoin aux 200 ms.
 *\par LED1 clignote toujours dès que votre microcontrôleur est alimenté.
 *\par LED2 clignote lorsque l’acquisition est en service.
 *\par LED3 s’allume et reste allumé si le « Message Queue » déborde au moins une fois.
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

		// Ouvrir le LED 3 si Message Queue a débordé
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
 *\brief Vérifie si des commandes sont reçues par le UART à chaque 200 ms
 */
static void UART_Cmd_RX(void *parametres)
{
	xTaskHandle task_ADC_Cmd_handle = *(xTaskHandle *) parametres;
	bool acquisition_received;
	int char_recu;
	for ( ;; )
	{
		// Vérifier si on a reçu un caractère
		if ((usart->csr & AVR32_USART_CSR_RXRDY_MASK) != 0)
		{
			// Lire un caractère
			char_recu = (usart->rhr & AVR32_USART_RHR_RXCHR_MASK) >> AVR32_USART_RHR_RXCHR_OFFSET;

			// Est-ce qu'on a reçu start (true) ou stop (false)?
			acquisition_received = char_recu == CHAR_START_ACQUISITION;

			// Mettre à jour la variable globale acquisition
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
 *\brief Vide le Message Queue et envoie les échantillons UART vers le PC.
 */
static void UART_SendSample(void *parametres)
{
	portBASE_TYPE received;
	int recv_value = 0, i = 0;

	for ( ;; )
	{
		// Prendre deux fois le sémaphore d'envoi pour avoir deux échantillons
		xSemaphoreTake(sem_sync_send, portMAX_DELAY);
		xSemaphoreTake(sem_sync_send, portMAX_DELAY);

		// Début de la section critique d'envoi USART
		taskENTER_CRITICAL();

		// Traiter deux échantillons
		for (i = 0; i < 2; i++)
		{
			// Recevoir la valeur dans la variable recv_value
			received = xQueueReceive(mq_handle, &recv_value, MSG_QUEUE_BLOCK_MS);
		
			// Il y a une valeur à envoyer dans la queue
			if (received == pdTRUE) {
				
				// Attendre (activement) que le USART soit prêt à transmettre (sinon on va aller trop vite)
				while (!(COMM_USART.csr & AVR32_USART_CSR_TXRDY_MASK));

				// Transmettre sur le USART
				COMM_USART.thr = recv_value;
			}
		}

		// Fin de la section critique
		taskEXIT_CRITICAL();

		// Redonner le sémaphore de conversion
		xSemaphoreGive(sem_sync_convert);
	}
}

/**
 *\fn ADC_Cmd(void *parametres)
 *\brief Démarre les conversions, obtient les échantillons et les place dans la Message Queue
 *\par Si le Message Queue est plein, envoie information à la tâche AlarmMsgQ.
 *\par Le paramètre reçu doit être le handle pour la tâche AlarmMsgQ.
 */
static void ADC_Cmd(void *parametres)
{
	xTaskHandle task_AlarmMsgQ_handle = *(xTaskHandle *) parametres;
	int i, adc_val, adc_msg;
	portBASE_TYPE erreur;
	
	vTaskSuspend(NULL);
	for ( ;; )
	{
		// Prendre la sémaphore de conversion
		xSemaphoreTake(sem_sync_convert, portMAX_DELAY);

		// Demander une conversion ADC
		adc_start(adc);
		
		// Effectuer deux fois : 1 = potentiomètre, 2 = lumière (doivent être continus, donc forcément lié au board EVK1100)
		for (i = ADC_POTENTIOMETER_CHANNEL; i <= ADC_LIGHT_CHANNEL; i++)
		{
			// Récupérer la valeur ADC
			adc_val = *((uint32_t *)((&(adc->cdr0)) + i));

			// Convertir en message pour le logiciel PC en ajoutant le canal
			if (i == ADC_POTENTIOMETER_CHANNEL)
				adc_msg = ((adc_val >> 2) & 0xFE) & AVR32_USART_THR_TXCHR_MASK;
			else
				adc_msg = ((adc_val >> 2) | 0x1) & AVR32_USART_THR_TXCHR_MASK;

			// Envoyer dans la queue pour envoi au PC
			erreur = xQueueSend(mq_handle, &adc_msg, MSG_QUEUE_BLOCK_MS);
		
			// Le Message Queue déborde - indiquer sur le LEDs
			if (erreur == errQUEUE_FULL)
				vTaskResume(task_AlarmMsgQ_handle);

			// Augmenter le sémaphore de synchronisation d'envoi
			xSemaphoreGive(sem_sync_send);
		}
		
		vTaskDelay(ADC_DELAY_MS);
	}
}

/**
 *\fn AlarmMsgQ(void *parametres)
 *\brief Réveillée lorsque débordement de la Message Queue
 */
static void AlarmMsgQ(void *parametres)
{
	for ( ;; )
	{
		// Suspendre la tâche actuelle (elle se fera réveiller de l'extérieur si nécessaire)
		vTaskSuspend(NULL);
		
		// Indiquer à LED_Flash qu'il faut allumer le LED 3 via la globale "deborde"
		xSemaphoreTake(sem_deborde, portMAX_DELAY);
		deborde = true;
		xSemaphoreGive(sem_deborde);
	}
}

/**
* \fn void hdw_init()
* \brief Initialisation du matériel (analog-digital converter et du USART0)
*/
static inline void hdw_init()
{
	// Utiliser le cristal OSC0 12 MHz
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
	
	// Éteindre tous les LEDs par défaut
	LED_Display(0);

	// USART et ADC sur GPIO
	gpio_enable_module(GPIO_MAP, sizeof(GPIO_MAP) / sizeof(GPIO_MAP[0]));
	
	// Initialisation USART
	usart_init_rs232(&COMM_USART, &USART_OPTION, FPBA);
	
	// Initialisation ADC
	adc_configure(adc);

	// Ajuster la valeur de l'horloge ADC (pris de l'exemple Atmel pour ADC sans interruption)
	AVR32_ADC.mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;

	// Activer les canaux ADC du potentiomètre et du capteur de lumière
	// Note : Le fait d'activer les deux canaux fait en sorte que la valeur de la lumière est un peu modifiée selon la valeur du potentiomètre...
	adc_enable(adc, ADC_POTENTIOMETER_CHANNEL);
	adc_enable(adc, ADC_LIGHT_CHANNEL);
}

/**
 * \fn void sem_init()
 * \brief Initialisation des sémaphores lors du lancement du programme.
 */
 static inline void sem_init()
 {
	// Sémaphores de région critique
	sem_acquisition = xSemaphoreCreateCounting(1, 1);
	sem_deborde = xSemaphoreCreateCounting(1, 1);
	
	// Sémaphore de synchronisation : permet de débuter la conversion 
	sem_sync_convert = xSemaphoreCreateCounting(1, 1);

	// Sémaphore de synchronisation : permet d'envoyer les échantillons 
	sem_sync_send = xSemaphoreCreateCounting(2, 0);
 }

 /**
  * \fn void task_init()
  * \brief Initialise les tâches, la queue et l'ordonnanceur FreeRTOS.
  */
 static inline void task_init()
 {
	// Message Queue
	mq_handle = xQueueCreate(QUEUE_LENGTH, sizeof(int));

	// L'allocation de laqueue a réussi
	if (mq_handle != NULL)
	{
		// La tâche AlarmMsgQ doit être conservée pour qu'elle puisse se faire réveiller par ADC_Cmd si la queue est pleine
		xTaskHandle task_AlarmMsgQ_handle = NULL;

		// Elle a la priorité la plus élevée pour recevoir du temps d'exécution dès qu'elle est réveillée
		xTaskCreate(AlarmMsgQ, (const signed portCHAR *)"AlarmMsgQ", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 5, &task_AlarmMsgQ_handle);

		// La tâche ADC_Cmd doit être conservée pour que la tâche UART_Cmd_RX puisse la réveiller et l'arrêter
		xTaskHandle task_ADC_Cmd_handle = NULL;
		xTaskCreate(ADC_Cmd, (const signed portCHAR *)"ADC_Cmd", configMINIMAL_STACK_SIZE, &task_AlarmMsgQ_handle, tskIDLE_PRIORITY + 1, &task_ADC_Cmd_handle);

		// Initialiser les autres tâches
		xTaskCreate(LED_Flash, (const signed portCHAR *)"LED_Flash", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 4, NULL);
		xTaskCreate(UART_Cmd_RX, (const signed portCHAR *)"UART_Cmd_RX", configMINIMAL_STACK_SIZE, &task_ADC_Cmd_handle, tskIDLE_PRIORITY + 3, NULL);
		xTaskCreate(UART_SendSample, (const signed portCHAR *)"UART_SendSample", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);

		// Démarrer l'ordonnanceur
		vTaskStartScheduler();
	}
	else
	{
		// Allumer le LED rouge (6) pour indiquer l'erreur d'allocation de mémoire
		gpio_clr_gpio_pin(LED5_GPIO);
	}
 }

int main(void)
{
	// Initialiser le matériel
	hdw_init();

	// Initialiser les sémaphores
	sem_init();

	// Initialiser les tâches et l'ordonnanceur
	task_init();

	// Mémoire insuffisante...
	return 0;
}

// - Fin des fonctions