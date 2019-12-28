/*****************************************/
/*                                       */
/*  Control de reles para el termostato  */
/*                                       */
/*****************************************/
#define TOPIC_MENSAJES "mensajes"

typedef struct{
  //int8_t id;
  String nombre;
  int8_t estado;
  int8_t pin;             // Pin al que esta conectado el rele
  int8_t pinLed;          // Pin al que esta conectado el led
  int8_t inicio;          // modo inicial del rele "on"-->1/"off"-->0
  String nombreEstados[2];//Son salidas binarias, solo puede haber 2 mensajes. El 0 nombre del estado en valor 0 y el 1 nombre del estado en valor 1
  String mensajes[2];     //Son salidas binarias, solo puede haber 2 mensajes. El 0 cuando pasa a 0 y el 1 cuando pasa a 1
  }rele_t; 

rele_t reles[MAX_RELES];
String nombre_reles[MAX_RELES];

//tabla de pines GPIOs
int8_t pinGPIOS[9]={16,5,4,0,2,14,12,13,15}; //tiene 9 puertos digitales, el indice es el puerto Dx y el valor el GPIO

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
    reles[i].inicio=0;
    reles[i].nombreEstados[0]="0";
    reles[i].nombreEstados[1]="1";
    reles[i].mensajes[0]="paso a cero";
    reles[i].mensajes[1]="paso a uno";
    }

  //leo la configuracion del fichero
  if(!recuperaDatosReles(debugGlobal)) Serial.println("Configuracion de los reles por defecto");
  else
    {  
    //Salidas
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
          digitalWrite(pinGPIOS[reles[i].pinLed], nivelActivo);  //lo inicializo a apagado
          }
        else 
          {
          digitalWrite(pinGPIOS[reles[i].pin], !nivelActivo);  //lo inicializo a apagado 
          digitalWrite(pinGPIOS[reles[i].pinLed], !nivelActivo);  //lo inicializo a apagado
          }
        
        Serial.printf("Nombre rele[%i]=%s | pin rele: %i | pin led: %i | inicio: %i |estado: %i\n",i,reles[i].nombre.c_str(),reles[i].pin,reles[i].pinLed,reles[i].inicio,reles[i].estado);
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
  
  if(!leeFicheroConfig(RELES_CONFIG_FILE, cad)) 
    {
    //Confgiguracion por defecto
    Serial.printf("No existe fichero de configuracion de Reles\n");    
    cad="{\"Reles\": []}";
    //salvo la config por defecto
    //if(salvaFicheroConfig(RELES_CONFIG_FILE, SALIDAS_CONFIG_BAK_FILE, cad)) Serial.printf("Fichero de configuracion de Salidas creado por defecto\n");
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

  int8_t max;
  max=(Reles.size()<MAX_RELES?Reles.size():MAX_RELES);
  for(int8_t i=0;i<max;i++)
    { 
    JsonObject& rele = json["Reles"][i];
    reles[i].nombre=rele.get<String>("nombre");
    reles[i].pin=rele.get<int8_t>("DxLed");
    reles[i].pinLed=rele.get<int8_t>("Dx");

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

  Serial.printf("Reles:\n"); 
  for(int8_t i=0;i<MAX_RELES;i++) 
    {
    Serial.printf("%01i: %s | pin: %i | inico: %i\n",i,reles[i].nombre.c_str(),reles[i].pin,reles[i].inicio); 
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
/*Si esta activo para ese intervalo de tiempo(1) */
/*Si esta por debajo de la tMin cierro rele      */
/*si no abro rele                                */
/*************************************************/
void actuaReles()
  { 
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

/*************************************************/
/*conmuta el rele indicado en id                 */
/*devuelve 1 si ok, -1 si ko                     */
/*************************************************/
int8_t conmutaRele(int8_t id, boolean estado_final, int debug)
  {
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

  mensaje="{\"origen\": \"" + reles[id_salida].nombre + "\",\"mensaje\":\"" + reles[id_salida].mensajes[estado] + "\"}";
  Serial.printf("Envia mensaje para la entrada con id %i y por cambiar a estado %i. Mensaje: %s\n\n",id_salida,estado,reles[id_salida].mensajes[estado].c_str());
  Serial.printf("A enviar: topic %s\nmensaje %s\n", TOPIC_MENSAJES,mensaje.c_str());
  enviarMQTT(TOPIC_MENSAJES, mensaje);
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
