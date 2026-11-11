#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "secrets.h" // WIFI_SSID, WIFI_PASS y SERVER_URL

// ---------- 1. Configuración de Red y API ----------
const char* ssid = WIFI_SSID;           
const char* password = WIFI_PASS;    
const char* serverApiUrl = SERVER_URL; // Cambia XX por tu IP local (Arch Linux)

// ---------- 2. Pines del ESP32 NodeMCU ----------
const int LED_ROJO     = 16;   // D0
const int LED_AMARILLO = 5;    // D1
const int LED_VERDE    = 4;    // D2
const int DHT_PIN      = 2;    // D4
const int TRIG_PIN     = 14;   // D5
const int ECHO_PIN     = 12;   // D6  (con divisor de voltaje si el HC-SR04 es de 5V)
const int PIR_PIN      = 13;   // D7
const int LDR_PIN      = A0;   // unico pin analogico del NodeMCU

#define DHT_TIPO DHT11 
DHT dht(DHT_PIN, DHT_TIPO);

// ---------- 3. Parámetros de detección y semáforo ----------
const float DIST_PEATON_CM = 20.0; 
const unsigned long CONFIRMA_MS = 1500;
const unsigned long PIR_MEMORIA_MS = 3000;

const unsigned long PAUSA_VEHICULOS_MS = 3000;
const unsigned long ROJO_MINIMO_MS     = 5000;
const unsigned long VERDE_BASE_MS      = 8000;
const unsigned long AMARILLO_MS        = 4000;
const unsigned long PARPADEO_MS        = 400;

// Ajustes ambientales
const int UMBRAL_NOCHE_ADC = 2000; // ESP32 tiene ADC de 12 bits (0-4095). Ajustar según calibración
const bool ALTO_ES_OSCURO = true;
const float HUMEDAD_ALTA = 80.0;
const float TEMP_ALTA = 35.0;
const unsigned long EXTRA_NOCHE_MS = 2000;
const unsigned long EXTRA_HUMEDAD_MS = 3000;
const unsigned long EXTRA_CALOR_MS = 2000;

// Tiempos
const unsigned long LECTURA_MS = 100;
const unsigned long DHT_MS = 2000;
const unsigned long ENVIO_API_MS = 5000; // Enviar a BD cada 5 seg

// ---------- 4. Variables de Estado ----------
enum Estado { ROJO, VERDE, AMARILLO };
Estado estado = ROJO;
unsigned long tEstado = 0;

int luzAdc = 0;
bool esNoche = false;
float temperatura = 25.0;
float humedad = 50.0;
float distanciaCm = -1;
bool pirActivo = false;
unsigned long tUltimoPIR = 0;

bool contando = false;
unsigned long tPresencia = 0;
bool solicitud = false;
unsigned long tSolicitud = 0;
unsigned long tFinCruce = 0;
unsigned long duracionVerde = 0;

unsigned long tLectura = 0, tDHT = 0, tEnvioAPI = 0;

// ---------- 5. Funciones Base ----------
void ponerLeds(bool rojo, bool amarillo, bool verde) {
  digitalWrite(LED_ROJO, rojo ? HIGH : LOW);
  digitalWrite(LED_AMARILLO, amarillo ? HIGH : LOW);
  digitalWrite(LED_VERDE, verde ? HIGH : LOW);
}

void leerLuz() {
  luzAdc = analogRead(LDR_PIN);
  esNoche = ALTO_ES_OSCURO ? (luzAdc >= UMBRAL_NOCHE_ADC) : (luzAdc <= UMBRAL_NOCHE_ADC);
}

void leerDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duracion = pulseIn(ECHO_PIN, HIGH, 25000); 
  distanciaCm = (duracion == 0) ? -1 : (duracion * 0.0343) / 2.0;
}

void leerPIR(unsigned long ahora) {
  pirActivo = (digitalRead(PIR_PIN) == HIGH);
  if (pirActivo) tUltimoPIR = ahora;
}

void leerDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperatura = t;
  if (!isnan(h)) humedad = h;
}

bool pirReciente(unsigned long ahora) {
  return tUltimoPIR != 0 && (ahora - tUltimoPIR) < PIR_MEMORIA_MS;
}

