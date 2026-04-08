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
#include <stdio.h>
#include <string.h>

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

// Send HTTP GET request and receive weather data
void TCP_Client_GetWeather(void)
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
        "\r\n", WEATHER_PATH, WEATHER_HOST);

    // Send HTTP request
    USART1_SendString("Sending HTTP request...\r\n");
    ret = send(TCP_CLIENT_SOCKET, tx_buffer, strlen((char*) tx_buffer));
    if(ret > 0)
    {
      USART1_SendString("HTTP request sent\r\n");
    }
    else
    {
      USART1_SendString("Send failed\r\n");
      return;
    }

    // Receive response
    USART1_SendString("Receiving weather data...\r\n\r\n");

    while(1)
    {
      ret = recv(TCP_CLIENT_SOCKET, rx_buffer, sizeof(rx_buffer) - 1);
      if(ret <= 0)
        break;

      rx_buffer[ret] = '\0';
      USART1_SendString((char*) rx_buffer);
      USART1_SendString("\r\n");
    }

    USART1_SendString("\r\n=== HTTP Request Complete ===\r\n");
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
