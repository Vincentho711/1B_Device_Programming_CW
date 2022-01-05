/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "platform/mbed_thread.h"
#include <vector>
#include <algorithm>

#define LM75_REG_TEMP (0x00) // Temperature Register
#define LM75_REG_CONF (0x01) // Configuration Register
#define LM75_ADDR     (0x90) // LM75 address

#define LM75_REG_TOS (0x03) // TOS Register
#define LM75_REG_THYST (0x02) // THYST Register

I2C i2c(I2C_SDA, I2C_SCL);

DigitalOut myled(LED1);

Serial pc(SERIAL_TX, SERIAL_RX);

// Red LED
DigitalOut led3(LED3);
// LM75 interrupt pin
InterruptIn lm75_int(D7);
int16_t i16; // This variable needs to be 16 bits wide for the TOS and THYST conversion to work

// Float to store temp
float temp = 0.0;

// Flag to store whether there is fire
bool fire = false;

// Data buffer array
float buffer[60] = {0.0};
// buffer counter
int buffer_counter;

Timeout debounce_timeout;
float time_interval = 0.3;


volatile char TempCelsiusDisplay[] = "+abc.d C";

void LM75_init()
{
    /* Configure the Temperature sensor device STLM75:
        - Thermostat mode Interrupt
        - Fault tolerance: 0
    */
    char data_write[2];
    char data_read[2];
    data_write[0] = LM75_REG_CONF;
    data_write[1] = 0x02;
    int status = i2c.write(LM75_ADDR, data_write, 2, 0);
    if (status != 0) { // Error
            while (1) {
                    myled = !myled;
                    wait(0.2);
            }
    }    
    float tos=28; // TOS temperature
    float thyst=26; // THYST tempertuare

    // This section of code sets the TOS register
    data_write[0]=LM75_REG_TOS;
    i16 = (int16_t)(tos*256) & 0xFF80;
    data_write[1]=(i16 >> 8) & 0xff;
    data_write[2]=i16 & 0xff;
    i2c.write(LM75_ADDR, data_write, 3, 0);

    //This section of codes set the THYST register
    data_write[0]=LM75_REG_THYST;
    i16 = (int16_t)(thyst*256) & 0xFF80;
    data_write[1]=(i16 >> 8) & 0xff;
    data_write[2]=i16 & 0xff;
    i2c.write(LM75_ADDR, data_write, 3, 0);
}

float LM75_read_temp()
{
    char data_write[2];
    char data_read[2];
    // Read temperature register
    data_write[0] = LM75_REG_TEMP;
    i2c.write(LM75_ADDR, data_write, 1, 1); // no stop
    i2c.read(LM75_ADDR, data_read, 2, 0);
    
    // Temp data
    int temp16 = (data_read[0] << 8) | data_read[1];
    // Read data as 2 complement integer
    float temp = temp16/256.0;
    
    // Calculate temperature value in Celcius
    int tempval = (int)((int)data_read[0] << 8) | data_read[1];
    tempval >>= 7;
    if (tempval <= 256) {
        TempCelsiusDisplay[0] = '+';
    } else {
        TempCelsiusDisplay[0] = '-';
        tempval = 512 - tempval;
    }

    // Decimal part (0.5°C precision)
    if (tempval & 0x01) {
        TempCelsiusDisplay[5] = 0x05 + 0x30;
    } else {
        TempCelsiusDisplay[5] = 0x00 + 0x30;
    }

    // Integer part
    tempval >>= 1;
    TempCelsiusDisplay[1] = (tempval / 100) + 0x30;
    TempCelsiusDisplay[2] = ((tempval % 100) / 10) + 0x30;
    TempCelsiusDisplay[3] = ((tempval % 100) % 10) + 0x30;   
    
    return temp;
}
// Forward Declaration
void fire_alert();
void reattach_interrupt(void)
{
        lm75_int.fall(&fire_alert);
}
// Callback function when temp > 28C
void fire_alert()
{
    led3=!led3;
    lm75_int.fall(NULL);
    // Set fire flag to output serial data
    fire = !fire;
    debounce_timeout.attach(reattach_interrupt,time_interval);
}

int main()
{
    // Initialise temp sensor
    LM75_init();
    // This line attaches the interrupt.
    // The interrupt line is active low so we trigger on a falling edge
    lm75_int.fall(&fire_alert);
    
    while (true) {
        // Read temp and save to buffer
        temp = LM75_read_temp();
        buffer[buffer_counter] = temp;
        
        // Update buffer when full
        if (buffer_counter < (60-1))
        {
            buffer_counter++;    
        }
        else
        {
            buffer_counter = 0;
        }
        
        // Check fire flag and output relevant info to serial terminal
        if (fire)
        {
            pc.printf("Fire! Last 60 temperature measurements.\r\n");
            pc.printf("-------------------------------\r\n");
            for (int i=0;i < (sizeof (buffer) /sizeof (buffer[0]));i++) {
                printf("%.3f\r\n",buffer[i]);
            }
        }
        else
        {
            pc.printf("Measuring temp.\r\n");
            pc.printf("Temperature = %.3f\r\n",temp);
        }
        
        wait(1.0);
    }
}