// ---------- 6. Lógica del Semáforo ----------
unsigned long calcularVerde() {
  unsigned long t = VERDE_BASE_MS;
  if (esNoche) t += EXTRA_NOCHE_MS;
  if (humedad >= HUMEDAD_ALTA) t += EXTRA_HUMEDAD_MS;
  if (temperatura >= TEMP_ALTA) t += EXTRA_CALOR_MS;
  return t;
}

void actualizarSemaforo(unsigned long ahora) {
  switch (estado) {
    case ROJO: {
      ponerLeds(true, false, false);
      bool presente = pirReciente(ahora) && distanciaCm > 0 && distanciaCm <= DIST_PEATON_CM;

      if (!solicitud) {
        if (presente) {
          if (!contando) {
            contando = true;
            tPresencia = ahora;
          } else if (ahora - tPresencia >= CONFIRMA_MS) {
            solicitud = true; 
            tSolicitud = ahora;
            contando = false;
            Serial.println("🚦 Solicitud de cruce confirmada");
          }
        } else {
          contando = false;
        }
      } else if (ahora - tSolicitud >= PAUSA_VEHICULOS_MS && ahora - tFinCruce >= ROJO_MINIMO_MS) {
        duracionVerde = calcularVerde();
        estado = VERDE;
        tEstado = ahora;
        Serial.println("🚦 Semáforo en VERDE");
      }
      break;
    }
    case VERDE:
      ponerLeds(false, false, true);
      if (ahora - tEstado >= duracionVerde) {
        estado = AMARILLO;
        tEstado = ahora;
        Serial.println("🚦 Semáforo en AMARILLO");
      }
      break;

    case AMARILLO: {
      bool encendido = ((ahora - tEstado) / PARPADEO_MS) % 2 == 0;
      ponerLeds(false, encendido, false);
      if (ahora - tEstado >= AMARILLO_MS) {
        estado = ROJO;
        tEstado = ahora;
        tFinCruce = ahora;
        solicitud = false;
        contando = false;
        Serial.println("🚦 Semáforo en ROJO");
      }
      break;
    }
  }
}

// ---------- 7. Envío a API REST ----------
void enviarMedicionAPI(int id_hw, float valor) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client; // <--- Instanciar el objeto WiFiClient
    HTTPClient http;
    
    // Pasar 'client' junto con la URL
    http.begin(client, serverApiUrl); 
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["id_hw"] = id_hw;
    doc["valor_medido"] = valor;
    
    String jsonBuffer;
    serializeJson(doc, jsonBuffer);

    int httpResponseCode = http.POST(jsonBuffer);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("   [API REST] HTTP ");
      Serial.print(httpResponseCode);
      Serial.print(" -> ");
      Serial.println(response);
    } else {
      Serial.print("   ❌ Error en petición POST. Código: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("❌ Error: Wi-Fi desconectado.");
  }
}

void sincronizarConBD() {
  Serial.println("📡 Enviando telemetría a la API...");
  enviarMedicionAPI(1, pirActivo ? 1.0 : 0.0); // ID 1: PIR
  enviarMedicionAPI(2, distanciaCm);           // ID 2: Ultrasónico
  enviarMedicionAPI(3, (float)luzAdc);         // ID 3: LDR
  enviarMedicionAPI(4, temperatura);           // ID 4: DHT Temperatura
}

// ---------- 8. Main ----------
void setup() {
  Serial.begin(9600);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  ponerLeds(true, false, false);
  dht.begin();

  Serial.print("Conectando a Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi Conectado. IP: " + WiFi.localIP().toString());
  
  leerDHT();
}

void loop() {
  unsigned long ahora = millis();
  leerPIR(ahora);

  if (ahora - tLectura >= LECTURA_MS) {
    tLectura = ahora;
    leerLuz();
    leerDistancia();
  }

  if (ahora - tDHT >= DHT_MS) {
    tDHT = ahora;
    leerDHT();
  }

  actualizarSemaforo(ahora);

  // Enviar a la base de datos sin bloquear el loop interno del semáforo
  if (ahora - tEnvioAPI >= ENVIO_API_MS) {
    tEnvioAPI = ahora;
    sincronizarConBD();
  }

  delay(10);
}