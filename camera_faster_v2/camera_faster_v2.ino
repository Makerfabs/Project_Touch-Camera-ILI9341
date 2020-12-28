#include <SPI.h>
//#include "Adafruit_GFX.h"
//#include "Adafruit_ILI9341.h"
#include <LovyanGFX.hpp>
#include "Adafruit_STMPE610.h"
#include "SD.h"
#include "FS.h"

#include "esp_camera.h"
#define CAMERA_MODEL_MAKERFABS
#include "camera_pins.h"

#define ARRAY_LENGTH 320 * 240 * 3
#define SCRENN_ROTATION 3

#define SDA 26
#define SCL 27

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

//SPI control
#define SPI_ON_TFT digitalWrite(TFT_CS, LOW)
#define SPI_OFF_TFT digitalWrite(TFT_CS, HIGH)
#define SPI_ON_SD digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD digitalWrite(SD_CS, HIGH)
#define STMPE_ON digitalWrite(STMPE_CS, LOW)
#define STMPE_OFF digitalWrite(STMPE_CS, HIGH)

//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 touch = Adafruit_STMPE610(STMPE_CS);

struct LGFX_Config
{
    static constexpr spi_host_device_t spi_host = VSPI_HOST;
    static constexpr int dma_channel = 1;
    static constexpr int spi_sclk = SPI_SCK;
    static constexpr int spi_mosi = SPI_MOSI;
    static constexpr int spi_miso = SPI_MISO;
};

static lgfx::LGFX_SPI<LGFX_Config> tft;
static LGFX_Sprite sprite(&tft);
static lgfx::Panel_ILI9341 panel;

void setup()
{

    esp32_init();
    camera_init();
}

void loop()
{
    camera_fb_t *fb = NULL;

    fb = esp_camera_fb_get();

    drawRGBBitmap(fb->buf, fb->width, fb->height);

    esp_camera_fb_return(fb);
}

//ILI9341 init and SD card init
void esp32_init()
{
    Serial.begin(115200);
    Serial.println("ILI9341 Test!");

    //I2C init
    Wire.begin(SDA, SCL);

    //SPI init
    pinMode(SD_CS, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    pinMode(STMPE_CS, OUTPUT);
    SPI_OFF_SD;
    SPI_OFF_TFT;
    STMPE_OFF;
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    //TFT(SPI) init
    SPI_ON_TFT;

    set_tft();

    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(3);

    Serial.println("TFT init over.");
    SPI_OFF_TFT;

    SPI_ON_SD;
    //SD(SPI) init
    if (!SD.begin(SD_CS, SPI, 80000000))
    {
        SPI_OFF_SD;
        SPI_ON_TFT;

        //tft.setRotation(1);
        tft.fillScreen(TFT_BLACK);

        tft.setTextColor(TFT_RED);
        tft.setTextSize(3);
        tft.setCursor(0, 0);
        tft.println("SD Card Mount Failed");
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(0, 60);
        tft.println("You can continue testing the camera after 3 seconds");
        tft.setTextColor(TFT_YELLOW);
        tft.setCursor(0, 150);
        tft.println("Or restart after re-inserting the SD card");
        Serial.println("Card Mount Failed");
        delay(3000);
        SPI_OFF_TFT;
        //while (1);
    }
    else
    {
        Serial.println("Card Mount Successed");
        tft.setRotation(3);
        print_img(SD, "/logo320240.bmp");
        delay(2000);
        tft.fillScreen(TFT_BLACK);
    }
    Serial.println("SD init over.");

    STMPE_ON;
    if (!touch.begin())
    {
        STMPE_OFF;
        SPI_ON_TFT;
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.setCursor(0, 0);
        tft.println("Touch Screen ERROR.");
        Serial.println("STMPE not found!");
        while (1)
            ;
    }

    //Draw Touch Button
    STMPE_OFF;
    SPI_ON_TFT;
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(0, 0);
    tft.println("Touch Button to continue.");
    tft.fillRect(100, 100, 120, 120, TFT_BLUE);
    tft.setCursor(110, 110);
    tft.setTextSize(2);
    tft.println("CONTINUE");
    tft.setRotation(3);
    SPI_OFF_TFT;
    STMPE_ON;

    while (1)
    {
        if (touch.touched())
        {
            uint16_t x, y;
            uint8_t z;
            int flag = 0;
            // read x & y & z;
            while (!touch.bufferEmpty())
            {
                Serial.print(touch.bufferSize());
                touch.readData(&x, &y, &z);

                x = x * 240 / 4096;
                y = 320 - y * 320 / 4096;

                Serial.print("->(");
                Serial.print(y);
                Serial.print(", ");
                Serial.println(x);

                if (y > 100 && y < 220 && x > 120 && x < 240)
                    flag = 1;
            }
            touch.writeRegister8(STMPE_INT_STA, 0xFF); // reset all ints, in this example unneeded depending in use
            if (flag == 1)
                break;
        }
        delay(10);
    }
    STMPE_OFF;
}

//Camera setting
void camera_init()
{
    //camera config
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;

    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        while (1)
            ;
    }

    sensor_t *s = esp_camera_sensor_get();
    //initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV2640_PID)
    {
        s->set_vflip(s, 0);      //flip it back
        s->set_brightness(s, 0); //up the blightness just a bit
        s->set_saturation(s, 1); //lower the saturation
    }
    //drop down frame size for higher initial frame rate
    s->set_framesize(s, FRAMESIZE_QVGA);
}

