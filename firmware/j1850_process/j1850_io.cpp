#include <Arduino.h>
#include <cppQueue.h>
#include <assert.h>
#include <avr/io.h>

#include "project.h"
#include "j1850_io.h"

typedef struct {
  uint32_t timestamp;  
  uint32_t duration_us;
  bool value;
} rx_timing_t;

static const timing_t vpw_timing[INDEX_COUNT] = {
  {-1, -1, -1, -1, -1},         // SOF_0
  {200, 182, 218, 163, 239},    // SOF_1
  {200, 182, 218, 163, 239},    // EOD
  {280, 261, -1, 239, -1},      // EOF
  {-1, -1, -1, -1, -1},         // BRK_0
  {300, 280, 5000, 238, 32767}, // BRK_1 (actually <= 1.0s for rx_max)
  {300, 280, -1, 280, -1},      // IFS
  {64, 49, 79, 34, 96},         // INACT_0
  {128, 112, 145, 96, 163},     // INACT_1
  {128, 112, 145, 96, 163},     // ACT_0
  {64, 49, 79, 34, 96},         // ACT_1
  {-1, -1, -1, -1, -1},         // ABORT
};

static const timing_t pwm_timing[INDEX_COUNT] = {
  {17, 18, 19, 11, 20},     // SOF_0
  {31, 29, 32, 27, 34},     // SOF_1
  {48, 47, 49, 42, 54},     // EOD (tp4)
  {72, 70, -1, 63, -1},     // EOF
  {9, 8, 10, 3, 21},        // BRK_0
  {39, 37, 41, 35, 43},     // BRK_1
  {96, 93, -1, 84, -1},     // IFS
  {8, 6, 8, 4, 10},         // INACT_0
  {16, 14, 16, 12, 18},     // INACT_1
  {16, 14, 16, 12, 18},     // ACT_0
  {8, 6, 8, 4, 10},         // ACT_1
  {-1, -1, -1, -1, -1},     // ABORT
};

uint32_t last_edge_us;
cppQueue rx_timing_q(sizeof(rx_timing_t), 16, FIFO);

J1850IO j1850io(PIN_J1850_RX, PIN_J1850_TX);

bool isInTiming(const timing_t *timings, uint8_t index, uint32_t duration);

void J1850IO::init(void) 
{
  // Not sure if anything needs to be done here, but let's set the mode for now
  setMode(digitalRead(PIN_SAE_PWM ? TYPE_PWM : TYPE_VPW));

  // bus is idle, so we are listening.
  last_edge_us = 0;
  _transmitting = false;
  _receiving = false;
  _last_activity = 0;
  digitalWrite(PIN_J1850_TX, LOW); 
}

int J1850IO::process(void)
{
  if (_transmitting) {
    // we are transmitting, so carry on where we were.
    return processTX();
  } else {
    // we are receiving.  Wait for the next pulse, and act accordingly
    return processRX();
  }
}

void J1850IO::kickTX(void)
{
  if (!isIdle()) {
    return;
  }

  _index = _index == INDEX_BRK_1 ? INDEX_BRK_1 : get_next_tx_index(true);
  _transmitting = true;
  _receiving = false;
  _tx_bit_count = 0;
  _last_level = digitalRead(PIN_J1850_RX);
}

uint8_t J1850IO::get_next_tx_index(bool new_packet)
{
  uint8_t ch;

  if (_tx_bit_count == 0) {
    if (txRingBuf.isEmpty()) {
      return INDEX_EOD;
    }
    
    ch = txRingBuf.shift();
    if (ch == CH_SOF) {
      return new_packet ? INDEX_SOF_1 : INDEX_EOD;
    }

    if (ch == CH_ESC) {
      if (txRingBuf.isEmpty()) {
        return INDEX_BRK_1;
      }
      ch = txRingBuf.shift() ^ 0x80;
      if (ch == CH_ABORT) {
        return INDEX_BRK_1;        
      }      
    }

    _tx_shift_reg = ch;
    _tx_bit_count = 8;
  }

  bool next_bit = _tx_shift_reg & 0x80;
  _tx_bit_count--;
  _tx_shift_reg <<= 1;  
  _tx_shift_reg = (uint8_t)_tx_shift_reg;

  if (_mode == TYPE_VPW && _last_level) {
    // Next bit is passive (VPW)
    return next_bit ? INDEX_INACT_1 : INDEX_INACT_0;
  } else {
    // Next bit is active (VPW) or it's PWM
    return next_bit ? INDEX_ACT_1 : INDEX_ACT_0;
  }
}

