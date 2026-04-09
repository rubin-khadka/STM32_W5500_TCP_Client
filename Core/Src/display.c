/*
 * display.c
 *
 *  Created on: Apr 9, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "display.h"
#include "lcd.h"
#include "usart1.h"
#include <stdio.h>
#include <string.h>

// Static variables for screen cycling
static WeatherData_t *current_weather = NULL;
static int current_screen = 0;
static uint32_t last_screen_change = 0;
static uint8_t display_ready = 0;

// Convert wind direction degrees to cardinal direction
static void GetWindDirection(int degrees, char *direction)
{
  if(degrees >= 337 || degrees < 22)
    strcpy(direction, "N");
  else if(degrees >= 22 && degrees < 67)
    strcpy(direction, "NE");
  else if(degrees >= 67 && degrees < 112)
    strcpy(direction, "E");
  else if(degrees >= 112 && degrees < 157)
    strcpy(direction, "SE");
  else if(degrees >= 157 && degrees < 202)
    strcpy(direction, "S");
  else if(degrees >= 202 && degrees < 247)
    strcpy(direction, "SW");
  else if(degrees >= 247 && degrees < 292)
    strcpy(direction, "W");
  else if(degrees >= 292 && degrees < 337)
    strcpy(direction, "NW");
  else
    strcpy(direction, "?");
}

// Screen 1: Location (Latitude, Longitude, Elevation)
static void DisplayLocationScreen(void)
{
  if(!current_weather)
    return;

  LCD_Clear();

  // Line 1: Latitude and Longitude with N/S/E/W indicators
  char line1[17];
  char ns = (current_weather->latitude >= 0) ? 'N' : 'S';
  char ew = (current_weather->longitude >= 0) ? 'E' : 'W';
  float lat_abs = (current_weather->latitude >= 0) ? current_weather->latitude : -current_weather->latitude;
  float lon_abs = (current_weather->longitude >= 0) ? current_weather->longitude : -current_weather->longitude;

  sprintf(line1, "%.1f%c %.1f%c", lat_abs, ns, lon_abs, ew);
  LCD_SetCursor(0, 0);
  LCD_SendString(line1);

  // Line 2: Elevation
  char line2[17];
  sprintf(line2, "Elev: %.0fm", current_weather->elevation);
  LCD_SetCursor(1, 0);
  LCD_SendString(line2);

  // Debug output
  USART1_SendString("\r\n=== Screen: Location ===\r\n");
  USART1_SendString(line1);
  USART1_SendString("\r\n");
  USART1_SendString(line2);
  USART1_SendString("\r\n");
}

// Screen 2: Time and Temperature
static void DisplayTempTimeScreen(void)
{
  if(!current_weather)
    return;

  LCD_Clear();

  // Line 1: Temperature
  char line1[17];
  sprintf(line1, "Temp: %.1f C", current_weather->temperature);
  LCD_SetCursor(0, 0);
  LCD_SendString(line1);

  // Line 2: Time (extract just HH:MM from 2026-04-09T17:45)
  char time_only[6] = "??:??";
  char *time_ptr = strchr(current_weather->time, 'T');
  if(time_ptr)
  {
    strncpy(time_only, time_ptr + 1, 5);
    time_only[5] = '\0';
  }

  char line2[17];
  sprintf(line2, "Time: %s", time_only);
  LCD_SetCursor(1, 0);
  LCD_SendString(line2);

  // Debug output
  USART1_SendString("\r\n=== Screen: Temp & Time ===\r\n");
  USART1_SendString(line1);
  USART1_SendString("\r\n");
  USART1_SendString(line2);
  USART1_SendString("\r\n");
}

// Screen 3: Wind Speed and Direction
static void DisplayWindScreen(void)
{
  if(!current_weather)
    return;

  LCD_Clear();

  // Line 1: Wind speed
  char line1[17];
  sprintf(line1, "Wind: %.1f km/h", current_weather->windspeed);
  LCD_SetCursor(0, 0);
  LCD_SendString(line1);

  // Line 2: Wind direction
  char dir_text[4];
  GetWindDirection(current_weather->winddirection, dir_text);

  char line2[17];
  sprintf(line2, "Dir: %s (%d deg)", dir_text, current_weather->winddirection);
  LCD_SetCursor(1, 0);
  LCD_SendString(line2);

  // Debug output
  USART1_SendString("\r\n=== Screen: Wind ===\r\n");
  USART1_SendString(line1);
  USART1_SendString("\r\n");
  USART1_SendString(line2);
  USART1_SendString("\r\n");
}

// Initialize display system
void Display_Init(void)
{
  LCD_Init();
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_SendString("Weather Station");
  LCD_SetCursor(1, 0);
  LCD_SendString("Initializing...");

  display_ready = 1;
  current_screen = 0;
  last_screen_change = HAL_GetTick();

  USART1_SendString("Display initialized\r\n");
}

// Update display with new weather data
void Display_UpdateWeather(WeatherData_t *weather)
{
  if(!display_ready)
    return;

  current_weather = weather;
  current_screen = 0;
  last_screen_change = HAL_GetTick();

  // Show first screen immediately
  DisplayLocationScreen();

  USART1_SendString("Weather data loaded to display\r\n");
}

// Call this in main loop to handle screen cycling
void Display_Handle(void)
{
  if(!display_ready || !current_weather)
    return;

  uint32_t current_time = HAL_GetTick();

  // Change screen every SCREEN_CHANGE_INTERVAL milliseconds
  if(current_time - last_screen_change >= SCREEN_CHANGE_INTERVAL)
  {
    current_screen++;
    if(current_screen >= 3)
      current_screen = 0;
    last_screen_change = current_time;

    // Update display based on current screen
    switch(current_screen)
    {
      case 0:
        DisplayLocationScreen();
        break;
      case 1:
        DisplayTempTimeScreen();
        break;
      case 2:
        DisplayWindScreen();
        break;
    }
  }
}
