/*
 * display.h
 *
 *  Created on: Apr 9, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#include "tcp_client.h"

// Screen cycling interval (milliseconds)
#define SCREEN_CHANGE_INTERVAL 3000  // 3 seconds per screen

// Initialize display system
void Display_Init(void);

// Update display with new weather data
void Display_UpdateWeather(WeatherData_t *weather);

// Call this in main loop to handle screen cycling
void Display_Handle(void);

#endif /* INC_DISPLAY_H_ */
