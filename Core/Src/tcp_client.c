/*
 * tcp_client.c
 *
 *  Created on: Apr 8, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "TCP_Client.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "dns.h"
#include "usart1.h"
#include "lcd.h"
#include "tcp_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// DNS Server
static uint8_t dns_ip[4] = DNS_SERVER_IP;
static uint8_t server_ip[4];

// Buffers
static uint8_t dns_buffer[512];
static uint8_t rx_buffer[1024];
static uint8_t tx_buffer[512];

// DNS Socket from wizchip_port.c
extern uint8_t dns_buffer[];

// Resolve hostname to IP using DNS
static int TCP_Client_ResolveDNS(void)
{
  USART1_SendString("Resolving ");
  USART1_SendString(WEATHER_HOST);
  USART1_SendString(" ...\r\n");

  DNS_init(DNS_SOCKET, dns_buffer);

  int8_t result = DNS_run(dns_ip, (uint8_t*) WEATHER_HOST, server_ip);

  if(result != 1)
  {
    USART1_SendString("DNS resolution failed!\r\n");
    return -1;
  }

  USART1_SendString("Resolved to: ");
  USART1_SendNumber(server_ip[0]);
  USART1_SendString(".");
  USART1_SendNumber(server_ip[1]);
  USART1_SendString(".");
  USART1_SendNumber(server_ip[2]);
  USART1_SendString(".");
  USART1_SendNumber(server_ip[3]);
  USART1_SendString("\r\n");

  return 0;
}

// Parse JSON response - extract only current_weather object
static void ParseWeatherData(char *response, WeatherData_t *weather)
{
  char *current_weather_start;
  char *ptr;
  char number_buffer[20];
  int i;

  // Initialize
  weather->temperature = 0;
  weather->windspeed = 0;
  weather->winddirection = 0;
  weather->weathercode = 0;

  USART1_SendString("Parsing JSON...\r\n");

  // Find "current_weather" object
  current_weather_start = strstr(response, "\"current_weather\"");
  if(!current_weather_start)
  {
    USART1_SendString("current_weather not found!\r\n");
    return;
  }

  // Find the opening '{' after current_weather
  current_weather_start = strchr(current_weather_start, '{');
  if(!current_weather_start)
  {
    USART1_SendString("No current_weather object found!\r\n");
    return;
  }

  USART1_SendString("Found current_weather object\r\n");

  // Debug: Print current_weather object (first 150 chars)
  char debug_buf[151];
  strncpy(debug_buf, current_weather_start, 150);
  debug_buf[150] = '\0';
  USART1_SendString("current_weather: ");
  USART1_SendString(debug_buf);
  USART1_SendString("\r\n");

  // Extract temperature from current_weather
  ptr = strstr(current_weather_start, "\"temperature\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      // Skip any spaces
      while(*ptr == ' ')
        ptr++;

      // Extract number
      i = 0;
      while(*ptr && ((*ptr >= '0' && *ptr <= '9') || *ptr == '.' || *ptr == '-') && i < 19)
      {
        number_buffer[i++] = *ptr++;
      }
      number_buffer[i] = '\0';

      weather->temperature = 0;
      // Manual conversion
      int is_negative = 0;
      int int_part = 0;
      int dec_part = 0;
      int dec_div = 1;
      int j = 0;

      if(number_buffer[0] == '-')
      {
        is_negative = 1;
        j = 1;
      }

      while(number_buffer[j] >= '0' && number_buffer[j] <= '9')
      {
        int_part = int_part * 10 + (number_buffer[j] - '0');
        j++;
      }

      if(number_buffer[j] == '.')
      {
        j++;
        while(number_buffer[j] >= '0' && number_buffer[j] <= '9')
        {
          dec_part = dec_part * 10 + (number_buffer[j] - '0');
          dec_div *= 10;
          j++;
        }
      }

      weather->temperature = int_part + (float) dec_part / dec_div;
      if(is_negative)
        weather->temperature = -weather->temperature;

      USART1_SendString("Temperature: ");
      USART1_SendNumber((int) weather->temperature);
      USART1_SendString(".");
      int dec = (int) ((weather->temperature - (int) weather->temperature) * 10);
      if(dec < 0)
        dec = -dec;
      USART1_SendNumber(dec);
      USART1_SendString(" C\r\n");
    }
  }

  // Extract windspeed from current_weather
  ptr = strstr(current_weather_start, "\"windspeed\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;

      i = 0;
      while(*ptr && ((*ptr >= '0' && *ptr <= '9') || *ptr == '.') && i < 19)
      {
        number_buffer[i++] = *ptr++;
      }
      number_buffer[i] = '\0';

      weather->windspeed = 0;
      int int_part = 0;
      int dec_part = 0;
      int dec_div = 1;
      int j = 0;

      while(number_buffer[j] >= '0' && number_buffer[j] <= '9')
      {
        int_part = int_part * 10 + (number_buffer[j] - '0');
        j++;
      }

      if(number_buffer[j] == '.')
      {
        j++;
        while(number_buffer[j] >= '0' && number_buffer[j] <= '9')
        {
          dec_part = dec_part * 10 + (number_buffer[j] - '0');
          dec_div *= 10;
          j++;
        }
      }

      weather->windspeed = int_part + (float) dec_part / dec_div;

      USART1_SendString("Wind Speed: ");
      USART1_SendNumber((int) weather->windspeed);
      USART1_SendString(" km/h\r\n");
    }
  }

  // Extract winddirection from current_weather
  ptr = strstr(current_weather_start, "\"winddirection\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->winddirection = 0;
      while(*ptr >= '0' && *ptr <= '9')
      {
        weather->winddirection = weather->winddirection * 10 + (*ptr - '0');
        ptr++;
      }
      USART1_SendString("Wind Direction: ");
      USART1_SendNumber(weather->winddirection);
      USART1_SendString(" deg\r\n");
    }
  }

  // Extract weathercode from current_weather
  ptr = strstr(current_weather_start, "\"weathercode\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->weathercode = 0;
      while(*ptr >= '0' && *ptr <= '9')
      {
        weather->weathercode = weather->weathercode * 10 + (*ptr - '0');
        ptr++;
      }
      USART1_SendString("Weather Code: ");
      USART1_SendNumber(weather->weathercode);
      USART1_SendString("\r\n");
    }
  }
}

// Initialize TCP client and connect to weather server
int TCP_Client_Init(void)
{
  int8_t ret;
  uint8_t status;

  USART1_SendString("Initializing TCP Client...\r\n");

  // Step 1: Resolve the hostname
  if(TCP_Client_ResolveDNS() != 0)
    return -1;

  // Step 2: Create socket
  ret = socket(TCP_CLIENT_SOCKET, Sn_MR_TCP, 3000, 0);
  if(ret != TCP_CLIENT_SOCKET)
  {
    USART1_SendString("Socket open failed\r\n");
    return -1;
  }

  // Step 3: Connect to remote server
  USART1_SendString("Connecting to weather server...\r\n");

  ret = connect(TCP_CLIENT_SOCKET, server_ip, HTTP_PORT);
  if(ret != SOCK_OK)
  {
    USART1_SendString("Connect failed\r\n");
    close(TCP_CLIENT_SOCKET);
    return -1;
  }

  // Step 4: Wait until the socket is established
  uint32_t timeout = HAL_GetTick() + 5000;
  do
  {
    status = getSn_SR(TCP_CLIENT_SOCKET);
    if(status == SOCK_ESTABLISHED)
    {
      USART1_SendString("TCP connection established!\r\n");
      return 0;
    }
    if(status == SOCK_CLOSED || status == SOCK_CLOSE_WAIT)
      break;
    HAL_Delay(10);
  }
  while(HAL_GetTick() < timeout);

  USART1_SendString("Connection timeout\r\n");
  close(TCP_CLIENT_SOCKET);
  return -1;
}

void TCP_Client_GetWeather(void)
{
  int32_t ret;
  uint8_t status;
  WeatherData_t weather = {0};
  char display_line1[17];
  char display_line2[17];

  status = getSn_SR(TCP_CLIENT_SOCKET);

  if(status == SOCK_ESTABLISHED)
  {
    // Build HTTP GET request
    sprintf((char*) tx_buffer, "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
    WEATHER_PATH, WEATHER_HOST);

    // Send HTTP request
    USART1_SendString("Sending HTTP request...\r\n");
    ret = send(TCP_CLIENT_SOCKET, tx_buffer, strlen((char*) tx_buffer));
    if(ret <= 0)
    {
      USART1_SendString("Send failed\r\n");
      return;
    }

    USART1_SendString("HTTP request sent\r\n");

    // Receive response
    USART1_SendString("Receiving weather data...\r\n");

    // Clear receive buffer
    memset(rx_buffer, 0, sizeof(rx_buffer));

    while(1)
    {
      ret = recv(
      TCP_CLIENT_SOCKET, rx_buffer + strlen((char*) rx_buffer), sizeof(rx_buffer) - strlen((char*) rx_buffer) - 1);
      if(ret <= 0)
        break;
    }

    USART1_SendString("Raw JSON:\r\n");
    USART1_SendString((char*) rx_buffer);
    USART1_SendString("\r\n");

    // Parse weather data
    ParseWeatherData((char*) rx_buffer, &weather);

    // Display on Serial
    USART1_SendString("\r\n=== WEATHER DATA ===\r\n");
    USART1_SendString("Temperature: ");
    USART1_SendNumber(weather.temperature * 10);  // Convert to integer for display
    USART1_SendString(".");
    USART1_SendNumber((int) (weather.temperature * 10) % 10);
    USART1_SendString(" C\r\n");

    USART1_SendString("Wind Speed: ");
    USART1_SendNumber((int) weather.windspeed);
    USART1_SendString(" km/h\r\n");

    USART1_SendString("Wind Direction: ");
    USART1_SendNumber(weather.winddirection);
    USART1_SendString(" deg\r\n");

    USART1_SendString("Weather Code: ");
    USART1_SendNumber(weather.weathercode);
    USART1_SendString("\r\n");

    // Display on LCD
    LCD_Clear();

    // Line 1: Temperature
    sprintf(display_line1, "Temp: %.1f C", weather.temperature);
    LCD_SetCursor(0, 0);
    LCD_SendString(display_line1);

    // Line 2: Wind speed
    sprintf(display_line2, "Wind: %.0f km/h", weather.windspeed);
    LCD_SetCursor(1, 0);
    LCD_SendString(display_line2);

    USART1_SendString("\r\n=== LCD Updated ===\r\n");
  }
  else if(status == SOCK_CLOSE_WAIT)
  {
    USART1_SendString("Server closed connection\r\n");
    disconnect(TCP_CLIENT_SOCKET);
  }
  else if(status == SOCK_CLOSED)
  {
    USART1_SendString("Socket closed\r\n");
  }
}

// Close TCP client connection
void TCP_Client_Close(void)
{
  close(TCP_CLIENT_SOCKET);
  USART1_SendString("TCP Client closed\r\n");
}
