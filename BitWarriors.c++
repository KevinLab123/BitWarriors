#include <LiquidCrystal.h>
#include <DHT.h>
#include <Adafruit_GPS.h>
#include <SD.h>

const float LAT_DEFECTO = 9.9281;
const float LON_DEFECTO = -84.0907;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

#define DHTPIN 8
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define PIN_HUMEDAD A0
#define PIN_SD_CS 53

Adafruit_GPS GPS(&Serial1);
uint32_t timer = millis();

#define LED_ROJO     22
#define LED_AMARILLO 23
#define LED_VERDE    24
#define LED_BLANCO   25
#define LED_AZUL     26
#define BUZZER       27

String estadoAnterior = "";

// Se evita sonido si humedad es 0 o muy baja
void bip(int cantidad, int humedad) {
  if (humedad <= 1) return;
  for (int i = 0; i < cantidad; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(BUZZER, LOW);
    delay(500);
  }
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  dht.begin();

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_BLANCO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("SD no detectada.");
  } else {
    Serial.println("SD lista.");
  }

  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();
}

void loop() {
  float temp = dht.readTemperature();
  float humAmb = dht.readHumidity();
  int humedadSuelo = analogRead(PIN_HUMEDAD);
  int humedadSueloPorc = map(humedadSuelo, 0, 1023, 100, 0);

  GPS.read();
  bool tieneFix = false;
  float lat = 0.0;
  float lon = 0.0;

  if (GPS.newNMEAreceived()) {
    GPS.parse(GPS.lastNMEA());
  }

  if (GPS.fix) {
    tieneFix = true;
    lat = GPS.latitude;
    lon = GPS.longitude;
  }
   humedadSueloPorc = 9;
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(isnan(temp) ? "Err " : String(temp) + "C ");
  lcd.print("H:");
  lcd.print(isnan(humAmb) ? "Err " : String(humAmb) + "% ");

  lcd.setCursor(0, 1);
  lcd.print("Suelo:");
  lcd.print(humedadSueloPorc);
  lcd.print("%    ");

  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_BLANCO, LOW);
  digitalWrite(LED_AZUL, LOW);
  digitalWrite(BUZZER, LOW);

  String estadosuelo = "";
  int bips = 0;

if (humedadSueloPorc >= 0 && humedadSueloPorc <= 10) {
  digitalWrite(LED_ROJO, HIGH);
  estadosuelo = "Muy seco";
  bips = 1;
} else if (humedadSueloPorc > 10 && humedadSueloPorc <= 30) {
  digitalWrite(LED_AMARILLO, HIGH);
  estadosuelo = "Seco";
} else if (humedadSueloPorc > 30 && humedadSueloPorc <= 60) {
  digitalWrite(LED_VERDE, HIGH);
  estadosuelo = "Ideal";

} else if (humedadSueloPorc > 60 && humedadSueloPorc <= 80) {
  digitalWrite(LED_BLANCO, HIGH);
  estadosuelo = "Húmedo";
} else if (humedadSueloPorc > 80 && humedadSueloPorc <= 100) {
  digitalWrite(LED_AZUL, HIGH);
  estadosuelo = "Muy húmedo";
  bips = 2;
} else {
  digitalWrite(LED_AZUL, HIGH);
  estadosuelo = "Muy húmedo";
}


  bip(bips, humedadSueloPorc);

  if (estadosuelo != estadoAnterior) {
    File dataFile = SD.open("datos.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.print("Temp: ");
      dataFile.print(temp);
      dataFile.print(" C, HumAmb: ");
      dataFile.print(humAmb);
      dataFile.print(" %, Suelo: ");
      dataFile.print(humedadSueloPorc);
      dataFile.print(" % - ");
      dataFile.print(estadosuelo);

      float latitudFinal = tieneFix ? lat : LAT_DEFECTO;
      float longitudFinal = tieneFix ? lon : LON_DEFECTO;

      dataFile.print(", Lat: ");
      dataFile.print(latitudFinal, 6);
      dataFile.print(", Lon: ");
      dataFile.print(longitudFinal, 6);
      dataFile.println();
      dataFile.close();

      Serial.println("Dato guardado en SD: " + estadosuelo);
    } else {
      Serial.println("Error al escribir en SD");
    }

    estadoAnterior = estadosuelo;
  }

  delay(3000);
}