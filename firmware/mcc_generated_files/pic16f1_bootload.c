/*
    (c) 2019 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

//
//
//
// Memory Map
//   -----------------
//   |    0x0000     |   Reset vector
//   |               |
//   |    0x0004     |   Interrupt vector
//   |               |
//   |               |
//   |  Boot Block   |   (this program)
//   |               |
//   |    0x0300     |   Re-mapped Reset Vector
//   |    0x0304     |   Re-mapped High Priority Interrupt Vector
//   |               |
//   |       |       |
//   |               |
//   |  Code Space   |   User program space
//   |               |
//   |       |       |
//   |               |
//   |    0x3FFF     |
//   -----------------
//
//
//
// Definitions:
//
//   STX     -   Start of packet indicator
//   DATA    -   General data up to 255 bytes
//   COMMAND -   Base command
//   DLEN    -   Length of data associated to the command
//   ADDR    -   Address up to 24 bits
//   DATA    -   Data (if any)
//
//
// Commands:
//
//   RD_VER      0x00    Read Version Information
//   RD_MEM      0x01    Read Program Memory
//   WR_MEM      0x02    Write Program Memory
//   ER_MEM      0x03    Erase Program Memory (NOT supported by PIC16)
//   RD_EE       0x04    Read EEDATA Memory
//   WR_EE       0x05    Write EEDATA Memory
//   RD_CONFIG   0x06    Read Config Memory (NOT supported by PIC16)
//   WT_CONFIG   0x07    Write Config Memory (NOT supported by PIC16)
//   CHECKSUM    0x08    Calculate 16 bit checksum of specified region of memory
//   RESET       0x09    Reset Device and run application
//
// *****************************************************************************

#define  READ_VERSION   0
#define  READ_FLASH     1
#define  WRITE_FLASH    2
#define  ERASE_FLASH    3
#define  READ_EE_DATA   4
#define  WRITE_EE_DATA  5
#define  READ_CONFIG    6
#define  WRITE_CONFIG   7
#define  CALC_CHECKSUM  8
#define  RESET_DEVICE   9
#define  CALC_CRC       10



// *****************************************************************************
    #include "xc.h"       // Standard include
    #include <stdint.h>
    #include <stdbool.h>
    #include "bootload.h"
    #include "mcc.h"


// *****************************************************************************
void     Get_Buffer (void);     // generic comms layer
uint8_t  Get_Version_Data(void);
uint8_t  Read_Flash(void);
uint8_t  Write_Flash(void);
uint8_t  Erase_Flash(void);
uint8_t  Read_EE_Data(void);
uint8_t  Write_EE_Data(void);
uint8_t  Read_Config(void);
uint8_t  Write_Config(void);
uint8_t  Calc_Checksum(void);
void     StartWrite(void);
void     BOOTLOADER_Initialize(void);
void     Run_Bootloader(void);
bool     Bootload_Required (void);

// *****************************************************************************
#define	MINOR_VERSION   0x08       // Version
#define	MAJOR_VERSION   0x00
//#define STX             			 0x55       // Actually code 0x55 is 'U'  But this is what the autobaud feature of the PIC16F1 EUSART is looking for
#define ERROR_ADDRESS_OUT_OF_RANGE   0xFE
#define ERROR_INVALID_COMMAND        0xFF
#define COMMAND_SUCCESS              0x01

// To be device independent, these are set by mcc in memory.h
#define  LAST_WORD_MASK          (WRITE_FLASH_BLOCKSIZE - 1)
#define  NEW_RESET_VECTOR        0x300
#define  NEW_INTERRUPT_VECTOR    0x304

#define _str(x)  #x
#define str(x)  _str(x)

// *****************************************************************************


// *****************************************************************************
    uint16_t check_sum;    // Checksum accumulator
    uint16_t counter;      // General counter
    uint8_t data_length;
    uint8_t rx_data;
    uint8_t tx_data;
    bool reset_pending = false;

// Force variables into Unbanked for 1-cycle accessibility 
    uint8_t EE_Key_1    __at(0x70);
    uint8_t EE_Key_2    __at(0x71);


frame_t  frame;

// *****************************************************************************
// The bootloader code does not use any interrupts.
// However, the application code may use interrupts.
// The interrupt vector on a PIC16F is located at
// address 0x0004. 
// The following function will be located
// at the interrupt vector and will contain a jump to
// the new application interrupt vector

    asm ("psect  intentry,global,class=CODE,delta=2");
    asm ("pagesel " str(NEW_INTERRUPT_VECTOR));
    asm ("GOTO " str(NEW_INTERRUPT_VECTOR));

void BOOTLOADER_Initialize ()
{
    EE_Key_1 = 0;
    EE_Key_2 = 0;
#ifdef BOOTLOADER_INDICATOR_ENABLED
	BOOTLOADER_INDICATOR = BL_INDICATOR_ON;
#endif
    if (Bootload_Required () == true)
    {
        Run_Bootloader ();     // generic comms layer
    }
    STKPTR = 0x1F;
    BSR = 0;
#ifdef BOOTLOADER_INDICATOR_ENABLED
    BOOTLOADER_INDICATOR = BL_INDICATOR_OFF;
#endif
    asm ("pagesel " str(NEW_RESET_VECTOR));
    asm ("goto  "  str(NEW_RESET_VECTOR));
}

// *****************************************************************************
bool Bootload_Required ()
{
// ******************************************************************
//  Check an IO pin to force entry into bootloader
// ******************************************************************

//#info  "You may need to add additional delay here between enabling weak pullups and testing the pin."
    for (uint8_t i = 0; i != 0xFF; i++) NOP();

    if (IO_PIN_ENTRY_PORT_PIN == IO_PIN_ENTRY_RUN_BL)
    {
        return (true);
    }

    uint16_t  Stored_Checksum;

    frame.address_L = (uint8_t) (NEW_RESET_VECTOR & 0x00FF);
    frame.address_H = (uint8_t) ((NEW_RESET_VECTOR & 0xFF00) >> 8);
    frame.data_length = (END_FLASH - NEW_RESET_VECTOR - 2) << 1;
    Calc_Checksum ();
    NVMADRL = (END_FLASH - 2) & 0xFF;
    NVMADRH = ((END_FLASH - 2) & 0xFF00) >> 8;
    NVMCON1bits.RD = 1;
    NOP();
    NOP();
    Stored_Checksum = NVMDATL;
    if ((++ NVMADRL) == 0x00)
    {
        ++ NVMADRH;
    }
    NVMCON1bits.RD = 1;
    NOP();
    NOP();
    Stored_Checksum += (((uint16_t)NVMDATL) << 8);

    if (Stored_Checksum != check_sum)
    {
        return(true);
    }

    return (false);
}


// *****************************************************************************
uint8_t  ProcessBootBuffer()
{
    uint8_t   len;

    EE_Key_1 = frame.EE_key_1;
    EE_Key_2 = frame.EE_key_2;

// ***********************************************
// Test the command field and sub-command.
    switch (frame.command)
    {
    case    READ_VERSION:
        len = Get_Version_Data();
        break;
    case    READ_FLASH:
        len = Read_Flash();
        break;
    case    WRITE_FLASH:
        len = Write_Flash();
        break;
    case    ERASE_FLASH:
        len = Erase_Flash();
        break;
    case    READ_CONFIG:
        len = Read_Config();
        break;
    case    CALC_CHECKSUM:
        len = Calc_Checksum();
        break;
    case    RESET_DEVICE:
        frame.data[0] = COMMAND_SUCCESS;
        reset_pending = true;
        len = 10;
        break;
    default:
        frame.data[0] = ERROR_INVALID_COMMAND;
        len = 10;
    }
    return (len);
}

// **************************************************************************************
// Commands
//
//        Cmd     Length----------------   Address---------------
// In:   [<0x00> <0x00><0x00><0x00><0x00> <0x00><0x00><0x00><0x00>]
// OUT:  [<0x00> <0x00><0x00><0x00><0x00> <0x00><0x00><0x00><0x00> <VERL><VERH>]
uint8_t  Get_Version_Data()
{
    frame.data[0] = MINOR_VERSION;
    frame.data[1] = MAJOR_VERSION;
    frame.data[2] = 0;       // Max packet size (256)
    frame.data[3] = 1;
    frame.data[4] = 0;
    frame.data[5] = 0;
    NVMADRL = 0x06;               // Get device ID
    NVMADRH = 0x00;
    NVMCON1bits.NVMREGS = 1;
    NVMCON1bits.RD = 1;
    NOP();
    NOP();
    frame.data[6] = NVMDATL;
    frame.data[7] = NVMDATH;
    frame.data[8] = 0;
    frame.data[9] = 0;

    frame.data[10] = ERASE_FLASH_BLOCKSIZE;
    frame.data[11] = WRITE_FLASH_BLOCKSIZE;

    NVMADRL = 0x00;
    NVMADRH = 0x00;
    NVMCON1bits.NVMREGS = 1;
    for (uint8_t  i= 12; i < 16; i++)
    {
        NVMCON1bits.RD = 1;
        NOP();
        NOP();
        frame.data[i] = NVMDATL;
        if ((++ NVMADRL) == 0x00)
        {
            ++ NVMADRH;
        }
    }

    return  25;   // total length to send back 9 byte header + 16 byte payload
}

// **************************************************************************************
// Read Flash
// In:	[<0x01><DLEN><ADDRL><ADDRH><ADDRU>]
// OUT:	[<0x01><DLEN><ADDRL><ADDRH><ADDRU><DATA>...]
uint8_t Read_Flash()
{
    NVMADRL = frame.address_L;
    NVMADRH = frame.address_H;
    NVMCON1 = 0x80;
    for (uint16_t i = 0; i < frame.data_length; i += 2)
    {
        NVMCON1bits.RD = 1;
        NOP();
        NOP();
        frame.data[i]  = NVMDATL;
        frame.data[i+1] = NVMDATH;
        if ((++ NVMADRL) == 0x00)
        {
            ++ NVMADRH;
        }
    }

    return (uint8_t)(frame.data_length + 9);
}

// **************************************************************************************
// Write Flash
// In:	[<0x02><DLENBLOCK><0x55><0xAA><ADDRL><ADDRH><ADDRU><DATA>...]
// OUT:	[<0x02>]
uint8_t Write_Flash()
{
    NVMADRL = frame.address_L;
    NVMADRH = frame.address_H;
    NVMCON1 = 0xA4;       // Setup writes
    for (uint16_t  i= 0; i < frame.data_length; i += 2)
    {
		
        if (((NVMADRL & LAST_WORD_MASK) == LAST_WORD_MASK)
          || (i == frame.data_length - 2))
            NVMCON1bits.LWLO = 0;
        NVMDATL = frame.data[i];
        NVMDATH = frame.data[i+1];

        StartWrite();
        if ((++ NVMADRL) == 0x00)
        {
            ++ NVMADRH;
        }
    }
    frame.data[0] = COMMAND_SUCCESS;
    EE_Key_1 = 0x00;  // erase EE Keys
    EE_Key_2 = 0x00;
    return (10);
}

// **************************************************************************************
// Erase Program Memory
// Erases data_length rows from program memory
uint8_t Erase_Flash ()
{
    NVMADRL = frame.address_L;
    NVMADRH = frame.address_H;
    for (uint16_t i=0; i < frame.data_length; i++)
    {
        if ((NVMADRH & 0x7F) >= ((END_FLASH & 0xFF00) >> 8))
        {
            frame.data[0] = ERROR_ADDRESS_OUT_OF_RANGE;
            return (10);
        }
        NVMCON1 = 0x94;       // Setup writes
        StartWrite();
        if ((NVMADRL += ERASE_FLASH_BLOCKSIZE) == 0x00)
        {
            ++ NVMADRH;
        }
    }
    frame.data[0]  = COMMAND_SUCCESS;
    frame.EE_key_1 = 0x00;  // erase EE Keys
    frame.EE_key_2 = 0x00;
    return (10);
}


// **************************************************************************************
// Read Config Words
// In:	[<0x06><DataLengthL><unused> <unused><unused> <ADDRL><ADDRH><ADDRU><unused>...]
// OUT:	[9 byte header + 4 bytes config1 + config 2]
uint8_t Read_Config ()
{
    NVMADRL = frame.address_L;
    NVMADRH = frame.address_H;
    NVMCON1 = 0x40;      // can these be combined?
    for (uint8_t  i= 0; i < frame.data_length; i += 2)
    {

        NVMCON1bits.RD = 1;
        NOP();
        NOP();
        frame.data[i]   = NVMDATL;
        frame.data[i+1] = NVMDATH;
        ++ NVMADRL;
    }
    return (13);           // 9 byte header + 4 bytes config words
}


// **************************************************************************************
// Calculate Checksum
// In:	[<0x08><DataLengthL><DataLengthH> <unused><unused> <ADDRL><ADDRH><ADDRU><unused>...]
// OUT:	[9 byte header + ChecksumL + ChecksumH]
uint8_t Calc_Checksum()
{
    NVMADRL = frame.address_L;
    NVMADRH = frame.address_H;
    NVMCON1 = 0x80;
    check_sum = 0;
    for (uint16_t i = 0;i < frame.data_length; i += 2)
    {
        NVMCON1bits.RD = 1;
        NOP();
        NOP();
        check_sum += (uint16_t)NVMDATL;
        check_sum += ((uint16_t)NVMDATH) << 8;
        if ((++ NVMADRL) == 0x00)
        {
            ++ NVMADRH;
        }
     }
     frame.data[0] = (uint8_t) (check_sum & 0x00FF);
     frame.data[1] = (uint8_t)((check_sum & 0xFF00) >> 8);
     return (11);
}





// *****************************************************************************
// Unlock and start the write or erase sequence.

void StartWrite()
{
    CLRWDT();
//    NVMCON2 = EE_Key_1;
//    NVMCON2 = EE_Key_2;
//    NVMCON1bits.WR = 1;       // Start the write
// had to switch to assembly - compiler doesn't comprehend no need for bank switch
    asm ("movf " str(_EE_Key_1) ",w");
    asm ("movwf " str(BANKMASK(NVMCON2)));
    asm ("movf  " str(_EE_Key_2) ",w");
    asm ("movwf " str(BANKMASK(NVMCON2)));
    asm ("bsf  "  str(BANKMASK(NVMCON1)) ",1");       // Start the write

    NOP();
    NOP();
    return;
}


// *****************************************************************************
// Check to see if a device reset had been requested.  We can't just reset when
// the reset command is issued.  Instead we have to wait until the acknowledgement
// is finished sending back to the host.  Then we reset the device.
void Check_Device_Reset ()
{
    if (reset_pending == true)
    {

#ifdef BOOTLOADER_INDICATOR_ENABLED
        TRIS_BOOTLOADER_INDICATOR = OUTPUT_PIN;
        BOOTLOADER_INDICATOR = BL_INDICATOR_OFF;
#endif
        RESET();
    }
}




// *****************************************************************************
// *****************************************************************************
