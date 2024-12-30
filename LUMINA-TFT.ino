#include <Adafruit_GFX.h>    
#include <Adafruit_TFTLCD.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>

#define SD_CS 10 // SD card chip select pin

#define LCD_CS A3 
#define LCD_CD A2 
#define LCD_WR A1 
#define LCD_RD A0 
#define LCD_RESET A4 

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Complete color palette
#define NEON_BLUE 0x07FF
#define DARK_BLUE 0x0000
#define LIGHT_BLUE 0x05FF
#define BRIGHT_NEON_BLUE 0x0FFF
#define SHADOW_COLOR 0x0010
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define BRIGHT_GREEN 0x0FE0
#define RED 0xF800
#define BRIGHT_RED 0xF900
#define PINK 0xFB6D
#define YELLOW 0xFFE0
#define PURPLE 0x780F
#define MEDIUM_BLUE 0x03FF
#define PALE_BLUE 0x067F
#define GLOW_BLUE 0x06FF
#define ORANGE 0xFD20
#define GRAY 0x7BEF
#define DREAM_PURPLE 0x981F
#define BUFFPIXEL 20

uint16_t read16(File &f) {
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read();
    ((uint8_t *)&result)[1] = f.read();
    return result;
}

uint32_t read32(File &f) {
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read();
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read();
    return result;
}
// Global variables for core functionality
String aiStatus = "Booting...";
String lastStatus = "";
String previousStatus = "";  // Store status before temperature warning
bool isOverheated = false;
unsigned long lastPingTime = 0;
const unsigned long powerOffTimeout = 65000; // 30 seconds

// Music playback variables
String currentSong = "";
String currentArtist = "";
unsigned long songStartTime = 0;
int songDuration = 0;
int currentVolume = 50;

// Forward declarations
void drawStatus(String status);
void drawTemperatures();
void drawCartoonishOrb(int centerX, int centerY, String status);
// Add these after the existing forward declarations
void drawVolumeIndicator(int centerX, int centerY);
void drawPlayButton(int centerX, int centerY);
void drawSongInfo();
uint16_t getStatusColor(String status);
void drawCentered3DText(String text, int y, int size, uint16_t color, uint16_t shadowColor);

// Temperature monitoring structure
struct Temperature {
    float current;
    float max;
    float min;
    String label;
    uint16_t color;
    bool isWarning;
};
// Temperature thresholds
#define TEMP_WARNING 80.0
#define TEMP_HIGH 60.0

// Temperature instances
Temperature cpuTemp = {00.0, 00.0, 00.0, "CPU", GREEN, false};
Temperature gpu0Temp = {00.0, 00.0, 00.0, "GPU0", GREEN, false};
Temperature gpu1Temp = {00.0, 00.0, 00.0, "GPU1", GREEN, false};

void bmpDraw(char *filename, uint8_t x, uint16_t y) {
    File bmpFile = SD.open(filename);
    if (!bmpFile) return;
    
    if(read16(bmpFile) != 0x4D42) {
        bmpFile.close();
        return;
    }
    
    bmpFile.seek(18);
    uint16_t w = read32(bmpFile);
    uint16_t h = read32(bmpFile);
    
    if(read16(bmpFile) != 1 || read16(bmpFile) != 24) {
        bmpFile.close();
        return;
    }
    
    bmpFile.seek(54);
    uint8_t r, g, b;
    uint16_t p;
    
    for(uint16_t row = 0; row < h; row++) {
        for(uint16_t col = 0; col < w; col++) {
            b = bmpFile.read();
            g = bmpFile.read();
            r = bmpFile.read();
            p = tft.color565(r, g, b);
            tft.drawPixel(x + col, y + (h-1) - row, p);
        }
        // Skip padding bytes
        uint8_t pad = (4 - ((w * 3) & 3)) & 3;
        while(pad--) bmpFile.read();
    }
    bmpFile.close();
}

void drawCyberpunkFrame() {
    tft.drawRect(5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, LIGHT_BLUE);
    tft.drawRect(10, 10, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20, NEON_BLUE);
}

