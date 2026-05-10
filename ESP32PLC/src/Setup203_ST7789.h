// ST7789 240 x 280 display
#define USER_SETUP_ID 203

#define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 280

#define CGRAM_OFFSET
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_OFF

#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC    5
#define TFT_RST   4

// Required on ESP32-S3: FSPI=0 → REG_SPI_BASE(0)=0 → crash without this
#define USE_FSPI_PORT

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
