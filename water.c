// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2024 Jerome Marchand
 *
 * Raspberry Pi Pico watering system
 *
 * Pins used:
 * ADC: 28 (for moisture reading)
 * Relay pin: 0
 * LED: 25 (onboard, used for debugging without a serial line)
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define DEBUG
#ifdef DEBUG
#define printdbg(...)  printf(__VA_ARGS__)
#else
#define printdbg(...)
#endif

// Humidity value: 0 very wet 4095 very dry
#define TARGET_MAX 2400

// Time interval (s) for each cycle of monitoring loop
#define DELAY 3600

/*
 * In case of an unreliable sensor, make sure we don't drown the plant
 * or let it dry
 */
#define CHECK_PERIOD 24
#define CHECK_MIN 1
#define CHECK_MAX 5
int check_time = 0;
int check_count = 0;

// Interval (s) at which to read the sensor for debugging purposes
#define DEBUG_DELAY 10
// It seems the sensor value is unconsistent when read early after boot
#define WARMING_DELAY 60

/*
 * How long to water when needed (s)
 * (Could be tuned depending on how far from target we are)
 */
#define WATERING_TIME 5

#ifndef PICO_DEFAULT_LED_PIN
#error need a board with a regular LED
#else
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif

// Command the relay
#define RELAY_PIN 0

// Read the humidity sensor GPIO 28, ADC input 2
#define HUMIDITY_PIN 28
#define HUMIDITY_ADC 2

void blink(int loops) {
	for(int i = 0; i < loops; i++) {
	        gpio_put(LED_PIN, 1);
		sleep_ms(250);
		gpio_put(LED_PIN, 0);
		sleep_ms(250);
	}
}

void blink_sleep(int s) {
	blink(2 * s);
}

uint16_t read_sensor() {
	uint16_t dryness = adc_read();

	printdbg("Dryness: %u\n", dryness);

	return dryness;
}

void wait(int delay) {
#ifdef DEBUG
	for(int i = 0; i < DELAY / DEBUG_DELAY; i++) {
		read_sensor();
		sleep_ms(1000 * DEBUG_DELAY);
	}
#else
	sleep_ms(1000 * DELAY);	
#endif
}

void watering(int dose) {
	if (check_count >= CHECK_MAX) {
		printdbg("Already watered enough today\n");
		return;
	}
	printdbg("Start watering %i dose\n", dose);
	gpio_put(RELAY_PIN, 1);
	blink_sleep(WATERING_TIME * dose);
	printdbg("Stop watering\n");
	gpio_put(RELAY_PIN, 0);
	check_count++;
	// keep score of the last reading
	gpio_put(LED_PIN, 1);
}

void update_check() {
	check_time = (check_time + 1) % CHECK_PERIOD;
	if (check_time == 0) {
		if (check_count < CHECK_MIN) {
			/*
			 * Didn't water  today.
			 * Faulty sensor or bad calibration?
			 */
			printdbg("Did't water enough today. Water %i doses\n",
				 CHECK_MIN - check_count);
			watering(CHECK_MIN - check_count);
		}
		check_count = 0;
	}
	printdbg("Check time: %2i, count: %2i\n", check_time, check_count);
}

int main() {
	stdio_init_all();

	// Initialise pins
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	blink(2);
	gpio_init(RELAY_PIN);
	gpio_set_dir(RELAY_PIN, GPIO_OUT);
	gpio_put(RELAY_PIN, 0);
	adc_init();
	adc_gpio_init(HUMIDITY_PIN);
	adc_select_input(HUMIDITY_ADC);

	wait(WARMING_DELAY);

	while(true) {
		// Get humidity
		uint16_t dryness = read_sensor();

		if (dryness > TARGET_MAX) {
			watering(1);
		} else {
			gpio_put(LED_PIN, 0);
		}
		wait(DELAY);
		update_check();
	}

}
