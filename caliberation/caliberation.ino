#include "HX711.h"

#define DOUT  4
#define CLK   5
HX711 scale(DOUT, CLK);

float weight; 
float calibration_factor = 317.27 ; // Start with your known value
//float calibration_factor = 97.27 ;
//float calibration_factor = 209.34 ; //for 3.3v just trying
void setup() {
  Serial.begin(9600);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale.set_scale();
  scale.tare(); // Reset the scale to 0

  long zero_factor = scale.read_average();
  Serial.print("Zero factor: ");
  Serial.println(zero_factor);
}

void loop() {
  // Adjust scale with the current calibration factor
  scale.set_scale(calibration_factor); 
  weight = scale.get_units(5); 

  if (weight < 0) {
    weight = 0.00;
  }

  // Print weight and calibration info
  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.print(" gm");
  Serial.print(" | Calibration factor: ");
  Serial.println(calibration_factor);

  // Check for serial input to adjust calibration factor
  if (Serial.available()) {
    char input = Serial.read();
    if (input == '+' || input == 'a') {
      calibration_factor += 10;
    } else if (input == '-' || input == 'z') {
      calibration_factor -= 10;
    }
  }

  delay(300);
}

