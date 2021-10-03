/*****************************************/
/*                                       */
/*  Control de reles para el termostato  */
/*                                       */
/*****************************************/
#define NO_ENVIAR_MENSAJES          ""

/*******************Definicion de tipos y variables comunes*********************/
typedef struct{
  //int8_t id;
  String nombre;
  int8_t estado;
  int8_t pin;             // Pin al que esta conectado el rele
  int8_t pinLed;          // Pin al que esta conectado el led
  String topicMensajes;   // Topic al que mandar los mensajes de cambio de estado. El valor NO_ENVIAR_MENSAJES significa que no se manda
  int8_t inicio;          // modo inicial del rele "on"-->1/"off"-->0
  String nombreEstados[2];//Son salidas binarias, solo puede haber 2 mensajes. El 0 nombre del estado en valor 0 y el 1 nombre del estado en valor 1
  String mensajes[2];     //Son salidas binarias, solo puede haber 2 mensajes. El 0 cuando pasa a 0 y el 1 cuando pasa a 1
  }rele_t; 

rele_t reles[MAX_RELES];
String nombre_reles[MAX_RELES];

//tabla de pines GPIOs
int8_t pinGPIOS[9]={16,5,4,0,2,14,12,13,15}; //tiene 9 puertos digitales, el indice es el puerto Dx y el valor el GPIO

//Seguridad
uint32_t contadorSeguridad=0;
uint32_t inicioSeguridad=0; 
/*******************Fin definicion de tipos y variables comunes*********************/

/*********************************************/
/* Inicializa los valores de los registros de*/
/* las salidas y recupera la configuracion   */
/*********************************************/
void inicializaReles()
  {  
  //preconfiguracion de fabrica de los reles
  //0 Calefaccion
  nombre_reles[0]="Calefaccion";
  //1 Otros
  nombre_reles[1]="Otros";  

  for(int8_t i=0;i<MAX_RELES;i++)
    {
    //inicializo la parte logica
    //reles[i].id=i;
    reles[i].nombre=nombre_reles[i];
    reles[i].estado=0;  
    reles[i].pin=-1;
    reles[i].pinLed=-1;
    reles[i].topicMensajes=NO_ENVIAR_MENSAJES;
    reles[i].inicio=0;
    reles[i].nombreEstados[0]="0";
    reles[i].nombreEstados[1]="1";
    reles[i].mensajes[0]="paso a cero";
    reles[i].mensajes[1]="paso a uno";
    }

  contadorSeguridad=0; 
  
  //leo la configuracion del fichero
  if(!recuperaDatosReles(debugGlobal)) Serial.println("Configuracion de los reles por defecto");
  else
    {  
    //Salidas
    Serial.printf("Reles:\nContador de seguridad: %i\n",contadorSeguridad);    
    for(int8_t i=0;i<MAX_RELES;i++)
      {    
      if(reles[i].pin!=-1)
        {
        pinMode(pinGPIOS[reles[i].pin], OUTPUT); //es salida
        pinMode(pinGPIOS[reles[i].pinLed], OUTPUT); //es salida
  
        //parte logica
        reles[i].estado=reles[i].inicio;  
        //parte fisica
        if(reles[i].inicio==1) 
          {
          digitalWrite(pinGPIOS[reles[i].pin], nivelActivo);  //lo inicializo a apagado
          digitalWrite(pinGPIOS[reles[i].pinLed], HIGH);  //lo inicializo encendido
          }
        else 
          {
          digitalWrite(pinGPIOS[reles[i].pin], !nivelActivo);  //lo inicializo a apagado 
          digitalWrite(pinGPIOS[reles[i].pinLed], LOW);  //lo inicializo a apagado
          }
        
        Serial.printf("Nombre rele[%i]=%s | pin rele: %i | pin led: %i | inicio: %i |estado: %i \ topic mensajes: %s\n",i,reles[i].nombre.c_str(),reles[i].pin,reles[i].pinLed,reles[i].inicio,reles[i].estado,reles[i].topicMensajes.c_str());
        }
      }
    }  
  }

/*********************************************/
/* Lee el fichero de configuracion de las    */
/* salidas o genera conf por defecto         */
/*********************************************/
boolean recuperaDatosReles(int debug)
  {
  String cad="";

  if (debug) Serial.println("Recupero configuracion de archivo...");
  
  if(!leeFichero(RELES_CONFIG_FILE, cad)) 
    {
    //Confgiguracion por defecto
    Serial.printf("No existe fichero de configuracion de Reles\n");    
    return false;
    }      
    
  return parseaConfiguracionReles(cad);
  }

