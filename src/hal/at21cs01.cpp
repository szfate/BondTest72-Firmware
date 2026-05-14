#include "at21cs01.h"
#include <Arduino.h>

static constexpr uint8_t SWI_PIN = 17;  // GP17 — docs/RP2350 PINMAP.md

// Timing: High-Speed mode (device default after every reset), AT21CS01 datasheet §1.5
static constexpr uint32_t T_RESET_US = 60;   // min 48 µs  — reset pulse
static constexpr uint32_t T_RRT_US   = 10;   // min 8 µs   — recovery after reset
static constexpr uint32_t T_DRR_US   = 1;    // 1–2 µs     — discovery request low
static constexpr uint32_t T_MSDR_US  = 2;    // 2–6 µs     — sample window into tDACK
static constexpr uint32_t T_DACK_US  = 24;   // max tDACK  — wait for device to release
static constexpr uint32_t T_HTSS_US  = 160;  // min 150 µs — Start / Stop high time
static constexpr uint32_t T_LOW0_US  = 8;    // 6–16 µs    — write '0' (and ACK)
static constexpr uint32_t T_LOW1_US  = 1;    // 1–2 µs     — write '1'
static constexpr uint32_t T_RD_US    = 1;    // 1–2 µs     — read strobe
static constexpr uint32_t T_RCV_US   = 3;    // min 2 µs   — inter-bit recovery
static constexpr uint32_t T_WR_MS    = 6;    // max 5 ms write cycle + margin

// Device address base bytes — opcode in [7:4], slave addr A2:A1:A0 in [3:1], R/W in [0].
// Slave address bits are factory-programmed; 0x00 is the default ordering code.
static constexpr uint8_t SLAVE_ADDR    = 0x00;
static constexpr uint8_t DEV_EEPROM    = 0xA0 | (SLAVE_ADDR << 1);  // opcode Ah
static constexpr uint8_t DEV_SECREG    = 0xB0 | (SLAVE_ADDR << 1);  // opcode Bh

// Open-drain helpers: drive = output+low; release = high-Z (external pullup → high)
static inline void pinLow()     { pinMode(SWI_PIN, OUTPUT); digitalWrite(SWI_PIN, LOW); }
static inline void pinRelease() { pinMode(SWI_PIN, INPUT); }
static inline bool pinRead()    { return digitalRead(SWI_PIN); }

// ---------------------------------------------------------------------------
// Low-level protocol
// ---------------------------------------------------------------------------

bool AT21CS01Driver::resetAndDiscover() {
    pinLow();
    delayMicroseconds(T_RESET_US);
    pinRelease();
    delayMicroseconds(T_RRT_US);
    // Discovery request: master pulls low briefly; device mirrors low if present
    pinLow();
    delayMicroseconds(T_DRR_US);
    pinRelease();
    delayMicroseconds(T_MSDR_US);
    bool present = !pinRead();       // device holds low → ACK → present
    delayMicroseconds(T_DACK_US);    // wait for device to release
    return present;
}

void AT21CS01Driver::start() {
    // Start condition: SI/O high for tHTSS. Line is already high after
    // resetAndDiscover() or after any previous stop/bit recovery.
    pinRelease();
    delayMicroseconds(T_HTSS_US);
}

void AT21CS01Driver::stop() {
    pinRelease();
    delayMicroseconds(T_HTSS_US);
}

void AT21CS01Driver::sendBit(bool b) {
    pinLow();
    delayMicroseconds(b ? T_LOW1_US : T_LOW0_US);
    pinRelease();
    delayMicroseconds(T_RCV_US);
}

bool AT21CS01Driver::recvBit() {
    pinLow();
    delayMicroseconds(T_RD_US);
    pinRelease();
    // tPUP: allow line to charge for a '1' before sampling.
    // pinMode() overhead (~430 ns) provides most of this; add a small margin.
    // Tune if pullup RC is slow (larger resistor or more bus capacitance).
    delayMicroseconds(1);
    bool bit = pinRead();
    delayMicroseconds(T_RCV_US);
    return bit;
}

bool AT21CS01Driver::sendByte(uint8_t b) {
    for (int8_t i = 7; i >= 0; i--)
        sendBit((b >> i) & 1u);
    return !recvBit();  // device ACK = '0' → true; NACK = '1' → false
}

uint8_t AT21CS01Driver::recvByte(bool ack) {
    uint8_t b = 0;
    for (int8_t i = 7; i >= 0; i--)
        b |= (recvBit() ? 1u : 0u) << i;
    sendBit(!ack);  // master sends ACK='0' to continue, NACK='1' to stop
    return b;
}

// ---------------------------------------------------------------------------
// Transaction helpers (assume resetAndDiscover() already called)
// ---------------------------------------------------------------------------

// Dummy-write to set address pointer, then restart and read len bytes.
// devBase: DEV_EEPROM or DEV_SECREG (R/W bit will be set internally).
bool AT21CS01Driver::readTransaction(uint8_t devBase, uint8_t addr,
                                     uint8_t* buf, uint8_t len) {
    start();
    if (!sendByte(devBase | 0)) return false;  // write to load address pointer
    if (!sendByte(addr))        return false;
    start();                                    // restart
    if (!sendByte(devBase | 1)) return false;  // switch to read
    for (uint8_t i = 0; i < len; i++)
        buf[i] = recvByte(i < len - 1u);       // NACK on last byte
    stop();
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AT21CS01Driver::begin() {
    // Plain INPUT — no internal pull-up. The adapter board provides the pull-up,
    // so without an adapter the line floats low and isPresent() correctly returns false.
    pinRelease();
}

bool AT21CS01Driver::isPresent() {
    return pinRead();
}

bool AT21CS01Driver::ping() {
    return resetAndDiscover();
}

bool AT21CS01Driver::read(uint8_t addr, uint8_t* buf, uint8_t len) {
    if (!resetAndDiscover()) return false;
    return readTransaction(DEV_EEPROM, addr, buf, len);
}

bool AT21CS01Driver::write(uint8_t addr, const uint8_t* buf, uint8_t len) {
    while (len > 0) {
        // Clamp chunk to current 8-byte page boundary
        uint8_t chunk = static_cast<uint8_t>(((addr & ~0x07u) + 8u) - addr);
        if (chunk > len) chunk = len;

        if (!resetAndDiscover()) return false;
        start();
        if (!sendByte(DEV_EEPROM | 0)) return false;
        if (!sendByte(addr))           return false;
        for (uint8_t i = 0; i < chunk; i++)
            if (!sendByte(buf[i])) return false;
        stop();
        delay(T_WR_MS);  // self-timed write cycle (max 5 ms)

        addr += chunk;
        buf  += chunk;
        len  -= chunk;
    }
    return true;
}
