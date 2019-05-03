#include <PubSubClient.h>
  
IPAddress IPBroker; //IP del bus MQTT
uint16_t puertoBroker; //Puerto del bus MQTT
String usuarioMQTT; //usuario par ala conxion al broker
String passwordMQTT; //password parala conexion al broker
String topicRoot; //raiz del topic a publicar. Util para separar mensajes de produccion y prepropduccion
String ID_MQTT="actuador"; //ID del modulo en su conexion al broker
String topicKeepAlive;

WiFiClient espClient;
PubSubClient clienteMQTT(espClient);

void inicializaMQTT(void)
  {
  //recupero datos del fichero de configuracion
  recuperaDatosMQTT(false);

  //confituro el servidor y el puerto
  clienteMQTT.setServer(IPBroker, puertoBroker);
  //configuro el callback, si lo hay
  clienteMQTT.setCallback(callbackMQTT);

  if (conectaMQTT()) Serial.println("connectado al broker");  
  else Serial.printf("error al conectar al broker con estado %i\n",clienteMQTT.state());

  //Variables adicionales
  topicKeepAlive=ID_MQTT + "/keepalive";
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
  usuarioMQTT="";
  passwordMQTT="";
  topicRoot="";
    
  if(leeFichero(MQTT_CONFIG_FILE, cad)) parseaConfiguracionMQTT(cad);

  return true;
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
    usuarioMQTT=((const char *)json["usuarioMQTT"]);
    passwordMQTT=((const char *)json["passwordMQTT"]);
    topicRoot=((const char *)json["topicRoot"]);
    Serial.printf("Configuracion leida:\nIP broker: %s\nIP Puerto del broker: %i\nUsuario: %s\nPassword: %s\nTopic root: %s\n",IPBroker.toString().c_str(),puertoBroker,usuarioMQTT.c_str(),passwordMQTT.c_str(),topicRoot.c_str());
//************************************************************************************************
    }
  }


/***********************************************Funciones de gestion de mensajes MQTT**************************************************************/
/***************************************************/
/* Funcion que recibe el mensaje cuando se publica */
/* en el bus un topic al que esta subscrito        */
/***************************************************/
void callbackMQTT(char* topic, byte* payload, unsigned int length)
  {
  char mensaje[length];  

  //Serial.printf("Entrando en callback: \n Topic: %s\nPayload %s\nLongitud %i\n", topic, payload, length);
  
  /**********compruebo el topic*****************/
  //Sirve para solo atender a los topic de medidas. Si se suscribe a otro habira que gestionarlo aqui
  //si es del tipo "casa/*"
  //copio el topic a la cadena cad
  String cad=String(topic);
  if(cad.substring(0,String(topicRoot + "/actuador").length())!=String(topicRoot + "/actuador")) //no deberia, solo se suscribe a los suyos
    {
    Serial.printf("topic no reconocido: \ntopic: %s\nroot: %s\n", cad.c_str(),cad.substring(0,cad.indexOf("/")).c_str());  
    return;
    }
  else//topic correcto
    {  
    //copio el payload en la cadena mensaje
    for(int8_t i=0;i<length;i++) mensaje[i]=payload[i];
  
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
    //Serial.println("Attempting MQTT connection...");
  
    // Attempt to connect
    if (clienteMQTT.connect(ID_MQTT.c_str()))
      {
      //Serial.println("connected");
      //Inicio la subscripcion al topic de las medidas boolean subscribe(const char* topic);
      String topic=topicRoot+ "/actuador"; //uso el + como comodin para culaquier habitacion
      if (clienteMQTT.subscribe(topic.c_str())) Serial.printf("Subscrito al topic %s\n", topic.c_str());
      else Serial.printf("Error al subscribirse al topic %s\n", topic.c_str());       
      return(true);
      }

    if(intentos++>3) return (false);
    
    Serial.println("Conectando al broker...");
    delay(500);      
    }
  }

/********************************************/
/* Funcion que envia un mensaje al bus      */
/* MQTT del broker                          */
/********************************************/
boolean enviarMQTT(String topic, String payload)
  {
  //si no esta conectado, conecto
  if (!clienteMQTT.connected()) conectaMQTT();

  //si y esta conectado envio, sino salgo con error
  if (clienteMQTT.connected()) 
    {
    //Serial.printf("Enviando:\ntopic:  %s | payload: %s\n",topic.c_str(),payload.c_str());
    return clienteMQTT.publish((topicRoot+"/"+topic).c_str(), payload.c_str());      
    }
  else return (false);
  }

/********************************************/
/* Funcion que revisa el estado del bus y   */
/* si se ha recibido un mensaje             */
/********************************************/
void atiendeMQTT(boolean debug)
  {
  String payload=String(millis());

  if(enviarMQTT(topicKeepAlive, payload)) if(debug)Serial.println("Enviado json al broker con exito.");
  else if(debug)Serial.println("¡¡Error al enviar json al broker!!");

  clienteMQTT.loop();
  }  