/*********************************************/
/* Parsea el json leido del fichero de       */
/* configuracio de los reles                 */
/*********************************************/
boolean parseaConfiguracionReles(String contenido)
  { 
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(contenido.c_str());
  
  json.printTo(Serial);
  if (!json.success()) return false;
        
  Serial.println("\nparsed json");
//******************************Parte especifica del json a leer********************************
  JsonArray& Reles = json["Reles"];

  if(json["contadorSeguridad"].is<unsigned int>()) contadorSeguridad=json.get<unsigned int>("contadorSeguridad") * 1000;  
  else contadorSeguridad=0;

  int8_t max;
  max=(Reles.size()<MAX_RELES?Reles.size():MAX_RELES);
  for(int8_t i=0;i<max;i++)
    { 
    JsonObject& rele = json["Reles"][i];
    reles[i].nombre=rele.get<String>("nombre");
    reles[i].pin=rele.get<int8_t>("Dx");
    reles[i].pinLed=rele.get<int8_t>("DxLed");
    reles[i].topicMensajes=rele.get<String>("topicMensajes");

    //Si de inicio debe estar activado o desactivado
    if(String((const char *)Reles[i]["inicio"])=="on") reles[i].inicio=1;
    else reles[i].inicio=0;   

    if(rele.containsKey("Estados"))
      {
      int8_t est_max=rele["Estados"].size();//maximo de mensajes en el JSON
      if (est_max>2) est_max=2;                     //Si hay mas de 2, solo leo 2
      for(int8_t e=0;e<est_max;e++)  
        {
        if (rele["Estados"][e]["valor"]==e) reles[i].nombreEstados[e]=String((const char *)rele["Estados"][e]["texto"]);
        }
      }

    if(rele.containsKey("Mensajes"))
      {
      int8_t men_max=rele["Mensajes"].size();//maximo de mensajes en el JSON
      if (men_max>2) men_max=2;                     //Si hay mas de 2, solo leo 2
      for(int8_t m=0;m<men_max;m++)  
        {
        if (rele["Mensajes"][m]["valor"]==m) reles[i].mensajes[m]=String((const char *)rele["Mensajes"][m]["texto"]);
        }
      }
    }

  Serial.printf("Reles:\nContador de seguridad: %i\n",contadorSeguridad); 
  for(int8_t i=0;i<MAX_RELES;i++) 
    {
    Serial.printf("%01i: %s | pin: %i | inico: %i | topic mensajes: %s\n",i,reles[i].nombre.c_str(),reles[i].pin,reles[i].inicio,reles[i].topicMensajes.c_str()); 
    Serial.printf("Estados:\n");
    for(int8_t e=0;e<2;e++) 
      {
      Serial.printf("Estado[%02i]: %s\n",e,reles[i].nombreEstados[e].c_str());     
      }    
    Serial.printf("Mensajes:\n");
    for(int8_t m=0;m<2;m++) 
      {
      Serial.printf("Mensaje[%02i]: %s\n",m,reles[i].mensajes[m].c_str());     
      }    
    }    
//************************************************************************************************
  return true; 
  }
/**********************************************************Fin configuracion******************************************************************/  

/*************************************************/
/*Logica de los reles:                           */
/*En modo actuador, solo sirve para verificar que*/
/*ha llegado un aorden en los ultimos            */
/*contadorSeguridad milisegundos                 */
/*si no, activa la seguridad y desconecta todo   */
/*************************************************/
void actuaReles(int debug)
  { 
  uint32_t intervaloTrascurrido=0;

  //calculo el tiempo desde que se inicio el intervalo de seguridada
  if(millis()>inicioSeguridad) intervaloTrascurrido=millis()-inicioSeguridad; //normal
  else intervaloTrascurrido=(UINT32_MAX-inicioSeguridad)+millis(); //si ha desbordado el contador de millis
  if(debug) Serial.printf("intervaloTrascurrido: %li | inicioSeguridad: %li | millis: %li\n",intervaloTrascurrido,inicioSeguridad,millis());

  //si se ha sobrepasado el tiempo de guarda, desconecto
  if(intervaloTrascurrido>contadorSeguridad) 
    {
    Serial.printf("¡¡ATENCION!! Se desconecta por tiempo de proteccion\nIntervalo transcurrido: %i \| tiempo de guarda: %i\n",intervaloTrascurrido,contadorSeguridad);

    //llevo todos al mismo estado que cuando se apaga el modulo, debe ser seguro
    for(int8_t id=0;id<MAX_RELES;id++) conmutaRele(id,!nivelActivo,false);
    }
  }

/*************************************************/
/*Devuelve el estado 0|1 del rele indicado en id */
/*************************************************/
int8_t estadoRele(int8_t id)
  {
  if(id <0 || id>=MAX_RELES) return KO; //Rele fuera de rango
  //Leo directamente los registros que guardan el estado

  //return reles[id].estado;
  if(digitalRead(pinGPIOS[reles[id].pin])==nivelActivo) return 1; 
  else return OK;
 }

