/* 
 * File:   i2c_slave.h
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 4:53 PM
 */

#ifndef I2C_SLAVE_H
#define	I2C_SLAVE_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define I2C_SLAVE_ADDRESS   (0x79 << 1)

    enum {
        I2C_STATE_IDLE,
        I2C_STATE_ADDR,
        I2C_STATE_REG_ADDR,
        I2C_STATE_DATA,
    };
    
    enum {
        I2C_REG_INT_ENABLES,
        I2C_REG_INT_FLAGS,
        I2C_REG_DATA_FIFO,
        I2C_REG_COUNT,
    };

    void i2c_slave_init(void);
    void ssp1_isr(void);
    void i2c_rx_kick(void);
    

#ifdef	__cplusplus
}
#endif

#endif	/* I2C_SLAVE_H */

