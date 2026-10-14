#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "S23FEdeBenja";
const char* WIFI_PASSWORD = "Benja5678";


// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================


const char* MQTT_SERVER = "10.130.167.213";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";


// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

const int PIN_DHT = 4;
const int PIN_LDR = 32;

DHT dht(PIN_DHT, DHT11);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

void conectarWiFi() {

  Serial.print("Conectando a WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}


// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {

  // Leer temperatura y humedad.
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Leer la LDR y convertir a porcentaje relativo.
  int lecturaLDR = analogRead(PIN_LDR);
  int luz = map(lecturaLDR, 0, 4095, 0, 100);

  // Convertir el porcentaje a texto y publicarlo.
  String textoLuz = String(luz);

  bool luzEnviada = mqttClient.publish(
    TOPIC_LUZ, textoLuz.c_str(), true
  );

  // Comprobar que el DHT11 entregó lecturas válidas.
  if (isnan(temperatura) || isnan(humedad)) {

    Serial.println("Error al leer DHT11. Revisa sus conexiones.");

  } else {

    // Convertir las mediciones a texto.
    String textoTemperatura = String(temperatura, 2);
    String textoHumedad = String(humedad, 2);

    // Publicar y conservar el último valor en Mosquitto.
    bool temperaturaEnviada = mqttClient.publish(
      TOPIC_TEMPERATURA, textoTemperatura.c_str(), true
    );

    bool humedadEnviada = mqttClient.publish(
      TOPIC_HUMEDAD, textoHumedad.c_str(), true
    );

    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" °C");

    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.println(" %");

    if (!temperaturaEnviada || !humedadEnviada) {
      Serial.println("No se pudieron publicar todos los datos del DHT11.");
    }
  }

  Serial.print("LDR ADC: ");
  Serial.print(lecturaLDR);
  Serial.print(" | Luz: ");
  Serial.print(luz);
  Serial.println(" %");

  if (!luzEnviada) {
    Serial.println("No se pudo publicar la lectura de luz.");
  }

  Serial.println("------------------------------");
}

void setup() {

  Serial.begin(115200);

  dht.begin();
pinMode(PIN_LDR, INPUT);
analogReadResolution(12);

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();


  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}