/* Barebones TFT_eSPI Setup for 3.5" CYD (Cheap Yellow Display) 
   with ILI9488 and Resistive Touch overlay.
   
   Supplied 'As is' and is a work in progress.
   
   Linker 3000 August 2025
   
   Released into the public domain
   
 */

#define USER_SETUP_ID 999

// Driver selection
#define ILI9488_DRIVER

// Display size
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// ESP32 pin connections for TFT
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1  // Set to -1 if not used (tie to 3.3V or ESP32 EN)
#define TFT_BL   27  // Backlight control pin

// Backlight control
#define TFT_BACKLIGHT_ON HIGH

// Color order
#define TFT_RGB_ORDER TFT_BGR

// Inversion - try both to see which works with your display
#define TFT_INVERSION_OFF
// #define TFT_INVERSION_ON

// SPI frequency in Hz
#define SPI_FREQUENCY  55000000  // Try 27000000 if you get display corruption
#define SPI_READ_FREQUENCY  20000000

// Optional touch screen setup (resistive)
#define TOUCH_CLK  14  // Same as TFT_SCLK
#define TOUCH_CS   33
#define TOUCH_MOSI 13  // Same as TFT_MOSI  
#define TOUCH_MISO 12  // Same as TFT_MISO
#define TOUCH_IRQ  36
#define SPI_TOUCH_FREQUENCY  2500000

// Font loading - include the fonts you want to use
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.

// Optional: Smooth fonts (anti-aliased)
#define SMOOTH_FONT

// Optional: Load custom fonts
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// SPI overlap mode (ESP32 only)
// Uncomment to use SPI overlap mode, this can improve performance but may cause issues with some displays
// #define SPI_OVERLAP_MODE
