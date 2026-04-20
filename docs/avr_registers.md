# AVR I/O Register Model

Reference for bare-metal GPIO on the ATmega328P (Arduino Uno R3).
This replaces Arduino's `pinMode()`, `digitalWrite()`, and `digitalRead()` abstractions.

---

## The Three Registers

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

## The Four Operations

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

## Quick Reference

| Arduino call                    | Bare-metal equivalent                                      |
|---------------------------------|------------------------------------------------------------|
| `pinMode(2, OUTPUT)`            | `DDRD \|= (1 << PD2)`                                     |
| `pinMode(2, INPUT_PULLUP)`      | `DDRD &= ~(1 << PD2); PORTD \|= (1 << PD2)`              |
| `digitalWrite(2, LOW)`          | `PORTD &= ~(1 << PD2)`                                    |
| `digitalWrite(2, HIGH)`         | `PORTD \|= (1 << PD2)`                                    |
| `digitalRead(2)`                | `(PIND >> PD2) & 1`                                       |
