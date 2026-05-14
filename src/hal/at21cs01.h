#pragma once
#include <stdint.h>

class AT21CS01Driver {
public:
    void begin();

    // Fast presence check: returns true if SI/O is high.
    // The pull-up is on the adapter board, so a high line means adapter is seated.
    // No internal pull-up is set on the pin — a floating line (no adapter) reads low.
    bool isPresent();

    // Full Reset + Discovery Response: returns true if device acknowledges.
    // Required before read() / write(); also used on first adapter detection.
    bool ping();

    // Read len bytes from EEPROM starting at addr.
    bool read(uint8_t addr, uint8_t* buf, uint8_t len);

    // Write len bytes to EEPROM starting at addr.
    // Handles page boundaries (8 bytes/page) automatically.
    // Blocks up to 6 ms per page for the self-timed write cycle.
    bool write(uint8_t addr, const uint8_t* buf, uint8_t len);

private:
    bool    resetAndDiscover();
    bool    readTransaction(uint8_t devBase, uint8_t addr, uint8_t* buf, uint8_t len);
    void    sendBit(bool b);
    bool    recvBit();
    bool    sendByte(uint8_t b);
    uint8_t recvByte(bool ack);
    void    start();
    void    stop();
};
