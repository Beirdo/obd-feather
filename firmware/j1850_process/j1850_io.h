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
  INDEX_SOF,
  INDEX_EOF,
  INDEX_BRK,
  INDEX_IFS,
  INDEX_INACT_0,
  INDEX_INACT_1,
  INDEX_ACT_0,
  INDEX_ACT_1,
  INDEX_COUNT,
};

enum {
  TYPE_VPW,
  TYPE_PWM,
  TYPE_COUNT,
};


class J1850IO {
  public:
    J1850IO(uint8_t rx_pin, uint8_t tx_pin, uint16_t noise_us = 1) : 
      _rx_pin(rx_pin), _tx_pin(tx_pin), _noise_us(noise_us), _mode(TYPE_PWM) {}
    void setMode(uint8_t mode) { _mode = mode };
    void init(void);
    uint8_t process(void);

  protected:
    uint8_t _rx_pin;
    uint8_t _tx_pin;
    uint8_t _mode;
    uint16_t _noise_us;
};

#endif