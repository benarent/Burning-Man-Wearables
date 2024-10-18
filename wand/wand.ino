#include <Adafruit_NeoPixel.h>

#define N_PIXELS  38  // Number of pixels you are using
#define MIC_PIN    1  // Microphone is attached to Trinket GPIO #2/Gemma D2 (A1)
#define LED_PIN    0  // NeoPixel LED strand is connected to GPIO #0 / D0
#define DC_OFFSET  0  // DC offset in mic signal - if unsure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   60  // Length of buffer for dynamic level adjustment
#define TOP       (N_PIXELS + 1) // Allow dot to go slightly off scale
#define PEAK_FALL 40  // Rate of peak falling dot
#define GAIN      8   // Increased gain for better sensitivity

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Halloween color palette
uint32_t halloweenColors[] = {
  strip.Color(128, 0, 0),    // Deep Red
  strip.Color(64, 0, 0),     // Dark Red
  strip.Color(128, 20, 0),   // Dark Orange
  strip.Color(64, 10, 0),    // Dim Orange
  strip.Color(48, 0, 48),    // Purple
  strip.Color(24, 0, 24),    // Dark Purple
  strip.Color(0, 48, 0),     // Dark Green
  strip.Color(0, 24, 0)      // Dim Green
};

// Ice color palette
uint32_t iceColors[] = {
  strip.Color(255, 255, 255),  // White
  strip.Color(200, 200, 255),  // Pale Blue
  strip.Color(150, 150, 255),  // Light Blue
  strip.Color(100, 100, 255),  // Medium Blue
  strip.Color(50, 50, 255),    // Deep Blue
  strip.Color(0, 0, 255),      // Pure Blue
  strip.Color(0, 100, 255),    // Sky Blue
  strip.Color(0, 200, 255)     // Cyan
};

#define NUM_HALLOWEEN_COLORS (sizeof(halloweenColors) / sizeof(halloweenColors[0]))
#define NUM_ICE_COLORS (sizeof(iceColors) / sizeof(iceColors[0]))

uint16_t sample[SAMPLES];
uint16_t samplePos = 0;
uint16_t peakToPeak = 0;
uint16_t minLvlAvg = 0;
uint16_t maxLvlAvg = 512;
uint8_t volCount = 0;
unsigned long lastThemeChange = 0;
bool isHalloweenTheme = true;

void setup() {
  memset(sample, 0, sizeof(sample));
  strip.begin();
  strip.show();
}

