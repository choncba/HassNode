//////////////////////////////////////////////////////////////////////////
///					NodoHass	- ESP8266 ESP12E  						//
//////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>         
#include <PubSubClient.h>        
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <FS.h>
#include "config.h"

//#define FORMAT_FS
#if defined(DEBUG_TELNET)
WiFiServer  telnetServer(23);
WiFiClient  telnetClient;
#define     DEBUG_PRINT(x)    telnetClient.print(x)
#define     DEBUG_PRINTLN(x)  telnetClient.println(x)
#elif defined(DEBUG_SERIAL)
#define     DEBUG_PRINT(x)    Serial.print(x)
#define     DEBUG_PRINTLN(x)  Serial.println(x)
#else
#define     DEBUG_PRINT(x)
#define     DEBUG_PRINTLN(x)
#endif

WiFiClient    		wifiClient;
PubSubClient  		mqttClient(wifiClient);
ESP8266WebServer 	server(80);

#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(OW_PIN);
DallasTemperature sensors(&oneWire);
Ticker read_time, publish_time, pub_all_time;

enum config_enum {DEF=0, WEB, LOC};

struct Status{
	boolean OUT[MAX_NUM_IO] = {0,0,0,0};
	uint8_t IN[MAX_NUM_IO] = {1,1,1,1};
	float temp = 0;
}nodo;

// Estructura de datos en memoria
struct Data
{
	char NODE_NAME[20];
    char AP_SSID[20];
	char AP_PASS[20];
    IPAddress AP_IP;
    char SSID[20];
    char PASS[20];
    bool DHCP;
    IPAddress IP;
    IPAddress MASK;
    IPAddress GW;
	bool MQTT_ACTIVE;
    char MQTT_SERVER[30];	
    uint32_t MQTT_PORT;
    char MQTT_USER[20];
    char MQTT_PASS[20];
	char setTopic[NUM_IO][20];
	char OutStateTopic[NUM_IO][20];
	char InStateTopic[NUM_IO][20];
	char stateTempTopic[20];
}config;

bool saveConfig(uint8_t);
bool loadConfig(uint8_t);
void PublishAll(void);
void PublishOutputs(uint8_t);

