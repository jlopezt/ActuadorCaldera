/*
 * Actuador para Termostato
 *
 * Actuador remoto
 * 
 * Reles de conexion
 * Servicio web levantado en puerto ZZZ
 */

//Defines generales
#define NOMBRE_FAMILIA "Termostatix"
#define VERSION "v1.8.0"
#define SEPARADOR        '|'
#define SUBSEPARADOR     '#'
#define KO               -1
#define OK                0
#define MAX_VUELTAS  UINT16_MAX// 32767 

#define PUERTO_WEBSERVER 80
#define MAX_RELES        2 //numero maximo de reles soportado

//Nombres de ficheros
#define GLOBAL_CONFIG_FILE     "/Config.json"
#define GLOBAL_CONFIG_BAK_FILE "/Config.json.bak"
#define RELES_CONFIG_FILE      "/RelesConfig.json"
#define RELES_CONFIG_BAK_FILE  "/RelesConfig.json.bak"
#define WIFI_CONFIG_FILE       "/WiFiConfig.json"
#define WIFI_CONFIG_BAK_FILE   "/WiFiConfig.json.bak"
#define MQTT_CONFIG_FILE       "/MQTTConfig.json"
#define MQTT_CONFIG_BAK_FILE   "/MQTTConfig.json.bak"

// Una vuela de loop son ANCHO_INTERVALO segundos 
#define MULTIPLICADOR_ANCHO_INTERVALO 5 //Multiplica el ancho del intervalo para mejorar el ahorro de energia
#define ANCHO_INTERVALO          100 //Ancho en milisegundos de la rodaja de tiempo
#define FRECUENCIA_OTA             5 //cada cuantas vueltas de loop atiende las acciones
#define FRECUENCIA_LOGICA         10 //cada cuantas vueltas de loop atiende la  logica de los reles
#define FRECUENCIA_SERVIDOR_WEB    1 //cada cuantas vueltas de loop atiende el servidor web
#define FRECUENCIA_MQTT           10 //cada cuantas vueltas de loop envia y lee del broket MQTT
#define FRECUENCIA_ORDENES         2 //cada cuantas vueltas de loop atiende las ordenes via serie 
#define FRECUENCIA_ENVIO_DATOS   300 //cada cuantas vueltas de loop envia al broker el estado de E/S
#define FRECUENCIA_WIFI_WATCHDOG 100 //cada cuantas vueltas comprueba si se ha perdido la conexion WiFi

//Includes generales
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <TimeLib.h>  // download from: http://www.arduino.cc/playground/Code/Time
//#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

//Indica si el rele se activa con HIGH o LOW
int nivelActivo=LOW; //Se activa con HIGH por defecto

/*-----------------Variables comunes---------------*/
String nombre_dispositivo(NOMBRE_FAMILIA);//Nombre del dispositivo, por defecto el de la familia
uint16_t vuelta = MAX_VUELTAS-100;//0; //vueltas de loop
int debugGlobal=0; //por defecto desabilitado
uint8_t ahorroEnergia=0;//inicialmente desactivado el ahorro de energia
time_t anchoLoop= ANCHO_INTERVALO;//inicialmente desactivado el ahorro de energia

//Contadores
uint16_t multiplicadorAnchoIntervalo=5;
uint16_t anchoIntervalo=100;
uint16_t frecuenciaOTA=5;
uint16_t frecuenciaLogica=10;
uint16_t frecuenciaServidorWeb=1;
uint16_t frecuenciaOrdenes=2;
uint16_t frecuenciaMQTT=50;
uint16_t frecuenciaEnvioDatos=100;
uint16_t frecuenciaWifiWatchdog=100;

