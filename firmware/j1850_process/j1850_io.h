#ifndef __j1850_io_h_
#define __j1850_io_h_

typedef struct {
  int16_t nom;
  int16_t tx_min;
  int16_t tx_max;
  int16_t rx_min;
  int16_t rx_max;
} timing_t;

enum {
  INDEX_SOF_0,
  INDEX_SOF_1,
  INDEX_EOD,
  INDEX_EOF,
  INDEX_BRK_0,
  INDEX_BRK_1,
  INDEX_IFS,
  INDEX_INACT_0,
  INDEX_INACT_1,
  INDEX_ACT_0,
  INDEX_ACT_1,
  INDEX_ABORT,
  INDEX_COUNT,
};

enum {
  TYPE_VPW,
  TYPE_PWM,
  TYPE_COUNT,
};


void j1850_rx_isr(void);

class J1850IO {
  public:
    J1850IO(uint8_t rx_pin, uint8_t tx_pin, uint16_t noise_us = 1) : 
      _rx_pin(rx_pin), _tx_pin(tx_pin), _mode(TYPE_PWM), _noise_us(noise_us) {}
    void setMode(uint8_t mode) { _mode = mode; };
    void init(void);
    int process(void);
    void kickTX(void);
    bool isTransmitting(void) { return _transmitting; };
    bool isIdle(void) { return !_transmitting && !_receiving; };

  protected:
    uint8_t _rx_pin;
    uint8_t _tx_pin;
    uint8_t _mode;
    uint16_t _noise_us;
    bool _transmitting;
    bool _receiving;
    uint8_t _index;
    bool  _last_level;
    uint32_t _last_activity;
    uint8_t _tx_shift_reg;
    uint8_t _tx_bit_count;
    uint8_t _rx_shift_reg;
    uint8_t _rx_bit_count;

    int processTX(void);
    int processRX(void);
    uint8_t get_next_tx_index(bool new_packet = false);
    int processPWM(uint32_t duration, bool level);
    int processVPW(uint32_t duration, bool level);

};

extern J1850IO j1850io;

#endif
