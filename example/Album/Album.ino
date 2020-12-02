#include "SPI.h"
#include "SD.h"
#include "FS.h"
#include "Adafruit_STMPE610.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define FS_NO_GLOBALS

#include <JPEGDecoder.h>

//SPI
#define SPI_MOSI 13
#define SPI_MISO 12
#define SPI_SCK 14

//SD Card
#define SD_CS 4

//Touch Screen
#define STMPE_CS 2

//TFT
#define TFT_CS 15
#define TFT_DC 33
#define TFT_LED -1 //1//-1
#define TFT_RST -1 //3//-1

#define LCD_WIDTH 240
#define LCD_HEIGHT 320

//SPI control
#define SPI_ON_TFT digitalWrite(TFT_CS, LOW)
#define SPI_OFF_TFT digitalWrite(TFT_CS, HIGH)
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)
#define STMPE_ON digitalWrite(STMPE_CS, LOW)
#define STMPE_OFF digitalWrite(STMPE_CS, HIGH)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 touch = Adafruit_STMPE610(STMPE_CS);

String file_list[20];
int file_num = 0;
int file_index = 0;

long int runtime = 0;
int flag = 1;

void setup()
{
    Serial.begin(115200);

    //SPI init
    pinMode(SD_CS, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    pinMode(STMPE_CS, OUTPUT);
    SPI_OFF_SD;
    SPI_OFF_TFT;
    STMPE_OFF;
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

    //Read SD
    file_num = get_pic_list(SD, "/", 0, file_list);
    Serial.print("jpg file count:");
    Serial.println(file_num);
    Serial.println("All jpg:");
    for (int i = 0; i < file_num; i++)
    {
        Serial.println(file_list[i]);
    }
    SPI_OFF_SD;
    Serial.println("SD init over.");

    //TFT(SPI) init
    SPI_ON_TFT;
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    SPI_OFF_TFT;
    Serial.println("TFT init over.");

    STMPE_ON;
    if (!touch.begin())
    {
        Serial.println("STMPE not found!");
        while (1)
            ;
    }
    STMPE_OFF;
    Serial.println("STMPE init over.");
}

//====================================================================================
//                                    Loop
//====================================================================================
void loop()
{

    if ((millis() - runtime > 10000) || flag == 1)
    {
        drawJpeg(file_list[file_index].c_str(), 0, 0);
        file_index++;
        if (file_index >= file_num)
        {
            file_index = 0;
        }
        runtime = millis();
        flag = 0;
    }
    //tft.fillScreen(ILI9341_BLACK);

    //createArray("/EagleEye.jpg");
    //delay(2000);

    STMPE_ON;

    uint16_t x, y;
    uint8_t z;
    if (touch.touched())
    {
        // read x & y & z;
        while (!touch.bufferEmpty())
        {
            int pos[2] = {0, 0};
            touch.readData(&x, &y, &z);
            pos[0] = x * LCD_WIDTH / 4096;
            pos[1] = y * LCD_HEIGHT / 4096;
            /*
            Serial.println("Pos:");
            Serial.println(pos[0]);
            Serial.println(pos[1]);
            */
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
        }
        while (touch.touched())
            ;
        touch.writeRegister8(STMPE_INT_STA, 0xFF); // reset all ints, in this example unneeded depending in use
    }
    STMPE_OFF;
}

// Return the minimum of two values a and b
#define minimum(a, b) (((a) < (b)) ? (a) : (b))

void drawJpeg(const char *filename, int xpos, int ypos)
{

    Serial.println("===========================");
    Serial.print("Drawing file: ");
    Serial.println(filename);
    Serial.println("===========================");

    // Open the named file (the Jpeg decoder library will close it after rendering image)
    //fs::File jpegFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
    File jpegFile = SD.open(filename, FILE_READ); // or, file handle reference for SD library

    if (!jpegFile)
    {
        Serial.print("ERROR: File \"");
        Serial.print(filename);
        Serial.println("\" not found!");
        return;
    }

    // Use one of the three following methods to initialise the decoder:
    //boolean decoded = JpegDec.decodeFsFile(jpegFile); // Pass a SPIFFS file handle to the decoder,
    boolean decoded = JpegDec.decodeSdFile(jpegFile); // or pass the SD file handle to the decoder,
    //boolean decoded = JpegDec.decodeFsFile(filename);  // or pass the filename (leading / distinguishes SPIFFS files)
    // Note: the filename can be a String or character array type
    if (decoded)
    {
        // print information about the image to the serial port
        jpegInfo();

        // render the image onto the screen at given coordinates
        jpegRender(xpos, ypos);
    }
    else
    {
        Serial.println("Jpeg file format not supported!");
    }
}

void jpegRender(int xpos, int ypos)
{

    // retrieve infomration about the image
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
    uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // read each MCU block until there are no more
    while (JpegDec.read())
    {

        // save a pointer to the image block
        pImg = JpegDec.pImage;

        // calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x)
            win_w = mcu_w;
        else
            win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y)
            win_h = mcu_h;
        else
            win_h = min_h;

        // copy pixels into a contiguous block
        if (win_w != mcu_w)
        {
            for (int h = 1; h < win_h - 1; h++)
            {
                memcpy(pImg + h * win_w, pImg + (h + 1) * mcu_w, win_w << 1);
            }
        }

        // draw image MCU block only if it will fit on the screen
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
        {
            tft.drawRGBBitmap(mcu_x, mcu_y, pImg, win_w, win_h);
            //pic_show(tft, mcu_x, mcu_y, pImg, win_w, win_h);
            //delay(100);
        }

        else if ((mcu_y + win_h) >= tft.height())
            JpegDec.abort();
    }

    // calculate how long it took to draw the image
    drawTime = millis() - drawTime; // Calculate the time it took

    // print the results to the serial port
    Serial.print("Total render time was    : ");
    Serial.print(drawTime);
    Serial.println(" ms");
    Serial.println("=====================================");
}

