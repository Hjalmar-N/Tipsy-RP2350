/**
 * Tipsy-RP2350 - Minimal bring-up
 * 
 * First verification step:
 * - USB serial heartbeat output
 * - Built-in LED blink as backup visual confirmation
 * 
 * No board-specific hardware assumptions yet.
 */

#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

int main() {
    // Initialize stdio for USB serial output
    stdio_init_all();
    
    // Initialize LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Wait for USB serial connection (optional, helps with early messages)
    sleep_ms(2000);
    
    printf("Tipsy-RP2350 boot\n");
    printf("Board: Waveshare RP2350-Touch-LCD-3.5\n");
    printf("SDK: Pico SDK 2.x\n");
    
    uint32_t count = 0;
    
    while (true) {
        // Toggle LED
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
        
        // Heartbeat every second
        if (count % 2 == 0) {
            printf("Heartbeat: %lu\n", count / 2);
        }
        
        count++;
    }
    
    return 0;
}