#pragma region Funciones FS
bool saveConfig(uint8_t mode){
	const size_t capacity = 2*JSON_ARRAY_SIZE(2) + 4*JSON_OBJECT_SIZE(1) + 9*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(7) + 620;
	DynamicJsonDocument doc(capacity);

	switch(mode){
		// Restore DEFAULT Config - Otra opcion es cargar el archivo config.json junto con la imagen de SPIFFS,
		// esto se ejecuta solo en caso que no exista el archivo
		case DEF:	{
					doc["NODE_NAME"] = "NodoHass";

					JsonObject AP = doc.createNestedObject("AP");
					AP["SSID"] = "NodoHass";
					AP["PASS"] = "12345678";
					AP["IP"] = "192.168.4.1";

					JsonObject WIFI = doc.createNestedObject("WIFI");
					WIFI["SSID"] = "YourSSID";
					WIFI["PASS"] = "YourPass";
					WIFI["DHCP"] = 1;
					WIFI["IP"] = "192.168.1.100";
					WIFI["MASK"] = "255.255.255.0";
					WIFI["GW"] = "192.168.1.1";

					JsonObject MQTT = doc.createNestedObject("MQTT");
					MQTT["ACTIVE"] = 0;
					MQTT["SERVER"] = "192.168.1.50";
					MQTT["PORT"] = 1883;
					MQTT["USER"] = "YourMqttUser";
					MQTT["PASS"] = "YourMqttPass";

					JsonObject OUTPUTS = doc.createNestedObject("OUTPUTS");
					OUTPUTS["NUM"] = NUM_IO;
					JsonArray OUTPUTS_OUT = OUTPUTS.createNestedArray("OUT");
					JsonObject OUTS[NUM_IO] = OUTPUTS_OUT.createNestedObject();

					JsonObject INPUTS = doc.createNestedObject("INPUTS");
					INPUTS["NUM"] = NUM_IO;
					JsonArray INPUTS_IN = INPUTS.createNestedArray("IN");
					JsonObject INS[NUM_IO] = INPUTS_IN.createNestedObject();
					
					for(int i=0;i<NUM_IO;i++)
					{
						JsonObject OUTS_mqtt = OUTS[i].createNestedObject("mqtt");
						OUTS[i]["name"] = "Output" + i;
						OUTS_mqtt["stateTopic"] = "/Output" + (String)i + "/status";
						OUTS_mqtt["setTopic"] = "/Output" + (String)i + "/set";
						JsonObject INS_mqtt = INS[i].createNestedObject("mqtt");
						INS[i]["name"] = "Input" + i;
						INS_mqtt["stateTopic"] = "/Input" + (String)i + "/status";
					}
					
					JsonObject SENSORS = doc.createNestedObject("SENSORS");

					JsonObject SENSORS_TEMP = SENSORS.createNestedObject("TEMP");
					#ifdef USE_TEMP					
					SENSORS_TEMP["active"] = 1;
					#else
					SENSORS_TEMP["active"] = 0;
					#endif
					JsonObject SENSORS_TEMP_mqtt = SENSORS_TEMP.createNestedObject("mqtt");
					SENSORS_TEMP_mqtt["stateTopic"] = "/temp/status";
					}
					break;
		// Save config.json sent by web client
		case WEB:	{
					DeserializationError err= deserializeJson(doc, server.arg("plain"));
					if(err)
					{
						DEBUG_PRINT(F("config.json from web client failed with code "));
						DEBUG_PRINTLN(err.c_str());
						return false;
					}
					else
					{
						strlcpy(config.NODE_NAME, doc["NODE_NAME"], sizeof(config.NODE_NAME));
						strlcpy(config.AP_SSID, doc["AP"]["SSID"], sizeof(config.AP_SSID));
						strlcpy(config.AP_PASS, doc["AP"]["PASS"], sizeof(config.AP_PASS));
						config.AP_IP.fromString(doc["AP"]["IP"].as<char*>());	
						strlcpy(config.SSID, doc["WIFI"]["SSID"], sizeof(config.SSID));
						strlcpy(config.PASS, doc["WIFI"]["PASS"], sizeof(config.PASS));
						config.DHCP = doc["WIFI"]["DHCP"];
						config.IP.fromString(doc["WIFI"]["IP"].as<char*>());
						config.MASK.fromString(doc["WIFI"]["MASK"].as<char*>());
						config.GW.fromString(doc["WIFI"]["GW"].as<char*>());
						bool set_mqtt = doc["MQTT"]["ACTIVE"];
						if(config.MQTT_ACTIVE && !set_mqtt)
						{
							mqttClient.disconnect();	// Desconecto el cliente mqtt si lo indica desde la web
						}
						config.MQTT_ACTIVE = set_mqtt;
						strlcpy(config.MQTT_SERVER, doc["MQTT"]["SERVER"], sizeof(config.MQTT_SERVER));
						config.MQTT_PORT = doc["MQTT"]["PORT"];
						strlcpy(config.MQTT_USER, doc["MQTT"]["USER"], sizeof(config.MQTT_USER));
						strlcpy(config.MQTT_PASS, doc["MQTT"]["PASS"], sizeof(config.MQTT_PASS));
						
						for(int i=0; i<NUM_IO;i++)
						{
							strlcpy(config.InStateTopic[i], doc["INPUTS"]["IN"][i]["mqtt"]["stateTopic"], sizeof(config.InStateTopic[i]));
							strlcpy(config.OutStateTopic[i], doc["OUTPUTS"]["OUT"][i]["mqtt"]["stateTopic"], sizeof(config.OutStateTopic[i]));
							strlcpy(config.setTopic[i], doc["OUTPUTS"]["OUT"][i]["mqtt"]["setTopic"], sizeof(config.setTopic[i]));
						}
						
						strlcpy(config.stateTempTopic, doc["SENSORS"]["TEMP"]["mqtt"]["stateTopic"], sizeof(config.stateTempTopic));
					}
					}
					break;
		// Save local in config.json
		case LOC:	{
					File readFile = SPIFFS.open("/config.json", "r");
					deserializeJson(doc, readFile);
					readFile.close();
					doc["WIFI"]["SSID"] = config.SSID;
					doc["WIFI"]["PASS"] = config.PASS;
					}
					break;
		default:	break;
	}
		
  	File configFile = SPIFFS.open("/config.json", "w");
  	if (!configFile) {
    	DEBUG_PRINTLN(F("Failed to open config file"));
    return false;
  	}
  	serializeJson(doc, configFile);	// Guarda el JSON en SPIFFS
	configFile.close();
	DEBUG_PRINTLN(F("Config Saved: "));
	serializeJsonPretty(doc, Serial);
  	return true;
}

