#include "DHT.h"
#define DHT_DEBUG
#define DEBUG_PRINTER Serial
#define DHTPIN 33     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);


void dht_setup() {
  Serial.println(F("Init dht"));
  dht.begin();
  pinMode(DHTPIN, INPUT);
}

void make_reading(float *h, float *t, float *f, float *hif, float *hic) {
 

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  *h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  *t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  *f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(*h) || isnan(*t) || isnan(*f)) {
    Serial.println(F("Failed to read from DHT sensor!"));

  }

  // Compute heat index in Fahrenheit (the default)
  *hif = dht.computeHeatIndex(*f, *h);
  // Compute heat index in Celsius (isFahreheit = false)
  *hic = dht.computeHeatIndex(*t, *h, false);

 
}

String dht_message()  {

  float h, t, f, hif, hic;
  h = 0;
  String result;

  make_reading(&h, &t, &f, &hif, &hic);
  result+= F("Humidity: ");
  result+= h;
  result+= F("%  Temperature: ");
  result+= t;
  result+= F("C ");
  result+= f;
  result+= F("F  Heat index: ");
  result+= hic;
  result+= F("C ");
  result+= hif;
  result+= F("F");

  return result;
}
