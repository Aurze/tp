/**
 * \file main.c
 * \brief Prototype 1 - LOG550
 * \author Guillaume Chevalier, Karl Thibault, Nicolas Arsenault
 * \version 0.1
 * \date 02 février 2017
 *
 * \par Système d’acquisition de données en temps réel
 *
 * \par Objectifs\n
 * • Se familiariser avec l'environnement matériel/logiciel pour le développement\n
 *   − Maîtriser l’interface de Atmel Studio 6.2.\n
 *   − Être capable de compiler et de télécharger un programme fourni en exemple\n
 *   − Être capable de créer un nouveau projet, de le compiler et de le télécharger\n
 * • Explorer la vaste documentation disponible\n
 * • Lire, analyser et comprendre les programmes\n
 * • Développer la structure du programme d’acquisition sur les microcontrôleurs.
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
* \mainpage Prototype 1 - LOG550
*
* Équipe
* ======
*   - Guillaume Chevalier
*   - Karl Thibault
*   - Nicolas Arsenault
*
* \par Application qui permet de faire l'acquisition de données de potentiomètre et de capteur de lumière sur l'EVK1100.
* L'application envoie les données via l'USART0 pour être reçues par l'application OscilloscopeTest sur l'ordinateur.
*/

#include <asf.h>
#include <avr32/io.h>
#include <stdbool.h>
#include "functions.h"

// - Début des constantes

/// Vitesse d'horloge sélectionnée (12 MHz)
#define FPBA FOSC0

/// Configuration ADC
#define ADC_SR_POTENTIOMETER_MASK AVR32_ADC_SR_EOC1_MASK
#define ADC_SR_LIGHT_MASK AVR32_ADC_SR_EOC2_MASK
#define ADC_IER_POTENTIOMETER_MASK AVR32_ADC_IER_EOC1_MASK
#define ADC_IER_LIGHT_MASK AVR32_ADC_IER_EOC2_MASK

/// Configuration USART
#define USART_BAUDRATE 57600
#define COMM_USART AVR32_USART0
#define USART_CHARLENGTH 8
#define CHAR_START_ACQUISITION 's'
#define CHAR_STOP_ACQUISITION 'x'

// Configuration GPIO88 (voir http://www.atmel.com/images/doc32078.pdf#page=10)
#define GPIO88_PORT 2
#define GPIO88_BIT_CONTROL 1 << (24 & 0x1F)

/// Fréquences d'échantillonnage disponibles pour la capture et l'envoi de données
#define FREQ_1 1000
#define FREQ_2 2000

/// Canaux utilisés pour les Timer Counter
#define ADC_TC_CHANNEL 0
#define LED_TC_CHANNEL 1

/// Fréquence de clignotement des LEDs
#define LED_FREQ 10

/// Map des interruptions utilisées pour le Timer Counter
static const tc_interrupt_t TIMER_COUNTER_INTERRUPT =
{
	.etrgs = 0,
	.ldrbs = 0,
	.ldras = 0,
	.cpcs  = 1,
	.cpbs  = 0,
	.cpas  = 0,
	.lovrs = 0,
	.covfs = 0
};

/// Map des GPIO utilisés par l'application
static const gpio_map_t GPIO_MAP =
{
	{AVR32_USART0_RXD_0_0_PIN, AVR32_USART0_RXD_0_0_FUNCTION},
	{AVR32_USART0_TXD_0_0_PIN, AVR32_USART0_TXD_0_0_FUNCTION},
	{AVR32_ADC_AD_2_PIN ,AVR32_ADC_AD_2_FUNCTION},
	{AVR32_ADC_AD_1_PIN ,AVR32_ADC_AD_1_FUNCTION}
};

// - Fin des constantes

// - Début des variables globales

/// Pointeur sur le ADC de l'AVR32
volatile avr32_adc_t *adc = &AVR32_ADC;

/// Fanion activé lorsqu'en capture de données
volatile bool running = false;

/// Fanion activé lorsque la valeur de lumière est disponible
volatile bool light_available = false;

/// Fanion activé lorsque la valeur de potentiomètre est disponible
volatile bool pot_available = false;

