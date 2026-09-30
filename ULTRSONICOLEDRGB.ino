#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// ---------------------- CONFIGURACIÓN WI-FI ----------------------

#define WLAN_SSID   "ARTEFACTOS"
#define WLAN_PASS   "87654321"


// ---------------------- CONFIGURACIÓN ADAFRUIT IO ----------------------

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "bendiaz1012"
#define AIO_KEY         "aio_Ectx01T5RAu2t07M4SFiPSZBrq2z"


// ---------------------- PINES ----------------------

#define TRIG_PIN 18
#define ECHO_PIN 19

#define PIN_R 25
#define PIN_G 26
#define PIN_B 27


// ---------------------- AJUSTES ----------------------

#define RGB_ANODO_COMUN false

#define PWM_FREQ 5000
#define PWM_RES 8

#define INTERVALO_PUBLICAR_MS 5000

#define DIST_CERCA_CM 10
#define DIST_MEDIA_CM 30



// ---------------------- MQTT ----------------------

WiFiClient client;

Adafruit_MQTT_Client mqtt(
  &client,
  AIO_SERVER,
  AIO_SERVERPORT,
  AIO_USERNAME,
  AIO_KEY
);


Adafruit_MQTT_Publish feedDistancia =
Adafruit_MQTT_Publish(
  &mqtt,
  AIO_USERNAME "/feeds/distancia"
);


// ---------------------- VARIABLES ----------------------

float ultimaDistancia = -1;

unsigned long ultimoPublicar = 0;
unsigned long ultimoPing = 0;


// ---------------------- PROTOTIPOS ----------------------

void conectarWiFi();

void conectarMQTT();

float leerDistanciaCm();

float distanciaPromedio(int muestras);

void escribirRGB(uint8_t r, uint8_t g, uint8_t b);

void actualizarLED();


// =====================================================================
// SETUP
// =====================================================================

void setup() {

  Serial.begin(115200);

  delay(10);


  // Sensor ultrasónico

  pinMode(TRIG_PIN, OUTPUT);

  pinMode(ECHO_PIN, INPUT);



  // LED RGB

  ledcAttach(PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_G, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_B, PWM_FREQ, PWM_RES);


  escribirRGB(255,255,255);


  conectarWiFi();

}



// =====================================================================
// LOOP
// =====================================================================

void loop() {


  conectarMQTT();



  // Leer y publicar distancia

  if (millis() - ultimoPublicar >= INTERVALO_PUBLICAR_MS) {


    ultimoPublicar = millis();


    float d = distanciaPromedio(5);


    if (d > 0) {


      ultimaDistancia = d;


      Serial.print("Distancia: ");

      Serial.print(d,1);

      Serial.println(" cm");



      if (!feedDistancia.publish(d)) {

        Serial.println("Error publicando distancia");

      }

    }

    else {

      Serial.println("Sin eco o fuera de rango");

    }


    actualizarLED();

  }



  // Mantener MQTT

  if (millis() - ultimoPing >= 30000) {


    ultimoPing = millis();

    mqtt.ping();

  }


}



// =====================================================================
// ULTRASONICO HC-SR04
// =====================================================================


float leerDistanciaCm() {


  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);



  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);



  digitalWrite(TRIG_PIN, LOW);



  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);



  if (duracion == 0)

    return -1;



  float distancia = (duracion * 0.0343) / 2;



  if(distancia < 2 || distancia > 100)

    return -1;



  return distancia;

}



// Promedio para eliminar ruido

float distanciaPromedio(int muestras) {


  float suma = 0;

  int validas = 0;



  for(int i=0;i<muestras;i++){


    float d = leerDistanciaCm();



    if(d > 0){

      suma += d;

      validas++;

    }


    delay(40);

  }



  if(validas > 0)

    return suma / validas;


  return -1;

}



// =====================================================================
// LED RGB
// =====================================================================


void escribirRGB(uint8_t r,uint8_t g,uint8_t b){



  if(RGB_ANODO_COMUN){


    r = 255-r;

    g = 255-g;

    b = 255-b;

  }



  ledcWrite(PIN_R,r);

  ledcWrite(PIN_G,g);

  ledcWrite(PIN_B,b);


}



// Cambiar color según distancia

void actualizarLED(){



  if(ultimaDistancia <= 0){


    escribirRGB(255,255,255);

    return;

  }



  if(ultimaDistancia <= DIST_CERCA_CM){


    // Rojo

    escribirRGB(255,0,0);


  }



  else if(ultimaDistancia <= DIST_MEDIA_CM){


    // Amarillo

    escribirRGB(255,255,0);


  }



  else{


    // Verde

    escribirRGB(0,255,0);


  }


}



// =====================================================================
// WIFI
// =====================================================================


void conectarWiFi(){



  Serial.print("Conectando a ");

  Serial.println(WLAN_SSID);



  WiFi.begin(WLAN_SSID,WLAN_PASS);



  while(WiFi.status()!=WL_CONNECTED){


    delay(500);

    Serial.print(".");

  }



  Serial.println();


  Serial.println("WiFi conectado");


  Serial.print("IP: ");

  Serial.println(WiFi.localIP());



}



// =====================================================================
// MQTT
// =====================================================================


void conectarMQTT(){



  if(mqtt.connected())

    return;



  Serial.print("Conectando a Adafruit IO...");



  int8_t ret;

  uint8_t intentos = 3;



  while((ret=mqtt.connect()) != 0){


    Serial.println(mqtt.connectErrorString(ret));


    mqtt.disconnect();


    delay(5000);



    intentos--;



    if(intentos==0){


      Serial.println("Reiniciando ESP32");

      ESP.restart();


    }


  }



  Serial.println("Conectado!");

}