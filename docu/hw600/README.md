Pinout notes for various connectors/boards within the unit shown in the disassembly images.

Control pinout when looking at the back, label on the right, left to right

| Pin | Function |
|-----|----------|
| 1   | GND      |
| 2   | Power On |
| 3   | BAT+     |
| 4   | ?        |
| 5   | ?        |
| 6   | ?        |
| 7   | ?        |

Hirose - DB9 cable, looking down in to both, numbers from DB9 molding, big key rotated to top on circular connector. Table is view of Hirose connector.

|          |       |
|----------|-------|
| DB9-2 RX | DB9-5 |
| DB9-3 TX | NC    |

Battery board connector, looking at board, board on top, pin 1 is pin 1 on cable. Pin 4 is connected to the battery center contact through two diodes and a 100 ohm resistor. The LED is connected to B+ and has a 470 ohm resistor, grounding this pin turns it on.

| Pin | Function       | Pin | Function |
|-----|----------------|-----|----------|
| 2   | NC             | 1   | GND      |
| 4   | Center contact | 3   | NC       |
| 6   | NC             | 5   | BAT+     |
| 8   | NC             | 7   | NC       |
| 10  | LED            | 9   | BAT+     |

Distance finder to main board wire, distance finder end, looking in to the connector pins with the polarity pin at the top. There's nothing weird despite it being discrete wires, pins are 1-1 at both ends.

| Pin | Function | Pin | Function |
|-----|----------|-----|----------|
| 1   | ?        | 2   | ?        |
| 3   | ?        | 4   | 12V      |
| 5   | ?        | 6   | -12V     |
| 7   | ?        | 8   | ?        |
| 9   | ?        | 10  | BAT+     |
| 11  | ?        | 12  | BAT+?    |
| 13  | CHASSIS  | 14  | CHASSIS  |

Control baseplate<->main wire. CTRL is contacts going to the control unit, numbered as in the control unit pinout.

| Pin | Function         | Pin | Function |
|-----|------------------|-----|----------|
| 1   | GDM_DAT to angle | 2   | B+?      |
| 3   | GDM_CLK to angle | 4   | GND      |
| 5   | GDM_ACK to angle | 6   | GND      |
| 7   | ?                | 8   | ?        |
| 9   | ?                | 10  | ?        |
| 11  | ?                | 12  | ?        |
| 13  | ?                | 14  | ?        |
| 15  | ?                | 16  | CTRL 2   |

Main board 4 pin header

| Pin | Function |
|-----|----------|
| 1   | GND      |
| 2   | 5V       |
| 3   | RXD      |
| 4   | TXD      |

8 pin discrete wire connector to main board, suspect this goes to the tribrach connector. DB9 assignments are opposite device function, DB9 RX is the device TX.

| Pin | Function | Pin | Function |
|-----|----------|-----|----------|
| 1   | ?        | 2   | ?        |
| 3   | ?        | 4   | ?        |
| 5   | DB9 RX   | 6   | ?        |
| 7   | DB9 TX   | 8   | ?        |

14 pin center board to angle board

| Pin | Function | Pin | Function |
|-----|----------|-----|----------|
| 1   | -12V     | 2   | 12V      |
| 3   | 5V       | 4   | ?        |
| 5   | ?        | 6   | ?        |
| 7   | ?        | 8   | ?        |
| 9   | ?        | 10  | ?        |
| 11  | ?        | 12  | ?        |
| 13  | ?        | 14  | ?        |

14 pin main board to servo board

| Pin | Function | Pin | Function |
|-----|----------|-----|----------|
| 1   | 5V       | 2   | ?        |
| 3   | 12V      | 4   | ?        |
| 5   | -12V     | 6   | ?        |
| 7   | GND      | 8   | ?        |
| 9   | GND      | 10  | ?        |
| 11  | ?        | 12  | ?        |
| 13  | ?        | 14  | ?        |

12 pin angle board to control base plate

| Pin | Function | Pin | Function |
|-----|----------|-----|----------|
| 1   | 80C32 RX | 2   | GND      |
| 3   | ?        | 4   | GND      |
| 5   | 80C32 TX | 6   | BATT+    |
| 7   | -12V     | 8   | BATT+    |
| 9   | ?        | 10  | ?        |
| 11  | ?        | 12  | ?        |

14 pin angle board to compensator

| Pin | Function | Pin | Function |
|-----|----------|-----|----------|
| 1   | ?        | 2   | ?        |
| 3   | ?        | 4   | ?        |
| 5   | 5V       | 6   | ?        |
| 7   | ?        | 8   | ?        |
| 9   | ?        | 10  | ?        |
| 11  | ?        | 12  | ?        |
| 13  | ?        | 14  | ?        |

Angle unit processor board - connector near EEPROM, bottom of board

| Pin | Function      | Pin | Function     |
|-----|---------------|-----|--------------|
| 1   | 80C32 GDM_ACK | 2   | 130kHz sine? |
| 3   | 80C32 GDM_DAT | 4   | GDM_CLK      |
| 5   | ?             | 6   | ?            |
| 7   | ?             | 8   | ?            |
| 9   | ?             | 10  | ?            |
| 11  | ?             | 12  | ?            |
| 13  | ?             | 14  | ?            |
| 15  | ?             | 16  | ?            |

Angle unit processor board - connector top middle of board

| Pin | Function     | Pin | Function     |
|-----|--------------|-----|--------------|
| 1   | ?            | 2   | GND          |
| 3   | 0.5V REF?    | 4   | ?            |
| 5   | ?            | 6   | 0.3V REF?    |
| 7   | GND          | 8   | GND          |
| 9   | ?            | 10  | ?            |
| 11  | -12V?        | 12  | 12V?         |
| 13  | 12.5kHz sine | 14  | 5V           |
| 15  | GND          | 16  | 12.5kHz sine |
| 17  | 12.5kHz sine | 18  | 12.5kHz sine |
| 19  | GND          | 20  | 12.5kHz sine |
