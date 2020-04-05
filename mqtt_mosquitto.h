
#define MQTT_MAX_PACKET_SIZE 455 //cambialo antes de incluir docpatth\Arduino\libraries\pubsubclient-master\src\pubsubclient.h
#define MQTT_KEEP_ALIVE 60
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7

/*************************************************
 ** -------- Valores Personalizados ----------- **
 * ***********************************************/
#define DEVICE_TYPE "ESP32"
#define ORG "canMorras"
#define DEVICE_ID "M_Oscura_1"
char* ssid;
char* password;
char ssid1[] = "Your SSID1";
char password1[] ="Password1";
char ssid2[] ="Your SSID2";
char password2[] = "Password2";
#include <personal.h>
/*************************************************
 ** ----- Fin de Valores Personalizados ------- **
 * ***********************************************/
#define ESPERA_NOCONEX 60000  // cuando no hay conexion, descanso un minuto
char server[] = "192.168.10.1";
char * authMethod = NULL;
char * token = NULL;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

char publishTopicMeteo[] = "meteo/envia" ;  // el dispositivo envia datos a Mosquitto
char publishTopicBateria[] = "bateria/envia";  // el dispositivo envia datos a Mosquitto
char metadataTopic[]= "bateria/envia/metadata/" DEVICE_ID; //el dispositivo envia sus metadatos a Mosquitto
char updateTopic[]  = "bateria/update/" DEVICE_ID;    // Mosquitto o node-red me actualiza los metadatos
char responseTopic[]= "bateria/response/" DEVICE_ID;
char rebootTopic[]  = "bateria/reboot/" DEVICE_ID;
