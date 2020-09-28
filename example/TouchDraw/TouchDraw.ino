#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_STMPE610.h"

#define LCD_MOSI 13
#define LCD_MISO 12
#define LCD_SCK 14 //14
#define LCD_CS 15
#define LCD_RST -1
#define LCD_DC 33 //33
#define LCD_BL -1 //5

#define LCD_WIDTH 240
#define LCD_HEIGHT 320
#define LCD_SPI_HOST VSPI_HOST

#define SDA 26
#define SCL 27

#define STMPE_CS 2

#define LCD_ON digitalWrite(LCD_CS, LOW)
#define LCD_OFF digitalWrite(LCD_CS, HIGH)
#define STMPE_ON digitalWrite(STMPE_CS, LOW)
#define STMPE_OFF digitalWrite(STMPE_CS, HIGH)

//Adafruit_ILI9341 tft = Adafruit_ILI9341(LCD_CS, LCD_DC, LCD_MOSI, LCD_SCK, LCD_RST, LCD_MISO);
Adafruit_ILI9341 tft = Adafruit_ILI9341(LCD_CS, LCD_DC, LCD_RST);
//I2C
//Adafruit_STMPE610 touch = Adafruit_STMPE610();
//SPI
Adafruit_STMPE610 touch = Adafruit_STMPE610(STMPE_CS);

int draw_color = ILI9341_WHITE;

int last_pos[2] = {0, 0};

void setup()
{

    Serial.begin(115200);
    Serial.println("ILI9341 Test!");
    SPI.begin(LCD_SCK, LCD_MISO, LCD_MOSI);

    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_DC, OUTPUT);
    pinMode(STMPE_CS, OUTPUT);
    STMPE_OFF;
    LCD_OFF;

    LCD_ON;
    tft.begin();
    tft.fillScreen(ILI9341_BLACK);
    tft.fillRect(0, 0, 60, 30, ILI9341_RED);
    tft.fillRect(60, 0, 60, 30, ILI9341_GREEN);
    tft.fillRect(120, 0, 60, 30, ILI9341_BLUE);
    tft.fillRect(180, 0, 60, 30, ILI9341_YELLOW);
    LCD_OFF;

    //Wire.begin(SDA, SCL);

    STMPE_ON;
    if (!touch.begin())
    {
        Serial.println("STMPE not found!");
        while (1)
            ;
    }
}

void loop()
{

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
            if (filter(last_pos, pos, 18))
            {
                tft.fillRect(pos[0], pos[1], 3, 3, ILI9341_RED);
            }

            
            last_pos[0] = pos[0];
            last_pos[1] = pos[1];
            */
            if (0 < pos[1] && pos[1] < 30)
            {
                if (0 < pos[0] && pos[0] < 60)
                {
                    draw_color = ILI9341_RED;
                }
                else if (60 < pos[0] && pos[0] < 120)
                {
                    draw_color = ILI9341_GREEN;
                }

                else if (120 < pos[0] && pos[0] < 180)
                {
                    draw_color = ILI9341_BLUE;
                }
                else if (180 < pos[0] && pos[0] < 240)
                {
                    draw_color = ILI9341_YELLOW;
                }
            }
            else
            {
                STMPE_OFF;
                
                LCD_ON;
                tft.fillRect(pos[0], pos[1], 2, 2, draw_color);
                LCD_OFF;
                
                STMPE_ON;
            }
        }
        touch.writeRegister8(STMPE_INT_STA, 0xFF); // reset all ints, in this example unneeded depending in use
    }
}

int filter(int last_pos[2], int pos[2], int level)
{
    int temp = (last_pos[0] - pos[0]) * (last_pos[0] - pos[0]) + (last_pos[1] - pos[1]) * (last_pos[1] - pos[1]);
    last_pos[0] = pos[0];
    last_pos[1] = pos[1];
    if (temp > level)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}