/// Fanion activé lorsqu'il y a un débordement du ADC (valeur convertie n'a pas été envoyée)
volatile bool adc_overflow = false;

/// Fanion pour faire clignoter les LEDs (lorsque vrai, la LED clignotera dans l'interruption)
volatile bool toggle_led = false;

/// Fanion indiquant si le statut de transmission a changé (lorsque vrai, on doit aller chercher le caractère sur USART)
volatile bool char_received = false;

/// Fanion indiquant si la transmission de la valeur de lumière est complétée (on peut maintenant transmettre le potentiomètre)
volatile bool light_tx_done = false;

/// Caractere reçu du USART
volatile U32 rx_char;

/// Valeur de la lumière récupérée du ADC
volatile unsigned int light_val = 0;

/// Valeur du potentiomètre récupérée du ADC
volatile unsigned int pot_val = 0;

/// Valeur d'échantillonage courante ADC
uint32_t sampling_rate = FREQ_1;

// - Fin des variables globales

/*
 * \fn int main(void)
 * \brief Fonction principales
 * Fonction appellée lors de l'exécution du programme sur le microcontrôleur.
 * Les composantes et leurs interruptions sont configurées, puis une boucle perpétuelle s'occupe
 * du traitement relié aux fanions levés par les interruptions.
 */
int main(void)
{
	// Initialiser l'horloge selon les valeurs contenues dans config/conf_clock.h
	sysclk_init();

	// Désactiver les interuptions lors de l'initialisation des composantes
	Disable_global_interrupt();

	// Initialiser les interruptions et les composantes
	INTC_init_interrupts();
	adc_init();
	usart_init();
	gpio_init();
	tc_init();

	// Activer les interruptions
	Enable_global_interrupt();

	// Boucle perpétuelle d'exécution
	while(true)
	{
		// Lorsque la lumière est disponible (via l'interruption ADC), envoyer la lumière
		if (light_available)
		{
			send_light_val();
		}
		// Lorsque la lumière a terminé de transmettre, envoyer le potentiomètre
		// Ceci s'assure que la lumière n'est pas toujours envoyée sans jamais envoyer le potentiomètre et force un ordre d'envoi
		if (light_tx_done && pot_available)
		{
			light_tx_done = false;
			send_pot_val();
		}
		// Lorsqu'un caractère a été reçu de l'ordinateur (via l'interuption USART), lire le caractère
		if (char_received)
		{
			get_received_char();
		}
		// Lorsque les LEDs doivent être commutées (via l'interuption timer counter pour les LEDs), commuter les LEDs
		if (toggle_led)
		{
			toggle_leds();
		}
	}
}

 /* Début des interrupts */

/**
* \fn static void adc_irq()
* \brief Gestionnaire d'interruption pour le analog-digital converter
* \par Cette interruption est lancée lorsque l'ADC a terminé de convertir une donnée.
* La donnée peut être la valeur du potentiomètre ou la valeur du capteur de lumière.
* 
* \par Cette interruption lit les valeurs de l'ADC, les met dans des variables globales
* et active des fanions globaux pour indiquer la réception de valeur.
* Si les dernières valeurs converties n'ont pas encore été envoyées,
* on lève le fanion de dépassement (overflow) de l'ADC.
*/
__attribute__((__interrupt__)) static void adc_irq()
{
	// Récupérer le statut de l'ADC
	U32 status = adc->sr;

	// Lorsque les valeurs précédentes n'ont pas éyé envoyées, il faut l'indiquer car c'est un dépassement
	if (light_available || pot_available)
	{
		adc_overflow = true;
	}
	// Le potentiomètre a été reçu : récupérer la valeur du potentiomètre et lever le fanion correspondant
	if (status & ADC_SR_POTENTIOMETER_MASK)
	{	
		// Lire la valeur du registre correspondant au potentiomètre
		pot_val = *((uint32_t *)((&(adc->cdr0)) + ADC_POTENTIOMETER_CHANNEL));
		pot_available = true;
	}
	// Le capteur de lumière a été reçu : récupérer la valeur du capteur et lever le fanion correspondant
	if (status & ADC_SR_LIGHT_MASK)
	{
		// Lire la valeur du registre correspondant au capteur de lumière
		light_val = *((uint32_t *)((&(adc->cdr0)) + ADC_LIGHT_CHANNEL));
		light_available = true;
	}
}

