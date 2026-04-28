# Nice Nano V2 Pinout for XIAORD Module

This document describes the GPIO pin configuration for running the XIAORD module on a Nice Nano V2 board.

## Pin Configuration Summary

### SPI Display (GC9A01 Round Display)

| Function | Nice Nano Pin | GPIO | Purpose |
|----------|---|---|---|
| SCK | P0.22 | gpio0 22 | SPI Clock |
| MOSI | P0.24 | gpio0 24 | SPI Data Out (Master Out) |
| MISO | P0.11 | gpio0 11 | SPI Data In (Master In) |
| CS | P0.09 | gpio0 9 | Chip Select |
| DC (Data/Command) | P0.10 | gpio0 10 | Display Data/Command Control |
| Reset | P1.00 | gpio1 0 | Display Reset |

### I2C Sensors & Touch (I2C0)

| Function | Nice Nano Pin | GPIO | I2C Address | Purpose |
|----------|---|---|---|---|
| SDA | P0.08 | gpio0 8 | — | I2C Data |
| SCL | P0.06 | gpio0 6 | — | I2C Clock |
| Touch IRQ | P1.04 | gpio1 4 | 0x15 | Touch Controller Interrupt (CST816S) |
| Touch Reset | P1.06 | gpio1 6 | 0x15 | Touch Controller Reset |
| RTC ISW | P0.17 | gpio0 17 | 0x68 | RTC Interrupt Switch (DS3231) |
| BH1750 | — | — | 0x23 | Light Sensor (I2C) |

### Backlight Control

| Function | Nice Nano Pin | GPIO | Purpose |
|----------|---|---|---|
| Backlight | P0.20 | gpio0 20 | Display Backlight LED Control (GPIO) |

## Peripherals Used

- **SPI3**: Display interface (GC9A01)
- **I2C0**: Touch, RTC, and light sensor
- **GPIO**: Backlight and control pins

## Notes

- All pins use **GPIO_ACTIVE_HIGH** except for reset and interrupt pins which use **GPIO_ACTIVE_LOW**
- SPI operates at up to 32 MHz
- I2C operates at Fast mode (400 kHz)
- The Nice Nano V2 uses the Nordic nRF52840 microcontroller, same as XIAO BLE
- UART0 is disabled to free pins for the display module

## Wiring Guide

When physically wiring the display module to Nice Nano V2:

1. **Power**: Connect VCC to 3.3V, GND to ground
2. **SPI Lines**: Connect display SPI pins to P0.22 (SCK), P0.24 (MOSI), P0.11 (MISO)
3. **Control Pins**: DC→P0.10, CS→P0.09, Reset→P1.00
4. **I2C Lines**: Connect to P0.08 (SDA), P0.06 (SCL)
5. **Touch**: IRQ→P1.04, Reset→P1.06
6. **Backlight**: Connect to P0.20
7. **RTC**: Separate I2C device at address 0x68 with interrupt at P0.17

Display Backlight (BL): Controlled by D6.
Touch Screen Interrupt (INT): Connected to D7.
Touch Screen Reset (RST): Connected to D3.
Touch Screen I2C (SDA/SCL): Connected to D4/D5.
SD Card CS: Connected to D2.
LCD SPI (SCK/MOSI): Connected to D8/D10.
LCD DC/CS/RST: Connected to D1/D9/D0


Description                     |Xiao Pinout    |Nice Nano V2 Pinout
______________________________________________________________________
LCD RST                         |D0             |P1.00
______________________________________________________________________
LCD DC                          |D1             |P0.10
______________________________________________________________________
SD Card CS                      |D2             |P0.17
______________________________________________________________________
Touch Screen Reset (RST)        |D3             |P1.06
______________________________________________________________________
Touch Screen I2C SDA            |D4             |P0.08 
______________________________________________________________________
Touch Screen I2C SCL            |D5             |P0.06
______________________________________________________________________
Display Backlight (BL)          |D6             |P0.20
______________________________________________________________________
Touch Screen Interrupt (INT)    |D7             |P1.04
______________________________________________________________________
LCD SPI SCK                     |D8             |P0.22 
______________________________________________________________________
LCD CS                          |D9             |P0.09
______________________________________________________________________
LCD SPI MOSI                    |D10            |P0.24