int J1850IO::processTX(void)
{
  _index = clamp<uint8_t>(_index, 0, INDEX_COUNT - 1);
    
  while (true) {
    const timing_t *timing = (_mode == TYPE_PWM) ? &pwm_timing[_index] : &vpw_timing[_index];
    switch (_index) {
      case INDEX_SOF_1:
        _last_level = true;
        _index = INDEX_SOF_0;
        break;        

      case INDEX_SOF_0:
        if (_mode == TYPE_VPW) {
          _index = get_next_tx_index();
          continue;          
        }

        _last_level = false;
        _index = get_next_tx_index();
        break;

      case INDEX_ACT_0:
        _last_level = true;
        _index = _mode == TYPE_PWM ? INDEX_INACT_0 : get_next_tx_index();
        break;

      case INDEX_INACT_0:
        _last_level = false;
        _index = get_next_tx_index();
        break;
      
      case INDEX_ACT_1:
        _last_level = true;
        _index = _mode == TYPE_PWM ? INDEX_INACT_1 : get_next_tx_index();
        break;

      case INDEX_INACT_1:
        _last_level = false;
        _index = get_next_tx_index();
        break;

      case INDEX_BRK_0:
        _index = INDEX_EOD;
        _last_level = false;
        break;

      case INDEX_BRK_1:
        _last_level = true;
        _index = _mode == TYPE_VPW ? INDEX_EOD : INDEX_BRK_0;
        break;
      
      case INDEX_EOD:
        _last_level = false;
        _transmitting = false;
        _rx_bit_count = 0;
        _index = INDEX_SOF_1;        
        break;
    }

    if (_last_level) {
      // set us up for measuring from last rising edge (PWM or VPW)      
      last_edge_us = micros();
    }
    digitalWrite(PIN_J1850_TX, _last_level);
    return timing->nom;
  }

  // should never get here  
  return -1;
}

int J1850IO::processRX(void)
{
  int delayUs = -1;
  rx_timing_t timing;

  while (!rx_timing_q.isEmpty()) {
    _receiving = true;    
    rx_timing_q.pop(&timing);

    if (_index == INDEX_SOF_1) {
      rxRingBuf.push(CH_SOF);
      _index = INDEX_ACT_0;
    }

    if (_mode == TYPE_PWM) {
      delayUs = processPWM(timing.duration_us, timing.value);
    } else {
      delayUs = processVPW(timing.duration_us, timing.value);
    }

    _last_level = timing.value;
    _last_activity = timing.timestamp;

    if (_index == INDEX_EOD || _index == INDEX_EOF) {
      rxRingBuf.push(CH_SOF);
      _index = INDEX_SOF_1;
      rxPacketCount++;
      setOutboundInterrupt();
      return -1;      
    }

    if (_index == INDEX_ABORT) {
      rxRingBuf.push(CH_ESC);
      rxRingBuf.push(CH_SOF);   // on Rx this maps to CH_ABORT after unescaping
      rxPacketCount++;
      setOutboundInterrupt();
      _index = INDEX_BRK_1;
      _receiving = false;
      kickTX();
      return -1;                  
    }
  }
  
  uint32_t time_idle = micros() - _last_activity;
  int16_t idle = clamp<int16_t>(time_idle, 0, 1000);
  int16_t ifs = _mode == TYPE_PWM ? pwm_timing[INDEX_IFS].nom : vpw_timing[INDEX_IFS].nom;     

  if (idle >= ifs) {
    // We can now transmit.  Woohoo!
    _receiving = false;
    kickTX();
    return -1;
  }

  return delayUs;
}

int J1850IO::processPWM(uint32_t duration, bool level)
{
  if (level == _last_level) {
    // we got out of sync here.  Hmmm.  Whatever
    return -1;
  }

  if (level) {
    // we saw a falling edge, positive pulse of width duration
    if (isInTiming(pwm_timing, INDEX_BRK_1, duration)) {
      // We are receiving a BREAK
      _index = INDEX_BRK_0;
    } else if (isInTiming(pwm_timing, INDEX_SOF_1, duration)) {
      // We are receiving a SOF
      if (_index != INDEX_SOF_1) {
        // What's going on?  we are not expecting this, abort
        _index = INDEX_ABORT;     
      } else {
        _index = INDEX_SOF_0;
      }
    } else if (isInTiming(pwm_timing, INDEX_ACT_0, duration)) {
      _index = INDEX_INACT_0;
    } else if (isInTiming(pwm_timing, INDEX_ACT_1, duration)) {
      _index = INDEX_INACT_1;
    } else {
      // This is messed up.  Let's abort
      _index = INDEX_ABORT;    
    }
  } else {
    // we saw a rising edge, negative pulse of width duration
    if (_index == INDEX_BRK_0 && isInTiming(pwm_timing, _index, duration)) {
      // That was a successful break incoming.   Now we wait for IFS
      _index = INDEX_IFS;
    } else if (_index == INDEX_IFS && isInTiming(pwm_timing, _index, duration)) {
      // We hit IFS.  We can now transmit, or get the next SOF
      _receiving = false;
      _index = INDEX_SOF_1;
    } else if (_index == INDEX_SOF_0 && isInTiming(pwm_timing, _index, duration)) {
      // That was the end of a SOF.  Now all the data be ours.timing
      _index = INDEX_ACT_0;
    } else if (_index == INDEX_INACT_0 && isInTiming(pwm_timing, _index, duration)) {
      // 0 bit
      _rx_shift_reg <<= 1;
      _rx_shift_reg |= 0;
      _rx_bit_count++;
    } else if (_index == INDEX_INACT_1 && isInTiming(pwm_timing, _index, duration)) {
      // 1 bit
      _rx_shift_reg <<= 1;
      _rx_shift_reg |= 1;
      _rx_bit_count++;     
    } else if ((_index == INDEX_INACT_0 || _index == INDEX_INACT_1) && isInTiming(pwm_timing, INDEX_EOF, duration)) {
      // end of frame
      if (_rx_bit_count) {
        // We have a partial byte in the shift register.  Abort and clear it.
        _rx_bit_count = 0;
        _index = INDEX_ABORT;        
      } else {
        _index = INDEX_EOF;
      }
    } else {
      // Huh?  Let's just abort
      _index = INDEX_ABORT;
    }

    if (_rx_bit_count == 8) {
      rxRingBuf.push(_rx_shift_reg);
      _rx_bit_count = 0;
    }
  }

  return -1;
}