void loop() {
  uint16_t minLvl, maxLvl;
  int n, height;

  n = analogRead(MIC_PIN);                        // Raw reading from mic 
  n = abs(n - 512 - DC_OFFSET); // Center on zero
  n = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  sample[samplePos] = n;                          // Save this reading
  samplePos = (samplePos + 1) % SAMPLES;          // Advance/rollover sample counter

  // Get volume range of prior 64 samples
  minLvl = maxLvl = sample[0];
  for(int i=1; i<SAMPLES; i++) {
    if(sample[i] < minLvl)      minLvl = sample[i];
    else if(sample[i] > maxLvl) maxLvl = sample[i];
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  
  // Adjust min and max levels towards desired targets
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (maxLvl - minLvl) / (maxLvlAvg - minLvlAvg);

  // Clip output and convert to byte:
  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;

  // Check if it's time to switch themes (every 5 minutes)
  if (millis() - lastThemeChange >= 300000) { // 300000 ms = 5 minutes
    isHalloweenTheme = !isHalloweenTheme; // Toggle between Halloween and Ice
    lastThemeChange = millis();
  }

  if (isHalloweenTheme) {
    advancedMagicWand(height, halloweenColors, NUM_HALLOWEEN_COLORS);
  } else {
    advancedIceEffect(height, iceColors, NUM_ICE_COLORS);
  }

  strip.show();

  volCount = (volCount + 1) % 64;

  delay(10);  // Small delay to control the speed of effects
}

void advancedMagicWand(int height, uint32_t* colors, int numColors) {
  static uint16_t step = 0;
  static uint8_t baseHue = 0;
  
  // Create a flowing effect
  for (int i = 0; i < N_PIXELS; i++) {
    // Calculate the hue for this pixel
    uint8_t hue = baseHue + (i * 256 / N_PIXELS);
    
    // Use a sine wave to create a flowing brightness effect
    uint8_t brightness = sin8(((i * 16) + step) % 256);
    
    // Adjust brightness based on sound level
    brightness = scale8(brightness, map(height, 0, TOP, 64, 255));
    
    // Add occasional sparkles
    if (random8() < 8) {
      brightness = qadd8(brightness, random8(64, 255));
    }
    
    // Set the pixel color
    strip.setPixelColor(i, strip.ColorHSV(hue, 255, brightness));
  }
  
  // Add a "magic spark" effect
  if (height > TOP / 2) {
    uint8_t sparkPos = random8(N_PIXELS);
    uint32_t sparkColor = colors[random8(numColors)];
    strip.setPixelColor(sparkPos, sparkColor);
    
    // Add a subtle trail to the spark
    for (int j = 1; j <= 2; j++) {
      int trailPos = (sparkPos + j) % N_PIXELS;
      uint32_t trailColor = fadeColor(sparkColor, 255 - (j * 64));
      strip.setPixelColor(trailPos, trailColor);
    }
  }
  
  // Slowly change the base hue for variety
  if (step % 32 == 0) {
    baseHue++;
  }
  
  step++;
}

void advancedIceEffect(int height, uint32_t* colors, int numColors) {
  static uint16_t step = 0;
  
  // Create a shimmering ice effect
  for (int i = 0; i < N_PIXELS; i++) {
    // Use a combination of sine waves for a more complex shimmer
    uint8_t shimmer = sin8(((i * 16) + step) % 256) + sin8(((i * 23) + step * 2) % 256);
    shimmer = shimmer / 2;  // Average the two sine waves
    
    // Adjust shimmer based on sound level
    shimmer = scale8(shimmer, map(height, 0, TOP, 128, 255));
    
    // Select a color from the ice palette
    uint32_t baseColor = colors[i % numColors];
    
    // Apply the shimmer effect
    uint32_t pixelColor = fadeColor(baseColor, shimmer);
    
    // Set the pixel color
    strip.setPixelColor(i, pixelColor);
  }
  
  // Add "ice crystal" sparkles
  if (height > TOP / 3) {
    for (int j = 0; j < 3; j++) {  // Add multiple sparkles
      uint8_t sparkPos = random8(N_PIXELS);
      uint32_t sparkColor = strip.Color(255, 255, 255);  // White sparkle
      strip.setPixelColor(sparkPos, sparkColor);
    }
  }
  
  step++;
}

// Helper function to fade a color
uint32_t fadeColor(uint32_t color, uint8_t fade) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  
  r = scale8(r, fade);
  g = scale8(g, fade);
  b = scale8(b, fade);
  
  return strip.Color(r, g, b);
}

// Fast 8-bit approximation of sin(x)
uint8_t sin8(uint8_t x) {
  uint8_t y = x - 64;
  uint8_t s = (y & 128) ? -y : y;
  uint8_t r = ((uint16_t)s * (uint16_t)s) >> 7;
  return ((s > 0) ? 255 - r : r) + 128;
}

// scale8 function from FastLED library
uint8_t scale8(uint8_t i, uint8_t scale) {
  return (((uint16_t)i) * (1 + (uint16_t)(scale))) >> 8;
}

// add8 with saturation function from FastLED library
uint8_t qadd8(uint8_t i, uint8_t j) {
  unsigned int t = i + j;
  if (t > 255) t = 255;
  return t;
}

// random8 function, similar to FastLED's implementation
uint8_t random8() {
  static uint16_t seed = 1337;
  seed = (seed * 2053) + 13849;
  return (uint8_t)(seed >> 8);
}

uint8_t random8(uint8_t lim) {
  return random8() % lim;
}

uint8_t random8(uint8_t min, uint8_t lim) {
  return min + random8(lim - min);
}