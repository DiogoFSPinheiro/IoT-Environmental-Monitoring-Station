# AVR I/O Register Model

Reference for bare-metal GPIO and timers on the ATmega328P (Arduino Uno R3).
This replaces Arduino's `pinMode()`, `digitalWrite()`, `digitalRead()`, `micros()`, and `delay()` abstractions.

---

## The Three GPIO Registers

Every GPIO pin is controlled by three registers, one set per port (B, C, D):

| Register | Full Name               | Purpose                                      |
|----------|-------------------------|----------------------------------------------|
| `DDRx`   | Data Direction Register | `1` = OUTPUT, `0` = INPUT                   |
| `PORTx`  | Output / Pull-up        | Sets output level, or enables pull-up on input |
| `PINx`   | Input                   | Read-only — current state of the pin         |

The `x` is the port letter: `DDRB`, `DDRC`, `DDRD`, etc.

---

## Pin Mapping — Arduino Uno R3

Digital pins 0–7 map to **Port D**. Bit number = Arduino pin number.

```
DDRD / PORTD / PIND bit layout:
  bit:  7    6    5    4    3    2    1    0
  pin: D7   D6   D5   D4   D3   D2   D1   D0
```

Digital pins 8–13 map to **Port B** (bits 0–5).  
Analog pins A0–A5 map to **Port C** (bits 0–5).

---

## The Four GPIO Operations

### Set pin as OUTPUT
```cpp
// Arduino:     pinMode(2, OUTPUT);
DDRD |= (1 << PD2);
```

### Set pin as INPUT with pull-up
```cpp
// Arduino:     pinMode(2, INPUT_PULLUP);
DDRD  &= ~(1 << PD2);   // clear bit → INPUT
PORTD |=  (1 << PD2);   // set bit in PORT → enables internal pull-up
```

### Set pin as INPUT without pull-up
```cpp
// Arduino:     pinMode(2, INPUT);
DDRD  &= ~(1 << PD2);   // clear bit → INPUT
PORTD &= ~(1 << PD2);   // clear bit in PORT → no pull-up (floating)
```

> Use this when the circuit has its own external pull-down (e.g. HC-SR501 PIR module).
> Enabling the internal pull-up on a pin that already has an external pull-down wastes
> current and may affect signal levels.

### Drive pin LOW
```cpp
// Arduino:     digitalWrite(2, LOW);
PORTD &= ~(1 << PD2);
```

### Drive pin HIGH
```cpp
// Arduino:     digitalWrite(2, HIGH);
PORTD |= (1 << PD2);
```

### Read pin state
```cpp
// Arduino:     digitalRead(2)  →  0 or 1
(PIND >> PD2) & 1
```

---

## The Four Bit Manipulation Primitives

These cover almost everything in embedded register work:

```cpp
reg |=  (1 << n);   // set bit n
reg &= ~(1 << n);   // clear bit n
reg ^=  (1 << n);   // toggle bit n
(reg >> n) & 1      // read bit n
```

`1 << PD2` is simply `0b00000100` — a mask with only bit 2 set.  
`PD2` is defined as `2` in `<avr/io.h>`.

---

## Why `|=` and `&=~` Instead of `=`

Writing `DDRD = (1 << PD2)` would clobber all 8 bits simultaneously — accidentally
changing the direction of D0, D1, D3, and every other pin on Port D.

`|=` sets a single bit without touching the others.  
`&= ~(mask)` clears a single bit without touching the others.

---

## Why `volatile uint8_t *` for Register Pointers

When passing a register to a function, the pointer must be `volatile`:

```cpp
static bool wait_for(volatile uint8_t *pin_reg, uint8_t mask, ...) {
    while ((*pin_reg & mask) == 0) { ... }
}

// Call:
wait_for(&PIND, (1 << PD2), ...);
```

`volatile` tells the compiler: "this memory location can change outside your control —
hardware is writing it." Without it, the compiler may cache the register value in a CPU
register on the first read and loop forever on that stale value instead of re-reading it.

---

## Pin Macro Pattern

Rather than scattering `DDRD`, `PORTD`, `PIND`, and `PD2` across driver code,
define them together in a single place (`pins.h`):

```cpp
// pins.h
#define DHT22_DDR   DDRD
#define DHT22_PORT  PORTD
#define DHT22_PINR  PIND
#define DHT22_BIT   PD2
```

Usage in the driver:
```cpp
DHT22_DDR  |=  (1 << DHT22_BIT);   // OUTPUT
DHT22_PORT &= ~(1 << DHT22_BIT);   // LOW
DHT22_DDR  &= ~(1 << DHT22_BIT);   // INPUT
DHT22_PORT |=  (1 << DHT22_BIT);   // pull-up on

wait_for(&DHT22_PINR, (1 << DHT22_BIT), LOW, 100);
```

