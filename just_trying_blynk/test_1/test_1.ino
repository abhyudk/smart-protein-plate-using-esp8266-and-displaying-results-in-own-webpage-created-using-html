\#define BLYNK\_TEMPLATE\_ID "TMPL3EHsxqfSF"
\#define BLYNK\_TEMPLATE\_NAME "temp "
\#include \<ESP8266WiFi.h>
\#include "HX711.h"
\#define BLYNK\_PRINT Serial
\#include \<Blynk.h>
\#include \<BlynkSimpleEsp8266.h>

const char \*ssid = "Abhyud4G\_EXT"; // replace with your wifi ssid and wpa2 key
const char \*pass = "Abhi1206";
char auth\[] = "XHw0zIyvciNfNxA\_C84QfvSfIMb\_dRV2"; // Your Blynk Auth Token

WiFiClient client;

HX711 scale(D2, D1);

int rbutton = D4; // button to reset the scale
float weight;
float calibration\_factor = 205.91; // calibration factor

void setup()
{
Serial.begin(115200);
pinMode(rbutton, INPUT\_PULLUP);

scale.set\_scale();
scale.tare(); // reset scale to 0
long zero\_factor = scale.read\_average(); // get a baseline reading

Blynk.begin(auth, ssid, pass);

Serial.println("Connecting to WiFi...");

while (WiFi.status() != WL\_CONNECTED) {
delay(1000);
Serial.print(".");
}

Serial.println("");
Serial.println("WiFi connected");
}

void loop()
{
Blynk.run();
scale.set\_scale(calibration\_factor); // apply calibration factor

weight = scale.get\_units(5);

Serial.print("Weight: ");
Serial.print(weight);
Serial.println(" KG");

Blynk.virtualWrite(V3, weight);

delay(2000); // delay between readings

if (digitalRead(rbutton) == LOW) {
scale.set\_scale();
scale.tare(); // reset the scale to 0
}
}


