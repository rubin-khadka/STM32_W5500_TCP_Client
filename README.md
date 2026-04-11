# STM32 TCP Client using W5500 - IoT Weather Station

[![STM32](https://img.shields.io/badge/STM32-F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![CubeIDE](https://img.shields.io/badge/IDE-STM32CubeIDE-darkblue)](http://st.com/en/development-tools/stm32cubeide.html)
[![EthernetW5500](https://img.shields.io/badge/Ethernet-W5500-darkgreen)](https://cdn.sparkfun.com/datasheets/Dev/Arduino/Shields/W5500_datasheet_v1.0.2_1.pdf)
[![API: Open-Meteo](https://img.shields.io/badge/API-Open--Meteo-red)](https://open-meteo.com/en/docs)

## Project Overview
This project implements a TCP Client on an STM32 Blue Pill (STM32F103C8T6) using the W5500 Ethernet module. It connects to the Open-Meteo weather API to fetch live weather data, parses the JSON response, and displays the information on a 16x2 I2C LCD. A push button allows the user to cycle through different data screens.

This project demonstrates a complete IoT workflow: DHCP → DNS → HTTP Request → JSON Parsing → Display.

**Key Features:**
- TCP Client connecting to a remote weather API (port 80)
- DHCP Client for automatic IP address assignment from a router or PC
- DNS Client to resolve `api.open-meteo.com` to an IP address
- HTTP GET Request to fetch current weather data (temperature, wind speed, direction, etc.)
- JSON Parsing to extract weather information from the API response
- 16x2 I2C LCD Display to show weather data
- Push Button to cycle through three display screens (Location, Temperature/Time, Wind)
- Periodic Updates (every five minute) to keep weather data fresh

## Video Demonstrations

### Hardware Configuration

https://github.com/user-attachments/assets/f9f8ae45-e85a-424b-948c-cd164cb4a6c3

Shows hardware connections. Button push to cycle through different display modes: Location → Temperature → Wind

### Hterm UART Debug

https://github.com/user-attachments/assets/03e0ca1d-9dd7-43e1-872d-498961c95909

UART debug shows getting IP from DHCP and receiving data from the Open-Meteo weather API.

## Project Schematic

<img width="1217" height="447" alt="Schematic Diagram" src="https://github.com/user-attachments/assets/a68d712b-8986-48c1-a645-bcf793d89a1b" />

## Pin Configuration
| Peripheral | Pin | Connection | Notes |
|------------|-----|------------|-------|
| **Wiznet W5500** | PA5 | SCK | SPI1 Clock |
| | PA6 | MISO | SPI1 Master In Slave Out |
| | PA7 | MOSI | SPI1 Master Out Slave In |
| | PA4 | CS | Chip Select |
| | PA3 | Reset | Reset Pin |
| | 3.3V | VCC | Power |
| | GND | GND | Common ground |
| **I2C LCD** | PB10 (SCL) | SCL | I2C1 Clock |
| | PB11 (SDA) | SDA | I2C1 Data |
| | 5V | VCC | Power |
| | GND | GND | Common ground |
| **Push Button** | PA0 | GPIO Input | Button |
| | GND | GND | Common ground |
| **USART1(Debug)** | PA9 | TX to USB-Serial RX | 115200 baud, 8-N-1 |
| | PA10 | RX to USB-Serial TX | Optional for commands |

## TCP Client

The TCP Client is the core network engine of this weather station. It handles all communication between the STM32 and the Open-Meteo weather server.

### How It Works
1. IP Address Acquisition (DHCP)
- When the STM32 powers on, the W5500 Ethernet controller requests an IP address from the local network router (or a PC with Internet Connection Sharing)
- The router automatically assigns an available IP address (e.g., `192.168.137.148`)
- This process is called DHCP (Dynamic Host Configuration Protocol) and eliminates the need for manual IP configuration

2. Domain Resolution (DNS)
- The weather API uses a human-readable domain name: `api.open-meteo.com` 
- Computers communicate using IP addresses, not domain names
- The DNS (Domain Name System) client sends a query to a DNS server (typically the router or Google's `8.8.8.8`)
- The DNS server responds with the corresponding IP address (e.g., `94.130.142.35`)

3. TCP Connection Establishment
- A TCP socket is created on the STM32
- The socket initiates a connection to the resolved IP address on port 80 (standard HTTP port)
- A three-way handshake (SYN, SYN-ACK, ACK) establishes a reliable connection with the weather server

4. HTTP Request Transmission
- An HTTP GET request is formatted and sent to the server
- The request includes:
    - The API endpoint (/v1/forecast?latitude=52.52&longitude=13.41&current_weather=true)
    - The Host header (api.open-meteo.com)
    - Connection: close (instructs the server to close the connection after responding)

5. Response Reception
- The server responds with a raw JSON payload containing current weather data 
- The response is received in chunks due to chunked transfer encoding
- All chunks are assembled into a complete JSON string

6. Connection Termination
- After receiving all data, the TCP connection is properly closed
- The socket is fully reset, making it ready for the next update cycle

## W5500 Ethernet Controller
The W5500 handles the entire TCP/IP stack in hardware, offloading network processing from the STM32. This project uses the Variable Data Mode (VDM) over SPI, which allows flexible, multi-byte data transfers to and from the chip's 32KB internal buffer. The SPI interface is configured in Mode 0 or 3 (clock polarity low, data sampled on the rising edge) for reliable communication.

Key W5500 configurations used:
- 8 independent hardware sockets (Socket 0 is used for the TCP client)
- Auto-negotiation enabled for 10/100 Mbps Ethernet
- Internal 32KB memory buffer for TX/RX packet processing

## Open-Meteo API
The HTTP GET request is made to the `/v1/forecast` endpoint. The specific parameters used are:

| Parameter | Value | Description |
|-----------|-------|-------------|
| `latitude` | `52.52` | Berlin latitude (can be changed for any location) |
| `longitude` | `13.41` | Berlin longitude |
| `current_weather` | `true` | Returns simplified `current_weather` object |

What the API returns:
- `temperature` – Current temperature in °C
- `windspeed` – Wind speed in km/h
- `winddirection` – Wind direction in degrees
- `weathercode` – WMO weather condition code (0 = clear sky, 1-3 = clouds, etc.)
- `time` – ISO8601 timestamp of the reading

> No API key is required for this level of usage, making Open-Meteo ideal for embedded projects.

## JSON Parsing Strategy
The raw JSON response is parsed using custom string search functions rather than a full JSON library. This approach:
- Minimizes code size (no external JSON library needed)
- Runs efficiently on the STM32's limited resources
- Extracts only the necessary fields (`temperature`, `windspeed`, `winddirection`, `weathercode`, `time`, `latitude`, `longitude`, `elevation`)

The parser locates the `current_weather` object in the response and extracts numeric values using manual character-by-character conversion.

## Related Projects 
- [STM32_W5500_TCP_Server (Non-RTOS version)](https://github.com/rubin-khadka/STM32_W5500_TCP_Server)
- [STM32_FreeRTOS_W5500_TCP_Server](https://github.com/rubin-khadka/STM32_FreeRTOS_W5500_TCP_Server)

## Resources
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [PCF8574 I2C Backpack Datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [WIZNET W5500 Datasheet](https://cdn.sparkfun.com/datasheets/Dev/Arduino/Shields/W5500_datasheet_v1.0.2_1.pdf)
- [Open-Meteo API Documentation](https://open-meteo.com/en/docs)

## Project Status
- **Status**: Complete
- **Version**: v1.0
- **Last Updated**: April 2026

## Contact
**Rubin Khadka Chhetri**  
📧 rubinkhadka84@gmail.com <br>
🐙 GitHub: https://github.com/rubin-khadka