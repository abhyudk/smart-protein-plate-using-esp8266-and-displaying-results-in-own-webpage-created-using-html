#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// WiFi credentials
const char* ssid = "Abhyuddd";  // Open network
const char* password = "abhi1206";             // No password

// Web server on port 80
ESP8266WebServer server(80);

// HX711 wiring
const int LOADCELL_DOUT_PIN = D2;  // GPIO2
const int LOADCELL_SCK_PIN = D1;   // GPIO0
const byte PULSE = 1;             // Gain 128

// HTML page to serve
static const char HOME[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>TIMBANGAN ONLEN</title>
    <meta name="author" content="Ganteng Permanen">
    <style>
      .center { text-align: center; font-size: 40px; margin-top: 50px; }
    </style>
  </head>
  <body>
    <div class="center">
      <span id="bobot">Loading...</span> <code>grams</code>
      <script>
        setInterval(() => {
          fetch("/weight").then(res => res.text()).then(weight => {
            document.getElementById("bobot").innerText = weight;
          });
        }, 1000);
      </script>
    </div>
  </body>
</html>
)rawliteral";

// ---------- HX711 Read with timeout ----------
unsigned long ReadCount() {
  unsigned long Count = 0;
  unsigned long start = millis();

  digitalWrite(LOADCELL_SCK_PIN, LOW);
  pinMode(LOADCELL_DOUT_PIN, INPUT);

  // Wait for data ready (DOUT goes LOW)
  while (digitalRead(LOADCELL_DOUT_PIN)) {
    if (millis() - start > 2000) {
      Serial.println("HX711 not responding!");
      return 0;
    }
  }

  // Read 24-bit data
  for (int i = 0; i < 24; i++) {
    digitalWrite(LOADCELL_SCK_PIN, HIGH);
    Count = Count << 1;
    digitalWrite(LOADCELL_SCK_PIN, LOW);
    if (digitalRead(LOADCELL_DOUT_PIN)) Count++;
  }

  // Set gain = 128
  digitalWrite(LOADCELL_SCK_PIN, HIGH);
  Count = Count ^ 0x800000; // convert from 2's complement
  digitalWrite(LOADCELL_SCK_PIN, LOW);

  return Count;
}

// Average multiple readings
unsigned long averageRaw(byte samples = 5) {
  unsigned long sum = 0;
  for (byte i = 0; i < samples; i++) {
    sum += ReadCount();
    delay(5);
  }
  return sum / samples;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LOADCELL_SCK_PIN, OUTPUT);
  pinMode(LOADCELL_DOUT_PIN, INPUT);

  // Setup WiFi Access Point (open network)
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Serve main page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", HOME);
  });

  // Serve weight as text
  server.on("/weight", HTTP_GET, []() {
    unsigned long raw = averageRaw(5);
    float grams = (float)(raw - 8388608) / 1000.0;  // fake calibration
    server.send(200, "text/plain", String(grams, 2));
  });

  server.begin();
}

// ---------- Loop ----------
void loop() {
  server.handleClient();
}
