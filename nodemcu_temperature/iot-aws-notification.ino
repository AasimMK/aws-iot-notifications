#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
extern "C" {
#include "libb64/cdecode.h"
}

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN,DHTTYPE);

struct dht11
{
  float humidity;
  float heatIndex;
  float temp;
};

int led_counter = 0;
int led_state = LOW;
struct dht11 sensor_data;
int read_sensor(struct dht11* data);
char sensorData;

// Update these with values suitable for your network.
const char* ssid = "<WIFI_SSID>";
const char* password = "<WIFI_PASSOWRD>";
const char* awsEndpoint = "<AWS_IOT_ENDPOINT>";
const char* DEVICE_ID = "<CUSTOM_DEVICE>";
const char* PUBLISH_TOPIC = DEVICE_ID;
const char* SUBSCRIBE_TOPIC = "<TOPIC_TO_SUBSCRIBE>";

// For the two certificate strings below paste in the text of your 
// device certificate and private key, comment out the BEGIN and 
// lines, add a quote character at the start of each line and a 
// and backslash at the end of each line:

const String certificatePemCrt = '<AWS_CERTIFICATE_PEM>'
const String privatePemKey = '<AWS_PRIVATE_PEM>'


int timeSinceLastRead = 0;
WiFiClientSecure wiFiClient;
void callback(char* topic, byte* payload, unsigned int len);
PubSubClient client(awsEndpoint, 8883, callback, wiFiClient);
long lastMsg = 0;
char msg[200];
int value = 0;
int analog_pin = A0;

void setup() {
  // Initialize the BUILTIN_LED pin as an output
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(analog_pin, INPUT);
  Serial.begin(115200);
  setup_wifi();
}

void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  uint8_t binaryCert[certificatePemCrt.length() * 3 / 4];
  int len = b64decode(certificatePemCrt, binaryCert);
  wiFiClient.setCertificate(binaryCert, len);
  uint8_t binaryPrivate[privatePemKey.length() * 3 / 4];
  len = b64decode(privatePemKey, binaryPrivate);
  wiFiClient.setPrivateKey(binaryPrivate, len);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(DEVICE_ID)) {
      Serial.println("connected");
      client.subscribe(SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    if(read_sensor(&sensor_data)==0)
    {
      char str_temp[6],str_humidity[6],str_hi[6];
      dtostrf(sensor_data.temp, 4, 2, str_temp);
      dtostrf(sensor_data.humidity, 4, 2, str_humidity);
      dtostrf(sensor_data.heatIndex, 4, 2, str_hi);
      snprintf(msg,100,"{\"device_id\":\"%s\",\"temperature\":\"%s\",\"humidity\":\"%s\",\"heat_index\":\"%s\"}",DEVICE_ID,str_temp,str_humidity,str_hi);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish(PUBLISH_TOPIC, msg);
    }
  }
  else if (now - lastMsg > 1000) {
    if(led_counter) {
      led_state = (led_state)?LOW:HIGH;
      digitalWrite(BUILTIN_LED, led_state);
    }
  }
}
int read_sensor(struct dht11* data) {
  data->humidity = dht.readHumidity();
  data->temp = dht.readTemperature();
  if(isnan(data->humidity)||isnan(data->temp)) {
    Serial.println("DHT11 Read Failed");
    return 1;
  }
  data->heatIndex = dht.computeHeatIndex(data->temp,data->humidity,false);
  return 0;
}

int b64decode(String b64Text, uint8_t* output) {
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64Text.c_str(), b64Text.length(), (char*)output, &s);
  return cnt;
}