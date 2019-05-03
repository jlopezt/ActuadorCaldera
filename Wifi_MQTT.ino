/**********************************************/
/*                                            */
/*  Gestion de la conexion MQTT               */
/*  Incluye la conexion al bus y la           */
/*  definicion del callback de suscripcion    */
/*                                            */
/* Librria de sooprte del protocolo MQTT      */
/* para arduino/ESP8266/ESP32                 */
/*                                            */
/* https://pubsubclient.knolleary.net/api.html*/
/**********************************************/
//Includes MQTT
//#define MQTT_KEEPALIVE 60
#include <PubSubClient.h>
#define CLEAN_SESSION TRUE

//definicion de los comodines del MQTT
#define WILDCARD_ALL      "#"
#define WILDCARD_ONELEVEL "+"

//definicion de constantes para WILL
#define WILL_TOPIC  "will"
#define WILL_QOS    1
#define WILL_RETAIN false
#define WILL_MSG    String("¡"+ID_MQTT+" caido!").c_str()

//Definicion de variables globales
IPAddress IPBroker; //IP del bus MQTT
uint16_t puertoBroker; //Puerto del bus MQTT
String usuarioMQTT; //usuario par ala conxion al broker
String passwordMQTT; //password parala conexion al broker
String topicRoot; //raiz del topic a publicar. Util para separar mensajes de produccion y prepropduccion
String ID_MQTT; //ID del modulo en su conexion al broker
int8_t publicarEstado; //Flag para determinar si se envia el json con los valores de las salidas

WiFiClient espClient;
PubSubClient clienteMQTT(espClient);

/************************************************/
/* Inicializa valiables y estado del bus MQTT   */
/************************************************/
void inicializaMQTT(void)
  {
  //recupero datos del fichero de configuracion
  if (!recuperaDatosMQTT(false)) Serial.printf("error al recuperar config MQTT.\nConfiguracion por defecto.\n");
  
  //Si va bien inicializo con los valores correstoc, si no con valores por defecto
  //confituro el servidor y el puerto
  clienteMQTT.setServer(IPBroker, puertoBroker);
  //configuro el callback, si lo hay
  clienteMQTT.setCallback(callbackMQTT);

  if (conectaMQTT()) Serial.println("connectado al broker");  
  else Serial.printf("error al conectar al broker con estado %i\n",clienteMQTT.state());
  }

/************************************************/
/* Recupera los datos de configuracion          */
/* del archivo de MQTT                          */
/************************************************/
boolean recuperaDatosMQTT(boolean debug)
  {
  String cad="";
  if (debug) Serial.println("Recupero configuracion de archivo...");

  //cargo el valores por defecto
  IPBroker.fromString("0.0.0.0");
  puertoBroker=0;
  ID_MQTT="";
  usuarioMQTT="";
  passwordMQTT="";
  topicRoot="";
  publicarEstado=1;//por defecto publico

  if(leeFichero(MQTT_CONFIG_FILE, cad)) return parseaConfiguracionMQTT(cad);
  else
    {
    //Confgiguracion por defecto
    Serial.printf("No existe fichero de configuracion MQTT\n");
    cad="{\"IPBroker\": \"10.68.1.100\", \"puerto\": 1883, \"ID_MQTT\": \"actuador\", \"usuarioMQTT\": \"usuario\", \"passwordMQTT\": \"password\", \"topicRoot\": \"casa\", \"publicarEstado\": 1}";
    salvaFichero(MQTT_CONFIG_FILE, MQTT_CONFIG_BAK_FILE, cad);
    Serial.printf("Fichero de configuracion MQTT creado por defecto\n");
    parseaConfiguracionWifi(cad);
    }
    
  return false;    
  }  

