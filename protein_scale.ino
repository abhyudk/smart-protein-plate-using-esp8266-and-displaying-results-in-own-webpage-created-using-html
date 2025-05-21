#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "HX711.h"

// WiFi credentials
const char* ssid = "Abhyuddd";
const char* password = "abhi1206";

// HX711 Setup
#define DOUT D2
#define CLK  D1
HX711 scale(DOUT, CLK);
float calibration_factor = 205.91;

// Protein Tracker Setup
const float MIN_INITIAL_WEIGHT = 150.0;
const float MIN_ADDED_WEIGHT   = 40.0;
const int   STABLE_TOLERANCE   = 100;
const float LOWER_WEIGHT_LIMIT = 150.0;
const float UPPER_WEIGHT_LIMIT = 750.0;

float prev_weight = 0.0;
float total_protein = 0.0;
bool first_food_added = false;
int current_food = 0;

String summaryString = "";  // for web summary

// Web Server
ESP8266WebServer server(80);

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
  {4, "Broccoli", 2.8, 0, 0}
};

// HTML Page
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>Team 13 Protein Summary</title>
    <meta name="author" content="Abhyud Krishna">
    <style>
      body { font-family: sans-serif; text-align: center; margin-top: 50px; }
      button { font-size: 18px; margin: 10px; }
      pre { text-align: left; display: inline-block; text-align: left; margin-top: 20px; font-size: 18px; }
    </style>
  </head>
  <body>
    <h2>Abhyud Krishna's Protein Tracker</h2>
    <button onclick="getSummary()">View Summary</button>
    <div id="summary"><pre>Summary will appear here...</pre></div>
    <script>
      function getSummary() {
        fetch("/summary")
          .then(res => res.text())
          .then(data => {
            document.getElementById("summary").innerHTML = "<pre>" + data + "</pre>";
          });
      }
    </script>
  </body>
</html>
)rawliteral";

// Wait for stable weight function
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

// Setup
void setup() {
  Serial.begin(115200);
  Serial.println("Protein Tracker Booting...");
  Serial.println("Enter food ID (1-4), then 'E' for summary.");

  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();

  WiFi.softAP(ssid, password);
  Serial.print("WiFi Access Point Started: ");
  Serial.println(ssid);
  Serial.print("Visit http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", MAIN_page);
  });

  server.on("/summary", HTTP_GET, []() {
    server.send(200, "text/plain", summaryString);
  });

  server.begin();
}

// Main loop
void loop() {
  server.handleClient();

  if (Serial.available()) {
    if (Serial.peek() == 'e' || Serial.peek() == 'E') {
      Serial.read();
      summaryString = "\n=== Protein Summary ===\n";
      for (int i = 0; i < sizeof(foodDB) / sizeof(FoodInfo); i++) {
        if (foodDB[i].total_weight > 0) {
          summaryString += String(foodDB[i].name) + ": " +
                           String(foodDB[i].total_weight, 1) + " g | Protein: " +
                           String(foodDB[i].total_protein, 2) + " g\n";
        }
      }
      summaryString += "Total Protein: " + String(total_protein, 2) + " g\n";
      summaryString += "========================\n";
      Serial.println(summaryString);
    } else {
      int input = Serial.parseInt();
      if (input >= 1 && input <= 4) {
        current_food = input;
        Serial.print("Selected: ");
        Serial.println(foodDB[current_food - 1].name);

        if (first_food_added) {
          Serial.println("Waiting for added weight...");
          while (true) {
            float current = scale.get_units(5);
            if (current < 0) current = 0;
            if (current >= prev_weight + MIN_ADDED_WEIGHT) break;
            delay(300);
          }
        }

        float stable_weight;
        if (waitForStableWeight(stable_weight)) {
          float added_weight;

          if (!first_food_added) {
            if (stable_weight < MIN_INITIAL_WEIGHT) {
              Serial.println("Initial food weight too low. Try again.");
              return;
            }
            added_weight = stable_weight;
            first_food_added = true;
          } else {
            added_weight = stable_weight - prev_weight;
            if (added_weight <= 0) {
              Serial.println("No new weight added.");
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

          Serial.print("Protein: ");
          Serial.print(added_protein);
          Serial.println(" g");

          Serial.print("Total protein: ");
          Serial.print(total_protein);
          Serial.println(" g");
        } else {
          Serial.println("Stable weight not detected.");
        }
      } else {
        Serial.println("Invalid input. Enter 1–4 or 'E'");
      }
    }
  }
}
//http://192.168.4.1