bool loadConfig(uint8_t mode){
	// VER config.json en https://arduinojson.org/v6/assistant/
	const size_t capacity = 2*JSON_ARRAY_SIZE(2) + 4*JSON_OBJECT_SIZE(1) + 9*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(7) + 620;
	DynamicJsonDocument doc(capacity);
	File configFile = SPIFFS.open("/config.json", "r");
	if (!configFile){
		DEBUG_PRINTLN("Failed to open config file");
		return false;
	}
	deserializeJson(doc, configFile);
	configFile.close();

	strlcpy(config.NODE_NAME, doc["NODE_NAME"], sizeof(config.NODE_NAME));
	strlcpy(config.AP_SSID, doc["AP"]["SSID"], sizeof(config.AP_SSID));
	strlcpy(config.AP_PASS, doc["AP"]["PASS"], sizeof(config.AP_PASS));
	config.AP_IP.fromString(doc["AP"]["IP"].as<char*>());	
	strlcpy(config.SSID, doc["WIFI"]["SSID"], sizeof(config.SSID));
	strlcpy(config.PASS, doc["WIFI"]["PASS"], sizeof(config.PASS));
	config.DHCP = doc["WIFI"]["DHCP"];
	config.IP.fromString(doc["WIFI"]["IP"].as<char*>());
	config.MASK.fromString(doc["WIFI"]["MASK"].as<char*>());
	config.GW.fromString(doc["WIFI"]["GW"].as<char*>());
	config.MQTT_ACTIVE = doc["MQTT"]["ACTIVE"];
	strlcpy(config.MQTT_SERVER, doc["MQTT"]["SERVER"], sizeof(config.MQTT_SERVER));
	config.MQTT_PORT = doc["MQTT"]["PORT"];
	strlcpy(config.MQTT_USER, doc["MQTT"]["USER"], sizeof(config.MQTT_USER));
	strlcpy(config.MQTT_PASS, doc["MQTT"]["PASS"], sizeof(config.MQTT_PASS));
	
	for(int i=0; i<NUM_IO;i++)
	{
		strlcpy(config.OutStateTopic[i], doc["OUTPUTS"]["OUT"][i]["mqtt"]["stateTopic"], sizeof(config.OutStateTopic[i]));
		strlcpy(config.setTopic[i], doc["OUTPUTS"]["OUT"][i]["mqtt"]["setTopic"], sizeof(config.setTopic[i]));
		strlcpy(config.InStateTopic[i], doc["INPUTS"]["IN"][i]["mqtt"]["stateTopic"], sizeof(config.InStateTopic[i]));
	}
	
	strlcpy(config.stateTempTopic, doc["SENSORS"]["TEMP"]["mqtt"]["stateTopic"], sizeof(config.stateTempTopic));

	DEBUG_PRINTLN(F("Config Loaded OK:"));
	serializeJsonPretty(doc, Serial);
	
	if(mode == WEB)
	{
		char buffer[capacity];
		serializeJson(doc, buffer);
		server.send(200, "text/json", buffer);
	}

	return true;
}

String formatBytes(size_t bytes){
  if(bytes < 1024)
	{
    return String(bytes) + "B";
  }
	else 	
	if (bytes < (1024 * 1024)){
  	return String(bytes / 1024.0) + "KB";
  } 
	else
	if (bytes < (1024 * 1024 * 1024)){
  	return String(bytes / 1024.0 / 1024.0) + "MB";
  } 
	else{
   	return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}
#pragma endregion

#pragma region Debug TELNET
///////////////////////////////////////////////////////////////////////////
//   TELNET
///////////////////////////////////////////////////////////////////////////
/*
   Function called to handle Telnet clients
   https://www.youtube.com/watch?v=j9yW10OcahI
*/
#if defined(DEBUG_TELNET)
void handleTelnet(void) {
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      if (telnetClient) {
        telnetClient.stop();
      }
      telnetClient = telnetServer.available();
    } else {
      telnetServer.available().stop();
    }
  }
}
#endif
#pragma endregion

