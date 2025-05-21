#define BLYNK_TEMPLATE_ID "TMPL3CUJkV4Zb"
#define BLYNK_TEMPLATE_NAME "protein calculator"
#define BLYNK_AUTH_TOKEN "0jAzJDLli4_P4YIEnBA641z_eB_CDUt2"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "HX711.h"

// HX711 Pins
#define DOUT  4  // D2
#define CLK   5  // D1
HX711 scale(DOUT, CLK);

// WiFi credentials
char ssid[] = "Abhyud4G_EXT";
char pass[] = "Abhi1206";

// Calibration and thresholds
float calibration_factor = 317.27;
const float MIN_INITIAL_WEIGHT = 135.0;
const float MIN_ADDED_WEIGHT = 40.0;
const int STABLE_TOLERANCE = 5;
const int STABLE_READINGS = 4;

BlynkTimer timer;

// Food info
struct FoodInfo {
  const char* name;
  float protein_per_100g;
  float total_weight;
  float total_protein;
};

FoodInfo foodDB[] = {
  {"Chicken", 27.0, 0, 0},
  {"Egg", 13.0, 0, 0},
  {"Rice", 2.7, 0, 0},
  {"Broccoli", 2.8, 0, 0}
};

int selectedFoodIndex = -1;
float prev_weight = 0.0;
float total_protein = 0.0;
bool first_food_added = false;

// ====== Stable Weight Function ======
float waitForStableWeight() {
  float readings[STABLE_READINGS];
  Serial.println("Waiting for stable weight...");

  while (true) {
    yield();  // prevent watchdog reset

    for (int i = 0; i < STABLE_READINGS; i++) {
      readings[i] = scale.get_units(5);
      if (readings[i] < 0) readings[i] = 0;
      delay(100);
    }

    float base = readings[0];
    bool stable = true;
    for (int i = 1; i < STABLE_READINGS; i++) {
      if (abs(readings[i] - base) > STABLE_TOLERANCE) {
        stable = false;
        break;
      }
    }

    if (stable) return base;

    Serial.println("Unstable. Retrying...");
    delay(200);
  }
}

// ====== Summary ======
void showSummary() {
  String summary = "=== Summary ===\n";
  for (int i = 0; i < 4; i++) {
    if (foodDB[i].total_weight > 0) {
      summary += String(foodDB[i].name) + ": " +
                 String(foodDB[i].total_weight, 1) + "g, " +
                 String(foodDB[i].total_protein, 2) + "g\n";
    }
  }
  summary += "Total Protein: " + String(total_protein, 2) + "g";
  Serial.println(summary);
  Blynk.virtualWrite(V2, summary);
}

// ====== Add Food ======
void measureAndAddFood() {
  if (selectedFoodIndex < 0 || selectedFoodIndex > 3) return;

  float current = waitForStableWeight();
  if (!first_food_added && current < MIN_INITIAL_WEIGHT) {
    Serial.println("Initial food too light. Put at least 135g.");
    Blynk.virtualWrite(V2, "Initial food too light. Put at least 135g.");
    return;
  }

  float added_weight = first_food_added ? current - prev_weight : current;
  if (added_weight < MIN_ADDED_WEIGHT && first_food_added) {
    Serial.println("Not enough additional food. Add at least 40g.");
    Blynk.virtualWrite(V2, "Add at least 40g more.");
    return;
  }

  float added_protein = (added_weight * foodDB[selectedFoodIndex].protein_per_100g) / 100.0;
  foodDB[selectedFoodIndex].total_weight += added_weight;
  foodDB[selectedFoodIndex].total_protein += added_protein;
  total_protein += added_protein;
  prev_weight = current;
  first_food_added = true;

  String msg = "Added " + String(added_weight, 1) + "g " + foodDB[selectedFoodIndex].name +
               " → " + String(added_protein, 2) + "g protein";
  Serial.println(msg);
  Blynk.virtualWrite(V2, msg);
  selectedFoodIndex = -1;
}

// ====== Handle Blynk Selection ======
BLYNK_WRITE(V1) {
  int choice = param.asInt();
  if (choice == 5) {
    showSummary();
    return;
  }

  selectedFoodIndex = choice - 1;
  Blynk.virtualWrite(V2, String("Selected: ") + foodDB[selectedFoodIndex].name + ". Place on scale.");
  Serial.println(String("Selected: ") + foodDB[selectedFoodIndex].name + ". Place food...");
  timer.setTimeout(5000L, measureAndAddFood);  // allow user time to place food
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  delay(1000);  // give time to Serial Monitor
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, pass);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries++ < 20) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi failed to connect!");
    return;
  }

  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  scale.set_scale(calibration_factor);
  scale.tare();

  Blynk.virtualWrite(V2, "Device Ready. Select food to begin.");
  Serial.println("Scale tared and device ready.");

  timer.setInterval(3000L, []() {
    float w = scale.get_units(5);
    if (w < 0) w = 0;
    Serial.print("Weight: ");
    Serial.println(w);
  });
}

// ====== Loop ======
void loop() {
  Blynk.run();
  timer.run();
}
