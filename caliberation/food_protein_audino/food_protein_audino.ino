#include "HX711.h"

#define DOUT  4
#define CLK   5
HX711 scale(DOUT, CLK);

// === CONFIGURABLE THRESHOLDS ===
const float MIN_INITIAL_WEIGHT = 135.0;   // Minimum weight for first food
const float MIN_ADDED_WEIGHT   = 40.0;    // Minimum weight difference for next foods
const int   STABLE_TOLERANCE   = 5;       // Max +/- range between stable readings

float calibration_factor = 317.27;
float prev_weight = 0.0;
float total_protein = 0.0;
bool first_food_added = false;
int current_food = 0;

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
  {4, "Broccoli", 2.8, 0, 0}
};

// Function to detect stable weight based on a max ±tolerance deviation
bool waitForStableWeight(float &stable_weight) {
  const int num_readings = 4;
  float readings[num_readings];

  while (true) {
    for (int i = 0; i < num_readings; i++) {
      readings[i] = scale.get_units(5);
      if (readings[i] < 0) readings[i] = 0;
      Serial.print("Reading ");
      Serial.print(i + 1);
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
      stable_weight = readings[num_readings - 1];
      return true;
    }

    Serial.println("Unstable weight. Retrying...");
    delay(200);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Protein Calculator Ready");
  Serial.print("Enter food ID (1-4). Place food >");
  Serial.print(MIN_INITIAL_WEIGHT);
  Serial.println("g. Enter 'E' to view summary.");

  scale.set_scale(calibration_factor);
  scale.tare();
}

void loop() {
  if (Serial.available()) {
    if (Serial.peek() == 'e' || Serial.peek() == 'E') {
      Serial.read();

      Serial.println("\n=== Protein Summary ===");
      for (int i = 0; i < sizeof(foodDB) / sizeof(FoodInfo); i++) {
        if (foodDB[i].total_weight > 0) {
          Serial.print(foodDB[i].name);
          Serial.print(": ");
          Serial.print(foodDB[i].total_weight);
          Serial.print(" g | Protein: ");
          Serial.print(foodDB[i].total_protein);
          Serial.println(" g");
        }
      }
      Serial.print("Total Protein: ");
      Serial.print(total_protein);
      Serial.println(" g");
      Serial.println("========================\n");
    } else {
      int input = Serial.parseInt();

      if (input >= 1 && input <= 4) {
        current_food = input;
        Serial.print("Food ");
        Serial.print(foodDB[current_food - 1].name);
        Serial.println(" selected. Place the food on the scale...");

        if (first_food_added) {
          Serial.print("Waiting for weight to exceed previous by at least ");
          Serial.print(MIN_ADDED_WEIGHT);
          Serial.println("g...");
          while (true) {
            float current = scale.get_units(5);
            if (current < 0) current = 0;
            Serial.print("Reading: ");
            Serial.println(current);
            if (current >= prev_weight + MIN_ADDED_WEIGHT) break;
            delay(300);
          }
        }

        float stable_weight;
        if (waitForStableWeight(stable_weight)) {
          float added_weight;

          if (!first_food_added) {
            if (stable_weight < MIN_INITIAL_WEIGHT) {
              Serial.print("Initial food weight must be at least ");
              Serial.print(MIN_INITIAL_WEIGHT);
              Serial.println("g. Try again.");
              return;
            }
            added_weight = stable_weight;
            first_food_added = true;
          } else {
            added_weight = stable_weight - prev_weight;
            if (added_weight <= 0) {
              Serial.println("No additional weight detected.");
              return;
            }
          }

          prev_weight = stable_weight;

          float protein_per_100g = foodDB[current_food - 1].protein_per_100g;
          float added_protein = (added_weight * protein_per_100g) / 100.0;

          total_protein += added_protein;
          foodDB[current_food - 1].total_weight += added_weight;
          foodDB[current_food - 1].total_protein += added_protein;

          Serial.print("Added weight: ");
          Serial.print(added_weight);
          Serial.println(" g");

          Serial.print("Protein added: ");
          Serial.print(added_protein);
          Serial.println(" g");

          Serial.print("Total protein so far: ");
          Serial.print(total_protein);
          Serial.println(" g");
        } else {
          Serial.println("Stable weight not detected. Try again.");
        }
      } else {
        Serial.println("Invalid input. Enter food ID between 1-4 or 'E' for summary.");
      }
    }
  }
}