void drawRedX(int centerX, int centerY, int size) {
    for(int i = -2; i <= 2; i++) {
        tft.drawLine(centerX - size + i, centerY - size, centerX + size + i, centerY + size, BRIGHT_RED);
        tft.drawLine(centerX - size + i, centerY + size, centerX + size + i, centerY - size, BRIGHT_RED);
    }
}

void drawPlayButton(int centerX, int centerY) {
    const int size = 15;
    int x1 = centerX - size/2;
    int x2 = centerX + size;
    int y1 = centerY - size;
    int y2 = centerY;
    int y3 = centerY + size;
    
    tft.fillTriangle(x1, y1, x1, y3, x2, y2, DARK_BLUE);
    tft.drawLine(x1, y1, x1, y3, WHITE);
    tft.drawLine(x1, y1, x2, y2, WHITE);
    tft.drawLine(x1, y3, x2, y2, WHITE);
}

void checkTemperatureWarnings() {
    bool wasOverheated = isOverheated;
    isOverheated = false;
    
    if (cpuTemp.current >= TEMP_WARNING || 
        gpu0Temp.current >= TEMP_WARNING || 
        gpu1Temp.current >= TEMP_WARNING) {
        isOverheated = true;
    }
    
    // Handle state changes
    if (isOverheated && !wasOverheated) {
        previousStatus = aiStatus;
        aiStatus = "critical";
        drawStatus(aiStatus);
    }
    else if (!isOverheated && wasOverheated) {
        aiStatus = previousStatus;
        drawStatus(aiStatus);
    }
}

void parseTemperature(String input) {
    int firstDelimiter = input.indexOf('|');
    int secondDelimiter = input.indexOf('|', firstDelimiter + 1);
    
    if (firstDelimiter != -1 && secondDelimiter != -1) {
        String device = input.substring(firstDelimiter + 1, secondDelimiter);
        float temp = input.substring(secondDelimiter + 1).toFloat();
        Temperature* targetTemp = nullptr;
        
        if (device.equalsIgnoreCase("cpu")) targetTemp = &cpuTemp;
        else if (device.equalsIgnoreCase("gpu0")) targetTemp = &gpu0Temp;
        else if (device.equalsIgnoreCase("gpu1")) targetTemp = &gpu1Temp;
        
        if (targetTemp) {
            targetTemp->current = temp;
            targetTemp->max = max(targetTemp->max, temp);
            targetTemp->min = min(targetTemp->min, temp);
            
            if (temp >= TEMP_WARNING) {
                targetTemp->color = RED;
                targetTemp->isWarning = true;
            }
            else if (temp >= TEMP_HIGH) {
                targetTemp->color = YELLOW;
                targetTemp->isWarning = false;
            }
            else {
                targetTemp->color = GREEN;
                targetTemp->isWarning = false;
            }
        }
        
        checkTemperatureWarnings();
    }
}

void drawTemperatureWarning() {
    if (isOverheated) {
        tft.fillRect(15, 45, SCREEN_WIDTH - 30, 20, RED);
        tft.setTextColor(WHITE);
        tft.setTextSize(1);
        String warning = "WARNING: High Temperature!";
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(warning, 0, 0, &x1, &y1, &w, &h);
        int x = (SCREEN_WIDTH - w) / 2;
        tft.setCursor(x, 50);
        tft.print(warning);
        
        // Add specific temperature warnings
        String details = "";
        if (cpuTemp.isWarning) details += "CPU ";
        if (gpu0Temp.isWarning) details += "GPU0 ";
        if (gpu1Temp.isWarning) details += "GPU1 ";
        details += "OVERHEATING";
        
        tft.setTextColor(YELLOW);
        tft.getTextBounds(details, 0, 0, &x1, &y1, &w, &h);
        x = (SCREEN_WIDTH - w) / 2;
        tft.setCursor(x, 60);
        tft.print(details);
    }
}