#pragma region Funciones OTA
///////////////////////////////////////////////////////////////////////////
//   OTA
///////////////////////////////////////////////////////////////////////////
#if defined(OTA)
/*
   Function called to setup OTA updates
*/
void setupOTA() {
//#if defined(OTA_HOSTNAME)
  ArduinoOTA.setHostname(config.NODE_NAME);
  DEBUG_PRINT(F("INFO: OTA hostname sets to: "));
  DEBUG_PRINTLN(config.NODE_NAME);
//#endif

#if defined(OTA_PORT)
  ArduinoOTA.setPort(OTA_PORT);
  DEBUG_PRINT(F("INFO: OTA port sets to: "));
  DEBUG_PRINTLN(OTA_PORT);
#endif

#if defined(OTA_PASSWORD)
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
  DEBUG_PRINT(F("INFO: OTA password sets to: "));
  DEBUG_PRINTLN(OTA_PASSWORD);
#endif

  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN(F("INFO: OTA starts"));
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN(F("INFO: OTA ends"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINT(F("INFO: OTA progresses: "));
    DEBUG_PRINT(progress / (total / 100));
    DEBUG_PRINTLN(F("%"));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINT(F("ERROR: OTA error: "));
    DEBUG_PRINTLN(error);
    if (error == OTA_AUTH_ERROR)
      DEBUG_PRINTLN(F("ERROR: OTA auth failed"));
    else if (error == OTA_BEGIN_ERROR)
      DEBUG_PRINTLN(F("ERROR: OTA begin failed"));
    else if (error == OTA_CONNECT_ERROR)
      DEBUG_PRINTLN(F("ERROR: OTA connect failed"));
    else if (error == OTA_RECEIVE_ERROR)
      DEBUG_PRINTLN(F("ERROR: OTA receive failed"));
    else if (error == OTA_END_ERROR)
      DEBUG_PRINTLN(F("ERROR: OTA end failed"));
  });
  ArduinoOTA.begin();
}

/*
   Function called to handle OTA updates
*/
void handleOTA() {
  ArduinoOTA.handle();
}
#endif
#pragma endregion

#pragma region Funciones Wifi
// void setupAP()
// {
// 	WiFi.mode(WIFI_AP);
// 	WiFi.softAP(config.AP_SSID, config.AP_PASS);
// 	DEBUG_PRINT(F("AP mode, connect to "));
// 	DEBUG_PRINT(config.AP_SSID);
// 	DEBUG_PRINT(F(", pass: "));
// 	DEBUG_PRINTLN(config.AP_PASS); 
// 	DEBUG_PRINT(F("Server IP: "));
// 	DEBUG_PRINTLN(WiFi.softAPIP());
// }

void handleWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case WIFI_EVENT_STAMODE_GOT_IP:
      DEBUG_PRINTLN(F("INFO: WiFi connected"));
      DEBUG_PRINT(F("INFO: IP address: "));
      DEBUG_PRINTLN(WiFi.localIP());
      break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      DEBUG_PRINTLN(F("ERROR: WiFi losts connection"));
      break;
    default:
      DEBUG_PRINT(F("INFO: WiFi event: "));
      DEBUG_PRINTLN(event);
      break;
  }
}

// void setupWiFi()
// {
// 	unsigned int timeout = 20;

// 	WiFi.disconnect();
// 	WiFi.softAPdisconnect();

// 	WiFi.mode(WIFI_STA);
// 	WiFi.onEvent(handleWiFiEvent);				
// 	WiFi.begin(config.SSID, config.PASS);

// 	DEBUG_PRINT(F("Connecting to "));
// 	DEBUG_PRINT(config.SSID);
// 	DEBUG_PRINT(F(", pass: "));
// 	DEBUG_PRINT(config.PASS);

