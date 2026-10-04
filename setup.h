#define USER_SETUP_INFO "ST7789_240x240_ESP32"

#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO -1
#define TFT_CS   -1
#define TFT_DC   2
#define TFT_RST  4

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SPI_FREQUENCY  20000000
#define SPI_READ_FREQUENCY  10000000