Moving the sensor to a different pin means changing 4 lines in `pins.h` — nothing else.

> **Why macros and not `constexpr`?**  
> Register addresses are `volatile` hardware locations. You cannot take a `constexpr`
> pointer to a `volatile` variable. Macros are the correct and idiomatic tool here.

---

## GPIO Quick Reference

| Arduino call                    | Bare-metal equivalent                                      |
|---------------------------------|------------------------------------------------------------|
| `pinMode(2, OUTPUT)`            | `DDRD \|= (1 << PD2)`                                               |
| `pinMode(2, INPUT_PULLUP)`      | `DDRD &= ~(1 << PD2); PORTD \|= (1 << PD2)`                        |
| `pinMode(2, INPUT)`             | `DDRD &= ~(1 << PD2); PORTD &= ~(1 << PD2)`                        |
| `digitalWrite(2, LOW)`          | `PORTD &= ~(1 << PD2)`                                              |
| `digitalWrite(2, HIGH)`         | `PORTD \|= (1 << PD2)`                                              |
| `digitalRead(2)`                | `(PIND >> PD2) & 1`                                                 |

---

## Timer1 — Free-Running Microsecond Counter

The ATmega328P has three hardware timers. Timer1 is 16-bit and well suited for
microsecond-resolution timing without ISR overhead.

### Timer registers

| Register | Purpose |
|----------|---------|
| `TCCR1A` | Control register A — waveform generation mode |
| `TCCR1B` | Control register B — clock source and prescaler |
| `TCNT1`  | The actual 16-bit counter value (read/write) |

### Prescaler selection — `TCCR1B` bits CS12:CS10

| CS12 | CS11 | CS10 | Clock source         | Tick period |
|------|------|------|----------------------|-------------|
| 0    | 0    | 0    | Timer stopped        | —           |
| 0    | 0    | 1    | No prescaler (16MHz) | 62.5 ns     |
| 0    | 1    | 0    | Prescaler 8 (2MHz)   | **0.5 µs**  |
| 0    | 1    | 1    | Prescaler 64         | 4 µs        |
| 1    | 0    | 0    | Prescaler 256        | 16 µs       |
| 1    | 0    | 1    | Prescaler 1024       | 64 µs       |

### Setup — free-running at 2MHz (0.5 µs per tick)

```cpp
#include <avr/io.h>

TCCR1A = 0;              // normal mode, no PWM
TCCR1B = (1 << CS11);   // prescaler 8 → 2 MHz
```

`TCCR1A = 0` disables all PWM/waveform output modes — the counter just counts freely.  
`CS11 = 1` with `CS12 = CS10 = 0` selects prescaler 8.

### Reading microseconds

```cpp
static inline uint16_t timer1_micros()
{
    return TCNT1 >> 1;   // 2 ticks per µs → divide by 2
}
```

`TCNT1 >> 1` is a single right-shift instruction on AVR — no division overhead.

### Overflow and wraparound

Timer1 at 2MHz overflows `0xFFFF → 0x0000` every **32.768 ms**.
This is fine for DHT22 timing (longest timeout: ~150 µs).

Timeout checks must use the **same unsigned type** as `timer1_micros()` so that
wraparound subtraction works correctly:

```cpp
uint16_t start = timer1_micros();
// ... later ...
if ((uint16_t)(timer1_micros() - start) >= timeout_us)   // correct wraparound
```

If `start = 32760` and the counter wraps to `10`, then `(uint16_t)(10 - 32760) = 14` —
the correct elapsed time. Casting to a wider type before subtracting would break this.

### Fixed delays — `_delay_ms()` / `_delay_us()`

For fixed blocking delays (e.g., the DHT22 1ms start signal) use `<util/delay.h>`.
These are pure compile-time busy-wait loops — no timer, no ISR, no Arduino needed.

```cpp
#include <util/delay.h>

_delay_ms(1);      // replaces delay(1)
_delay_us(40);     // replaces delayMicroseconds(40)
```

Requires `F_CPU` to be defined (PlatformIO sets this automatically for the Uno: `16000000UL`).

### Timer map — Arduino Uno R3

| Timer  | Bits | Used by Arduino           | Status in this project   |
|--------|------|---------------------------|--------------------------|
| Timer0 | 8    | `millis()` / `micros()`   | free (Arduino dropped)   |
| Timer1 | 16   | Servo, PWM pins 9/10      | DHT22 µs counter         |
| Timer2 | 8    | `tone()`, PWM pins 3/11   | free                     |
| WDT    | —    | Watchdog                  | FreeRTOS tick source     |
| TWI    | —    | I2C hardware (SDA/SCL)    | BH1750                   |
