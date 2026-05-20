#include "at21cs01.h"
#include "../debug/log.h"
#include <Arduino.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>

static constexpr uint8_t  SWI_PIN  = 17;           // GP17 — docs/RP2350 PINMAP.md
static constexpr uint32_t SWI_MASK = 1u << SWI_PIN;

// Timing: High-Speed mode (device default after every reset), AT21CS01 datasheet §1.5
static constexpr uint32_t T_BIT_US   = 20;  // bit frame duration        (spec max 25 µs)
static constexpr uint32_t T_RESET_US = 175; // reset pulse                (spec min 150 µs tDSCHG)
static constexpr uint32_t T_RRT_US   = 10;  // recovery after reset       (spec min 8 µs)
static constexpr uint32_t T_DRR_US   = 1;   // discovery request low      (spec 1–2 µs)
static constexpr uint32_t T_MSDR_US  = 6;   // tMSDR poll window          (spec 2–6 µs from DRR start)
static constexpr uint32_t T_DACK_US  = 24;  // tDACK max to wait out      (spec 8–24 µs)
static constexpr uint32_t T_HTSS_US  = 160; // start/stop idle high time  (spec min 150 µs)
static constexpr uint32_t T_LOW0_US  = 10;  // write '0' low time         (spec 6–16 µs)
static constexpr uint32_t T_LOW1_US  = 2;   // write '1' low time         (spec 1–2 µs)
static constexpr uint32_t T_RD_US    = 1;   // read strobe                (spec 1–2 µs)
static constexpr uint32_t T_WR_MS    = 6;   // write cycle margin         (spec max 5 ms)

// Device address base bytes — opcode in [7:4], slave addr A2:A1:A0 in [3:1], R/W in [0].
static constexpr uint8_t SLAVE_ADDR = 0x00;
static constexpr uint8_t DEV_EEPROM = 0xA0 | (SLAVE_ADDR << 1);  // opcode Ah
static constexpr uint8_t DEV_SECREG = 0xB0 | (SLAVE_ADDR << 1);  // opcode Bh

// Open-drain: output latch is permanently 0; toggling direction drives/releases the line.
// Direct SIO OE register manipulation was unreliable on RP2350 A4 silicon.
static inline void pinLow()     { gpio_set_dir(SWI_PIN, GPIO_OUT); }
static inline void pinRelease() { gpio_set_dir(SWI_PIN, GPIO_IN); }
static inline bool pinRead()    { return (sio_hw->gpio_in & SWI_MASK) != 0; }

// ---------------------------------------------------------------------------
// Sub-µs timing via DWT cycle counter (Cortex-M33)
// ---------------------------------------------------------------------------
#define DEMCR        (*(volatile uint32_t *)0xE000EDFCu)
#define DEMCR_TRCENA (1u << 24)
#define DWT_CTRL     (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT   (*(volatile uint32_t *)0xE0001004u)
#define DWT_CYCCNTENA (1u << 0)

static uint32_t _cpus;  // CPU cycles per microsecond

static void timingInit() {
    _cpus     = clock_get_hz(clk_sys) / 1000000u;
    DEMCR    |= DEMCR_TRCENA;
    DWT_CTRL |= DWT_CYCCNTENA;
}

static inline uint32_t cyc() { return DWT_CYCCNT; }
static inline void delayCyc(uint32_t n) { uint32_t t = cyc(); while ((cyc() - t) < n); }
static inline void delayUs(uint32_t us) { delayCyc(us * _cpus); }

// ---------------------------------------------------------------------------
// Low-level protocol
// ---------------------------------------------------------------------------

bool AT21CS01Driver::resetAndDiscover() {
    pinLow();
    delayUs(T_RESET_US);
    pinRelease();
    delayUs(T_RRT_US);

    // Discovery: master pulses low for tDRR, then polls through the full tMSDR
    // window. tDACK min (8 µs) > tMSDR max (6 µs), so any low seen here is a
    // genuine ACK regardless of where in the window the device responds.
    noInterrupts();
    uint32_t t0 = cyc();
    pinLow();
    delayCyc(T_DRR_US * _cpus);
    pinRelease();

    bool present = false;
    while ((cyc() - t0) < T_MSDR_US * _cpus) {
        if (!pinRead()) { present = true; break; }
    }
    interrupts();

    // Wait out full tDACK from DRR start before continuing.
    while ((cyc() - t0) < T_DACK_US * _cpus);

    // tHTSS: hold line high — this constitutes an implicit start condition so
    // the caller can send the first byte immediately without a separate start().
    delayUs(T_HTSS_US);
    return present;
}

void AT21CS01Driver::start() {
    pinRelease();
    delayUs(T_HTSS_US);
}

void AT21CS01Driver::stop() {
    pinRelease();
    delayUs(T_HTSS_US);
}