/**
* \fn static void usart_irq()
* \brief Gestionnaire d'interruption pour le USART
* \par Cette interruption est levée lorsqu'un caractère est reçu via l'USART
* et lorsque la transmission de la valeur de la lumière est terminée.
* \par Lorsqu'un caractère est reçu, l'interruption lit le caractère et le met
* dans une variable globale.
* \par Lorsque la transmission de la lumière est terminée, l'interruption
* correspondante est désactivée et un fanion global est activé.
*/
__attribute__((__interrupt__)) static void usart_irq()
{
	// Le fanion de réception est levé : recevoir le caractère
	if (COMM_USART.csr & (AVR32_USART_CSR_RXRDY_MASK))
	{
		// Lire le caractère (cela vide aussi le buffer)
		rx_char = COMM_USART.rhr & AVR32_USART_RHR_RXCHR_MASK;
		char_received = true;
	}
	// L'interruption a été appelée mais le fanion n'est pas levé : terminer la transmission de la lumière
	else
	{
		// Désactiver l'interruption pour la fin de transmission (on ne veut pas qu'elle soit activée pour le transfer du potentiomètre)
		COMM_USART.idr = AVR32_USART_IDR_TXRDY_MASK;
		light_tx_done = true;
	}
}

/**
* \fn static void led_timer_irq() 
* \brief Gestionnaire d'interruption pour le Timer Counter des LEDs
* \par À la fréquence d'oscillement des LEDs, change un fanion indiquant
* que les LEDs doivent être commutées.
*/
__attribute__((__interrupt__)) static void led_timer_irq() 
{
	tc_read_sr(&AVR32_TC, LED_TC_CHANNEL);
	toggle_led = true;
}

/**
* \fn static void button_irq()
* \brief Gestionnaire d'interruption pour le bouton
* \par Lorsque le bouton est appuyé, modifie la fréquence d'échantillonage.
*/
__attribute__((__interrupt__)) static void button_irq()
{
	// Confirmer l'interruption en lisant le IFR correspondant
	AVR32_GPIO.port[GPIO88_PORT].ifrc = GPIO88_BIT_CONTROL;

	// Modifier l'ID de fréquence entre les fréquences 1000 Hz et 2000 Hz
	sampling_rate = (sampling_rate == FREQ_1) ? FREQ_2 : FREQ_1;

	// Mettre à jour le Timer Counter pour utiliser la bonne fréquence de conversion ADC
	AVR32_TC.channel[ADC_TC_CHANNEL].rc = ((FPBA / 32) / sampling_rate) & AVR32_TC_RC_MASK;
}

/**
* \fn static void adc_timer_irq()
* \brief Gestionnaire d'interruption pour le timer du analog-digital converter
* \par Lorsque le Timer Counter du ADC est appelé, selon la fréquence
* déterminée entre 1000 Hz et 2000 Hz, l'ADC est sollicité
* (ce qui génèrera éventuellement une interruption ADC).
*/
__attribute__((__interrupt__)) static void adc_timer_irq()
{
	// Lire la valeur du TC
	tc_read_sr(&AVR32_TC, ADC_TC_CHANNEL);

	// Commencer la conversion ADC
	AVR32_ADC.cr = AVR32_ADC_START_MASK;
}

