#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>
#include <CircularBuffer.h>

#include "project.h"
#include "j1850_io.h"

enum {
  I2C_REG_INT_ENABLES,
  I2C_REG_INT_FLAGS,
  I2C_REG_DATA_FIFO,
  I2C_REG_COUNT,
};

enum {
  IRQ_RX_PROTOCOL_ERROR,
  IRQ_RX_PACKETS_AVAILABLE,
  IRQ_TX_OVERFLOW,
  IRQ_TX_ACTIVE,
  IRQ_TX_SPACE_AVAILABLE,  
};

uint8_t reg_address;
volatile uint8_t registerBank[I2C_REG_COUNT];

CircularBuffer<uint8_t, 128> rxRingBuf;
int rxPacketCount = 0;

CircularBuffer<uint8_t, 128> txRingBuf;

void setOpenDrainOutput(uint8_t pin, bool value, bool invert = false);
void i2c_request_event(void);
void i2c_receive_event(int count);
void reset_isr(void);
void setOutboundInterrupt(void);


void reset_isr(void)
{
  void (* reboot)(void) = 0;
  reboot();
}

void setOpenDrainOutput(uint8_t pin, bool value, bool invert)
{
  if (invert) {
    value = !value;
  }

  if (!value) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else {
    pinMode(pin, INPUT_PULLUP);
    digitalWrite(pin, HIGH);
  }
}

void setOutboundInterrupt(void)
{
  uint8_t flags = registerBank[I2C_REG_INT_FLAGS];
  uint8_t mask  = registerBank[I2C_REG_INT_ENABLES];

  flags |= (rxPacketCount ? BIT(IRQ_RX_PACKETS_AVAILABLE) : 0);
  flags |= (txRingBuf.available() / MAX_PACKET_SIZE ? BIT(IRQ_TX_SPACE_AVAILABLE) : 0);

  registerBank[I2C_REG_INT_FLAGS] = flags;
  flags &= mask;

  digitalWrite(PIN_J1850_INT, !flags);
}


void setup() {
  pinMode(PIN_AVR_LED, OUTPUT);
  digitalWrite(PIN_AVR_LED, HIGH);
  
  pinMode(PIN_J1850_LED, OUTPUT);
  digitalWrite(PIN_J1850_LED, HIGH);

  pinMode(PIN_RESET, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_RESET), reset_isr, FALLING);

  pinMode(PIN_J1850_TX, OUTPUT);
  digitalWrite(PIN_J1850_TX, LOW);

  pinMode(PIN_J1850_RX, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_J1850_RX), j1850_rx_isr, CHANGE);

  pinMode(PIN_J1850_INT, OUTPUT);
  digitalWrite(PIN_J1850_INT, HIGH);

  pinMode(PIN_SAE_PWM, INPUT);

  memset((void *)registerBank, 0x00, sizeof(registerBank));
  
  setOutboundInterrupt();

  Wire.begin(I2C_SLAVE_ADDR);
  Serial.begin(115200);

  j1850io.init();
}

void loop() {
  static uint16_t ledCounter = 0;
  int topOfLoop = micros();

  bool ledOn = ((ledCounter++ & 0x07) == 0x01);
  digitalWrite(PIN_AVR_LED, !ledOn);

  int delayUs = j1850io.process();

  int elapsed = micros() - topOfLoop;
  delayUs = clamp<int>(delayUs - elapsed, 1, 100);
  delayMicroseconds(delayUs);
}


void i2c_request_event(void)
{
  if (reg_address >= I2C_REG_COUNT) {
    return;
  }

  // reads - we always will send an entire packet, if this is for the fifo, otherwise, one byte
  if (reg_address != I2C_REG_DATA_FIFO) {
    Wire.write(registerBank[reg_address]);
    return;
  }

  uint8_t buf[MAX_PACKET_SIZE];
  uint8_t len;
  bool in_frame = false;
  bool in_escape = false;

  for (len = 0; !rxRingBuf.isEmpty() && len < MAX_PACKET_SIZE;) {
    uint8_t ch = rxRingBuf.shift();
    if (!in_frame) {
      if (ch == CH_SOF) {
        in_frame = true;
      }
      // toss everything between SOF characters
      continue;
    }

    if (ch == CH_SOF) {
      // And we are done.
      in_frame = false;
      break;      
    }

    if (in_escape) {
      ch = ch ^ 0x80;
      if (ch & 0x80) {
        // this is buggered.  never mind!  Abort!
        break;        
      }
      in_escape = false;
    } else {
      if (ch == CH_ESC) {
        in_escape = true;
        continue;
      }                   
    }

    buf[len++] = ch;
  }

  if (in_frame || in_escape) {
    registerBank[I2C_REG_INT_FLAGS] |= BIT(IRQ_RX_PROTOCOL_ERROR);
  }
  Wire.write(buf, len);
  rxPacketCount = clamp<int>(rxPacketCount - 1, 0, rxRingBuf.capacity / MAX_PACKET_SIZE);
  setOutboundInterrupt();
}

void i2c_receive_event(uint8_t *buf, int count)
{
  if (count < 1) {
    return;
  }

  // writes - we always will send the register number 
  reg_address = buf[0];
  if (count == 1) {
    return;
  }
  
  count = clamp<int>(count - 1, 0, MAX_PACKET_SIZE);
  buf++;

  switch (reg_address) {
    case I2C_REG_INT_ENABLES:
      registerBank[reg_address] = *buf;
      break;

    case I2C_REG_INT_FLAGS:   
      {
        uint8_t ch = *buf;
        uint8_t mask = ch & (BIT(IRQ_RX_PROTOCOL_ERROR) | BIT(IRQ_TX_OVERFLOW));
        ch &= ~mask;
        registerBank[reg_address] = ch;
      }
      break;

    case I2C_REG_DATA_FIFO:
      {
        bool failure = false;
        failure |= !txRingBuf.push(CH_SOF);

        for (int i = 0; i < count; i++, buf++) {
          uint8_t ch = *(buf++);
          if (ch == CH_SOF || ch == CH_ESC) {
            failure |= txRingBuf.push(CH_ESC);
            failure |= txRingBuf.push(ch ^ 0x80);
          } else {
            failure |= txRingBuf.push(ch);
          }
        }

        failure |= txRingBuf.push(CH_SOF);

        if (failure) {
          registerBank[I2C_REG_INT_FLAGS] |= BIT(IRQ_TX_OVERFLOW);
        }
      }
      break;    

    default:
      break;
  }

  setOutboundInterrupt();
}