/*********************************************/
/* Parsea el json leido del fichero de       */
/* configuracio MQTT                         */
/*********************************************/
boolean parseaConfiguracionMQTT(String contenido)
  {  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(contenido.c_str());
  //json.printTo(Serial);
  if (json.success()) 
    {
    Serial.println("parsed json");
//******************************Parte especifica del json a leer********************************
    IPBroker.fromString((const char *)json["IPBroker"]);
    puertoBroker = atoi(json["puerto"]); 
    ID_MQTT=((const char *)json["ID_MQTT"]);
    usuarioMQTT=((const char *)json["usuarioMQTT"]);
    passwordMQTT=((const char *)json["passwordMQTT"]);
    topicRoot=((const char *)json["topicRoot"]);
    publicarEstado==atoi(json["publicarEstado"]);     
    Serial.printf("Configuracion leida:\nIP broker: %s\nIP Puerto del broker: %i\nID_MQTT: %s\nUsuario: %s\nPassword: %s\nTopic root: %s\nPublicar estado: %i\n",IPBroker.toString().c_str(),puertoBroker,ID_MQTT.c_str(),usuarioMQTT.c_str(),passwordMQTT.c_str(),topicRoot.c_str(),publicarEstado);
//************************************************************************************************
    return true;
    }
  return false;
  }


/***********************************************Funciones de gestion de mensajes MQTT**************************************************************/
/***************************************************/
/* Funcion que recibe el mensaje cuando se publica */
/* en el bus un topic al que esta subscrito        */
/***************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length)
  {
  char mensaje[length];  

  if(debugGlobal) Serial.printf("Entrando en callback: \n Topic: %s\nPayload %s\nLongitud %i\n", topic, payload, length);
  
  /**********compruebo el topic*****************/
  //Sirve para solo atender a los topic de medidas. Si se suscribe a otro habira que gestionarlo aqui
  //si es del tipo "casa/*"
  //copio el topic a la cadena cad
  String cad=String(topic);
  if(cad.substring(0,String(topicRoot + "/" + ID_MQTT).length())!=String(topicRoot + "/" + ID_MQTT)) //no deberia, solo se suscribe a los suyos
    {
    Serial.printf("topic no reconocido: \ntopic: %s\nroot: %s\n", cad.c_str(),cad.substring(0,cad.indexOf("/")).c_str());  
    return;
    }
  else//topic correcto
    {  
    //copio el payload en la cadena mensaje
    for(int8_t i=0;i<length;i++) mensaje[i]=payload[i];
    mensaje[length]=0;//acabo la cadena
  
    /**********************Leo el JSON***********************/
    const size_t bufferSize = JSON_OBJECT_SIZE(3) + 50;
    DynamicJsonBuffer jsonBuffer(bufferSize);     
    JsonObject& root = jsonBuffer.parseObject(mensaje);
    if (root.success()) 
      {  
      //Registro el satelite y copio sobre la habitacion correspondiente del array los datos recibidos
      int id=atoi(root["id"]);
      
      int estado;
      if(root["estado"]=="on") estado=nivelActivo;
      else estado=!nivelActivo;
    /**********************Fin JSON***********************/    
      if(conmutaRele(id, estado, debugGlobal)==OK) Serial.printf("Rele conmutado\n");;
      }
    }
  }

/********************************************/
/* Funcion que gestiona la conexion al bus  */
/* MQTT del broker                          */
/********************************************/
boolean conectaMQTT(void)  
  {
  int8_t intentos=0;
  
  while (!clienteMQTT.connected()) 
    {    
    if(debugGlobal) Serial.println("No conectado, intentando conectar.");
  
    // Attempt to connect
    //boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage, boolean cleanSession);    
    if (clienteMQTT.connect(ID_MQTT.c_str(), usuarioMQTT.c_str(), passwordMQTT.c_str(), (topicRoot+"/"+String(WILL_TOPIC)).c_str(), WILL_QOS, WILL_RETAIN, String(WILL_MSG).c_str(), CLEAN_SESSION))    
      {
      if(debugGlobal) Serial.println("conectado");

      //Inicio la subscripcion al topic de las medidas boolean subscribe(const char* topic);
      String topic=topicRoot + "/" + ID_MQTT; //uso el + como comodin para culaquier habitacion
      if (clienteMQTT.subscribe(topic.c_str())) Serial.printf("Subscrito al topic %s\n", topic.c_str());
      else Serial.printf("Error al subscribirse al topic %s\n", topic.c_str());       

      return(true);
      }

    if(intentos++>3) return (false);
    
    if(debugGlobal) Serial.printf("Error al conectar al broker. Estado: %s\n",stateTexto().c_str());
    delay(500);      
    }
  }