void drawRGBBitmap(uint8_t *bitmap, int16_t w, int16_t h)
{
    SPI_ON_TFT;
    tft.startWrite();
    for (int16_t j = 0; j < h; j++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            uint16_t temp = bitmap[j * w * 2 + 2 * i] * 256 + bitmap[j * w * 2 + 2 * i + 1];
            tft.writePixel(i, j, temp);
        }
    }
    tft.endWrite();
    SPI_OFF_TFT;
}

//From SD
int print_img(fs::FS &fs, String filename)
{
    SPI_ON_SD;
    File f = fs.open(filename);
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        return 0;
    }

    f.seek(54);
    int X = 320;
    int Y = 240;
    uint8_t RGB[3 * X];
    for (int row = 0; row < Y; row++)
    {
        f.seek(54 + 3 * X * row);
        f.read(RGB, 3 * X);
        SPI_OFF_SD;
        SPI_ON_TFT;
        for (int col = 0; col < X; col++)
        {
            tft.drawPixel(col, row, tft.color565(RGB[col * 3 + 2], RGB[col * 3 + 1], RGB[col * 3]));
        }
        SPI_OFF_TFT;
        SPI_ON_SD;
    }

    f.close();
    SPI_OFF_SD;
    return 0;
}

void set_tft()
{
    // パネルクラスに各種設定値を代入していきます。
    // （LCD一体型製品のパネルクラスを選択した場合は、
    //   製品に合った初期値が設定されているので設定は不要です）

    // 通常動作時のSPIクロックを設定します。
    // ESP32のSPIは80MHzを整数で割った値のみ使用可能です。
    // 設定した値に一番近い設定可能な値が使用されます。
    panel.freq_write = 60000000;
    //panel.freq_write = 20000000;

    // 単色の塗り潰し処理時のSPIクロックを設定します。
    // 基本的にはfreq_writeと同じ値を設定しますが、
    // より高い値を設定しても動作する場合があります。
    panel.freq_fill = 60000000;
    //panel.freq_fill  = 27000000;

    // LCDから画素データを読取る際のSPIクロックを設定します。
    panel.freq_read = 16000000;

    // SPI通信モードを0~3から設定します。
    panel.spi_mode = 0;

    // データ読み取り時のSPI通信モードを0~3から設定します。
    panel.spi_mode_read = 0;

    // 画素読出し時のダミービット数を設定します。
    // 画素読出しでビットずれが起きる場合に調整してください。
    panel.len_dummy_read_pixel = 8;

    // データの読取りが可能なパネルの場合はtrueを、不可の場合はfalseを設定します。
    // 省略時はtrueになります。
    panel.spi_read = true;

    // データの読取りMOSIピンで行うパネルの場合はtrueを設定します。
    // 省略時はfalseになります。
    panel.spi_3wire = false;

    // LCDのCSを接続したピン番号を設定します。
    // 使わない場合は省略するか-1を設定します。
    panel.spi_cs = TFT_CS;

    // LCDのDCを接続したピン番号を設定します。
    panel.spi_dc = TFT_DC;

    // LCDのRSTを接続したピン番号を設定します。
    // 使わない場合は省略するか-1を設定します。
    panel.gpio_rst = TFT_RST;

    // LCDのバックライトを接続したピン番号を設定します。
    // 使わない場合は省略するか-1を設定します。
    panel.gpio_bl = TFT_LED;

    // バックライト使用時、輝度制御に使用するPWMチャンネル番号を設定します。
    // PWM輝度制御を使わない場合は省略するか-1を設定します。
    panel.pwm_ch_bl = -1;

    // バックライト点灯時の出力レベルがローかハイかを設定します。
    // 省略時は true。true=HIGHで点灯 / false=LOWで点灯になります。
    panel.backlight_level = true;

    // invertDisplayの初期値を設定します。trueを設定すると反転します。
    // 省略時は false。画面の色が反転している場合は設定を変更してください。
    panel.invert = false;

    // パネルの色順がを設定します。  RGB=true / BGR=false
    // 省略時はfalse。赤と青が入れ替わっている場合は設定を変更してください。
    panel.rgb_order = false;

    // パネルのメモリが持っているピクセル数（幅と高さ）を設定します。
    // 設定が合っていない場合、setRotationを使用した際の座標がずれます。
    // （例：ST7735は 132x162 / 128x160 / 132x132 の３通りが存在します）
    panel.memory_width = 240;
    panel.memory_height = 320;

    // パネルの実際のピクセル数（幅と高さ）を設定します。
    // 省略時はパネルクラスのデフォルト値が使用されます。
    panel.panel_width = 240;
    panel.panel_height = 320;

    // パネルのオフセット量を設定します。
    // 省略時はパネルクラスのデフォルト値が使用されます。
    panel.offset_x = 0;
    panel.offset_y = 0;

    // setRotationの初期化直後の値を設定します。
    panel.rotation = 0;

    // setRotationを使用した時の向きを変更したい場合、offset_rotationを設定します。
    // setRotation(0)での向きを 1の時の向きにしたい場合、 1を設定します。
    panel.offset_rotation = 0;

    // 設定を終えたら、LGFXのsetPanel関数でパネルのポインタを渡します。
    tft.setPanel(&panel);
}