void jpegInfo()
{

    Serial.println("===============");
    Serial.println("JPEG image info");
    Serial.println("===============");
    Serial.print("Width      :");
    Serial.println(JpegDec.width);
    Serial.print("Height     :");
    Serial.println(JpegDec.height);
    Serial.print("Components :");
    Serial.println(JpegDec.comps);
    Serial.print("MCU / row  :");
    Serial.println(JpegDec.MCUSPerRow);
    Serial.print("MCU / col  :");
    Serial.println(JpegDec.MCUSPerCol);
    Serial.print("Scan type  :");
    Serial.println(JpegDec.scanType);
    Serial.print("MCU width  :");
    Serial.println(JpegDec.MCUWidth);
    Serial.print("MCU height :");
    Serial.println(JpegDec.MCUHeight);
    Serial.println("===============");
    Serial.println("");
}

void createArray(const char *filename)
{

    // Open the named file
    //fs::File jpgFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
    File jpgFile = SD.open(filename, FILE_READ); // or, file handle reference for SD library

    if (!jpgFile)
    {
        Serial.print("ERROR: File \"");
        Serial.print(filename);
        Serial.println("\" not found!");
        return;
    }

    uint8_t data;
    byte line_len = 0;
    Serial.println("");
    Serial.println("// Generated by a JPEGDecoder library example sketch:");
    Serial.println("// https://github.com/Bodmer/JPEGDecoder");
    Serial.println("");
    Serial.println("#if defined(__AVR__)");
    Serial.println("  #include <avr/pgmspace.h>");
    Serial.println("#endif");
    Serial.println("");
    Serial.print("const uint8_t ");
    while (*filename != '.')
        Serial.print(*filename++);
    Serial.println("[] PROGMEM = {"); // PROGMEM added for AVR processors, it is ignored by Due

    while (jpgFile.available())
    {

        data = jpgFile.read();
        Serial.print("0x");
        if (abs(data) < 16)
            Serial.print("0");
        Serial.print(data, HEX);
        Serial.print(","); // Add value and comma
        line_len++;
        if (line_len >= 32)
        {
            line_len = 0;
            Serial.println();
        }
    }

    Serial.println("};\r\n");
    jpgFile.close();
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
            /*
            else if (temp.endsWith(".bmp"))
            {
                wavlist[i] = temp;
                i++;
            }
            */
        }
        file = root.openNextFile();
    }
    return i;
}

void pic_show(Adafruit_ILI9341 tft, int16_t x, int16_t y, uint16_t *bitmap,
              int16_t w, int16_t h)
{
    for (int16_t col = 1; col <= w; col++)
    {
        tft.startWrite();
        for (int16_t j = 0; j < h; j++) //pic_y
        {
            for (int16_t i = 0; i < col; i++) //pic_x
            {
                tft.writePixel(x + i, y + j, bitmap[j * w + i]);
            }
        }
        tft.endWrite();
        //delay(100);
    }
}