void drawTemperatures() {
    // Position for temperature display
    int y = 210;  // below status text
    int spacing = 20;  // Spacing between temperatures
    tft.setTextSize(1);

    // Clear temperature area
    tft.fillRect(0, y, SCREEN_WIDTH, 12, DARK_BLUE);

    // Calculate total width of the line of temperatures
    int totalWidth = 0;
    auto calculateWidth = [&totalWidth, spacing](Temperature& temp) {
        String tempStr = temp.label + ": " + String(temp.current, 1) + " C";
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
        totalWidth += w + spacing;
    };

    calculateWidth(cpuTemp);
    calculateWidth(gpu0Temp);
    calculateWidth(gpu1Temp);
    totalWidth -= spacing;  // Remove extra spacing after the last temperature

    // Calculate starting x position for centering
    int startX = (SCREEN_WIDTH - totalWidth) / 2;
    int x = startX;

    // Draw all temperatures in a single line
    auto drawTemp = [&x, y, spacing](Temperature& temp) {
        String tempStr = temp.label + ": " + String(temp.current, 1) + " C";
        tft.setTextColor(temp.color);
        tft.setCursor(x, y);
        tft.print(tempStr);

        // Calculate width for spacing
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
        x += w + spacing;
    };

    drawTemp(cpuTemp);
    drawTemp(gpu0Temp);
    drawTemp(gpu1Temp);

    if (isOverheated) {
        drawTemperatureWarning();
    }
}


// Enhanced song information parsing
void parseSongInfo(String input) {
    int firstDelimiter = input.indexOf('|');
    int secondDelimiter = input.indexOf('|', firstDelimiter + 1);
    int thirdDelimiter = input.indexOf('|', secondDelimiter + 1);
    
    if (firstDelimiter != -1 && secondDelimiter != -1 && thirdDelimiter != -1) {
        currentSong = input.substring(firstDelimiter + 1, secondDelimiter);
        currentArtist = input.substring(secondDelimiter + 1, thirdDelimiter);
        songDuration = input.substring(thirdDelimiter + 1).toInt();
        songStartTime = millis();
        
        // Debug print to Serial
        Serial.println("Song: " + currentSong);
        Serial.println("Artist: " + currentArtist);
        Serial.println("Duration: " + String(songDuration));
    }
}

void drawSongInfo() {
    tft.fillRect(20, 200, SCREEN_WIDTH - 40, 30, DARK_BLUE);
    
    if (currentSong.length() > 0) {
        tft.setTextSize(1);
        int16_t x1, y1;
        uint16_t w, h;
        
        // Draw song name
        tft.setTextColor(YELLOW);
        tft.getTextBounds(currentSong, 0, 0, &x1, &y1, &w, &h);
        int x = (SCREEN_WIDTH - w) / 2;
        tft.setCursor(x, 205);
        tft.println(currentSong);
        
        // Draw artist name
        if (currentArtist.length() > 0) {
            tft.setTextColor(GRAY);
            tft.getTextBounds(currentArtist, 0, 0, &x1, &y1, &w, &h);
            x = (SCREEN_WIDTH - w) / 2;
            tft.setCursor(x, 220);
            tft.println(currentArtist);
        }
    }
}

uint16_t getStatusColor(String status) {
    // System states
    if (status.equalsIgnoreCase("online")) return BRIGHT_GREEN;
    if (status.equalsIgnoreCase("off") || status.equalsIgnoreCase("offline")) return RED;
    if (status.equalsIgnoreCase("booting")) return NEON_BLUE;
    if (status.equalsIgnoreCase("sleep")) return PALE_BLUE;
    
    // Emergency states
    if (status.equalsIgnoreCase("emergency")) return BRIGHT_RED;
    if (status.equalsIgnoreCase("critical")) return RED;
    if (status.equalsIgnoreCase("error")) return RED;
    
    // Emotional states
    if (status.equalsIgnoreCase("angry")) return BRIGHT_RED;
    
    // Activity states
    if (status.equalsIgnoreCase("talking")) return BRIGHT_NEON_BLUE;
    if (status.equalsIgnoreCase("playing")) return YELLOW;
    if (status.equalsIgnoreCase("coding")) return GREEN;
    if (status.equalsIgnoreCase("recalling")) return ORANGE;
    if (status.equalsIgnoreCase("saving")) return BRIGHT_GREEN;
    
    // Awareness states
    if (status.equalsIgnoreCase("aware")) return BRIGHT_NEON_BLUE;
    if (status.equalsIgnoreCase("oblivious")) return GRAY;
    
    // Control states
    if (status.equalsIgnoreCase("control")) return ORANGE;
    if (status.equalsIgnoreCase("settings")) return ORANGE;
    
    return NEON_BLUE;
}

