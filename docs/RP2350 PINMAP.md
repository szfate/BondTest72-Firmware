# RP2350 (Pico 2) Pin Map

Pins listed in physical order (pin 1 = top-left with USB facing up).
Internal-only pins (GP23, GP24, GP25, GP29) included for completeness.

| Pin | GPIO | SPI | UART | I2C | ADC | Assigned To | Notes |
|-----|------|-----|------|-----|-----|-------------|-------|
| 1 | GP0 | SPI0 RX | UART0 TX | I2C0 SDA | — | START | active low, use internal pullup |
| 2 | GP1 | SPI0 CSn | UART0 RX | I2C0 SCL | — | LED_DIN | |
| 3 | GND | — | — | — | — | GND | |
| 4 | GP2 | SPI0 SCK | UART0 CTS | I2C1 SDA | — | GP2 | Breakout header |
| 5 | GP3 | SPI0 TX | UART0 RTS | I2C1 SCL | — | GP3 | Breakout header |
| 6 | GP4 | SPI0 RX | UART1 TX | I2C0 SDA | — | STB3 | |
| 7 | GP5 | SPI0 CSn | UART1 RX | I2C0 SCL | — | STB2 | |
| 8 | GND | — | — | — | — | GND | |
| 9 | GP6 | SPI0 SCK | UART1 CTS | I2C1 SDA | — | - | |
| 10 | GP7 | SPI0 TX | UART1 RTS | I2C1 SCL | — | RST | RST connected to all CH446X chips |
| 11 | GP8 | SPI1 RX | UART1 TX | I2C0 SDA | — | DAT3 | |
| 12 | GP9 | SPI1 CSn | UART1 RX | I2C0 SCL | — | CLK3 | |
| 13 | GND | — | — | — | — | GND | |
| 14 | GP10 | SPI1 SCK | UART1 CTS | I2C1 SDA | — | DAT2 | |
| 15 | GP11 | SPI1 TX | UART1 RTS | I2C1 SCL | — | CLK2 | |
| 16 | GP12 | SPI1 RX | UART0 TX | I2C0 SDA | — | STB1 | |
| 17 | GP13 | SPI1 CSn | UART0 RX | I2C0 SCL | — | DAT1 | |
| 18 | GND | — | — | — | — | GND | |
| 19 | GP14 | SPI1 SCK | UART0 CTS | I2C1 SDA | — | CLK1 | |
| 20 | GP15 | SPI1 TX | UART0 RTS | I2C1 SCL | — | CON0 | Adapter connector defined signal |
| 21 | GP16 | SPI0 RX | UART0 TX | I2C0 SDA | — | GP16 | Breakout header |
| 22 | GP17 | SPI0 CSn | UART0 RX | I2C0 SCL | — | CON6 | Adapter connector signal; dedicated for EEPROM |
| 23 | GND | — | — | — | — | GND | |
| 24 | GP18 | SPI0 SCK | UART0 CTS | I2C1 SDA | — | CON5 | Adapter connector defined signal |
| 25 | GP19 | SPI0 TX | UART0 RTS | I2C1 SCL | — | CON4 | Adapter connector defined signal |
| 26 | GP20 | SPI0 RX | UART1 TX | I2C0 SDA | — | CON3 | Adapter connector defined signal |
| 27 | GP21 | SPI0 CSn | UART1 RX | I2C0 SCL | — | CON2 | Adapter connector defined signal |
| 28 | GND | — | — | — | — | GND | |
| 29 | GP22 | SPI0 SCK | UART1 CTS | I2C1 SDA | — | CON1 | Adapter connector defined signal |
| 30 | RUN | — | — | — | — | Reset | Active-low reset |
| 31 | GP26 | SPI1 SCK | UART1 CTS | I2C1 SDA | ADC0 | COMD | Current source + sense |
| 32 | GP27 | SPI1 TX | UART1 RTS | I2C1 SCL | ADC1 | COMA | Previous neighbouring bond |
| 33 | AGND | — | — | — | — | Analog GND | |
| 34 | GP28 | SPI1 RX | UART0 TX | I2C0 SDA | ADC2 | COMC | Next neighbouring bond |
| 35 | ADC_VREF | — | — | — | — | ADC reference | Tie to 3V3 or external ref |
| 36 | 3V3 OUT | — | — | — | — | 3.3 V supply | 300 mA max |
| 37 | 3V3_EN | — | — | — | — | 3.3 V enable | Pull low to disable regulator |
| 38 | GND | — | — | — | — | GND | |
| 39 | VSYS | — | — | — | — | System power in | 1.8–5.5 V |
| 40 | VBUS | — | — | — | — | USB 5 V | |
| — | GP23 | — | — | — | — | (internal) | SMPS power mode control |
| — | GP24 | — | — | — | — | (internal) | VBUS sense |
| — | GP25 | — | — | — | — | (internal) | Onboard LED (LED_BUILTIN) |
| — | GP29 | — | — | — | ADC3 | (internal) | VSYS/3 monitoring |