/********************************************************/
/*                                                      */
/*  Devuelve el nombre del rele con el id especificado  */
/*                                                      */
/********************************************************/
String nombreRele(int8_t id)
  { 
  if(id <0 || id>=MAX_RELES) return "ERROR"; //Rele fuera de rango    
  return reles[id].nombre;
  } 

/********************************************************/
/*                                                      */
/*  Devuelve el mensaje asociado al estado del          */
/*  rele con el id especificado                         */
/*                                                      */
/********************************************************/
String mensajeRele(int8_t id)
  { 
  if(id <0 || id>=MAX_RELES) return "ERROR"; //Rele fuera de rango    
  return reles[id].mensajes[reles[id].estado];
  } 

/********************************************************/
/*                                                      */
/*  Devuelve el nombre del estado actual del            */
/*  rele con el id especificado                         */
/*                                                      */
/********************************************************/
String nombreEstadoRele(int8_t id)
  { 
  if(id <0 || id>=MAX_RELES) return "ERROR"; //Rele fuera de rango    
  return reles[id].nombreEstados[reles[id].estado];
  } 

/*************************************************/
/*conmuta el rele indicado en id                 */
/*devuelve 1 si ok, -1 si ko                     */
/*************************************************/
int8_t conmutaRele(int8_t id, boolean estado_final, int debug)
  {
  //registro el momento del ultimo cambio en un rele
  inicioSeguridad=millis();
    
  if(id <0 || id>=MAX_RELES) return KO; //Rele fuera de rango
  if(reles[id].pin==-1) return KO; //El rele no esta configurado

  //parte fisica
  digitalWrite(pinGPIOS[reles[id].pin], estado_final); //controlo el rele
  //controlo el led asociado
  if (nivelActivo) digitalWrite(pinGPIOS[reles[id].pinLed], estado_final); 
  else digitalWrite(pinGPIOS[reles[id].pinLed], !estado_final); 

  if(debug)
    {
    Serial.printf("id: %i; GPIO: %i; estado: ",(int)id,(int)pinGPIOS[reles[id].pin]);
    Serial.println(digitalRead(pinGPIOS[reles[id].pinLed]));
    }
    
  //parte logica
  int8_t valor_inicial=reles[id].estado;    

  if(estado_final==nivelActivo) reles[id].estado=1;
  else reles[id].estado=0;
  
  if(valor_inicial!=reles[id].estado) enviaMensajeSalida(id,reles[id].estado);

  return OK;
  }

/*************************************************/
/*                                               */
/* Envia un mensaje MQTT para que se publique un */
/* audio en un GHN                               */
/*                                               */
/*************************************************/
void enviaMensajeSalida(int8_t id_salida, int8_t estado)
  {
  String mensaje="";
  //Serial.printf("Envio de mensaje para la entrada con id %i y por cambiar a estado %i.\ntopic mensajes: %s\n",id_salida,estado,reles[id_salida].topicMensajes.c_str());
  
  if(reles[id_salida].topicMensajes==String(NO_ENVIAR_MENSAJES)) return; //no se envia mensaje

  mensaje="{\"origen\": \"" + reles[id_salida].nombre + "\",\"mensaje\":\"" + reles[id_salida].mensajes[estado] + "\"}";
  //Serial.printf("Mensaje: %s\n\n",reles[id_salida].mensajes[estado].c_str());
  //Serial.printf("A enviar topic: *%s*\nmensaje: *%s*\n", reles[id_salida].topicMensajes.c_str(),mensaje.c_str());
  enviarMQTT(reles[id_salida].topicMensajes, mensaje);
  }

/**********************************************/
/* Genera el json con el estado de los reles  */
/**********************************************/
String generaJsonEstado(void)
  {
  String cad="";
  
  //genero el json con el estado de los reles--> {"rele_0": 0,"rele_1": 1}
  cad  = "{\n\t";
  for(int8_t i=0;i<MAX_RELES;i++)
    {
    if(i>0) cad += ",\n\t"; //si no es la primera
    cad += nombreRele(i);
    cad += ": ";
    cad += estadoRele(i);  
    }
  cad += "\n}";  

  return cad;
  } 

/**********************************************/
/* Genera el json con el estado para la web   */
/**********************************************/
String generaJson(void)
  {
  String cad="";

  //const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(8);
  const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(5);

  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& root = jsonBuffer.createObject();
  
  JsonArray& Salidas = root.createNestedArray("salidas");
  for(int8_t id=0;id<MAX_RELES;id++){

    JsonObject& Salidas_0 = Salidas.createNestedObject();
    Salidas_0["nombre"] = nombreRele(id);
    Salidas_0["estado"] = estadoRele(id);
    Salidas_0["nombreEstado"] = nombreEstadoRele(id);
    Salidas_0["mensaje"] = mensajeRele(id);
  }

  root["estadoBloqueo"] = bloqueoMQTT();
  
  root.printTo(cad);
  return cad;  
  } 
