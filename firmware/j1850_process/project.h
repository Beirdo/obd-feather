#ifndef __project_h_
#define __project_h_

#include <Arduino.h>
#include <CircularBuffer.h>

#define PIN_UPDI	      PIN_PA0
#define PIN_J1850_TX    PIN_PA1
#define PIN_J1850_RX    PIN_PA2
#define PIN_J1850_INT   PIN_PA3
#define PIN_RESET       PIN_PA4
#define PIN_SAE_PWM     PIN_PA5
#define PIN_AVR_LED     PIN_PA6
#define PIN_J1850_LED   PIN_PA7
#define PIN_SCL         PIN_PB0
#define PIN_SDA         PIN_PB1
#define PIN_AVR_TX      PIN_PB2
#define PIN_AVR_RX      PIN_PB3

#define I2C_SLAVE_ADDR  0x79

#define MAX_PACKET_SIZE 12          // defined in the SAE J1850 spec
#define CH_SOF          ((uint8_t)0x7E)
#define CH_ESC          ((uint8_t)0x7D)

#define HI_BYTE(x)     ((uint8_t)(((int)(x) >> 8) & 0xFF))
#define LO_BYTE(x)     ((uint8_t)(((int)(x) & 0xFF)))

#define HI_NIBBLE(x)  ((uint8_t)(((int)(x) >> 4) & 0x0F))
#define LO_NIBBLE(x)  ((uint8_t)(((int)(x) & 0x0F)))

#define BIT(x)        (1 << (x))


extern CircularBuffer<uint8_t, 128> rxRingBuf;
extern CircularBuffer<uint8_t, 128> txRingBuf;


template <typename T>
inline T clamp(T value, T minval, T maxval)
{
  return max(min(value, maxval), minval);
}

template <typename T>
inline T map(T x, T in_min, T in_max, T out_min, T out_max)
{
  // the perfect map fonction, with constraining and float handling
  x = clamp<T>(x, in_min, in_max);
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


#endif