// 	while ((WiFi.status() != WL_CONNECTED) && timeout)
// 	{
// 		timeout--;
// 		delay(500);
// 		DEBUG_PRINT(F("."));
// 	}
// 	if (timeout > 0)
// 	{
// 		if (!config.DHCP)
// 		{
// 			WiFi.config(config.IP, config.GW, config.MASK);
// 		}
// 	}
// 	else
// 	{
// 		DEBUG_PRINTLN(F("Wifi Connection Timeout, going to AP Mode"));
// 		setupAP();
// 	}
// }

#pragma endregion

#pragma region Funciones MQTT
// ///////////////////////////////////////////////////////////////////////////
// //   MQTT
// ///////////////////////////////////////////////////////////////////////////
String StringTopic = "";
char CharTopic[100];
String StringData = "";
char CharData[10];

volatile unsigned long lastMQTTConnection = TIMEOUT;
/*
   Function called when a MQTT message has arrived
   @param p_topic   The topic of the MQTT message
   @param p_payload The payload of the MQTT message
   @param p_length  The length of the payload
*/
void handleMQTTMessage(char* p_topic, byte* p_payload, unsigned int p_length) {
  uint8_t i = 0;
  String topic = String(p_topic);

  String payload;
  for (i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  
  DEBUG_PRINTLN(F("INFO: New MQTT message received"));
  DEBUG_PRINT(F("INFO: MQTT topic: "));
  DEBUG_PRINTLN(topic);
  DEBUG_PRINT(F("INFO: MQTT payload: "));
  DEBUG_PRINTLN(payload);
  
  boolean set_value = false;
  for(i=0;i<NUM_IO;i++)           // Controla las salidas
  {
    if (topic.indexOf(config.setTopic[i])>0)
    {
      DEBUG_PRINT(F("FOUND: "));
      DEBUG_PRINTLN(config.setTopic[i]);
      if(payload.equals(ON))
      {
        set_value = true;
        DEBUG_PRINTLN(F("PAYLOAD: ON"));
      }  
      if(payload.equals(OFF))
      {
        set_value = false;
        DEBUG_PRINTLN(F("PAYLOAD: OFF"));
      } 
      
      if(set_value != nodo.OUT[i])
      {
        nodo.OUT[i] = set_value;
        digitalWrite(Output[i], nodo.OUT[i]);
        DEBUG_PRINT(F("Output "));
		DEBUG_PRINT(i);
        DEBUG_PRINT(F(" TURNED: "));
        DEBUG_PRINTLN((nodo.OUT[i])?"ON":"OFF");
      }
      PublishOutputs(i);
    }
  }
  
}

/*
  Function called to subscribe to a MQTT topic
*/
void subscribeToMQTT(char* p_topic) {
  if (mqttClient.subscribe(p_topic)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded for topic: "));
    DEBUG_PRINTLN(p_topic);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed for topic: "));
    DEBUG_PRINTLN(p_topic);
  }
}

/*
  Function called to publish to a MQTT topic with the given payload
*/
void publishToMQTT(char* p_topic, char* p_payload) {
  if (mqttClient.publish(p_topic, p_payload, RETAIN)) {
    DEBUG_PRINT(F("INFO: MQTT message published successfully, topic: "));
    DEBUG_PRINT(p_topic);
    DEBUG_PRINT(F(", payload: "));
    DEBUG_PRINTLN(p_payload);
  } else {
    DEBUG_PRINTLN(F("ERROR: MQTT message not published, either connection lost, or message too large. Topic: "));
    DEBUG_PRINT(p_topic);
    DEBUG_PRINT(F(" , payload: "));
    DEBUG_PRINTLN(p_payload);
  }
}

/*
  Function called to connect/reconnect to the MQTT broker
*/
void connectToMQTT()
{
  if (!mqttClient.connected())
  {
    if (lastMQTTConnection + TIMEOUT < millis())
    {
      StringTopic = "/" + String(config.NODE_NAME) + STATUS_TOPIC;
      StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
      if (mqttClient.connect(config.NODE_NAME, config.MQTT_USER, config.MQTT_PASS, CharTopic, QoS, RETAIN, MQTT_DISCONNECTED_STATUS))
      {
        DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
        publishToMQTT(CharTopic, MQTT_CONNECTED_STATUS);

        for(int i=0; i<NUM_IO; i++) // Me suscribo a los topics para encender luces
        {
          StringTopic = "/" + String(config.NODE_NAME) + String(config.setTopic[i]);
          StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
          subscribeToMQTT(CharTopic);
        }
		PublishAll();
      } 
      else
      {
        DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
        DEBUG_PRINT(F("INFO: MQTT username: "));
        DEBUG_PRINTLN(config.MQTT_USER);
        DEBUG_PRINT(F("INFO: MQTT password: "));
        DEBUG_PRINTLN(config.MQTT_PASS);
        DEBUG_PRINT(F("INFO: MQTT broker: "));
        DEBUG_PRINTLN(config.MQTT_SERVER);
      }
      lastMQTTConnection = millis();
    }
  }
}

void PublishInputs(uint8_t i)
{
  StringTopic = "/" + String(config.NODE_NAME) + String(config.InStateTopic[i]);
  StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
  StringData = String((nodo.IN[i])?ON:OFF);
  StringData.toCharArray(CharData, sizeof(CharData));
  publishToMQTT(CharTopic, CharData);
}

void PublishOutputs(uint8_t i)
{
  StringTopic =  "/" + String(config.NODE_NAME) + String(config.OutStateTopic[i]);
  StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
  StringData = String((nodo.OUT[i])?ON:OFF);
  StringData.toCharArray(CharData, sizeof(CharData));
  publishToMQTT(CharTopic, CharData);
}

void PublishTemp()
{
  StringTopic = "/" + String(config.NODE_NAME) + String(config.stateTempTopic);
  StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
  StringData = String(nodo.temp);
  StringData.toCharArray(CharData, sizeof(CharData));
  publishToMQTT(CharTopic, CharData);
}

void PublishWiFi()
{
  StringTopic = "/" + String(config.NODE_NAME) + "/wifi/ssid";
  StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
  StringData = String(WiFi.SSID());
  StringData.toCharArray(CharData, sizeof(CharData));
  publishToMQTT(CharTopic, CharData);

  StringTopic = "/" + String(config.NODE_NAME) + "/wifi/ch";
  StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
  StringData = String(WiFi.channel());
  StringData.toCharArray(CharData, sizeof(CharData));
  publishToMQTT(CharTopic, CharData);

  StringTopic = "/" + String(config.NODE_NAME) + "/wifi/rssi";
  StringTopic.toCharArray(CharTopic, sizeof(CharTopic));
  StringData = String(WiFi.RSSI());
  StringData.toCharArray(CharData, sizeof(CharData));
  publishToMQTT(CharTopic, CharData);
}

void PublishAll()
{
  for(int i = 0; i<NUM_IO; i++)
  {
    PublishOutputs(i);
    PublishInputs(i);
  }

  PublishTemp();

  PublishWiFi();
}
#pragma endregion

#pragma region Lectura de Teclas y sensor
void CheckTeclas(){
  static uint8_t RoundCheck[NUM_IO] = { 0,0 };
  static uint32_t LongPressCheck[NUM_IO] = {0,0};
  const uint8_t RoundCheckThreshole = 10;  // 50 mSeg
  const uint32_t LongPressThreshole = 600; // 3 Seg

  for (uint8_t cnt = 0; cnt < NUM_IO; cnt++)
  {
    uint8_t curStatus = digitalRead(Input[cnt]);
    if (nodo.IN[cnt] != curStatus)
    {
      LongPressCheck[cnt] = 0;
      delay(5);
      curStatus = digitalRead(Input[cnt]);
      if (nodo.IN[cnt] != curStatus)
      {
        RoundCheck[cnt]++;
      }
      else  RoundCheck[cnt] = 0;

      if (RoundCheck[cnt] >= RoundCheckThreshole)
      {
        if(nodo.IN[cnt]&!curStatus) // Si la tecla pasa de 1 a 0
        {
          nodo.OUT[cnt]^=1;            // Invierto la salida 
          digitalWrite(Output[cnt], nodo.OUT[cnt]);  // Activo/Desactivo Salida
          LongPressCheck[cnt]=1;
		  PublishOutputs(cnt);
        }
        else
        {
          LongPressCheck[cnt] = 0;
        }
        
        nodo.IN[cnt] = curStatus;
        RoundCheck[cnt] = 0;
        
        PublishInputs(cnt);
      }
    }
    else
    {
      if(LongPressCheck[cnt]>0)
      {
        delay(5);
        curStatus = digitalRead(Input[cnt]);
        if (nodo.IN[cnt] == curStatus)
        {
          LongPressCheck[cnt]++;
        }
        else  LongPressCheck[cnt] = 0;

        if (LongPressCheck[cnt] >= LongPressThreshole)
        {
          // DEBUG_PRINTLN("Long PRESS Action: Reboot");
          // delay(1000);
          // LongPressCheck[cnt] = 0;  // En caso de usarlo para resetear el ESP, no es necesario :P
          // ESP.restart();

					DEBUG_PRINTLN("Restoring Default Config and going to AP mode");
					saveConfig(DEF);

					WiFiManager wifiManager;
					if(!wifiManager.startConfigPortal(config.AP_SSID, config.AP_PASS)){
						DEBUG_PRINTLN("failed to connect and hit timeout");
						delay(3000);
						ESP.restart();
    				}
					DEBUG_PRINTLN("Connected! Rebooting...");
					delay(3000);
					ESP.restart();
        }
      }
    }
  }
}

void ReadTemp()
{
  sensors.requestTemperatures();
  float int_temp = sensors.getTempCByIndex(0);
  if(!isnan(int_temp))  nodo.temp = int_temp;
}
#pragma endregion

#pragma region Funciones Web Server
String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool handleFileRead(String path) {
  DEBUG_PRINTLN("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz)) {
      path += ".gz";
    }
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void SendJsonConfig(){
	loadConfig(WEB);
}

// Envio el JSON:
// {
//     "tempvalor":"20.5 °C",
//     "salidas":[1,0,1,0],
//     "teclas":[1,1,1,1],
//	   "mqtt": 1
// }
void SendJsonData(){
	const size_t capacity = 2*JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(4);
	DynamicJsonDocument doc(capacity);

	doc["tempvalor"] = (String)nodo.temp + " °C";

	JsonArray salidas = doc.createNestedArray("salidas");
	JsonArray teclas = doc.createNestedArray("teclas");
	for(int i=0; i< NUM_IO; i++)
	{
		salidas.add(nodo.OUT[i]);
		teclas.add(nodo.IN[i]);
	}

	if(config.MQTT_ACTIVE){
		doc["mqtt"] = mqttClient.connected()?1:0;
	}
	
	char buffer[capacity];
	serializeJson(doc, buffer);
	server.send(200, "text/json", buffer);
	// DEBUG_PRINTLN("Enviando estados:");
	// serializeJsonPretty(doc,Serial);
}

// Recibe un array con la forma [1,0,...1] desde el cleinte web y actua sobre la salida
bool SetOutWeb(){
	const size_t capacity = JSON_ARRAY_SIZE(NUM_IO);
	DynamicJsonDocument doc(capacity);

	DeserializationError err= deserializeJson(doc, server.arg("plain"));
	if(err)	{
		DEBUG_PRINT(F("config.json from web client failed with code "));
		DEBUG_PRINTLN(err.c_str());
		return false;
	}
	else
	{
		for(int i=0; i<NUM_IO; i++){
			if(nodo.OUT[i] != doc[i]){
				nodo.OUT[i] = doc[i];
				digitalWrite(Output[i],nodo.OUT[i]);
				if(config.MQTT_ACTIVE){
					PublishOutputs(i);
					//SendJsonData(); Ver si quiero hacerlo asincrono
					// ver https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/
				}
			}
		}
	}
	DEBUG_PRINTLN("Comando recibido:");
	serializeJsonPretty(doc,Serial);
	return true;
}
#pragma endregion

void setup()
{
	pinMode(INPUT1, INPUT);
  	pinMode(INPUT2, INPUT);
	pinMode(INPUT3, INPUT);
  	pinMode(INPUT4, INPUT);
  	pinMode(OUTPUT1, OUTPUT);
  	pinMode(OUTPUT2, OUTPUT);
	pinMode(OUTPUT3, OUTPUT);
  	pinMode(OUTPUT4, OUTPUT);
  	digitalWrite(OUTPUT1, LOW);
  	digitalWrite(OUTPUT2, LOW);
	digitalWrite(OUTPUT3, LOW);
  	digitalWrite(OUTPUT4, LOW);

#if defined(DEBUG_SERIAL)
	Serial.begin(115200);
#elif defined(DEBUG_TELNET)
  	telnetServer.begin();
  	telnetServer.setNoDelay(true);
#endif

	DEBUG_PRINTLN("");
	delay(1000);
	DEBUG_PRINTLN(F("Mounting FS..."));

	if (!SPIFFS.begin()){
		DEBUG_PRINTLN(F("Failed to mount file system"));
	}
	else{
		Dir dir = SPIFFS.openDir("/");
		DEBUG_PRINTLN("FS Files:");
    	while (dir.next()){
			String fileName = dir.fileName();
			size_t fileSize = dir.fileSize();
			DEBUG_PRINTLN((String) fileName.c_str() + " - " + (String) formatBytes(fileSize).c_str());
    	}
	}

	if (!loadConfig(DEF))
	{
		DEBUG_PRINTLN(F("Missing Config File, restore default values"));
		saveConfig(DEF);	// Restore defaults
		ESP.restart();
	}

	WiFi.onEvent(handleWiFiEvent);
	WiFiManager wifiManager;
	if(!config.DHCP){
		wifiManager.setSTAStaticIPConfig(config.IP, config.GW, config.MASK);
	}
	wifiManager.setAPStaticIPConfig(config.AP_IP,config.AP_IP,config.MASK);
	wifiManager.autoConnect(config.AP_SSID,config.AP_PASS);
	
	// Cargo los valores de SSID/PASS obtenidos por el portal
	String data = WiFi.SSID();
	data.toCharArray(config.SSID, sizeof(config.SSID));
	data = WiFi.psk();
	data.toCharArray(config.PASS, sizeof(config.PASS));
	saveConfig(LOC);
	DEBUG_PRINT(F("Connected to SSID: "));
	DEBUG_PRINTLN(config.SSID);
	DEBUG_PRINT(F("with password: "));
	DEBUG_PRINTLN(config.PASS);

	setupOTA();

	server.on("/", []() {if (!handleFileRead("/index.html")){server.send(404, "text/plain", "FileNotFound");}});
	server.on("/funciones.js", []() {if (!handleFileRead("/funciones.js")){server.send(404, "text/plain", "FileNotFound");}});
	server.on("/favicon.ico", []() {if (!handleFileRead("/favicon.ico")){server.send(404, "text/plain", "FileNotFound");}});
	server.on("/estilo.css", []() {if (!handleFileRead("/estilo.css")){server.send(404, "text/plain", "FileNotFound");}});
	server.on("/logo.png", []() {if (!handleFileRead("/logo.png")){server.send(404, "text/plain", "FileNotFound");}});
	server.on("/config.json", SendJsonConfig);
	server.on("/data.json", SendJsonData);
	server.on("/config", HTTP_PUT, []() {saveConfig(WEB); server.send(200, "text/json", "{success:true}"); });
	server.on("/setout", HTTP_PUT, []() {SetOutWeb(); server.send(200, "text/json", "{success:true}"); });
	server.on("/reset", HTTP_GET, []() { saveConfig(DEF); server.send(200, "text/plain", "OK"); WiFi.disconnect(); ESP.restart(); }); 
	server.on("/reboot", HTTP_GET, []() { server.send(200, "text/html", "OK"); ESP.restart(); }); 
	server.onNotFound([]() { server.send(400, "text/html", "Contenido no Encontrado"); });
	server.begin();

	sensors.begin();
  	ReadTemp();
	read_time.attach(60, ReadTemp);

	mqttClient.setServer(config.MQTT_SERVER, config.MQTT_PORT);
  	mqttClient.setCallback(handleMQTTMessage);
  	if(config.MQTT_ACTIVE)	connectToMQTT();  
}

void loop()
{
	server.handleClient();

	if(config.MQTT_ACTIVE){
		connectToMQTT();
  		mqttClient.loop();
  		yield();
	}
	
#if defined(DEBUG_TELNET)
	handleTelnet();
	yield();
#endif

	handleOTA();
	yield();

	CheckTeclas();
	yield();
}
