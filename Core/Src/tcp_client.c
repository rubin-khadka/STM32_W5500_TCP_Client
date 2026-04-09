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

// Helper function to extract number from string
static float extract_float(char *str)
{
  float result = 0;
  float decimal = 0;
  int sign = 1;
  int decimal_place = 0;

  if(*str == '-')
  {
    sign = -1;
    str++;
  }

  while(*str >= '0' && *str <= '9')
  {
    result = result * 10 + (*str - '0');
    str++;
  }

  if(*str == '.')
  {
    str++;
    decimal_place = 1;
    while(*str >= '0' && *str <= '9')
    {
      decimal = decimal * 10 + (*str - '0');
      decimal_place *= 10;
      str++;
    }
    result += decimal / decimal_place;
  }

  return result * sign;
}

// Helper function to extract integer from string
static int extract_int(char *str)
{
  int result = 0;
  while(*str >= '0' && *str <= '9')
  {
    result = result * 10 + (*str - '0');
    str++;
  }
  return result;
}

// Parse JSON response - extract all weather data
static void ParseWeatherData(char *response, WeatherData_t *weather)
{
  char *ptr;
  char *root_start;
  char *current_weather_start;

  // Initialize
  memset(weather, 0, sizeof(WeatherData_t));

  USART1_SendString("Parsing JSON...\r\n");

  // Find the start of JSON (first '{')
  root_start = strchr(response, '{');
  if(!root_start)
  {
    USART1_SendString("No JSON found!\r\n");
    return;
  }

  // ===== Extract from ROOT object =====

  // Extract latitude
  ptr = strstr(root_start, "\"latitude\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->latitude = extract_float(ptr);
      USART1_SendString("Latitude: ");
      USART1_SendNumber((int) (weather->latitude * 10));
      USART1_SendString("\r\n");
    }
  }

  // Extract longitude
  ptr = strstr(root_start, "\"longitude\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->longitude = extract_float(ptr);
      USART1_SendString("Longitude: ");
      USART1_SendNumber((int) (weather->longitude * 10));
      USART1_SendString("\r\n");
    }
  }

  // Extract elevation
  ptr = strstr(root_start, "\"elevation\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->elevation = extract_float(ptr);
      USART1_SendString("Elevation: ");
      USART1_SendNumber((int) weather->elevation);
      USART1_SendString(" m\r\n");
    }
  }

  // ===== Find "current_weather" object =====
  current_weather_start = strstr(root_start, "\"current_weather\"");
  if(!current_weather_start)
  {
    USART1_SendString("current_weather not found!\r\n");
    return;
  }

  current_weather_start = strchr(current_weather_start, '{');
  if(!current_weather_start)
  {
    USART1_SendString("No current_weather object found!\r\n");
    return;
  }

  USART1_SendString("Found current_weather object\r\n");

  // ===== Extract from CURRENT_WEATHER object =====

  // Extract time
  ptr = strstr(current_weather_start, "\"time\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ' || *ptr == '"')
        ptr++;
      int i = 0;
      while(*ptr && *ptr != '"' && i < 19)
      {
        weather->time[i++] = *ptr++;
      }
      weather->time[i] = '\0';
      USART1_SendString("Time: ");
      USART1_SendString(weather->time);
      USART1_SendString("\r\n");
    }
  }

  // Extract temperature
  ptr = strstr(current_weather_start, "\"temperature\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->temperature = extract_float(ptr);
      USART1_SendString("Temperature: ");
      USART1_SendNumber((int) (weather->temperature * 10));
      USART1_SendString(" C\r\n");
    }
  }

  // Extract windspeed
  ptr = strstr(current_weather_start, "\"windspeed\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->windspeed = extract_float(ptr);
      USART1_SendString("Wind Speed: ");
      USART1_SendNumber((int) (weather->windspeed * 10));
      USART1_SendString(" km/h\r\n");
    }
  }

  // Extract winddirection
  ptr = strstr(current_weather_start, "\"winddirection\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->winddirection = extract_int(ptr);
      USART1_SendString("Wind Direction: ");
      USART1_SendNumber(weather->winddirection);
      USART1_SendString(" deg\r\n");
    }
  }

  // Extract weathercode
  ptr = strstr(current_weather_start, "\"weathercode\"");
  if(ptr)
  {
    ptr = strchr(ptr, ':');
    if(ptr)
    {
      ptr++;
      while(*ptr == ' ')
        ptr++;
      weather->weathercode = extract_int(ptr);
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

// Get weather data and fill the structure
void TCP_Client_GetWeather(WeatherData_t *weather)
{
  int32_t ret;
  uint8_t status;

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

    int total_received = 0;
    while(1)
    {
      ret = recv(TCP_CLIENT_SOCKET, rx_buffer + total_received, sizeof(rx_buffer) - total_received - 1);
      if(ret <= 0)
        break;
      total_received += ret;
    }

    USART1_SendString("Raw JSON received\r\n");

    // Parse weather data into the provided structure
    ParseWeatherData((char*) rx_buffer, weather);

    USART1_SendString("\r\n=== WEATHER DATA PARSED ===\r\n");
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