void drawCentered3DText(String text, int y, int size, uint16_t color, uint16_t shadowColor) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.setTextSize(size);
    tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    int x = (SCREEN_WIDTH - w) / 2;
    
    tft.setTextColor(shadowColor);
    tft.setCursor(x + 2, y + 2);
    tft.println(text);
    
    tft.setTextColor(color);
    tft.setCursor(x, y);
    tft.println(text);
}

void drawCartoonishOrb(int centerX, int centerY, String status) {
    if (status == lastStatus && !isOverheated) return;
    
    tft.fillRect(centerX - 50, centerY - 50, 100, 100, DARK_BLUE);
    
    // Special handling for off states
    if (status.equalsIgnoreCase("off") || status.equalsIgnoreCase("offline")) {
        tft.fillCircle(centerX, centerY, 40, DARK_BLUE);
        tft.drawCircle(centerX, centerY, 40, RED);
        tft.drawCircle(centerX, centerY, 39, RED);
        drawRedX(centerX, centerY, 25);
        return;
    }
    
    // Outer glow effect for active states
    for(int r = 45; r > 40; r--) {
        tft.drawCircle(centerX, centerY, r, GLOW_BLUE);
    }
    
    // Main orb body
    uint16_t primaryColor = getStatusColor(status);
    for(int r = 40; r > 0; r -= 5) {
        uint16_t color = (r > 30) ? primaryColor : (r > 15 ? NEON_BLUE : LIGHT_BLUE);
        tft.fillCircle(centerX, centerY, r, color);
    }
    
    // Standard highlight (except for off states)
    if (!status.equalsIgnoreCase("off") && !status.equalsIgnoreCase("poweroff")) {
        tft.fillCircle(centerX - 15, centerY - 15, 8, WHITE);
        tft.fillCircle(centerX - 12, centerY - 12, 4, BRIGHT_NEON_BLUE);
    }
    
    // Bold outer ring
    tft.drawCircle(centerX, centerY, 40, WHITE);
    tft.drawCircle(centerX, centerY, 39, BRIGHT_NEON_BLUE);
    tft.drawCircle(centerX, centerY, 38, WHITE);
    
    // Status-specific internal designs
    if (status.equalsIgnoreCase("online") || status.equalsIgnoreCase("aware")) {
        // Alert, focused eye
        tft.fillCircle(centerX, centerY, 20, BRIGHT_NEON_BLUE);
        tft.fillCircle(centerX, centerY, 15, WHITE);
        tft.fillCircle(centerX, centerY, 10, BRIGHT_NEON_BLUE);
    } 
    else if (status.equalsIgnoreCase("sleep") || status.equalsIgnoreCase("oblivious")) {
        // Closed eye
        tft.fillCircle(centerX, centerY, 25, DARK_BLUE);
        tft.drawLine(centerX - 20, centerY, centerX + 20, centerY, WHITE);
        tft.drawLine(centerX - 20, centerY + 1, centerX + 20, centerY + 1, WHITE);
    }
    else if (status.equalsIgnoreCase("thinking") || status.equalsIgnoreCase("recalling")) {
        // Spinning dots
        for(int i = 0; i < 8; i++) {
            float angle = i * PI / 4;
            int x1 = centerX + cos(angle) * 15;
            int y1 = centerY + sin(angle) * 15;
            tft.fillCircle(x1, y1, 3, YELLOW);
        }
    }
    else if (status.equalsIgnoreCase("saving")) {
        // Download arrow
        tft.fillTriangle(centerX - 15, centerY - 10, centerX + 15, centerY - 10, 
                        centerX, centerY + 10, BRIGHT_GREEN);
        tft.fillRect(centerX - 15, centerY + 10, 30, 5, BRIGHT_GREEN);
    }
    else if (status.equalsIgnoreCase("angry")) {
        // Angry expression
        tft.fillTriangle(centerX - 20, centerY - 10, centerX + 20, centerY - 10,
                        centerX, centerY + 15, BRIGHT_RED);
        tft.drawLine(centerX - 15, centerY - 15, centerX + 15, centerY - 15, BRIGHT_RED);
    }
    else if (status.equalsIgnoreCase("emergency") || status.equalsIgnoreCase("critical")) {
        // Warning triangle
        tft.fillTriangle(centerX - 20, centerY + 10, centerX + 20, centerY + 10,
                        centerX, centerY - 15, BRIGHT_RED);
        tft.fillCircle(centerX, centerY + 2, 3, WHITE);
        
        // Additional warning effect for critical state
        if (status.equalsIgnoreCase("critical")) {
            // Pulsing ring effect
            unsigned long currentMillis = millis() / 200; // Adjust speed of pulse
            int pulseSize = (currentMillis % 2 == 0) ? 44 : 42;
            tft.drawCircle(centerX, centerY, pulseSize, RED);
        }
    }
    else if (status.equalsIgnoreCase("error")) {
        // Error symbol (!)
        tft.fillRect(centerX - 3, centerY - 15, 6, 20, BRIGHT_RED);
        tft.fillCircle(centerX, centerY + 10, 3, BRIGHT_RED);
    }
    else if (status.equalsIgnoreCase("talking")) {
        // Sound wave effect
        for(int i = 0; i < 3; i++) {
            tft.drawCircle(centerX, centerY, 10 + i * 8, BRIGHT_NEON_BLUE);
        }
    }
    else if (status.equalsIgnoreCase("playing")) {
        // Play button and song info
        drawPlayButton(centerX, centerY);
        drawSongInfo();
       // drawProgressBar(centerX, centerY);
    }
    else if (status.equalsIgnoreCase("coding")) {
        // Code brackets
        tft.drawLine(centerX - 15, centerY - 15, centerX - 5, centerY, WHITE);
        tft.drawLine(centerX - 5, centerY, centerX - 15, centerY + 15, WHITE);
        tft.drawLine(centerX + 15, centerY - 15, centerX + 5, centerY, GREEN);
        tft.drawLine(centerX + 5, centerY, centerX + 15, centerY + 15, GREEN);
    }
    else if (status.equalsIgnoreCase("control")) {
        // Control panel dots
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                tft.fillCircle(centerX + (i-1)*12, centerY + (j-1)*12, 3, ORANGE);
            }
        }
    }
    else if (status.equalsIgnoreCase("settings")) {
        // Gear symbol
        for(int i = 0; i < 8; i++) {
            float angle = i * PI / 4;
            int x1 = centerX + cos(angle) * 15;
            int y1 = centerY + sin(angle) * 15;
            tft.fillCircle(x1, y1, 4, ORANGE);
        }
        tft.fillCircle(centerX, centerY, 8, ORANGE);

        // Add gear teeth for more detail
        for(int i = 0; i < 8; i++) {
            float angle = i * PI / 4;
            int x1 = centerX + cos(angle) * 18;
            int y1 = centerY + sin(angle) * 18;
            int x2 = centerX + cos(angle) * 12;
            int y2 = centerY + sin(angle) * 12;
            tft.drawLine(x1, y1, x2, y2, WHITE);
        }
    }
}