/********************************************/
/* Funcion que envia un mensaje al bus      */
/* MQTT del broker                          */
/********************************************/
boolean enviarMQTT_old(String topic, String payload)
  {
  //si no esta conectado, conecto
  if (!clienteMQTT.connected()) conectaMQTT();

  //si y esta conectado envio, sino salgo con error
  if (clienteMQTT.connected()) 
    {
    if(!topic.startsWith("/")) topic = "/" + topic;  
    topic=topicRoot + "/" + ID_MQTT + topic;  
    
    Serial.printf("Enviando:\ntopic:  %s | payload: %s\n",topic.c_str(),payload.c_str());
    return clienteMQTT.publish(topic.c_str(), payload.c_str());
    }
  else return (false);
  }

/********************************************/
/* Funcion que envia un mensaje al bus      */
/* MQTT del broker                          */
/* Eliminado limite del buffer de envio     */
/********************************************/
boolean enviarMQTT(String topic, String payload)
  {
  //si no esta conectado, conecto
  if (!clienteMQTT.connected()) conectaMQTT();

  //si y esta conectado envio, sino salgo con error
  if (clienteMQTT.connected()) 
    {
    if(!topic.startsWith("/")) topic = "/" + topic;  
    topic=topicRoot + "/" + ID_MQTT + topic;
      
    //Serial.printf("Enviando:\ntopic:  %s\n payload: %s\n",topic.c_str(),payload.c_str());
  
    if(clienteMQTT.beginPublish(topic.c_str(), payload.length(), false))//boolean beginPublish(const char* topic, unsigned int plength, boolean retained)
      {
      for(uint8_t i=0;i<payload.length();i++) clienteMQTT.write((uint8_t)payload.charAt(i));//virtual size_t write(uint8_t);
      return(clienteMQTT.endPublish()); //int endPublish();
      }
    }
  else return (false);
  }
  
/********************************************/
/* Funcion que revisa el estado del bus y   */
/* si se ha recibido un mensaje             */
/********************************************/
void atiendeMQTT(boolean debug)
  {
  /*  
  String topic="/estado";
  String payload=generaJsonEstado();

  if(enviarMQTT(topic, payload)) if(debug)Serial.println("Enviado json al broker con exito.");
  else if(debug)Serial.println("¡¡Error al enviar json al broker!!");
  */
  clienteMQTT.loop();
  }  

/********************************************/
/*                                          */
/* Funcion que envia datos de estado del    */
/* controlador al broker                    */
/*                                          */
/********************************************/
void enviaDatos(boolean debug)
  {
  String topic="/estado";  
  String payload;

  if(publicarEstado==1)
    {
    payload=generaJsonEstado();//genero el json de las entradas
    //Lo envio al bus    
    if(enviarMQTT(topic, payload)) if(debug)Serial.println("Enviado json al broker con exito.");
    else if(debug)Serial.println("¡¡Error al enviar json al broker!!");
    }
  else if(debugGlobal) Serial.printf("No publico estado. Publicar estado es %i\n",publicarEstado);  
  }
  
/******************************* UTILIDADES *************************************/
/********************************************/
/* Funcion que devuleve el estado           */
/* de conexion MQTT al bus                  */
/********************************************/
String stateTexto(void)  
  {
  int r = clienteMQTT.state();

  String cad=String(r) + " : ";
  
  switch (r)
    {
    case -4:
      cad += "MQTT_CONNECTION_TIMEOUT";
      break;
    case -3:
      cad += "MQTT_CONNECTION_LOST";
      break;
    case -2:
      cad += "MQTT_CONNECT_FAILED";
      break;
    case -1:
      cad += "MQTT_DISCONNECTED";
      break;
    case  0:
      cad += "MQTT_CONNECTED";
      break;
    case  1:
      cad += "MQTT_CONNECT_BAD_PROTOCOL";
      break;
    case  2:
      cad += "MQTT_CONNECT_BAD_CLIENT_ID";
      break;
    case  3:
      cad += "MQTT_CONNECT_UNAVAILABLE";
      break;
    case  4:
      cad += "MQTT_CONNECT_BAD_CREDENTIALS";
      break;     
    case  5:
      cad += "MQTT_CONNECT_UNAUTHORIZED";
      break;
    default:
      cad += "????";
    }
      
  return (cad);
  }
