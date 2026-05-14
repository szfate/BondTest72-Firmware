# CH446X Mux Map

Maps each logical DUT pad index (used in firmware) to the physical CH446X chip and
input channel determined by PCB routing.

This table is the source of truth for `src/hal/mux_map.h`.

## Reference

| Item | Detail |
|------|--------|
| Chips | U2 (CLK1/DAT1/STB1), U3 (CLK2/DAT2/STB2), U4 (CLK3/DAT3/STB3) |
| Inputs per chip | 24 (X0–X23) |
| Total channels | 72 |
| Bus assignment | Dynamic — set by firmware at runtime, not part of this map |

Fixed bus wiring (from schematic):

| CH446X Bus | COM pin | Role |
|------------|---------|------|
| BUS_D | COM_D | Injection + sense (27 kΩ pullup → GP26 / ADC0) |
| BUS_A | COM_A | Left neighbour sense (1 MΩ divider → GP27 / ADC1) |
| BUS_C | COM_C | Right neighbour sense (1 MΩ divider → GP28 / ADC2) |
| BUS_B | — | Tester GND (return path, no ADC) |
| BUS_E | — | Spare |

---

## Map Table

Logical pad index = CH net number from schematic.

| Logical Pad | Chip | Channel (X) | Net   |
|-------------|------|-------------|-------|
| 0           | U2   | 0           | CH0   |
| 1           | U2   | 16          | CH1   |
| 2           | U2   | 1           | CH2   |
| 3           | U2   | 2           | CH3   |
| 4           | U2   | 3           | CH4   |
| 5           | U2   | 17          | CH5   |
| 6           | U2   | 4           | CH6   |
| 7           | U2   | 5           | CH7   |
| 8           | U2   | 18          | CH8   |
| 9           | U2   | 19          | CH9   |
| 10          | U2   | 12          | CH10  |
| 11          | U2   | 13          | CH11  |
| 12          | U2   | 6           | CH12  |
| 13          | U2   | 14          | CH13  |
| 14          | U2   | 15          | CH14  |
| 15          | U3   | 0           | CH15  |
| 16          | U3   | 16          | CH16  |
| 17          | U3   | 1           | CH17  |
| 18          | U3   | 2           | CH18  |
| 19          | U3   | 3           | CH19  |
| 20          | U3   | 17          | CH20  |
| 21          | U3   | 4           | CH21  |
| 22          | U3   | 5           | CH22  |
| 23          | U3   | 18          | CH23  |
| 24          | U3   | 19          | CH24  |
| 25          | U3   | 12          | CH25  |
| 26          | U3   | 13          | CH26  |
| 27          | U3   | 6           | CH27  |
| 28          | U3   | 15          | CH28  |
| 29          | U3   | 14          | CH29  |
| 30          | U4   | 0           | CH30  |
| 31          | U4   | 16          | CH31  |
| 32          | U4   | 1           | CH32  |
| 33          | U4   | 2           | CH33  |
| 34          | U4   | 3           | CH34  |
| 35          | U4   | 17          | CH35  |
| 36          | U4   | 4           | CH36  |
| 37          | U4   | 5           | CH37  |
| 38          | U4   | 18          | CH38  |
| 39          | U4   | 19          | CH39  |
| 40          | U4   | 12          | CH40  |
| 41          | U4   | 23          | CH41  |
| 42          | U4   | 11          | CH42  |
| 43          | U4   | 10          | CH43  |
| 44          | U4   | 22          | CH44  |
| 45          | U4   | 21          | CH45  |
| 46          | U4   | 20          | CH46  |
| 47          | U4   | 9           | CH47  |
| 48          | U4   | 8           | CH48  |
| 49          | U4   | 7           | CH49  |
| 50          | U4   | 6           | CH50  |
| 51          | U4   | 15          | CH51  |
| 52          | U4   | 14          | CH52  |
| 53          | U4   | 13          | CH53  |
| 54          | U3   | 7           | CH54  |
| 55          | U3   | 8           | CH55  |
| 56          | U3   | 23          | CH56  |
| 57          | U3   | 11          | CH57  |
| 58          | U3   | 10          | CH58  |
| 59          | U3   | 9           | CH59  |
| 60          | U3   | 22          | CH60  |
| 61          | U3   | 21          | CH61  |
| 62          | U3   | 20          | CH62  |
| 63          | U2   | 7           | CH63  |
| 64          | U2   | 8           | CH64  |
| 65          | U2   | 23          | CH65  |
| 66          | U2   | 11          | CH66  |
| 67          | U2   | 10          | CH67  |
| 68          | U2   | 9           | CH68  |
| 69          | U2   | 22          | CH69  |
| 70          | U2   | 21          | CH70  |
| 71          | U2   | 20          | CH71  |
