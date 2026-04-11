# STM32 TCP Client using W5500 

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

## Video Demonstration
### Harware Configuration

https://github.com/user-attachments/assets/f9f8ae45-e85a-424b-948c-cd164cb4a6c3

Shows harware connections. Button push to cycle through different display mode: Location → Temperature → Wind

### Hterm UART Debug

https://github.com/user-attachments/assets/03e0ca1d-9dd7-43e1-872d-498961c95909

Uart debug shows getting ip from DHCP and getting data from meteo weather site

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
| | PB7 (SDA) | SDA | I2C1 Data |
| | 5V | VCC | Power |
| | GND | GND | Common ground |
| **I2C LCD** | PA0 | GPIO Input | Button |
| | GND | GND | Common ground |
| **USART1(Debug)** | PA9 | TX to USB-Serial RX | 115200 baud, 8-N-1 |
| | PA10 | RX to USB-Serial TX | Optional for commands |

## Related Projects 
- [STM32_W5500_TCP_Server (Non-RTOS version)](https://github.com/rubin-khadka/STM32_W5500_TCP_Server)
- [STM32_FreeRTOS_W5500_TCP_Server](https://github.com/rubin-khadka/STM32_FreeRTOS_W5500_TCP_Server)

## Resources
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
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