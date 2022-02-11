# CalipDecoder
Decoding data output of a digital indicator in serial strings
This is the first version of a data decoder for Digital indicators, based on AVR 328p micro (tested also on a LGT8F328P clone)
- Input: 24-bit ck/data stream from a digital indicator (maybe some calipers use the same format)
- The output of indicator is low level (0-1.5V), needs a level shifter
- Simplest level shifter is in form of 2 npn transistors, the firmware expects to read the signals inverted
- The input pins of the micro have pull up resistors activated
- Pin C1 is Data in Pin C2 is CK in
- Output is in text format on UART (PD1) 19200 bps 8N1
