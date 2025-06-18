#include <LiquidCrystal.h>
#include <DHT.h>
#include <Adafruit_GPS.h>
#include <SD.h>

// Coordenadas por defecto (San José, Costa Rica)
const float LAT_DEFECTO = 9.9281;
const float LON_DEFECTO = -84.0907;


// LCD: RS, E, D4-D7
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// DHT11
#define DHTPIN 8
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Sensor de humedad de suelo
#define PIN_HUMEDAD A0

// Módulo SD
#define PIN_SD_CS 53

// GPS (Serial1 en Mega)
Adafruit_GPS GPS(&Serial1);
uint32_t timer = millis();

// Pines de LEDs y zumbador
#define LED_ROJO     22
#define LED_AMARILLO 23
#define LED_VERDE    24
#define LED_BLANCO   25
#define LED_AZUL     26
#define BUZZER       27

// Estado anterior del suelo
String estadoAnterior = "";

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
  // Leer sensores
  float temp = dht.readTemperature();
  float humAmb = dht.readHumidity();
  int humedadSuelo = analogRead(PIN_HUMEDAD);
  int humedadSueloPorc = map(humedadSuelo, 0, 1023, 100, 0);

  // Leer GPS
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

  // Mostrar en LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(isnan(temp) ? "Err " : String(temp) + "C ");
  lcd.print("H:");
  lcd.print(isnan(humAmb) ? "Err " : String(humAmb) + "% ");

  lcd.setCursor(0, 1);
  lcd.print("Suelo:");
  lcd.print(humedadSueloPorc);
  lcd.print("%    ");

  // Apagar indicadores
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_BLANCO, LOW);
  digitalWrite(LED_AZUL, LOW);
  digitalWrite(BUZZER, LOW);

  // Determinar estado del suelo
  String estadosuelo = "";
  if (humedadSueloPorc <= 10) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(BUZZER, HIGH);
    estadosuelo = "Muy seco";
  } else if (humedadSueloPorc <= 30) {
    digitalWrite(LED_AMARILLO, HIGH);
    estadosuelo = "Seco";
  } else if (humedadSueloPorc <= 60) {
    digitalWrite(LED_VERDE, HIGH);
    estadosuelo = "Ideal";
  } else if (humedadSueloPorc <= 80) {
    digitalWrite(LED_BLANCO, HIGH);
    estadosuelo = "Humedo";
  } else {
    digitalWrite(LED_AZUL, HIGH);
    digitalWrite(BUZZER, HIGH);
    estadosuelo = "Muy humedo";
  }

  // Solo guardar si el estado del suelo cambió
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

      // Usar coordenadas del GPS si hay fix, si no, usar las de respaldo
    float latitudFinal = tieneFix ? lat : LAT_DEFECTO;
    float longitudFinal = tieneFix ? lon : LON_DEFECTO;

    dataFile.print(", Lat: ");
    dataFile.print(latitudFinal, 6);
    dataFile.print(", Lon: ");
    dataFile.print(longitudFinal, 6);

      //if (tieneFix) {
        //dataFile.print(", Lat: ");
        //dataFile.print(lat, 6);
        //dataFile.print(", Lon: ");
        //dataFile.print(lon, 6);
      //} else {
        //dataFile.print(", Sin GPS");
      //}

      dataFile.println();
      dataFile.close();
      Serial.println("Dato guardado en SD: " + estadosuelo);
    } else {
      Serial.println("Error al escribir en SD");
    }

    // Actualizar estado anterior
    estadoAnterior = estadosuelo;
  }

  delay(3000);
}