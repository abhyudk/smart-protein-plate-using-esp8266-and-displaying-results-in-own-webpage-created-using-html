// Blynk credentials
#define BLYNK_TEMPLATE_ID "TMPL3r4D0Uzrk"
#define BLYNK_TEMPLATE_NAME "smart protein"
char auth[] = "9n9qId9Ol8XlVAgG-d_UQUWd_pdG3jXA"; 
char ssid[] = "Abhyud4G_EXT";
char pass[] = "Abhi1206";



#include "HX711.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>


// HX711 setup
#define DOUT  D2  // GPIO4
#define CLK   D1  // GPIO5
HX711 scale(DOUT, CLK);

// Constants
const float MIN_INITIAL_WEIGHT = 150.0;
const float MIN_ADDED_WEIGHT   = 40.0;
const int   STABLE_TOLERANCE   = 100;
const float LOWER_WEIGHT_LIMIT = 150.0;
const float UPPER_WEIGHT_LIMIT = 750.0;
float calibration_factor = 205.91;  // Adjusted for 3.3V

// Global variables
float prev_weight = 0.0;
float total_protein = 0.0;
bool first_food_added = false;
int current_food = 0;

BlynkTimer timer;

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

// 🧠 Detects stable weight
bool waitForStableWeight(float &stable_weight) {
  const int num_readings = 4;
  float readings[num_readings];
  int valid_count = 0;
  float valid_sum = 0;

  while (true) {
    yield();
    valid_count = 0;
    valid_sum = 0;

    for (int i = 0; i < num_readings; i++) {
      float reading = scale.get_units(5);
      if (reading < 0) reading = 0;

      Serial.print("Reading ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(reading);

      if (reading >= LOWER_WEIGHT_LIMIT && reading <= UPPER_WEIGHT_LIMIT) {
        readings[valid_count++] = reading;
        valid_sum += reading;
      } else {
        Serial.println("⛔ Ignored (out of range)");
      }

      delay(100);
    }

    if (valid_count < 2) {
      Serial.println("⚠️ Not enough valid readings. Retrying...");
      delay(200);
      continue;
    }

    float average = valid_sum / valid_count;
    bool within_range = true;

    for (int i = 0; i < valid_count; i++) {
      if (abs(readings[i] - average) > STABLE_TOLERANCE) {
        within_range = false;
        break;
      }
    }

    if (within_range) {
      stable_weight = average;
      Serial.print("✅ Stable weight detected: ");
      Serial.println(stable_weight);
      return true;
    } else {
      Serial.println("Unstable weight. Retrying...");
      delay(200);
    }
  }
}

// 🟢 Send protein summary to Blynk (V1)
void sendSummaryToBlynk() {
  String summary = "=== Summary ===\n";
  for (int i = 0; i < sizeof(foodDB) / sizeof(FoodInfo); i++) {
    if (foodDB[i].total_weight > 0) {
      summary += String(foodDB[i].name) + ": " + String(foodDB[i].total_weight) + "g | ";
      summary += "Protein: " + String(foodDB[i].total_protein) + "g\n";
    }
  }
  summary += "Total Protein: " + String(total_protein) + "g";
  Blynk.virtualWrite(V1, summary);
}

// 🟡 Handle dropdown input from Blynk (V0)
BLYNK_WRITE(V0) {
  int selection = param.asInt();  // 1-5
  if (selection >= 1 && selection <= 4) {
    current_food = selection;
    Serial.print("Blynk selected food: ");
    Serial.println(foodDB[current_food - 1].name);
  } else if (selection == 5) {
    sendSummaryToBlynk();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Protein Calculator Ready (ESP8266)");
  Serial.print("Enter food ID (1-4). Place food >");
  Serial.print(MIN_INITIAL_WEIGHT);
  Serial.println("g. Enter 'E' to view summary.");

  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();

  Blynk.begin(auth, ssid, pass);
  //timer.setInterval(5000L, sendSummaryToBlynk);  // Optional: push updates every 5 sec
}

void loop() {
  Blynk.run();
  timer.run();

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
            yield();
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
