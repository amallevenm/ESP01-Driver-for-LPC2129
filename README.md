# ESP01 Driver for LPC2129

A bare-metal C driver to connect an LPC2129 microcontroller to a TCP server over Wi-Fi using the ESP-01 (ESP8266) module.

---
## Overview

This project demonstrates how to:

- Configure UART0 (debug/HyperTerminal) and UART1 (ESP-01) on the LPC2129
- Drive the ESP-01 using AT commands 
- Connect to a Wi-Fi network and establish a TCP connection to a remote server
- Send data to the server and receive notifications (Wi-Fi disconnect, TCP close, incoming data)

## Hardware Connections

### LPC2129 ↔ ESP-01 (UART1)

| LPC2129 | ESP-01 |
|---|---|
| P0.8 (TXD1) | RX |
| P0.9 (RXD1) | TX |
| 3.3V | VCC  |
| GND | GND |

### LPC2129 ↔ USB-UART Debug (UART0)

| LPC2129 | USB-UART |
|---|---|
| P0.0 (TXD0) | RX |
| P0.1 (RXD0) | TX |


---

## Project Structure

```
├── main.c      # Example application
├── esp01.c/h   # ESP-01 driver (init, send, response, notification)
├── uart.c/h    # UART config, ISRs, message stack
├── delay.c/h   # Timer0-based blocking delays
```

---

## How It Works
### Initialization State Machine
 `esp_init()` in `esp01.c` walks through a linear sequence of AT command stages. Each stage sends a command over UART1, waits for a response, and advances to the next state on success.

```
AT → Echo Off → WiFi Mode → Check WiFi → Connect WiFi → Connect Server → Done
```
| State | AT Command Sent | Success Condition |
|---|---|---|
| `at` | `AT` | `OK` |
| `eccho_off` | `ATE0` | `OK` |
| `wifi_mode` | `AT+CWMODE=1` | `OK` |
| `is_wifi_connected` | `AT+CWJAP?` | `+CWJAP:` (connected) or `No AP` (not connected) |
| `connect_wifi` | `AT+CWJAP="ssid","pass"` | `OK` |
| `connect_server` | `AT+CIPSTART="TCP","ip",port` | `CONNECT` or `ALREADY CONNECTED` |
| `exit_init` | — | Returns success |

Each step sends a command over UART1, waits for a response, and moves to the next state. You can re-enter mid-way (e.g. `esp_init(connect_wifi)`) after a disconnect.

### UART Stack 
 Incoming ESP-01 messages are captured byte-by-byte in the UART1 ISR. Each complete line is pushed onto a 10-slot stack. The main code pops and processes them after each AT command.
 - `uartstack_push()` — called from ISR to store a received line
- `uartstack_pop()` — called from main context to retrieve a line
- `uartstack_clr()` — clears all messages after processing

### Response LUT
 Instead of hardcoded `if/else` checks, a lookup table maps each state to the response strings it expects (like `OK`, `CONNECT`, `No AP`). This keeps the parsing logic clean and easy to extend.
 
```c
// Indices into esp_responses[]
const char lut[8][5] = {
    {1, 2, '\0'},           // at:               OK, ERROR
    {1, 2, '\0'},           // eccho_off:         OK, ERROR
    {1, 2, '\0'},           // wifi_mode:         OK, ERROR
    {7, 4, 2, '\0'},        // is_wifi_connected: No AP, +CWJAP:, ERROR
    {1, 5, 6, 16, '\0'},    // connect_wifi:      OK, +CWJAP:1, +CWJAP:3, FAIL
    {10, 11, 9, '\0'},      // connect_server:    CONNECT, ALREADY CONNECTED, CLOSED
    {1, 15, 2, '\0'},       // send:              OK, SEND OK, ERROR
    {8, 9, 12, '\0'}        // notification:      WIFI DISCONNECT, CLOSED, +IPD
};
```

---
### Receiving Messages
Incoming server messages are received as `+IPD` packets from the ESP-01. The driver detects this in `esp_notification()` and prints everything after the `:` directly on UART0, stripping the `+IPD,<length>:` header automatically.

### Auto Reconnect
The main loop continuously calls `esp_notification()` to monitor the connection. If a drop is detected:

- **Wi-Fi disconnected** — calls `esp_init(is_wifi_connected)` to check and reconnect to Wi-Fi, then reconnects to the server.
- **TCP connection closed** — waits 60 seconds (to let the server reset) then calls `esp_init(connect_wifi)` to reconnect from the Wi-Fi step onward.

---

## Usage

1. Flash the code to LPC2129 using Flash Magic
2. Enter your Wi-Fi name, password, server IP, and port when prompted.
3. The module will connect automatically and print status over UART0.
4. Type `SEND` in the terminal to send a message to the server.
5. Any message received from the server is printed directly on UART0

```
Enter wifi name:     MyNetwork
Enter wifi password: MyPassword
Enter server ip:     192.168.1.100
Enter server port:   2023

ESP01 Initialized
> SEND
enter the string
> Hello Server
uart string 

message received
Hello back from server
```

---

## Author

**Amal Leven**

