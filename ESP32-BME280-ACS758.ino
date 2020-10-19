/************************************************************************
****          Sensor BME280 y 2x ACS758 (para mediar amperios)       **** 
****       cada ACS758 se lee con diferentes atenuaciones            **** 
****       envia mensajes por MQTT (ver mosquitto.h)                 ****
****         Morrastronics -- by chuRRuscat                          ****
****         version 1.0 08/2018   Initial version                   **** 
************************************************************************/ 
/************************************************************************
*********** CALIBRACION DE SENSORES ver sensores.h                  *****
**** Opcions de atenuacion del ADC del ESP32                        *****
**** ADC_0db = 1.1v                                                 *****
**** ADC_2_5db = 1.4v                                               *****
**** ADC_6db = 1.9v                                                 *****
**** ADC_11db = 3.2v                                                *****
**** al ADC CH0 (GPIO36) le conectamos el sensor de orriente del    *****
**** panel solar, ya que para el sensor V=0.04*I, o sea, que puedo  ***** 
**** medir hasta 27A con 1.1V (atenuacion 0)                        *****
**** al ADC CH3 (GPIO39) le conecto el de bateria, que puede tener  *****
**** cargas mas fuertes; como es bidireccional V=Vcc/2+-0.02*I      *****
**** lo que da una corriente maxima de 70A con 3.9V (atenuacion 11) *****
**** OJO con el sentido de la corriente, lo suyo es que la descarga *****
**** sea positivo (se supone que el alternador da 80A)              *****
**** Para conectar, uso anillas de 10 mm diámetro+cable 3.5 mm diam *****
************************************************************************/    

#undef PRINT_SI
#define PRINT_SI
#ifdef PRINT_SI
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
  #define DPRINT(...)     //linea en blanco
  #define DPRINTLN(...)   //linea en blanco
#endif
/*************************************************
 ** -------- Valores Personalizados ----------- **
 * *****************************************/
#include "mqtt_mosquitto.h"
#define NUMERO_DE_ENVIOS_TEMP 3000 // cada cuanto tiempo envio datos de temp, humedad y presion
#define NUMERO_DE_ENVIOS_AMP 60    //numero de iteraciones para enviar datos de amperios (aprox 60seg)


/*************************************************
 ** ----- Fin de Valores Personalizados ------- **
 * ***********************************************/
//#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <Adafruit_BME280.h> //https://github.com/Takatsuki0204/BME280-I2C-ESP32
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson

#define AJUSTA_T 10000 // para ajustar las esperas que hay en algunos sitios
#define I2C_SDA 21 
#define I2C_SCL 22
#define ADC1_CH0 36  // Aqui va el amperimetro de los paneles solares
#define ADC1_CH3 39  // y este es el de descarga y carga general
#define BME280_ADDR 0x76  //Si no funciona, prueba con la direccion 0x77

#include <Adafruit_BME280.h> //https://github.com/Takatsuki0204/BME280-I2C-ESP32
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson

#define JSONBUFFSIZE 250
#define DATOSJSONSIZE 250

Adafruit_BME280 sensorBME280(I2C_SDA, I2C_SCL);
// ********* Variables del sensor que expondremos ********** 
float temperatura,humedadAire,presionHPa;
int sensorSolar=0,sensorBat=0 ;

// otras variables
int tocaTemp=NUMERO_DE_ENVIOS_TEMP+1;
int tocaAmp=NUMERO_DE_ENVIOS_AMP+1;
int intervaloConex=60000;
int numMedidas=0;
char datosJson[DATOSJSONSIZE];

void setup() 
{
  #ifdef PRINT_SI
  Serial.begin(115200);
  #endif 
  DPRINTLN("arranco"); 
  bool status = sensorBME280.begin(BME280_ADDRESS);
  if (!status) {
    DPRINTLN("No encuentro sensor BME280");
  }
  // ajusto la sensibilidad  de CH0 (36, Solar) y CH3 (39, bat)
  analogSetPinAttenuation(ADC1_CH0,ADC_0db) ;
  analogSetPinAttenuation(ADC1_CH3,ADC_11db);  
  //wifiConnect();
  //mqttConnect();
  delay(50);
  //initManagedDevice(); 
  tomaDatos();
  //publicaDatos();
}

uint32_t ultima=0;

void loop() 
{
  DPRINT("*");
  /*if (!loopMQTT()) {  // leo si hay mensajes de IoT y si tengo conexion    
   DPRINTLN("He perdido la conexion,reconecto");
   sinConectividad();        
   mqttConnect();
   initManagedDevice();  // Hay que suscribirse cada vez
  } 
  */tomaDatos();
  //publicaDatos();
  ultima=millis();
  espera(1000);
}

boolean tomaDatos ()
{
  // Obtengo la temperatura y humedad
  float bufTemp,bufTemp1,bufHumedad,bufHumedad1,bufPresion,bufPresion1;
  boolean escorrecto=true; 

  humedadAire= sensorBME280.readHumidity();
  temperatura= sensorBME280.readTemperature();
  presionHPa=sensorBME280.readPressure()/100.0F;
  sensorSolar += analogRead(ADC1_CH0);
  sensorBat   += analogRead(ADC1_CH3);
  numMedidas++;
  
  DPRINTLN("tomo datos"); 
  if (isnan(bufHumedad) || isnan(bufTemp) || isnan(bufHumedad1) || isnan(bufTemp1) ) {       
     DPRINTLN("no he podido leer del sensor !");       
     escorrecto=false;
  } else {
    DPRINT("Temperatura     \t") ;DPRINT(temperatura);
    DPRINT("Humedad aire    \t");DPRINT(humedadAire);
    DPRINT("Presion HPa     \t");DPRINT(presionHPa);
    DPRINT("Sensor P. solar \t");DPRINTLN(sensorSolar);  
    DPRINT("Sensor Bateria  \t");DPRINTLN(sensorBat);
  } 
  return escorrecto;
}

void publicaDatos() 
{
  int k=0;
  char signo;
  boolean pubresult=true;  

  if (tocaTemp > NUMERO_DE_ENVIOS_TEMP) {
    if (temperatura<0) {       // Preparo los datos en modo JSON.
      signo='-';
      temperatura*=-1;
    }  else signo=' ';
    sprintf(datosJson,"[{\"temp\":%c%d.%1d,\"hAire\":%d,\"HPa\":%d,\
               {\"deviceId\":\"%s\"}]",\
          signo,(int)temperatura, (int)(temperatura * 10.0) % 10,\
          (int)humedadAire, (int)presionHPa,DEVICE_ID);
    tocaTemp=0;      
    pubresult = enviaDatos(publishTopicMeteo,datosJson);     
  } 
  if (tocaAmp>NUMERO_DE_ENVIOS_AMP) { 
    sensorSolar=sensorSolar/numMedidas;   // calculo la media
    sensorBat=sensorBat/numMedidas; 
    sprintf(datosJson,"[{\"ISolar\":%d,\"IBat\":%d},{\"deviceId\":\"%s\"}]",\
         sensorSolar,sensorBat,DEVICE_ID);
    sensorSolar=0;      //reinicio los valores a cero
    sensorBat=0;
    numMedidas=0;
    pubresult = enviaDatos(publishTopicBateria,datosJson);
    tocaAmp=0;             
  }
  tocaTemp++;
  tocaAmp++;

}
