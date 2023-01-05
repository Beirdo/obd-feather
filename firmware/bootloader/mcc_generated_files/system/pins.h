/**
 * Generated Pins header File
 * 
 * @file pins.h
 * 
 * @defgroup  pinsdriver Pins Driver
 * 
 * @brief This is generated driver header for pins. 
 *        This header file provides APIs for all pins selected in the GUI.
 *
 * @version Driver Version  3.0.0
*/

/*
© [2023] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#ifndef PINS_H
#define PINS_H

#include <xc.h>

#define INPUT   1
#define OUTPUT  0

#define HIGH    1
#define LOW     0

#define ANALOG      1
#define DIGITAL     0

#define PULL_UP_ENABLED      1
#define PULL_UP_DISABLED     0

// get/set IO_RA0 aliases
#define BL_INDICATOR_TRIS                 TRISAbits.TRISA0
#define BL_INDICATOR_LAT                  LATAbits.LATA0
#define BL_INDICATOR_PORT                 PORTAbits.RA0
#define BL_INDICATOR_WPU                  WPUAbits.WPUA0
#define BL_INDICATOR_OD                   ODCONAbits.ODCA0
#define BL_INDICATOR_ANS                  ANSELAbits.ANSA0
#define BL_INDICATOR_SetHigh()            do { LATAbits.LATA0 = 1; } while(0)
#define BL_INDICATOR_SetLow()             do { LATAbits.LATA0 = 0; } while(0)
#define BL_INDICATOR_Toggle()             do { LATAbits.LATA0 = ~LATAbits.LATA0; } while(0)
#define BL_INDICATOR_GetValue()           PORTAbits.RA0
#define BL_INDICATOR_SetDigitalInput()    do { TRISAbits.TRISA0 = 1; } while(0)
#define BL_INDICATOR_SetDigitalOutput()   do { TRISAbits.TRISA0 = 0; } while(0)
#define BL_INDICATOR_SetPullup()          do { WPUAbits.WPUA0 = 1; } while(0)
#define BL_INDICATOR_ResetPullup()        do { WPUAbits.WPUA0 = 0; } while(0)
#define BL_INDICATOR_SetPushPull()        do { ODCONAbits.ODCA0 = 0; } while(0)
#define BL_INDICATOR_SetOpenDrain()       do { ODCONAbits.ODCA0 = 1; } while(0)
#define BL_INDICATOR_SetAnalogMode()      do { ANSELAbits.ANSA0 = 1; } while(0)
#define BL_INDICATOR_SetDigitalMode()     do { ANSELAbits.ANSA0 = 0; } while(0)

// get/set IO_RA2 aliases
#define IO_RA2_TRIS                 TRISAbits.TRISA2
#define IO_RA2_LAT                  LATAbits.LATA2
#define IO_RA2_PORT                 PORTAbits.RA2
#define IO_RA2_WPU                  WPUAbits.WPUA2
#define IO_RA2_OD                   ODCONAbits.ODCA2
#define IO_RA2_ANS                  ANSELAbits.ANSA2
#define IO_RA2_SetHigh()            do { LATAbits.LATA2 = 1; } while(0)
#define IO_RA2_SetLow()             do { LATAbits.LATA2 = 0; } while(0)
#define IO_RA2_Toggle()             do { LATAbits.LATA2 = ~LATAbits.LATA2; } while(0)
#define IO_RA2_GetValue()           PORTAbits.RA2
#define IO_RA2_SetDigitalInput()    do { TRISAbits.TRISA2 = 1; } while(0)
#define IO_RA2_SetDigitalOutput()   do { TRISAbits.TRISA2 = 0; } while(0)
#define IO_RA2_SetPullup()          do { WPUAbits.WPUA2 = 1; } while(0)
#define IO_RA2_ResetPullup()        do { WPUAbits.WPUA2 = 0; } while(0)
#define IO_RA2_SetPushPull()        do { ODCONAbits.ODCA2 = 0; } while(0)
#define IO_RA2_SetOpenDrain()       do { ODCONAbits.ODCA2 = 1; } while(0)
#define IO_RA2_SetAnalogMode()      do { ANSELAbits.ANSA2 = 1; } while(0)
#define IO_RA2_SetDigitalMode()     do { ANSELAbits.ANSA2 = 0; } while(0)

// get/set IO_RA4 aliases
#define BL_ENTRY_TRIS                 TRISAbits.TRISA4
#define BL_ENTRY_LAT                  LATAbits.LATA4
#define BL_ENTRY_PORT                 PORTAbits.RA4
#define BL_ENTRY_WPU                  WPUAbits.WPUA4
#define BL_ENTRY_OD                   ODCONAbits.ODCA4
#define BL_ENTRY_ANS                  ANSELAbits.ANSA4
#define BL_ENTRY_SetHigh()            do { LATAbits.LATA4 = 1; } while(0)
#define BL_ENTRY_SetLow()             do { LATAbits.LATA4 = 0; } while(0)
#define BL_ENTRY_Toggle()             do { LATAbits.LATA4 = ~LATAbits.LATA4; } while(0)
#define BL_ENTRY_GetValue()           PORTAbits.RA4
#define BL_ENTRY_SetDigitalInput()    do { TRISAbits.TRISA4 = 1; } while(0)
#define BL_ENTRY_SetDigitalOutput()   do { TRISAbits.TRISA4 = 0; } while(0)
#define BL_ENTRY_SetPullup()          do { WPUAbits.WPUA4 = 1; } while(0)
#define BL_ENTRY_ResetPullup()        do { WPUAbits.WPUA4 = 0; } while(0)
#define BL_ENTRY_SetPushPull()        do { ODCONAbits.ODCA4 = 0; } while(0)
#define BL_ENTRY_SetOpenDrain()       do { ODCONAbits.ODCA4 = 1; } while(0)
#define BL_ENTRY_SetAnalogMode()      do { ANSELAbits.ANSA4 = 1; } while(0)
#define BL_ENTRY_SetDigitalMode()     do { ANSELAbits.ANSA4 = 0; } while(0)

// get/set IO_RC2 aliases
#define IO_RC2_TRIS                 TRISCbits.TRISC2
#define IO_RC2_LAT                  LATCbits.LATC2
#define IO_RC2_PORT                 PORTCbits.RC2
#define IO_RC2_WPU                  WPUCbits.WPUC2
#define IO_RC2_OD                   ODCONCbits.ODCC2
#define IO_RC2_ANS                  ANSELCbits.ANSC2
#define IO_RC2_SetHigh()            do { LATCbits.LATC2 = 1; } while(0)
#define IO_RC2_SetLow()             do { LATCbits.LATC2 = 0; } while(0)
#define IO_RC2_Toggle()             do { LATCbits.LATC2 = ~LATCbits.LATC2; } while(0)
#define IO_RC2_GetValue()           PORTCbits.RC2
#define IO_RC2_SetDigitalInput()    do { TRISCbits.TRISC2 = 1; } while(0)
#define IO_RC2_SetDigitalOutput()   do { TRISCbits.TRISC2 = 0; } while(0)
#define IO_RC2_SetPullup()          do { WPUCbits.WPUC2 = 1; } while(0)
#define IO_RC2_ResetPullup()        do { WPUCbits.WPUC2 = 0; } while(0)
#define IO_RC2_SetPushPull()        do { ODCONCbits.ODCC2 = 0; } while(0)
#define IO_RC2_SetOpenDrain()       do { ODCONCbits.ODCC2 = 1; } while(0)
#define IO_RC2_SetAnalogMode()      do { ANSELCbits.ANSC2 = 1; } while(0)
#define IO_RC2_SetDigitalMode()     do { ANSELCbits.ANSC2 = 0; } while(0)

/**
 * @ingroup  pinsdriver
 * @brief GPIO and peripheral I/O initialization
 * @param none
 * @return none
 */
void PIN_MANAGER_Initialize (void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt on Change Handling routine
 * @param none
 * @return none
 */
void PIN_MANAGER_IOC(void);


#endif // PINS_H
/**
 End of File
*/