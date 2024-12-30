# LUMINA-TFT

A **status screen** using a TFT shield and Arduino Uno for my personal LUMINA AI-dedicated hardware build.

## About

**LUMINA-TFT** is a display system designed to provide real-time updates on the status, temperature, and state of the LUMINA AI system. The display uses a TFT LCD shield and features an **animated orb** that reflects various operational, activity, and emotional states. It is inspired by **cyberpunk aesthetics** and offers an interactive interface.

---

## Features

- **Animated Orb Display:**
  - Orb changes colors and effects based on the system state.
- **Temperature Monitoring:**
  - Displays CPU and GPU temperatures in real-time.
  - Includes automatic warnings for high temperatures.
- **Status Persistence:**
  - Maintains the last known state even after timeouts or disconnections.
- **Safety Features:**
  - Automatic timeout after 65 seconds of inactivity.
  - Critical state activated when temperatures exceed safe thresholds.
- **Cyberpunk-inspired Design:**
  - Unique visual elements inspired by sci-fi interfaces.

---

## Hardware Requirements

- **Arduino Uno or Mega**
- **TFT LCD Shield**
  - Resolution: 320x240 pixels
- **USB Cable**
  - For serial communication and power

### Pin Configuration
| Pin | Function    |
|-----|-------------|
| A3  | LCD_CS      |
| A2  | LCD_CD      |
| A1  | LCD_WR      |
| A0  | LCD_RD      |
| A4  | LCD_RESET   |

---

## Status Commands

Commands can be sent to the system via **Serial Communication** (9600 baud rate). Commands are case-insensitive and should be followed by a newline character (`\n`).

### Basic States
| Command   | Description               | Color       |
|-----------|---------------------------|-------------|
| `online`  | Normal operation          | Green       |
| `offline` | System off                | Red         |
| `error`   | Error state               | Red         |
| `critical`| Critical state            | Red/Warning |
| `sleep`   | Sleep mode                | Pale Blue   |
| `booting` | Boot sequence             | Neon Blue   |

### Activity States
| Command    | Description               | Effect/Color          |
|------------|---------------------------|-----------------------|
| `talking`  | Indicates speech          | Blue with sound waves |
| `coding`   | Coding activity           | Green with brackets   |
| `dreaming` | Processing in background  | Purple spiral effect  |
| `recalling`| Retrieving data           | Orange with dots      |
| `saving`   | Saving data               | Green with arrow      |
| `control`  | Control state             | Orange control panel  |
| `settings` | Adjusting configurations  | Orange gear display   |

### Emotional States
| Command   | Description               | Effect/Color        |
|-----------|---------------------------|---------------------|
| `happy`   | Cheerful mood             | Yellow with smile   |
| `sad`     | Low mood                  | Purple with frown   |
| `angry`   | Frustrated state          | Red with expression |

---

## Temperature Monitoring

### Command Format
```plaintext
temp|device|value
```

| Example Command      | Description               |
|-----------------------|---------------------------|
| `temp|cpu|45.5`       | Set CPU temperature       |
| `temp|gpu0|60.2`      | Set GPU0 temperature      |
| `temp|gpu1|55.8`      | Set GPU1 temperature      |

### Temperature Thresholds
| Level    | Range (°C) | Color       |
|----------|------------|-------------|
| Normal   | < 60.0     | Green       |
| High     | 60.0 - 80.0| Yellow      |
| Warning  | > 80.0     | Red         |

---

## Optional Features

### Music Display
The system can display music currently playing.

#### Command Format
```plaintext
playing|song_name|artist_name|duration_in_seconds
```
| Example Command                       | Description             |
|---------------------------------------|-------------------------|
| `playing|My Song|Artist Name|180`     | Displays song info      |

---

## Communication Protocol

- **Baud Rate:** 9600
- **Line Ending:** Newline (`\n`)
- **Format:** ASCII text commands
- **Commands:** Case-insensitive

---

## Installation

1. **Install Required Libraries:**
   - [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
   - [Adafruit TFTLCD Library](https://github.com/adafruit/TFTLCD-Library)
2. **Upload the Code:**
   - Load the provided sketch onto your Arduino Uno or Mega.
3. **Connect the TFT Shield:**
   - Attach the shield to the Arduino pins as specified in the **Pin Configuration** section.
4. **Open Serial Monitor:**
   - Set the baud rate to 9600.

---

## Usage Example

Here is an example of sending commands via the Serial Monitor:

```cpp
// Open serial connection at 9600 baud
Serial.println("online");              // Set to online status
Serial.println("temp|cpu|45.5");       // Update CPU temperature
Serial.println("temp|gpu0|55.2");      // Update GPU0 temperature
Serial.println("temp|gpu1|50.8");      // Update GPU1 temperature
```

---

## Error Recovery

If the system becomes unresponsive:

1. Send the `offline` command.
2. Reset the Arduino.
3. Resume operation by sending the `online` command.

---

## Safety Features

- **Auto-timeout:**
  - System enters a offline state after 65 seconds of inactivity.
- **Critical State Activation:**
  - Triggered automatically when temperature thresholds are exceeded.
- **Status Recovery:**
  - System returns to normal operation once conditions stabilize.

---

## Contributing

Contributions are welcome! To make changes:

1. Fork the repository.
2. Create a feature branch (`git checkout -b feature-name`).
3. Commit your changes (`git commit -m 'Add feature-name'`).
4. Push to your branch (`git push origin feature-name`).
5. Open a pull request.

For major changes, please open an issue first to discuss your ideas.

---

## Acknowledgments

- Inspired by sci-fi interfaces and cyberpunk aesthetics.
- Built using [Adafruit Libraries](https://learn.adafruit.com/adafruit-gfx-graphics-library).
- Designed for AI system monitoring with an **Arduino Uno**.

---

## License

MIT

---

## Notes

- The project was built using an Arduino Uno, but due to memory constraints, further expansion was limited. Consider using an Arduino Mega for additional features.
