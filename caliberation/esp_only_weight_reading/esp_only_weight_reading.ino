#include "HX711.h"

#define DOUT D2  // GPIO4
#define CLK  D1  // GPIO5

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(DOUT, CLK);

  // Set your calibration factor here
  scale.set_scale(205.91);  // Use your calibrated value
  scale.tare();  // Reset scale to 0

  Serial.println("HX711 Weight Scale Ready");
}

void loop() {
  // Get average of 5 readings
  float weight = scale.get_units(5);  

  if (weight < 0) weight = 0; // Clamp negative readings to zero

  Serial.print("Weight: ");
  Serial.print(weight, 2);  // Print with 2 decimal places
  Serial.println(" g");

  delay(500);  // Half a second delay between readings
}