void drawStatus(String status) {
    uint16_t color = getStatusColor(status);
    
    if (status != lastStatus && !status.equalsIgnoreCase("Booting...")) {
        tft.fillRect(11, 11, SCREEN_WIDTH - 22, SCREEN_HEIGHT - 22, DARK_BLUE);
        drawCartoonishOrb(SCREEN_WIDTH / 2, 90, status);
        drawCentered3DText("L.U.M.I.N.A", 150, 2, NEON_BLUE, SHADOW_COLOR);
        
        
        // Draw temperatures only for specific states
        if (status.equalsIgnoreCase("online") || 
            status.equalsIgnoreCase("error") || 
            status.equalsIgnoreCase("critical")) {
            drawTemperatures();
        }
        
        // Draw status text
        tft.setTextColor(color);
        tft.setTextSize(1);
        String statusText = "Status: " + status;
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(statusText, 0, 190, &x1, &y1, &w, &h);
        int x = (SCREEN_WIDTH - w) / 2;
        tft.setCursor(x, 190);
        tft.println(statusText);
        
        // Draw song info if in playing state
        if (status.equalsIgnoreCase("playing")) {
            drawSongInfo();
        }
    } else if (status.equalsIgnoreCase("Booting...")) {
        tft.fillRect(40, 190, 240, 40, DARK_BLUE);
        tft.setTextColor(color);
        tft.setTextSize(2);
        String statusText = "Status: " + status;
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(statusText, 0, 200, &x1, &y1, &w, &h);
        int x = (SCREEN_WIDTH - w) / 2;
        tft.setCursor(x, 200);
        tft.println(statusText);
    }
    
    lastStatus = status;
}