/**
* \fn void tc_init()
* \brief Initialisation du timer counter
*/
static void tc_init()
{
	// Configuation de forme d'onde pour le TC du ADC
	static const tc_waveform_opt_t ADC_WAVEFORM_OPT =
	{
		.channel  = ADC_TC_CHANNEL,                    // Channel selection.

		.bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
		.beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
		.bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
		.bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

		.aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
		.aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
		.acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
		.acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
		.enetrg   = false,                             // External event trigger enable.
		.eevt     = 0,                                 // External event selection.
		.eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
		.cpcdis   = false,                             // Counter disable when RC compare.
		.cpcstop  = false,                             // Counter clock stopped with RC compare.

		.burst    = false,                             // Burst signal selection.
		.clki     = false,                             // Clock inversion.
		.tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 3, connected to fPBA / 8.
	};

	// Configuration de forme d'onde pour le TC de LEDs
	static const tc_waveform_opt_t LED_WAVEFORM_OPT =
	{
		.channel  = LED_TC_CHANNEL,                    // Channel selection.

		.bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
		.beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
		.bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
		.bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

		.aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
		.aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
		.acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
		.acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
		.enetrg   = false,                             // External event trigger enable.
		.eevt     = 0,                                 // External event selection.
		.eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
		.cpcdis   = false,                             // Counter disable when RC compare.
		.cpcstop  = false,                             // Counter clock stopped with RC compare.

		.burst    = false,                             // Burst signal selection.
		.clki     = false,                             // Clock inversion.
		.tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 3, connected to fPBA / 8.
	};

	// Activer les interruptions des deux TC
	INTC_register_interrupt(&adc_timer_irq, AVR32_TC_IRQ0, AVR32_INTC_INT0);
	INTC_register_interrupt(&led_timer_irq, AVR32_TC_IRQ1, AVR32_INTC_INT1);

	// Initialiser le TC du ADC
	tc_init_waveform(&AVR32_TC, &ADC_WAVEFORM_OPT);
	tc_write_rc(&AVR32_TC, ADC_TC_CHANNEL, (FPBA / 32) / sampling_rate);
	tc_configure_interrupts(&AVR32_TC, ADC_TC_CHANNEL, &TIMER_COUNTER_INTERRUPT);

	// Désactiver l'interruption RC Compare
	AVR32_TC.channel[ADC_TC_CHANNEL].idr = AVR32_TC_IDR0_CPCS_MASK;

	// Initialiser le TC des LEDs
	tc_init_waveform(&AVR32_TC, &LED_WAVEFORM_OPT);
	tc_write_rc(&AVR32_TC, LED_TC_CHANNEL, (FPBA / 32) / LED_FREQ);
	tc_configure_interrupts(&AVR32_TC, LED_TC_CHANNEL, &TIMER_COUNTER_INTERRUPT);

	// Démarrer les TC
	tc_start(&AVR32_TC, ADC_TC_CHANNEL);
	tc_start(&AVR32_TC, LED_TC_CHANNEL);
}

/**
* \fn void adc_init()
* \brief Initialisation du analog-digital converter
*/
static void adc_init()
{
	INTC_register_interrupt(&adc_irq, AVR32_ADC_IRQ, AVR32_INTC_INT0);
	adc_configure(adc);

	// Activer les canaux ADC du potentiomètre et du capteur de lumière
	// Note : Le fait d'activer les deux canaux fait en sorte que la valeur de la lumière est un peu modifiée selon la valeur du potentiomètre...
	adc_enable(adc, ADC_POTENTIOMETER_CHANNEL);
	adc_enable(adc, ADC_LIGHT_CHANNEL);

	// Activer les interruptions ADC en fin de conversion pour le canal du potentiomètre et du capteur de lumière
	AVR32_ADC.ier = ADC_IER_POTENTIOMETER_MASK | ADC_IER_LIGHT_MASK;
}

/**
* \fn void usart_init()
* \brief Initialisation du USART 0
*/
static void usart_init() 
{
	static const usart_options_t USART_OPTION =
	{
		.baudrate = USART_BAUDRATE,
		.channelmode = USART_NORMAL_CHMODE,
		.charlength = USART_CHARLENGTH,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT
	};

	INTC_register_interrupt(&usart_irq, AVR32_USART0_IRQ, AVR32_INTC_INT0);

	gpio_enable_module(GPIO_MAP, sizeof(GPIO_MAP) / sizeof(GPIO_MAP[0]));
	usart_init_rs232(&COMM_USART, &USART_OPTION, FPBA);
	COMM_USART.ier = AVR32_USART_IER_RXRDY_MASK;
}

