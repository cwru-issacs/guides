
#include "esp32_digital_led_lib.h"
#include "esp32_digital_led_funcs.h"
#include "tbx_banner.h"

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"  // It's noisy here with `-Wall`

strand_t strand = {.rmtChannel = 0, .gpioNum = 5, .ledType = LED_WS2812B_V3, .brightLimit = 64, .numPixels = 200};

strand_t * STRANDS [] = { &strand };

int STRANDCNT = COUNT_OF(STRANDS); 

#pragma GCC diagnostic pop

#define GRID_X 8
#define GRID_Y 25

void setGridPixelRGB(strand_t &strand, uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {

  int pixel_index;
  
  if ( (x % 2) == 0 )
    pixel_index = x * GRID_Y + y;
  else
    pixel_index = x * GRID_Y + (GRID_Y-1) - y;

  // Verify x/y position is on LED strip
  if (pixel_index > strand.numPixels) {
    Serial.print("Bad pixel Index"); Serial.println(pixel_index);
    return;
  }

  // Set the pixel color
  strand.pixels[pixel_index] = pixelFromRGB(r, g, b);

  return;  
}


//**************************************************************************//
void setup()
{
  // Start the Serial Monitor
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize the LED Strands
  digitalLeds_initDriver();

  gpioSetup(strand.gpioNum, OUTPUT, LOW);
  int rc = digitalLeds_addStrands(STRANDS, STRANDCNT);
  if (rc) {
    Serial.print("Init rc = ");
    Serial.println(rc);
  }

  if (digitalLeds_initDriver()) {
    Serial.println("Init FAILURE: halting");
    while (true) {};
  }
  digitalLeds_resetPixels(STRANDS, STRANDCNT);
 
  delay(100);
  Serial.println("Init complete");
}


void displayBanner(strand_t &strand, const uint8_t * banner_data, uint8_t banner_rows) {

  uint32_t dimmer = 0x001F1F1F;
  
  for (int row = 0; row < banner_rows; row++) {

    // Write Solid Background
    for (int x = 0; x < GRID_X; x++) {
      for (int y = 0; y < GRID_Y; y++) {
        setGridPixelRGB(strand, x, y, 0, 0, 42); // Blue
      }
    }

//    // Random Background
//    for (uint16_t i = 0; i < strand.numPixels; i++) {
//      strand.pixels[i].raw32 = (esp_random() & dimmer);
//    }


    // Write Banner    
    for (int y = 0; y < GRID_Y; y++) {
      uint8_t b = 0x80;
      for (int x = 0; x < GRID_X; x++) {
        if (b & banner_data[(row + y)%banner_rows]) {
          setGridPixelRGB(strand, x, y, 64, 64, 64);
        }
        b >>= 1;
      }
    }
    digitalLeds_drawPixels(STRANDS, STRANDCNT);
    delay(100);
  }

}

//**************************************************************************//
void loop()
{
  Serial.println("Rendering");

  // Strand Test
//  for(int i = 0; i < strand.numPixels; i++) {
//    if (i > 0)
//      strand.pixels[i-1] = pixelFromRGB(0, 0, 0);
//    else
//      strand.pixels[strand.numPixels-1] = pixelFromRGB(0, 0, 0);
//
//    strand.pixels[i] = pixelFromRGB(64, 64, 64);
//
//    digitalLeds_drawPixels(STRANDS, STRANDCNT);
//    delay(100);
//  }

  // Test Grid Pixels
//  for (int y = 0; y < GRID_Y; y++) {
//    for (int x = 0; x < GRID_X; x++) {
//      setGridPixelRGB(strand, x, y, 64, 64, 64);
//      digitalLeds_drawPixels(STRANDS, STRANDCNT);
//      delay(100);
//      setGridPixelRGB(strand, x, y, 0, 0, 0);
//    }
//  }

  // Display the think[box] banner
  displayBanner(strand, thinkbox_banner_bitmap, TBX_BANNER_ROWS);
  
}
