#ifndef _USER_H_
#define _USER_H_

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define led _LATA2
#define PIN_LAISSE _RC3         // 5V num 4
#define PIN_TEAM _RA4
#define TRIS_TEAM _TRISA4 
// ultrason : en RC4 // defini dans ultrason .h
#define BOUTON_COULEUR _RA9

/******************************************************************************/
/* System Level #define Macros                                                */
/******************************************************************************/

/* Microcontroller MIPs (FCY) */
#define SYS_FREQ        80000000UL
#define FCY             SYS_FREQ/2

#define BAUDRATEAX12 57600

//#define TEST_ALEX_UART_SPEED

#ifdef TEST_ALEX_UART_SPEED
    #define BRGVALAX12 ((FCY / BAUDRATEAX12 / 4) - 1)
#else
    #define BRGVALAX12 ((FCY / BAUDRATEAX12 / 16) - 1)
#endif



 

/******************************************************************************/
/* System Function Prototypes                                                 */
/******************************************************************************/

/* Custom oscillator configuration funtions, reset source evaluation
functions, and other non-peripheral microcontroller initialization functions
go here. */

void ConfigureOscillator(void); /* Handles clock switching/osc initialization */
void InitTimers();
void Init_CN(void);
void writeStringToUART (const char* msg);
// initialize all things
void Init_All(int);

#endif // _USER_H_
