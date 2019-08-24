/*
 * Actuador para Termostato
 *
 * Actuador remoto
 * 
 * Reles de conexion
 * Servicio web levantado en puerto ZZZ
 */

//Defines generales
#define NOMBRE_FAMILIA "Actuador_rele"
#define VERSION "1.5.0 (ESP8266v2.5.1 OTA|json|MQTT|Cont. dinamicos)" //ESP8266v2.5.1
#define SEPARADOR '|'
#define SUBSEPARADOR '#'
#define KO               -1
#define OK                0
#define MAX_VUELTAS  UINT16_MAX// 32767 

#define PUERTO_WEBSERVER 80
#define MAX_RELES        2 //numero maximo de reles soportado

//Nombres de ficheros
#define FICHERO_CANDADO        "/Candado"
#define GLOBAL_CONFIG_FILE     "/Config.json"
#define GLOBAL_CONFIG_BAK_FILE "/Config.json.bak"
#define WIFI_CONFIG_FILE       "/WiFiConfig.json"
#define WIFI_CONFIG_BAK_FILE   "/WiFiConfig.json.bak"
#define MQTT_CONFIG_FILE       "/MQTTConfig.json"
#define MQTT_CONFIG_BAK_FILE   "/MQTTConfig.json.bak"

//Definiciopn de pines
#define RELES_PIN      5 //es el Dx Pin del primer rele, los demas consecutivos
#define LEDS_PIN       1 //Pin del led de desborde de tiempo

// Una vuela de loop son ANCHO_INTERVALO segundos 
#define MULTIPLICADOR_ANCHO_INTERVALO 5 //Multiplica el ancho del intervalo para mejorar el ahorro de energia
#define ANCHO_INTERVALO          100 //Ancho en milisegundos de la rodaja de tiempo
#define FRECUENCIA_OTA             5 //cada cuantas vueltas de loop atiende las acciones
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
boolean candado=false; //Candado de configuracion. true implica que la ultima configuracion fue mal

//Contadores
uint16_t multiplicadorAnchoIntervalo=5;
uint16_t anchoIntervalo=100;
uint16_t frecuenciaOTA=5;
uint16_t frecuenciaServidorWeb=1;
uint16_t frecuenciaOrdenes=2;
uint16_t frecuenciaMQTT=50;
uint16_t frecuenciaEnvioDatos=100;
uint16_t frecuenciaWifiWatchdog=100;

void setup()
  {
  Serial.begin(115200);
  Serial.printf("\n\n\n");
  Serial.printf("*************** %s ***************\n",NOMBRE_FAMILIA);
  Serial.printf("*************** %s ***************\n",VERSION);
  Serial.println("***************************************************************");
  Serial.println("*                                                             *");
  Serial.println("*             Inicio del setup del modulo                     *");
  Serial.println("*                                                             *");    
  Serial.println("***************************************************************");

  Serial.printf("\n\nInit Ficheros ---------------------------------------------------------------------\n");
  //Ficheros - Lo primero para poder leer los demas ficheros de configuracion
  inicializaFicheros(debugGlobal);

  //Compruebo si existe candado, si existe la ultima configuracion fue mal
  if(existeFichero(FICHERO_CANDADO)) 
    {
    Serial.printf("Candado puesto. Configuracion por defecto");
    candado=true; 
    debugGlobal=1;
    }
  else
    {
    candado=false;
    //Genera candado
    if(salvaFichero(FICHERO_CANDADO,"","JSD")) Serial.println("Candado creado");
    else Serial.println("ERROR - No se pudo crear el candado");
    }
    
  //Configuracion general
  Serial.println("Init Config -----------------------------------------------------------------------");
  if(!inicializaConfiguracion(debugGlobal)) Serial.printf("Error al recuperar configuracion general.\nIniciando con valores por defecto.\n");

  //Wifi
  Serial.println("Init WiFi -----------------------------------------------------------------------");
  if (inicializaWifi(true))//debugGlobal)) No tien esentido debugGlobal, no hay manera de activarlo
    {
    /*----------------Inicializaciones que necesitan red-------------*/
    //OTA
    Serial.println("Init OTA -----------------------------------------------------------------------");
    inicializaOTA(debugGlobal);
    //MQTT
    Serial.println("Init MQTT -----------------------------------------------------------------------");
    inicializaMQTT();
    //WebServer
    Serial.println("Init Web --------------------------------------------------------------------------");
    inicializaWebServer();
    }
  else Serial.println("No se pudo conectar al WiFi");

  //Reles
  Serial.println("Init reles ------------------------------------------------------------------------");
  inicializaReles();

  //Ordenes serie
  Serial.println("Init Ordenes ----------------------------------------------------------------------");  
  inicializaOrden();//Inicializa los buffers de recepcion de ordenes desde PC

  //Si ha llegado hasta aqui, todo ha ido bien y borro el candado
  if(borraFichero(FICHERO_CANDADO))Serial.println("Candado borrado");
  else Serial.println("ERROR - No se pudo borrar el candado");
  
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
  if ((vuelta % frecuenciaOTA)==0) atiendeOTA();//ArduinoOTA.handle(); //Gestion de actualizacion OTA
  //Prioridad 2: Funciones de control.
  //if ((vuelta % frecuenciaLogica)==0) actuaReles(); DESTAPAR SI EL ACTUADOR ES AUTONOMO, ESTE ESTA CONTROLADO POR EL CONTROLADOR VBIA MQTT
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
  while(millis()<EntradaBucle+anchoLoop)//while(millis()<EntradaBucle+ANCHO_INTERVALO)
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
  frecuenciaOTA=5;
  frecuenciaServidorWeb=1;
  frecuenciaOrdenes=2;
  frecuenciaMQTT=50;
  frecuenciaEnvioDatos=50;
  frecuenciaWifiWatchdog=100; 
  anchoLoop=anchoIntervalo; 
  
  ahorroEnergia=0; //ahorro de energia desactivado por defecto
  nivelActivo=LOW;  
  
  if(leeFichero(GLOBAL_CONFIG_FILE, cad)) 
    {
    if (parseaConfiguracionGlobal(cad)) 
      {
      //Ajusto el ancho del intervalo segun el modo de ahorro de energia  
      if(ahorroEnergia==0) anchoLoop=anchoIntervalo;
      else anchoLoop=multiplicadorAnchoIntervalo*anchoIntervalo;

      return true;
      }
    }  
    
  return false;
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
    multiplicadorAnchoIntervalo=json.get<uint16_t>("multiplicadorAnchoIntervalo");
    anchoIntervalo=json.get<uint16_t>("anchoIntervalo");
    frecuenciaOTA=json.get<uint16_t>("frecuenciaOTA");
    frecuenciaServidorWeb=json.get<uint16_t>("frecuenciaServidorWeb");
    frecuenciaOrdenes=json.get<uint16_t>("frecuenciaOrdenes");
    frecuenciaMQTT=json.get<uint16_t>("frecuenciaMQTT");
    frecuenciaEnvioDatos=json.get<uint16_t>("frecuenciaEnvioDatos");
    frecuenciaWifiWatchdog=json.get<uint16_t>("frecuenciaWifiWatchdog");  
    
    ahorroEnergia=json.get<uint16_t>("ahorroEnergia");

    if((int)json["NivelActivo"]==0) nivelActivo=LOW;
    else nivelActivo=HIGH;
    
    Serial.printf("Configuracion leida:\nNivelActivo: %i\n", nivelActivo);
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
