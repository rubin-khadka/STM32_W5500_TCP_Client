/*
 * display.h
 *
 *  Created on: Apr 9, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#include "tcp_client.h"

// Function Prototype
void Display_Init(void);
void Display_UpdateWeather(WeatherData_t *weather);
void Display_Handle(void);

#endif /* INC_DISPLAY_H_ */
