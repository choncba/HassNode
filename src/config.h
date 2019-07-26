///////////////////////////////////////////////////////////////////////////
//   PINS
///////////////////////////////////////////////////////////////////////////
#define INPUT1   D1  // GPIO5 - Tecla Patio    
#define INPUT2   D2  // GPIO4 - Tecla Galeria  
#define INPUT3    0 // 
#define INPUT4    0 //  
#define OUTPUT1     D6  // GPIO12 - Luz Patio  
#define OUTPUT2     D7  // GPIO13 - Luz Galeria    
#define OUTPUT3     0  // 
#define OUTPUT4     0  // 
#define OW_PIN      D4    
///////////////////////////////////////////////////////////////////////////
// MISC
///////////////////////////////////////////////////////////////////////////
#define USE_TEMP

#define NUM_IO 		2
#define MAX_NUM_IO 	4
const uint8_t Input[MAX_NUM_IO] = { INPUT1, INPUT2, INPUT3, INPUT4 };
enum Input_enum { input1 = 0, input2, input3, input4 };
const uint8_t Output[MAX_NUM_IO] = { OUTPUT1, OUTPUT2, OUTPUT3, OUTPUT4 };
enum Output_enum { output1 = 0, output2, output3, output4 };
///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////
#define BASE_TOPIC "/" MQTT_CLIENT_ID
#define STATUS_TOPIC "/status"
#define MQTT_AVAILABILITY_TOPIC "/LWT"
#define MQTT_CONNECTED_STATUS "online"
#define MQTT_DISCONNECTED_STATUS "offline"

#define ON   "1"
#define OFF  "0"

#define TIMEOUT 5000

#define RETAIN false
#define QoS     0

///////////////////////////////////////////////////////////////////////////
//   DEBUG
///////////////////////////////////////////////////////////////////////////
//#define DEBUG_TELNET
#define DEBUG_SERIAL

///////////////////////////////////////////////////////////////////////////
//   OTA
///////////////////////////////////////////////////////////////////////////
#define OTA
//#define OTA_HOSTNAME  MQTT_CLIENT_ID  // hostname esp8266-[ChipID] by default
//#define OTA_PASSWORD  "password"  // no password by default
//#define OTA_PORT      8266        // port 8266 by default
