/*
 * button.h
 *
 *  Created on: Feb 22, 2026
 *      Author: Rubin Khadka
 */

#ifndef BUTTON_H_
#define BUTTON_H_

// Display modes structure
typedef enum
{
  DISPLAY_LOCATION = 0,
  DISPLAY_TEMPERATURE,
  DISPLAY_WIND,
  DISPLAY_MODE_COUNT
} DisplayMode_t;

// Function Prototypes
void Button_Init(void);
void TIMER4_Init(void);

// Display swap mode functions
DisplayMode_t Button_GetMode(void);
void Button_NextMode(void);

#endif /* BUTTON_H_ */
