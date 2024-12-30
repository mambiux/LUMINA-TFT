LUMINA-TFT
A Status Screen using a TFT shield and Arduino Uno for my AI dedicated hardware build called LUMINA
About
LUMINA-TFT is a display system that shows real-time status, temperature, and state information for an AI system on a TFT LCD shield. It features an animated orb display with various states, temperature monitoring, and a cyberpunk-inspired interface.
Hardware Requirements

Arduino Uno/Mega
TFT LCD Shield (320x240 resolution)
USB cable for serial communication

Pin Configuration
  LCD_CS: A3
  LCD_CD: A2
  LCD_WR: A1
  LCD_RD: A0
  LCD_RESET: A4
Features

Animated status orb with multiple states
Real-time temperature monitoring for CPU and GPUs
Automatic temperature warnings
Status state persistence
Auto-timeout safety feature
Cyberpunk-inspired design

Status Commands
Send these commands via Serial (9600 baud rate)
Basic States

online    - Normal operation (green)
offline   - System off (red)
error     - Error state (red)
critical  - Critical state (red with warning)
sleep     - Sleep mode (pale blue)
booting   - Boot sequence (neon blue)

Activity States

talking   - Blue with sound waves
coding    - Green with code brackets
dreaming  - Purple spiral effect
recalling - Orange with spinning dots
saving    - Green with download arrow
control   - Orange control panel
settings  - Orange gear display

Emotional States

happy   - Yellow with smile
sad     - Purple with frown
angry   - Red with angry expression

Temperature Monitoring
Format: temp|device|value

temp|cpu|45.5    - Set CPU temperature
temp|gpu0|60.2   - Set GPU0 temperature
temp|gpu1|55.8   - Set GPU1 temperature

Temperature thresholds:

Warning: 80.0°C (red)
High: 60.0°C (yellow)
Normal: <60.0°C (green)

Music Display (Optional)
Format: playing|song_name|artist_name|duration_in_seconds

playing|My Song|Artist Name|180

  playing|My Song|Artist Name|180


Safety Features

Auto-timeout after 65 seconds of no commands
Automatic critical state on high temperature
Status recovery after temperature normalization

Communication Protocol

Baud Rate: 9600
Line Ending: Newline (\n)
Format: ASCII text commands
Case-insensitive commands

Installation

Install required libraries:

Adafruit_GFX
Adafruit_TFTLCD


Upload the code to your Arduino
Connect the TFT shield to the specified pins

Usage Example

// Open serial connection at 9600 baud
// Send commands:
Serial.println("online");              // Set online status
Serial.println("temp|cpu|45.5");       // Update CPU temperature
Serial.println("temp|gpu0|55.2");      // Update GPU0 temperature
Serial.println("temp|gpu1|50.8");      // Update GPU1 temperature

Error Recovery
If system becomes unresponsive:

Send "offline" command
Reset Arduino
Resume with "online" command

License
[Your License Here]
Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
Acknowledgments

Inspired by sci-fi interfaces
Built with Adafruit libraries
Designed for AI system monitoring
Built with an Arduino uno I had gathering dust, sadly it runned out of memory before making more cool stuff work
  
  
