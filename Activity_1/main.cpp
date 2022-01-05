/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
// Green LED
DigitalOut led1(LED1);
// Blue LED
DigitalOut led2(LED2);
// Red LED
DigitalOut led3(LED3);

Timeout button_debounce_timeout;
float debounce_time_interval = 0.3;

InterruptIn button(USER_BUTTON);

Ticker cycle_ticker;
float cycle_time_interval = 1;

Serial pc(SERIAL_TX, SERIAL_RX);

// LED counter
int LED_counter = 0;
// Save last LED counter
int last_LED = 0;

// Array to store LED pressed
int buffer[5] = {0}; 
// Length of buffer
int buffer_size = sizeof(buffer)/sizeof(buffer[0]);

// Int to keep track of number of LED stored
int stored_LED = 0;

void onButtonStopDebouncing(void);

void select_led(int l)
{
        if (l==1) {
                led1 = true;
                led2 = false;
                led3 = false;
        }
        else if (l==2) {
                led1 = false;
                led2 = true;
                led3 = false;
        }
        else if (l==3) {
                led1 = false;
                led2 = false;
                led3 = true;
        }
}
void onButtonPress(void)
{   
        // Check whether we have fully stored the buffer
        if (stored_LED < buffer_size)
        {
            // Check the LED_counter value and set buffer to last LED lit up
            buffer[stored_LED] = last_LED;
            // Increment stored_LED
            stored_LED++;
        }
        
        button.rise(NULL);
        button_debounce_timeout.attach(onButtonStopDebouncing, debounce_time_interval);

}

void onButtonStopDebouncing(void)
{
        button.rise(onButtonPress);
}


void onCycleTicker(void)
{
        // Check if the last byte is populated in buffer
        // If it hasn't, cycle through normal sequence
        if (buffer[buffer_size-1] == 0)
        {
            select_led(LED_counter);
            last_LED = LED_counter;
            LED_counter=(LED_counter%3)+1;
        }
        // Else, cycle through the saved sequence using stored_LED as counter
        else
        {   
            stored_LED =(stored_LED%buffer_size)+1;
            select_led(buffer[stored_LED-1]);
        }
}


int main()
{
        pc.baud(9600);
        pc.printf("Start!\r\n");
        pc.printf("Buffer size: %d. \r\n",buffer_size);
        button.rise(onButtonPress);
        cycle_ticker.attach(onCycleTicker, cycle_time_interval);
}