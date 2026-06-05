// IT WORKS!

#include <TFT_eSPI.h>
#include "RLEed_Frames_new.h" // Your generated RLE data

// ---------- TUNING PARAMETERS ----------
#define ADC_MIN 2000
#define ADC_MAX 4000

const float FRAME_SMOOTHING = 0.25f;  // 0.1 = very smooth, 0.5 = very responsive
const uint32_t FRAME_INTERVAL = 16;   // 16ms = ~60 FPS

// ---------- VARIABLES ----------
float filteredFrame = 0.0f;
uint32_t lastDrawTime = 0;

// ---------- HELPER ----------
int median3(int a, int b, int c)
{
  int t;

  if (a > b)
  {
    t = a;
    a = b;
    b = t;
  }

  if (b > c)
  {
    t = b;
    b = c;
    c = t;
  }

  if (a > b)
  {
    t = a;
    a = b;
    b = t;
  }

  return b;
}

TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite sprite = TFT_eSprite(&tft);

#define POT_PIN 36

int last_frame = -1;
float smoothed_pot = 0.0;

void setup() {
  Serial.begin(115200);
  
  tft.init();
  tft.setRotation(0); // Adjust to your physical rotation
  tft.fillScreen(TFT_PINK);

  // Initialize the sprite. bounding box defined in animation .h file (ANIM_WIDTH, ANIM_HEIGHT)
  if (!sprite.createSprite(ANIM_WIDTH, ANIM_HEIGHT)) {
    Serial.println("Failed to allocate Sprite! Check RAM.");
    while (1);
  }
  
  sprite.setSwapBytes(false); // We handle byte order in Python

  smoothed_pot = analogRead(POT_PIN);
}

/*
void loop() {
  int raw_pot = analogRead(POT_PIN);
  
  // 2. Apply Exponential Moving Average (EMA) filter to kill noise
  // 0.1 is the filter strength. Lower = smoother but slight lag. Higher = faster but jittery.
  smoothed_pot = (0.1 * raw_pot) + (0.9 * smoothed_pot); 
  
  // 3. Map the smoothed value. 
  // We use 50 to 4000 instead of 0 to 4095 because ESP32 ADCs struggle at absolute extremes.
  int current_frame = map((int)smoothed_pot, 2000, 4000, 0, ANIM_TOTAL_FRAMES - 1);
  
  // 4. ABSOLUTE CONSTRAINT: This prevents the looping and jumping to 0
  current_frame = constrain(current_frame, 0, ANIM_TOTAL_FRAMES - 1);

    if (current_frame != last_frame) {
      drawRLEFrame(current_frame);
      last_frame = current_frame;
    }
}
*/

// improved by ChatGPT
void loop(){
  // FPS limiter
  uint32_t now = millis();

  if (now - lastDrawTime < FRAME_INTERVAL)
    return;

  lastDrawTime = now;

  // Read ADC 3 times and take median
  int adc1 = analogRead(POT_PIN);
  int adc2 = analogRead(POT_PIN);
  int adc3 = analogRead(POT_PIN);

  int rawADC = median3(adc1, adc2, adc3);

  // Clamp ADC range
  rawADC = constrain(rawADC, ADC_MIN, ADC_MAX);

  // Convert ADC to target frame
  int targetFrame = map(
      rawADC,
      ADC_MIN,
      ADC_MAX,
      0,
      ANIM_TOTAL_FRAMES - 1);

  // Smooth frame movement
  filteredFrame +=
      (targetFrame - filteredFrame) * FRAME_SMOOTHING;

  int currentFrame = round(filteredFrame);

  currentFrame = constrain(
      currentFrame,
      0,
      ANIM_TOTAL_FRAMES - 1);

  // Draw only when frame changes
  if (currentFrame != last_frame)
  {
    drawRLEFrame(currentFrame);
    last_frame = currentFrame;
  }
}
  
  //delay(5); // Small delay to let ADC breathe

void drawRLEFrame(int frame_index) {
  // Get pointer to the chosen frame data in PROGMEM
  const uint8_t* frame_data = (const uint8_t*)pgm_read_ptr(&animation_frames[frame_index]);
  
  // Get the direct memory pointer to the Sprite's VRAM for ultra-fast writing
  uint16_t* sprite_ptr = (uint16_t*)sprite.getPointer();
  
  uint32_t data_idx = 0;
  uint32_t pixel_count = 0;
  const uint32_t total_pixels = ANIM_WIDTH * ANIM_HEIGHT;
  
  // Decode RLE and blast it into the Sprite RAM
  while (pixel_count < total_pixels) {
    uint8_t count = pgm_read_byte(&frame_data[data_idx++]);
    uint8_t color_high = pgm_read_byte(&frame_data[data_idx++]);
    uint8_t color_low = pgm_read_byte(&frame_data[data_idx++]);
    
    // FIX: Swap the bytes! The ESP32 is Little-Endian, but the SPI display expects Big-Endian.
    uint16_t color = (color_low << 8) | color_high;
    
    for (uint8_t i = 0; i < count; i++) {
      if (pixel_count < total_pixels) {
        sprite_ptr[pixel_count++] = color;
      }
    }
  }
  
  // Push the completed Sprite to the display at your defined X, Y coordinates
  sprite.pushSprite(ANIM_X,ANIM_Y);
}