/**
* \fn void gpio_init()
* \brief Initialisation du bouton et LEDs (GPIO)
*/
static void gpio_init()
{
	// Activer les interruptions pour le bouton
	INTC_register_interrupt(&button_irq, AVR32_GPIO_IRQ_11, AVR32_INTC_INT1);
	
	// http://www.atmel.com/images/doc32078.pdf#page=10
	// Initialisation GPIO88
	AVR32_GPIO.port[GPIO88_PORT].gfers = GPIO88_BIT_CONTROL;
	
	// Configuration du détecteur de bord du GPIO88
	AVR32_GPIO.port[GPIO88_PORT].imr0s = GPIO88_BIT_CONTROL;
	AVR32_GPIO.port[GPIO88_PORT].imr1c = GPIO88_BIT_CONTROL;

	// Activer les interruptions sur GPIO88
	AVR32_GPIO.port[GPIO88_PORT].iers = GPIO88_BIT_CONTROL;

	// Initialiser tous les LEDs à fermé
	LED_Display(0);
}

// Événements à réaliser suite à des interrupts

/**
 * \fn void toggle_leds()
 * \brief Faire clignoter les LEDs lorsque nécessaire.
 */
inline void toggle_leds()
{
	toggle_led = false;

	LED_Toggle(LED0);

	if (running)
	{
		LED_Toggle(LED1);
	}

	if (adc_overflow)
	{
		adc_overflow = false;
		LED_On(LED2);
	}
}
/**
 * \fn get_received_char()
 * \brief Traiter le caractère reçu du USART
 * \par Traite le caractère reçu sur USART (appelé après une interruption).
 * Traite le départ (s) et la fin (x) de transmission.
 */
inline void get_received_char()
{
	char_received = false;

	if (rx_char == CHAR_START_ACQUISITION)
	{
		running = true;
		LED_Off(LED0);
		LED_Off(LED1);
		
		// CLKEN – Enables the clock if set. SWTRG – Asserts a software trigger if set.
		AVR32_TC.channel[ADC_TC_CHANNEL].ccr = AVR32_TC_SWTRG_MASK | AVR32_TC_CLKEN_MASK;

		// Enable RC Compare Interrupt
		AVR32_TC.channel[ADC_TC_CHANNEL].ier = AVR32_TC_IER0_CPCS_MASK;
	}
	else 
	{
	if (rx_char == CHAR_STOP_ACQUISITION)
	{
		running = false;
		LED_Off(LED1);

		// CPCS – Disable RC Compare Interrupt
		AVR32_TC.channel[ADC_TC_CHANNEL].ccr = AVR32_TC_CLKDIS_MASK;

		//CLDIS – Disables the clock if set.
		AVR32_TC.channel[ADC_TC_CHANNEL].idr = AVR32_TC_IDR0_CPCS_MASK;
	}
	}
}

/**
 * \fn void send_pot_val()
 * \brief Envoie la valeur du potentiomètre sur le lien USART
 */
inline void send_pot_val()
{
	pot_available = false;
	COMM_USART.thr = ((pot_val >> 2) & 0xFE) & AVR32_USART_THR_TXCHR_MASK;
}

/**
 * \fn void send_light_val()
 * \brief Envoie la valeur du capteur de lumière sur le lien USART
 * \par Envoie la valeur de la lumière lorsque l'USART est prêt pour la transmission.
 * Sinon, allume le quatrième LED pour indiquer le problème
 */
inline void send_light_val()
{
	light_available = false;

	// L'USART est prêt à transmettre
	if (COMM_USART.csr & AVR32_USART_CSR_TXRDY_MASK)
	{
		COMM_USART.thr = ((light_val >> 2) | 0x1) & AVR32_USART_THR_TXCHR_MASK;
		COMM_USART.ier = AVR32_USART_IER_TXRDY_MASK;
	}
	// L'USART n'est pas prêt à transmettre : ça ne devrait pas arriver en opération normale, erreur sur LED 4
	else
	{
		LED_On(LED3);
	}
}
