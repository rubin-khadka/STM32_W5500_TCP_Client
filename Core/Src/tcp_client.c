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
static uint8_t cached_ip[4] = {0};  // Cache for IP address
static uint8_t ip_resolved = 0;      // Flag to track if IP is resolved

// Buffers
static uint8_t dns_buffer[512];
static uint8_t rx_buffer[1024];
static uint8_t tx_buffer[512];

// DNS Socket from wizchip_port.c
extern uint8_t dns_buffer[];

static void TCP_Client_PrintSocketStatus(void);

// Resolve hostname to IP using DNS (only once)
static int TCP_Client_ResolveDNS(void)
{
  // Use cached IP if already resolved
  if(ip_resolved)
  {
    memcpy(server_ip, cached_ip, 4);
    USART1_SendString("Using cached IP\r\n");
    return 0;
  }

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

  // Cache the resolved IP
  memcpy(cached_ip, server_ip, 4);
  ip_resolved = 1;

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

  // Find the start of JSON (first '{')
  root_start = strchr(response, '{');
  if(!root_start)
    return;

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
    }
  }

  // ===== Find "current_weather" object =====
  current_weather_start = strstr(root_start, "\"current_weather\"");
  if(!current_weather_start)
    return;

  current_weather_start = strchr(current_weather_start, '{');
  if(!current_weather_start)
    return;

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
    }
  }
}

// Initialize TCP client and connect to weather server
int TCP_Client_Init(void)
{
  int8_t ret;
  uint8_t status;

  USART1_SendString("Initializing TCP Client...\r\n");

  // Step 1: Resolve the hostname (cached after first time)
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

    // Parse weather data into the provided structure
    ParseWeatherData((char*) rx_buffer, weather);
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

void TCP_Client_Close(void)
{
  USART1_SendString("Closing TCP Client...\r\n");
  TCP_Client_PrintSocketStatus();

  // Try to disconnect properly if connected
  uint8_t status = getSn_SR(TCP_CLIENT_SOCKET);
  if(status == SOCK_ESTABLISHED)
  {
    USART1_SendString("Disconnecting...\r\n");
    disconnect(TCP_CLIENT_SOCKET);
    HAL_Delay(200);  // Increased delay
  }

  // Close the socket
  close(TCP_CLIENT_SOCKET);
  HAL_Delay(200);  // Increased delay

  // Extra: Force reset if still not closed
  status = getSn_SR(TCP_CLIENT_SOCKET);
  if(status != SOCK_CLOSED)
  {
    USART1_SendString("Socket not closed, forcing reset...\r\n");
    TCP_Client_ForceReset();
  }

  TCP_Client_PrintSocketStatus();
  USART1_SendString("TCP Client closed\r\n");
}

// Force complete socket reset
void TCP_Client_ForceReset(void)
{
  USART1_SendString("Force resetting socket...\r\n");

  // Close if open
  close(TCP_CLIENT_SOCKET);
  HAL_Delay(100);

  // Re-initialize the socket
  socket(TCP_CLIENT_SOCKET, Sn_MR_TCP, 0, 0);
  HAL_Delay(50);
  close(TCP_CLIENT_SOCKET);
  HAL_Delay(100);

  TCP_Client_PrintSocketStatus();
  USART1_SendString("Socket reset complete\r\n");
}

// Debug function to check socket status
static void TCP_Client_PrintSocketStatus(void)
{
  uint8_t status = getSn_SR(TCP_CLIENT_SOCKET);
  const char *status_str;

  switch(status)
  {
    case SOCK_CLOSED:
      status_str = "CLOSED";
      break;
    case SOCK_INIT:
      status_str = "INIT";
      break;
    case SOCK_LISTEN:
      status_str = "LISTEN";
      break;
    case SOCK_ESTABLISHED:
      status_str = "ESTABLISHED";
      break;
    case SOCK_CLOSE_WAIT:
      status_str = "CLOSE_WAIT";
      break;
    case SOCK_SYNSENT:
      status_str = "SYNSENT";
      break;
    case SOCK_SYNRECV:
      status_str = "SYNRECV";
      break;
    case SOCK_FIN_WAIT:
      status_str = "FIN_WAIT";
      break;
    case SOCK_TIME_WAIT:
      status_str = "TIME_WAIT";
      break;
    default:
      status_str = "UNKNOWN";
      break;
  }

  USART1_SendString("Socket ");
  USART1_SendNumber(TCP_CLIENT_SOCKET);
  USART1_SendString(" status: ");
  USART1_SendString(status_str);
  USART1_SendString("\r\n");
}