int J1850IO::processVPW(uint32_t duration, bool level)
{
  if (level == _last_level) {
    // we got out of sync here.  Hmmm.  Whatever
    return -1;
  }

  bool found = false;
  bool bit = false;

  if (level) {
    // we saw a falling edge, positive pulse of width duration
    if (isInTiming(pwm_timing, INDEX_BRK_1, duration)) {
      // We are receiving a BREAK
      _index = INDEX_IFS;
    } else if (isInTiming(pwm_timing, INDEX_SOF_1, duration)) {
      // We are receiving a SOF
      if (_index != INDEX_SOF_1) {
        // What's going on?  we are not expecting this, abort
        _index = INDEX_ABORT;     
      } else {
        _index = INDEX_INACT_0;
      }
    } else if (isInTiming(pwm_timing, INDEX_ACT_0, duration)) {
      if (_index == INDEX_EOD) {
        // Normalization bit, dump it
        _index = INDEX_INACT_0;
        return -1;        
      }
      found = true;
      bit = false;
      _index = INDEX_INACT_0;
    } else if (isInTiming(pwm_timing, INDEX_ACT_1, duration)) {
      if (_index == INDEX_EOD) {
        // Normalization bit, dump it
        _index = INDEX_INACT_1;
        return -1;        
      }
      found = true;
      bit = true;
      _index = INDEX_INACT_1;
    } else {
      // This is messed up.  Let's abort
      _index = INDEX_ABORT;    
    }
  } else {
    // we saw a rising edge, negative pulse of width duration
    if (_index == INDEX_IFS && isInTiming(pwm_timing, _index, duration)) {
      // We hit IFS.  We can now transmit, or get the next SOF
      _receiving = false;
      _index = INDEX_SOF_1;
    } else if (isInTiming(pwm_timing, INDEX_INACT_0, duration)) {
      found = true;
      bit = false;
      _index = INDEX_ACT_0;
    } else if (isInTiming(pwm_timing, INDEX_INACT_1, duration)) {
      found = true;
      bit = true;
      _index = INDEX_ACT_1;
    } else if (_index == INDEX_INACT_0 || _index == INDEX_INACT_1) {
      uint8_t index = INDEX_COUNT;
      if (isInTiming(pwm_timing, INDEX_EOF, duration)) {
        index = INDEX_EOF;
      } else if (isInTiming(pwm_timing, INDEX_EOD, duration)) {
        index = INDEX_EOD;        
      }

      if (index != INDEX_COUNT) {
        // end of frame
        if (_rx_bit_count) {
          // We have a partial byte in the shift register.  Abort and clear it.
          _rx_bit_count = 0;
          _index = INDEX_ABORT;        
        } else {
          _index = index;
        }
      } else {
        _index = INDEX_ABORT;
      }
    } else {
      // Huh?  Let's just abort
      _index = INDEX_ABORT;
    }

    if (found) {
      _rx_shift_reg <<= 1;
      _rx_shift_reg |= bit;
      _rx_bit_count++;
    }    

    if (_rx_bit_count == 8) {
      rxRingBuf.push(_rx_shift_reg);
      _rx_bit_count = 0;
    }
  }

  return -1;  
}

static_assert(PIN_J1850_RX == PIN_PA2, "Need to adjust hardcoded RX pin logic");

void j1850_rx_isr(void)
{
  // We saw an edge
  if (j1850io.isTransmitting()) {
    // will need to add some collsion detection here
    return;
  }

  // Hardcoded to read the pin at PA2
  uint32_t now = micros();
  uint32_t duration = now - last_edge_us;
  rx_timing_t timing = {now, duration, !(PORTA_IN & BIT(2))};
  rx_timing_q.push(&timing);
}

bool isInTiming(const timing_t *timings, uint8_t index, uint32_t duration)
{
  index = clamp<uint8_t>(index, 0, INDEX_COUNT - 1);
  int16_t value = clamp<int16_t>(duration, 0, 32767); 
  
  const timing_t *timing = timings + index;
  if (timing->nom < 0) {
    return false;
  }

  return (timing->rx_min <= value && (timing->rx_max >= value || timing->rx_max < 0));   
}
