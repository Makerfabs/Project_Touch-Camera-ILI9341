

#include <SPI.h>
#include "SD.h"
#include "FS.h"
#include "Adafruit_STMPE610.h"
#include <TJpg_Decoder.h>

#define SD_CS 4

#define TFT_CS 15
#define STMPE_CS 2

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define TFT_GREY 0x5AEB // New colour

//SPI control
#define SPI_ON_TFT digitalWrite(TFT_CS, LOW)
#define SPI_OFF_TFT digitalWrite(TFT_CS, HIGH)
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)
#define STMPE_ON digitalWrite(STMPE_CS, LOW)
#define STMPE_OFF digitalWrite(STMPE_CS, HIGH)

//SPI
#define SPI_MOSI 13
#define SPI_MISO 12
#define SPI_SCK 14

Adafruit_STMPE610 touch = Adafruit_STMPE610(STMPE_CS);

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}


String file_list[40];
int file_num = 0;
int file_index = 0;

void setup(void) {

  Serial.begin(115200);
  
    //SPI init
    pinMode(SD_CS, OUTPUT);
    pinMode(STMPE_CS, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    SPI_OFF_SD;
    STMPE_OFF;
    SPI_OFF_TFT;
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    SPI_ON_SD;
    //SD(SPI) init
    if (!SD.begin(SD_CS, SPI, 80000000))
    {
        Serial.println("Card Mount Failed");
        while (1)
            ;
    }
    else
    {
        Serial.println("Card Mount Successed");
    }
    //sd_test();
    file_num = get_pic_list(SD, "/", 0, file_list);
    Serial.print("jpg file count:");
    Serial.println(file_num);
    Serial.println("All jpg:");
    for (int i = 0; i < file_num; i++)
    {
        Serial.println(file_list[i]);
    }
    SPI_OFF_SD;

    
    SPI_ON_TFT;
    tft.init();    
    tft.setRotation(1);        
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(true); // We need to swap the colour bytes (endianess)

  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
    TJpgDec.setJpgScale(1);

  // The decoder must be given the exact name of the rendering function above
    TJpgDec.setCallback(tft_output);
    
    SPI_OFF_TFT;



    STMPE_ON;
    if (!touch.begin())
    {
        Serial.println("STMPE not found!");
        while (1)
            ;
    }
    STMPE_OFF;
}

long int runtime = millis();
int flag = 1;

void loop()
{       
    if ((millis() - runtime > 10000) || flag == 1)
    {
        tft.fillScreen(TFT_BLACK);
        TJpgDec.drawSdJpg(0, 0, file_list[file_index].c_str());
        file_index++;
        if (file_index >= file_num)
        {
            file_index = 0;
        }
        runtime = millis();
        flag = 0;
    }

    STMPE_ON;

    uint16_t x, y;
    uint8_t z;
    if (touch.touched())
    {
        // read x & y & z;
        int pos[2] = {0, 0};
        delay(100);      // delay for SPI receive the data
        while (!touch.bufferEmpty())
        {
            
            touch.readData(&x, &y, &z);
            pos[0] = x * 240 / 4096;
            pos[1] = y * 320 / 4096;
            Serial.println(pos[1]);
            
        }
        if (pos[1] > 160)
            {
                flag = 1;
                Serial.println("Next");
            }
            else
            {
                flag = 1;
                file_index = (file_index + file_num - 2) % file_num;
                Serial.println("Last");
            }
         
        while (touch.touched())
            ;
        //touch.writeRegister8(STMPE_INT_STA, 0xFF); // reset all ints, in this example unneeded depending in use
    }
    STMPE_OFF;
    
}

//Gets all image files in the SD card root directory
int get_pic_list(fs::FS &fs, const char *dirname, uint8_t levels, String wavlist[30])
{
    Serial.printf("Listing directory: %s\n", dirname);
    int i = 0;

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return i;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return i;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
        }
        else
        {
            String temp = file.name();
            if (temp.endsWith(".jpg"))
            {
                wavlist[i] = temp;
                i++;
            }
        }
        file = root.openNextFile();
    }
    return i;
}