void setup() {
    Serial.begin(9600);
    Serial.print("Initializing SD card...");
    if (!SD.begin(SD_CS)) {
        Serial.println("failed!");
        return;
    }
    Serial.println("SD card initialization done."); 
    tft.reset(); // Remove duplicate reset
    tft.begin(0x9341);
    tft.setRotation(1);
    tft.fillScreen(DARK_BLUE);
    drawCyberpunkFrame();
    drawCentered3DText("L.U.M.I.N.A", 80, 4, NEON_BLUE, SHADOW_COLOR);
    drawStatus(aiStatus);
    
    // Initialize temperatures with safe values
    cpuTemp.current = 00.0;
    gpu0Temp.current = 00.0;
    gpu1Temp.current = 00.0;
}

void loop() {
    
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        aiStatus.trim();
        drawStatus(aiStatus);
        lastPingTime = millis(); // Reset timeout

        if (input.startsWith("playing|")) {
            if (!isOverheated) {
                aiStatus = "playing";
                parseSongInfo(input);
                drawStatus(aiStatus);  // Force redraw after song info update
            }
        }
        // Inside your if (Serial.available() > 0) block, add this before your other conditions:
if (input.startsWith("image|")) {
    String imageName = input.substring(6);
    imageName.trim();
    char filename[13]; // 8.3 format max
    imageName.toCharArray(filename, 13);
    bmpDraw(filename, (SCREEN_WIDTH-64)/2, (SCREEN_HEIGHT-64)/2);
}
        else if (input.startsWith("temp|")) {
            parseTemperature(input);
            if (aiStatus.equalsIgnoreCase("online") || 
                aiStatus.equalsIgnoreCase("error") || 
                aiStatus.equalsIgnoreCase("critical")) {
                drawTemperatures();
            }
        }
        else {
            if (!isOverheated || input.equalsIgnoreCase("off") || 
                input.equalsIgnoreCase("offline") || input.equalsIgnoreCase("control") || 
                input.equalsIgnoreCase("settings")) {
                aiStatus = input;
                currentSong = "";
                currentArtist = "";
            }
            drawStatus(aiStatus);
        }
    }
    
    if (isOverheated && aiStatus.equalsIgnoreCase("critical")) {
        static unsigned long lastTempUpdate = 0;
        if (millis() - lastTempUpdate > 500) {
            drawCartoonishOrb(SCREEN_WIDTH / 2, 90, aiStatus);
            drawTemperatureWarning();
            lastTempUpdate = millis();
        }
    }
      // Check for timeout
  if (millis() - lastPingTime > powerOffTimeout) {
    aiStatus = "offline";
    drawStatus(aiStatus);
  }
    delay(30);
}
