#pragma once

#include <stdint.h>
#include <avr/io.h>

// One-wire sensor — D2 = Port D, bit 2
constexpr uint8_t PIN_DHT22 = 2;
#define DHT22_DDR   DDRD
#define DHT22_PORT  PORTD
#define DHT22_PINR  PIND
#define DHT22_BIT   PD2

// PIR motion sensor — D3 = Port D, bit 3
constexpr uint8_t PIN_PIR = 3;
#define PIR_DDR     DDRD
#define PIR_PORT    PORTD
#define PIR_PINR    PIND
#define PIR_BIT     PD3

// BH1750FVI uses hardware I2C: SDA = A4, SCL = A5 (fixed on Uno R3)