void AT21CS01Driver::sendBit(bool b) {
    uint32_t frameCyc = T_BIT_US * _cpus;
    uint32_t lowCyc   = (b ? T_LOW1_US : T_LOW0_US) * _cpus;
    uint32_t tFrame   = cyc();
    pinLow();
    uint32_t tLow = cyc();  // start measuring low time after pin is driven
    while ((cyc() - tLow) < lowCyc);
    pinRelease();
    while ((cyc() - tFrame) < frameCyc);  // wait out full bit frame from start
}

bool AT21CS01Driver::recvBit() {
    uint32_t tFrame = cyc();
    pinLow();
    uint32_t tLow = cyc();
    while ((cyc() - tLow) < _cpus);  // 1 µs tRD strobe from when pin went low
    pinRelease();
    delayCyc(_cpus / 5);              // ~200 ns tPUP — line settles before sample
    bool val = pinRead();
    while ((cyc() - tFrame) < T_BIT_US * _cpus);
    return val;
}

bool AT21CS01Driver::sendByte(uint8_t b) {
    noInterrupts();
    for (int8_t i = 7; i >= 0; i--)
        sendBit((b >> i) & 1u);
    bool ack = !recvBit();  // device ACK = '0' → true; NACK = '1' → false
    interrupts();
    return ack;
}

uint8_t AT21CS01Driver::recvByte(bool ack) {
    noInterrupts();
    uint8_t b = 0;
    for (int8_t i = 7; i >= 0; i--)
        b |= (recvBit() ? 1u : 0u) << i;
    sendBit(!ack);  // master ACK='0' to continue, NACK='1' to stop
    interrupts();
    return b;
}

// ---------------------------------------------------------------------------
// Transaction helpers
// ---------------------------------------------------------------------------

// Random read: dummy-write to set address pointer, then repeated start (tHTSS)
// before the read phase. resetAndDiscover() already ends with tHTSS, so no
// explicit start() is needed before phase 1.
bool AT21CS01Driver::readTransaction(uint8_t devBase, uint8_t addr,
                                     uint8_t* buf, uint8_t len) {
    // Phase 1: write address pointer (resetAndDiscover ended with tHTSS)
    if (!sendByte(devBase | 0)) { LOG_E("eeprom: NACK on addr-write (devBase=0x%02X)", devBase); return false; }
    if (!sendByte(addr))        { LOG_E("eeprom: NACK on addr byte (addr=0x%02X)", addr);         return false; }

    // Repeated start (tHTSS) + read phase
    start();
    if (!sendByte(devBase | 1)) { LOG_E("eeprom: NACK on addr-read (devBase=0x%02X)", devBase);  return false; }
    for (uint8_t i = 0; i < len; i++)
        buf[i] = recvByte(i < len - 1u);
    stop();
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AT21CS01Driver::begin() {
    timingInit();
    gpio_init(SWI_PIN);                  // set function = SIO, direction = IN
    gpio_put(SWI_PIN, 0);               // output latch = 0; never changes
    // Direction stays IN (high-Z). pinLow() enables OE; pinRelease() disables it.
}

bool AT21CS01Driver::isPresent() {
    return pinRead();
}

bool AT21CS01Driver::ping() {
    return resetAndDiscover();
}

bool AT21CS01Driver::readSerial(uint8_t serial[8]) {
    if (!resetAndDiscover()) { LOG_E("eeprom: serial: discovery failed"); return false; }
    return readTransaction(DEV_SECREG, 0x00, serial, 8);
}

bool AT21CS01Driver::read(uint8_t addr, uint8_t* buf, uint8_t len) {
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        if (resetAndDiscover() && readTransaction(DEV_EEPROM, addr, buf, len))
            return true;
        LOG_W("eeprom: read attempt %u failed, retrying", attempt + 1);
    }
    LOG_E("eeprom: read() failed after 3 attempts");
    return false;
}

void AT21CS01Driver::scanAddresses() {
    LOG_I("eeprom: scanning all 8 slave addresses...");
    for (uint8_t sa = 0; sa < 8; sa++) {
        uint8_t devBase = 0xA0 | (sa << 1);
        if (!resetAndDiscover()) { LOG_E("eeprom: scan: discovery failed at sa=%u", sa); continue; }
        bool ack = sendByte(devBase | 0);
        stop();
        LOG_I("eeprom: sa=%u (devBase=0x%02X) -> %s", sa, devBase, ack ? "ACK" : "NACK");
    }
}

bool AT21CS01Driver::write(uint8_t addr, const uint8_t* buf, uint8_t len) {
    while (len > 0) {
        uint8_t chunk = static_cast<uint8_t>(((addr & ~0x07u) + 8u) - addr);
        if (chunk > len) chunk = len;

        if (!resetAndDiscover()) return false;
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