/************************* FUNCIONES PARA EL BUITIN LED ***************************/
void configuraLed(void){pinMode(LED_BUILTIN, OUTPUT);}
void enciendeLed(void){digitalWrite(LED_BUILTIN, LOW);}//En esp8266 es al reves que en esp32
void apagaLed(void){digitalWrite(LED_BUILTIN, HIGH);}//En esp8266 es al reves que en esp32
void parpadeaLed(uint8_t veces, uint16_t espera=100)
  {
  for(uint8_t i=0;i<2*veces;i++)
    {
    delay(espera/2);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }
/***********************************************************************************/  

void setup()
  {
  Serial.begin(115200);
  configuraLed();
  enciendeLed();
    
  Serial.printf("\n\n\n");
  Serial.printf("*************** %s ***************\n",NOMBRE_FAMILIA);
  Serial.printf("*************** %s ***************\n",VERSION);
  Serial.println("***************************************************************");
  Serial.println("*                                                             *");
  Serial.println("*             Inicio del setup del modulo                     *");
  Serial.println("*                                                             *");    
  Serial.println("***************************************************************");
  
  Serial.printf("\nMotivo del reinicio: %s\n",ESP.getResetReason().c_str());
  
  Serial.printf("\n\nInit Ficheros ---------------------------------------------------------------------\n");
  //Ficheros - Lo primero para poder leer los demas ficheros de configuracion
  inicializaFicheros(debugGlobal);
  apagaLed();
    
  //Configuracion general
  Serial.println("Init Config -----------------------------------------------------------------------");
  inicializaConfiguracion(debugGlobal);
  parpadeaLed(1);

  //Wifi
  Serial.println("Init WiFi -----------------------------------------------------------------------");
  if (inicializaWifi(true))//debugGlobal)) No tien esentido debugGlobal, no hay manera de activarlo
    {
    parpadeaLed(5,200);

    /*----------------Inicializaciones que necesitan red-------------*/
    //OTA
    Serial.println("Init OTA -----------------------------------------------------------------------");
    inicializaOTA(debugGlobal);
    parpadeaLed(1);
    //MQTT
    Serial.println("Init MQTT -----------------------------------------------------------------------");
    inicializaMQTT();
    parpadeaLed(2);
    //WebServer
    Serial.println("Init Web ------------------------------------------------------------------------");
    inicializaWebServer();
    parpadeaLed(3);
    }
  else Serial.println("No se pudo conectar al WiFi");
  apagaLed();

  //Reles
  Serial.println("Init reles ------------------------------------------------------------------------");
  inicializaReles();

  //Ordenes serie
  Serial.println("Init Ordenes ----------------------------------------------------------------------");  
  inicializaOrden();//Inicializa los buffers de recepcion de ordenes desde PC
  
  parpadeaLed(2);
  apagaLed();//Por si acaso....
  
  Serial.println("***************************************************************");
  Serial.println("*                                                             *");
  Serial.println("*               Fin del setup del modulo                      *");
  Serial.println("*                                                             *");    
  Serial.println("***************************************************************");
  }  

void  loop()
  {  
  //referencia horaria de entrada en el bucle
  time_t EntradaBucle=millis();//Hora de entrada en la rodaja de tiempo

  //------------- EJECUCION DE TAREAS --------------------------------------
  //Acciones a realizar en el bucle   
  //Prioridad 0: OTA es prioritario.
  if ((vuelta % frecuenciaOTA)==0) ArduinoOTA.handle(); //Gestion de actualizacion OTA
  //Prioridad 2: Funciones de control.
  if ((vuelta % frecuenciaLogica)==0) actuaReles(debugGlobal);
  //Prioridad 3: Interfaces externos de consulta    
  if ((vuelta % frecuenciaServidorWeb)==0) webServer(debugGlobal); //atiende el servidor web
  if ((vuelta % frecuenciaMQTT)==0) atiendeMQTT();
  if ((vuelta % frecuenciaEnvioDatos)==0) enviaDatos(debugGlobal); //publica via MQTT los datos de entradas y salidas, segun configuracion  
  if ((vuelta % frecuenciaOrdenes)==0) while(HayOrdenes(debugGlobal)) EjecutaOrdenes(debugGlobal); //Lee ordenes via serie
  if ((vuelta % frecuenciaWifiWatchdog)==0) WifiWD();  
  //------------- FIN EJECUCION DE TAREAS ---------------------------------  

  //sumo una vuelta de loop, si desborda inicializo vueltas a cero
  vuelta++;//sumo una vuelta de loop  
      
  //Espero hasta el final de la rodaja de tiempo
  while(millis()<EntradaBucle+anchoLoop)
    {
    if(millis()<EntradaBucle) break; //cada 49 dias el contador de millis desborda
    //delayMicroseconds(1000);
    delay(1);
    }
  }

///////////////CONFIGURACION GLOBAL/////////////////////
/************************************************/
/* Recupera los datos de configuracion          */
/* del archivo global                           */
/************************************************/
boolean inicializaConfiguracion(boolean debug)
  {
  String cad="";
  if (debug) Serial.println("Recupero configuracion de archivo...");

  //cargo el valores por defecto
  //Contadores
  multiplicadorAnchoIntervalo=5;
  anchoIntervalo=1200;
  anchoLoop=anchoIntervalo;  
  frecuenciaOTA=5;
  frecuenciaServidorWeb=1;
  frecuenciaOrdenes=2;
  frecuenciaMQTT=50;
  frecuenciaEnvioDatos=50;
  frecuenciaWifiWatchdog=100; 
  anchoLoop=anchoIntervalo; 
  
  ahorroEnergia=0; //ahorro de energia desactivado por defecto
  nivelActivo=LOW;  
  
  if(!leeFichero(GLOBAL_CONFIG_FILE, cad)) 
    {
    Serial.printf("No existe fichero de configuracion global\n");    
    return false;
    }
    
  parseaConfiguracionGlobal(cad);

  //Ajusto el ancho del intervalo segun el modo de ahorro de energia  
  if(ahorroEnergia==0) anchoLoop=anchoIntervalo;
  else anchoLoop=multiplicadorAnchoIntervalo*anchoIntervalo;

  return true;
  }

/*********************************************/
/* Parsea el json leido del fichero de       */
/* configuracio global                       */
/*********************************************/
boolean parseaConfiguracionGlobal(String contenido)
  {  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(contenido.c_str());
  //json.printTo(Serial);
  if (json.success()) 
    {
    Serial.println("parsed json");
//******************************Parte especifica del json a leer********************************
    if (json.containsKey("multiplicadorAnchoIntervalo")) multiplicadorAnchoIntervalo=json.get<uint16_t>("multiplicadorAnchoIntervalo"); 
    if (json.containsKey("anchoIntervalo")) anchoIntervalo=json.get<uint16_t>("anchoIntervalo");
    if (json.containsKey("frecuenciaOTA")) frecuenciaOTA=json.get<uint16_t>("frecuenciaOTA");

    if (json.containsKey("frecuenciaLogica")) frecuenciaLogica=json.get<uint16_t>("frecuenciaLogica");
    if (json.containsKey("frecuenciaServidorWeb")) frecuenciaServidorWeb=json.get<uint16_t>("frecuenciaServidorWeb");
    if (json.containsKey("frecuenciaOrdenes")) frecuenciaOrdenes=json.get<uint16_t>("frecuenciaOrdenes");
    if (json.containsKey("frecuenciaMQTT")) frecuenciaMQTT=json.get<uint16_t>("frecuenciaMQTT");
    if (json.containsKey("frecuencioEnviaDatos")) frecuenciaEnvioDatos=json.get<uint16_t>("frecuenciaEnvioDatos");
    if (json.containsKey("frecuenciaWifiWatchdog")) frecuenciaWifiWatchdog=json.get<uint16_t>("frecuenciaWifiWatchdog");

    if (json.containsKey("ahorroEnergia")) ahorroEnergia=json.get<uint16_t>("ahorroEnergia");
    if (json.containsKey("nombre")) nombre_dispositivo=json.get<String>("nombre");

    if((int)json["NivelActivo"]==0) nivelActivo=LOW;
    else nivelActivo=HIGH;
    
    Serial.printf("Configuracion leida:\nNombre: %s\nNivelActivo: %i\n", nombre_dispositivo.c_str(),nivelActivo);
    Serial.printf("\nContadores\nmultiplicadorAnchoIntervalo: %i\nanchoIntervalo: %i\nfrecuenciaOTA: %i\nfrecuenciaServidorWeb: %i\nfrecuenciaOrdenes: %i\nfrecuenciaMQTT: %i\nfrecuenciaEnvioDatos: %i\nfrecuenciaWifiWatchdog: %i\n",multiplicadorAnchoIntervalo, anchoIntervalo, frecuenciaOTA, frecuenciaServidorWeb, frecuenciaOrdenes, frecuenciaMQTT, frecuenciaEnvioDatos, frecuenciaWifiWatchdog);    
    return true;
//************************************************************************************************
    }
  return false;    
  }

/**********************************************************************/
/* Salva la configuracion general en formato json                     */
/**********************************************************************/  
String generaJsonConfiguracionNivelActivo(String configActual, int nivelAct)
  {
  boolean nuevo=true;
  String salida="";

  if(configActual=="") 
    {
    Serial.println("No existe el fichero. Se genera uno nuevo");
    return "{\"NivelActivo\": \"" + String(nivelAct) + "}";
    }
    
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(configActual.c_str());
  json.printTo(Serial);
  if (json.success()) 
    {
    Serial.println("parsed json");          

//******************************Parte especifica del json a leer********************************
    json["NivelActivo"]=nivelAct;  
//************************************************************************************************

    json.printTo(salida);//pinto el json que he creado
    Serial.printf("json creado:\n#%s#\n",salida.c_str());
    }//la de parsear el json

  return salida;  
  } 
