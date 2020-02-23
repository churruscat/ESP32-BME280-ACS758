// hay que inluir mqtt_mosquitto.h en el programa principal
#include <WiFi.h>
void funcallback(char* topic, byte* payload, unsigned int payloadLength);
WiFiClient wifiClient;
PubSubClient clienteMQTT(server, 1883, funcallback, wifiClient);

boolean wifiConnect() 
{
  int i=0,j=0;  
  ssid=ssid1;
  password=password1;

  DPRINT("Conectando a WiFi  "); DPRINTLN(ssid);  
  WiFi.mode(WIFI_STA);  //Estacion, no AP ni mixto
  WiFi.disconnect();
  WiFi.begin(ssid,password);

  while ((WiFi.status() != WL_CONNECTED )) {
    espera(500);
    DPRINT(i++);
    DPRINT(".");   
    if (i>120) { 
      if (ssid==ssid1){
        ssid=ssid2;
        password=password2;
      } else {
        ssid=ssid1;
        password=password1;
      }
      i=0;
      j++;
      if (j>4) { return false;} /* no funciona ninguna */   
      DPRINTLN();
      DPRINT("cambio de red num ");DPRINTLN(j);      
      DPRINT("Me conecto a "); DPRINTLN(ssid);
      WiFi.disconnect();
      espera(1000);
      WiFi.begin(ssid,password);      
      }
  }
  DPRINTLN(ssid);  DPRINT("*******Conectado; ADDR= ");
  DPRINTLN(WiFi.localIP());
  return true;
}

void sinConectividad()
{
  int j=0;

  clienteMQTT.disconnect(); 
  espera(500);
  while(!wifiConnect()) {   
  DPRINT("Sin conectividad, espero secs  ");DPRINTLN(int(intervaloConex/2000));
  espera(ESPERA_NOCONEX);
  }
}

void mqttConnect() 
{
  int j=0;
 
  if ((WiFi.status() == WL_CONNECTED )) {
   while (!clienteMQTT.connect(clientId, authMethod, token)) {      
     DPRINT(j);DPRINTLN("  Reintento conexion del MQTT client  ");
     j++;
     espera(2000);
     if (j>20) {
       sinConectividad();  
       j=0;
      }
     }
   } else {
    sinConectividad();   
  } 
}

boolean loopMQTT() 
{
  return clienteMQTT.loop();
}

void initManagedDevice() 
{
  int rReboot,rUpdate,rResponse; 

  rReboot=  clienteMQTT.subscribe(rebootTopic,1);
  rUpdate=  clienteMQTT.subscribe(updateTopic,1);
  rResponse=clienteMQTT.subscribe(responseTopic,1);
  DPRINTLN("Suscripcion. Response= ");
  DPRINT("\tReboot= ");DPRINT(rReboot);
  DPRINT("\tUpdate= ");DPRINTLN(rUpdate); 
}

void funcallback(char* topic, byte* payload, unsigned int payloadLength) 
{
   DPRINT("funcallback invocado para el topic: "); DPRINTLN(topic);
   if (strcmp (updateTopic, topic) == 0) {
     handleUpdate(payload);  
  }
  else if (strcmp (responseTopic, topic) == 0) { 
    DPRINTLN("handleResponse payload: ");
    DPRINTLN((char *)payload); 
  } 
  else if (strcmp (rebootTopic, topic) == 0) {
    DPRINTLN("Rearrancando...");    
    ESP.restart(); // da problemas, no siempre rearranca
    //ESP.reset();
 }
}

void handleUpdate(byte* payload) 
{
  StaticJsonBuffer<JSONBUFFSIZE> jsonBuffer; 
  JsonObject& root = jsonBuffer.parseObject((char*)payload);
  boolean cambia=false,pubresult;
  char sensor[20],elpayload[150];

  DPRINTLN("handleUpdate payload, el payload:");
  DPRINTLN((char *)payload);
  DPRINTLN(" payload, antes de actualizar:");

  if (!root.success()) {
   return;
  }
  if (root.containsKey("sensor")) {
   if (strcmp (root["sensor"],DEVICE_ID) !=0) {
    strcpy(sensor,root["sensor"]);
    DPRINT("no va conmigo, es para ");
    DPRINTLN(sensor);
    return;
   }
  }
  return;
}

boolean enviaDatos(char * topic, char * datosJSON) 
{
  int k=0;
  char signo;
  boolean pubresult=false;  
  
  while (!clienteMQTT.loop() & k<20 ) {
    DPRINTLN("Estaba desconectado ");   
    mqttConnect();
    initManagedDevice();
    k++; 
  } 
  pubresult = clienteMQTT.publish(topic,datosJSON);
  DPRINT("Envio ");DPRINT(datosJson);
  DPRINT("a ");DPRINTLN(publishTopicBateria
  );
  if (pubresult) 
    DPRINTLN("... OK envio conseguido");      
  else
    DPRINTLN(".....KO envio fallado");
  return pubresult;    
}

void espera(unsigned long tEspera) 
{
  uint32_t principio = millis();
  
  while ((millis()-principio)<tEspera) {
    yield();
    delay(500);
  }
}
