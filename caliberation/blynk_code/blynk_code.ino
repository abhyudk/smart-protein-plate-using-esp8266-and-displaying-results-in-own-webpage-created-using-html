// Blynk Template Configuration - MUST be before BlynkSimpleEsp8266.h
#define BLYNK_TEMPLATE_ID "TMPL3CUJkV4Zb"
#define BLYNK_TEMPLATE_NAME "protein calculator"
#define BLYNK_AUTH_TOKEN "0jAzJDLli4_P4YIEnBA641z_eB_CDUt2"


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "HX711.h"


// HX711 Pins
#define DOUT  4  // D2 on ESP8266
#define CLK   5  // D1 on ESP8266
HX711 scale(DOUT, CLK);

// Configuration Thresholds
const float MIN_INITIAL_WEIGHT = 135.0;
const float MIN_ADDED_WEIGHT   = 40.0;
const int   STABLE_TOLERANCE   = 5;

// WiFi Credentials
char ssid[] = "Abhyud4G_EXT";
char pass[] = "Abhi1206";

// Global Variables
float calibration_factor = 317.27;
float prev_weight = 0.0;
float total_protein = 0.0;
bool first_food_added = false;
int current_food = 0;

// Food Database
struct FoodInfo {
  int id;
  const char* name;
  float protein_per_100g;
  float total_weight;
  float total_protein;
};

FoodInfo foodDB[] = {
  {1, "Chicken", 27.0, 0, 0},
  {2, "Egg", 13.0, 0, 0},
  {3, "Rice", 2.7, 0, 0},
  {4, "Broccoli", 2.8, 0, 0},
  {5, "Show Summary", 0, 0, 0} // Special entry for summary
};

void showSummary() {
  String summary = "=== Protein Summary ===\n";
  for (int i = 0; i < sizeof(foodDB)/sizeof(FoodInfo)-1; i++) {
    if (foodDB[i].total_weight > 0) {
      summary += String(foodDB[i].name) + ": " + 
                String(foodDB[i].total_weight) + "g | " +
                String(foodDB[i].total_protein) + "g\n";
    }
  }
  summary += "Total: " + String(total_protein) + "g";
  
  Blynk.virtualWrite(V2, summary);
  Serial.println(summary);
}

float waitForStableWeight() {
  const int num_readings = 4;
  float readings[num_readings];

  Serial.println("Measuring weight...");

  while (true) {
    for (int i = 0; i < num_readings; i++) {
      readings[i] = scale.get_units(5);
      if (readings[i] < 0) readings[i] = 0;
      Serial.print("Reading ");
      Serial.print(i+1);
      Serial.print(": ");
      Serial.println(readings[i]);
      delay(100);
    }

    float base = readings[0];
    bool within_range = true;

    for (int i = 1; i < num_readings; i++) {
      if (abs(readings[i] - base) > STABLE_TOLERANCE) {
        within_range = false;
        break;
      }
    }

    if (within_range) {
      return readings[num_readings-1];
    }
    Serial.println("Unstable weight. Retrying...");
    delay(200);
  }
}

BLYNK_WRITE(V1) {
  int selection = param.asInt();
  
  if (selection == 5) { // Show Summary selected
    showSummary();
    return;
  }

  // Regular food selection (1-4)
  current_food = selection-1;
  Serial.print(foodDB[current_food].name);
  Serial.println(" selected. Place food on scale...");

  if (first_food_added) {
    Serial.print("Waiting for additional weight (");
    Serial.print(MIN_ADDED_WEIGHT);
    Serial.println("g minimum)...");
    while (true) {
      float current = scale.get_units(5);
      if (current < 0) current = 0;
      if (current >= prev_weight + MIN_ADDED_WEIGHT) break;
      delay(300);
    }
  }

  float stable_weight = waitForStableWeight();
  
  if (!first_food_added && stable_weight < MIN_INITIAL_WEIGHT) {
    Serial.print("Weight too low! Minimum ");
    Serial.print(MIN_INITIAL_WEIGHT);
    Serial.println("g required");
    return;
  }

  float added_weight = first_food_added ? (stable_weight - prev_weight) : stable_weight;
  
  if (added_weight <= 0) {
    Serial.println("No additional weight detected");
    return;
  }

  prev_weight = stable_weight;
  float added_protein = (added_weight * foodDB[current_food].protein_per_100g) / 100.0;

  total_protein += added_protein;
  foodDB[current_food].total_weight += added_weight;
  foodDB[current_food].total_protein += added_protein;
  first_food_added = true;

  Serial.print("Added weight: ");
  Serial.print(added_weight);
  Serial.println(" g");
  Serial.print("Protein added: ");
  Serial.print(added_protein);
  Serial.println(" g");
  Serial.print("Total protein so far: ");
  Serial.print(total_protein);
  Serial.println(" g");

  showSummary();
  Serial.println("Ready for next food selection");
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  scale.set_scale(calibration_factor);
  scale.tare();
  
  Serial.println("\nProtein Calculator Ready");
  Serial.print("Initial food weight minimum: ");
  Serial.print(MIN_INITIAL_WEIGHT);
  Serial.println("g");
  
  Blynk.virtualWrite(V2, "No foods added yet");
}

void loop() {
  Blynk.run();
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    float current = scale.get_units(5);
    if (current < 0) current = 0;
    Serial.print("Current weight: ");
    Serial.println(current);
  }
}