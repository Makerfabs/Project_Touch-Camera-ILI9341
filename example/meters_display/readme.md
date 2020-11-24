This is an example of TFT_eSPI library, you need to update the User_Setup.h file in the library. <br>
<pre>
// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins

#define TFT_MISO 12//19
#define TFT_MOSI 13//23
#define TFT_SCLK 14//18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC   33// 2  // Data Command control pin
//#define TFT_RST  // 4  // Reset pin (could connect to RST pin)
#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
</pre>
