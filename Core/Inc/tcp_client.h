/*
 * tcp_client.h
 *
 *  Created on: Apr 8, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_TCP_CLIENT_H_
#define INC_TCP_CLIENT_H_

#include <stdint.h>
#include <stdbool.h>

// Socket configuration
#define TCP_CLIENT_SOCKET      0
#define DNS_SOCKET             6

// HTTP configuration
#define HTTP_PORT             80

// DNS server (Google DNS)
#define DNS_SERVER_IP          {8, 8, 8, 8}

// Weather API configuration
#define WEATHER_HOST           "api.open-meteo.com"
#define WEATHER_PATH           "/v1/forecast?latitude=52.52&longitude=13.41&current_weather=true"

// Buffer sizes
#define RX_BUFFER_SIZE         1024
#define TX_BUFFER_SIZE         512

// Function Prototypes
int TCP_Client_Init(void);
void TCP_Client_GetWeather(void);
void TCP_Client_Close(void);

#endif /* INC_TCP_CLIENT_H_ */
