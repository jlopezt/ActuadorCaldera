/************************************************************************************************
Servicio                       URL                             Formato entrada Formato salida                                            Comentario                                            Ejemplo peticion                      Ejemplo respuesta
Estado de los reles            http://IPActuador/estado        N/A             <id_0>#<nombre_0>#<estado_0>|<id_1>#<nombre_1>#<estado_1> Devuelve el id de cada rele y su estado               http://IPActuador/estado              1#1|2#0
Activa rele                    http://IPActuador/activaRele    id=<id>         <id>#<estado>                                             Activa el rele indicado y devuelve el estado leido    http://IPActuador/activaRele?id=1     1|1
Desactivarele                  http://IPActuador/desactivaRele id=<id>         <id>#<estado>                                             Desactiva el rele indicado y devuelve el estado leido http://IPActuador/desactivaRele?id=0  0|0
Test                           http://IPActuador/test          N/A             HTML                                                      Verifica el estado del Actuadot   
Reinicia el controlador.       http://IPControlador/restart
Informacion del Hw del sistema http://IPControlador/info
************************************************************************************************/

//enum HTTPMethod { HTTP_ANY, HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_OPTIONS };
#define IDENTIFICACION "Modulo Actuador. <BR>Version " + String(VERSION) + "." + "<BR>" //#define IDENTIFICACION "<BR><BR><BR><BR><BR>Modulo actuador. Version " + String(VERSION) + ".";

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

ESP8266WebServer server(PUERTO_WEBSERVER);

String cabeceraHTML="<HTML><HEAD><TITLE>Control " + nombre_dispoisitivo + " </TITLE></HEAD><BODY><h1>" + nombre_dispoisitivo + "<br></h1>";
String pieHTML="</BODY></HTML>";

void handleRoot() 
  {
  String cad="";
  
  //genero la respuesta por defecto
  cad += IDENTIFICACION;
  cad +="<BR><BR>";
  cad += "<table>";  
  cad += "<tr><td>Servicio</td><td>URL</td><td>Formato entrada</td><td>Formato salida</td><td>Comentario</td><td>Ejemplo peticion</td><td>Ejemplo respuesta</td></tr>";
  cad += "<tr><td>Estado de los reles</td><td>http://" + String(getIP(debugGlobal)) + "/estado</td><td>N/A</td><td><id_0>#<nombre_0>#<estado_0>|<id_1>#<nombre_1>#<estado_1></td><td>Devuelve el id de cada rele y su estado</td><td>http://" + String(getIP(debugGlobal)) + "/estado</td><td>1#1|2#0</td></tr>";
  cad += "<tr><td>Activa rele</td><td>http://" + String(getIP(debugGlobal)) + "/activaRele</td><td>id=<id></td><td><id>#<estado></td><td>Activa el rele indicado y devuelve el estado leido</td><td>http://" + String(getIP(debugGlobal)) + "/activaRele?id=1</td><td>1|1</td></tr>";
  cad += "<tr><td>Desactivarele</td><td>http://" + String(getIP(debugGlobal)) + "/desactivaRele id=<id></td><td><id>#<estado></td><td>Desactiva el rele indicado y devuelve el estado leido</td><td>http://" + String(getIP(debugGlobal)) + "/desactivaRele?id=0</td><td>0|0</td></tr>";  
  cad += "<tr><td>Test</td><td>http://" + String(getIP(debugGlobal)) + "/test</td><td>N/A</td><td>HTML</td><td>Verifica el estado del Actuador</td></tr>";   
  cad += "<tr><td>Reinicia el controlador.</td><td>http://" + String(getIP(debugGlobal)) + "/restart</td></tr>";
  cad += "<tr><td>Informacion del Hw del sistema</td><td>http://" + String(getIP(debugGlobal)) + "/info</td></tr>";
  cad += "</table>";
  cad +="<BR><BR>";
  cad += "vueltas= " + String(vuelta) + " / " + String(UINT16_MAX);
  server.send(200, "text/html", cad);
  }

/*********************************************/
/*                                           */
/*    Web de consulta del estado de          */
/*    los reles                              */
/*                                           */
/*********************************************/
void handleWeb() 
  {
  String cad=cabeceraHTML;

  cad += "<h1>Reles: </h1><BR>";
  for(int8_t id=0;id<MAX_RELES;id++)
    {
    cad += "Rele ";
    cad += id;
    cad += ": ";
    cad += nombreRele(id);
    cad += ": ";     
    cad += estadoRele(id);
    cad += "<BR>";
    }
  cad += pieHTML;
  
  server.send(200, "text/html", cad);
  }

/*********************************************/
/*                                           */
/*  Servicio de actuacion de rele            */
/*                                           */
/*********************************************/  
void handleEstadoReles(void)
  {
  String cad="";

  for(int8_t id=0;id<MAX_RELES;id++)
    {
    if(cad!="") cad+= SEPARADOR;
    cad += id;
    cad += SUBSEPARADOR;
    cad += nombreRele(id);
    cad += SUBSEPARADOR;    
    cad += estadoRele(id);
    }
        
  server.send(200, "text/plain", cad); 
  }
  
/*********************************************/
/*                                           */
/*  Servicio de actuacion de rele            */
/*                                           */
/*********************************************/  
void handleActivaRele(void)
  {
  String cad="";
  int8_t id=0;

  if(server.hasArg("id") ) 
    {
    int8_t id=server.arg("id").toInt();

    //activaRele(id);
    conmutaRele(id, nivelActivo, debugGlobal);
    
    cad += id;
    cad += SEPARADOR;
    cad += estadoRele(id);
      
    server.send(200, "text/plain", cad); 
    }
    else server.send(404, "text/plain", cad);  
  }


/*********************************************/
/*                                           */
/*  Servicio de desactivacion de rele        */
/*                                           */
/*********************************************/  
void handleDesactivaRele(void)
  {
  String cad="";
  int8_t id=0;

  if(server.hasArg("id") ) 
    {
    int8_t id=server.arg("id").toInt();

    //desactivaRele(id);
    conmutaRele(id, !nivelActivo, debugGlobal);
  
    cad += id;
    cad += SEPARADOR;
    cad += estadoRele(id);  
      
    server.send(200, "text/plain", cad); 
    }
  else server.send(404, "text/plain", cad); 
  }

/*********************************************/
/*                                           */
/*  Servicio de test                         */
/*                                           */
/*********************************************/  
void handleTest(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION; //"Modulo " + String(direccion) + " Habitacion= " + nombres[direccion];
  
  cad += "Test OK<br>";
  cad += pieHTML;
    
  server.send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Resetea el dispositivo mediante          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleReset(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION; //"Modulo " + String(direccion) + " Habitacion= " + nombres[direccion];
  
  cad += "Reseteando...<br>";
  cad += pieHTML;
    
  server.send(200, "text/html", cad);
  delay(100);     
  ESP.reset();
  }
  
/*********************************************/
/*                                           */
/*  Reinicia el dispositivo mediante         */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleRestart(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION;
  
  cad += "Reiniciando...<br>";
  cad += pieHTML;
    
  server.send(200, "text/html", cad);     
  delay(100);
  ESP.restart();
  }

/*********************************************/
/*                                           */
/*  Lee info del chipset mediante            */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleInfo(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION; //"Modulo " + String(direccion) + " Habitacion= " + nombres[direccion];

  cad+= "<BR>-----------------info logica-----------------<BR>";
  cad += "IP: " + String(getIP(debugGlobal));
  cad += "<BR>";  
  cad += "nivelActivo: " + String(nivelActivo);
  cad += "<BR>";  
  for(int8_t i=0;i<MAX_RELES;i++)
    {
    cad += "Rele " + String(i) + " nombre: " + reles[i].nombre + "| estado: " + reles[i].estado;
    cad += "<BR>";   
    }
  cad += "-----------------------------------------------<BR>";  
  
  cad += "<BR>-----------------WiFi info-----------------<BR>";
  cad += "SSID: " + nombreSSID();
  cad += "<BR>";    
  cad += "IP: " + WiFi.localIP().toString();
  cad += "<BR>";    
  cad += "Potencia: " + String(WiFi.RSSI());
  cad += "<BR>";    
  cad += "-----------------------------------------------<BR>";    
/*
  cad += "<BR>-----------------MQTT info-----------------<BR>";
  cad += "IP broker: " + IPBroker.toString();
  cad += "<BR>";
  cad += "Puerto broker: " +   puertoBroker=0;
  cad += "<BR>";  
  cad += "Usuario: " + usuarioMQTT="";
  cad += "<BR>";  
  cad += "Password: " + passwordMQTT="";
  cad += "<BR>";  
  cad += "Topic root: " + topicRoot="";
  cad += "<BR>";  
  cad += "-----------------------------------------------<BR>";  
*/
  cad += "<BR>-----------------Hardware info-----------------<BR>";
  cad += "Vcc: " + String(ESP.getVcc());
  cad += "<BR>";  
  cad += "FreeHeap: " + String(ESP.getFreeHeap());
  cad += "<BR>";
  cad += "ChipId: " + String(ESP.getChipId());
  cad += "<BR>";  
  cad += "SdkVersion: " + String(ESP.getSdkVersion());
  cad += "<BR>";  
  cad += "CoreVersion: " + ESP.getCoreVersion();
  cad += "<BR>";  
  cad += "FullVersion: " + ESP.getFullVersion();
  cad += "<BR>";  
  cad += "BootVersion: " + String(ESP.getBootVersion());
  cad += "<BR>";  
  cad += "BootMode: " + String(ESP.getBootMode());
  cad += "<BR>";  
  cad += "CpuFreqMHz: " + String(ESP.getCpuFreqMHz());
  cad += "<BR>";  
  cad += "FlashChipId: " + String(ESP.getFlashChipId());
  cad += "<BR>";  
     //gets the actual chip size based on the flash id
  cad += "FlashChipRealSize: " + String(ESP.getFlashChipRealSize());
  cad += "<BR>";  
     //gets the size of the flash as set by the compiler
  cad += "FlashChipSize: " + String(ESP.getFlashChipSize());
  cad += "<BR>";  
  cad += "FlashChipSpeed: " + String(ESP.getFlashChipSpeed());
  cad += "<BR>";  
     //FlashMode_t ESP.getFlashChipMode());
  cad += "FlashChipSizeByChipId: " + String(ESP.getFlashChipSizeByChipId());  
  cad += "<BR>";  
  cad += "-----------------------------------------------<BR>";  

  cad += "<BR>-----------------info fileSystem-----------------<BR>";   
  FSInfo fs_info;
  if(SPIFFS.info(fs_info)) 
    {
    /*        
     struct FSInfo {
        size_t totalBytes;
        size_t usedBytes;
        size_t blockSize;
        size_t pageSize;
        size_t maxOpenFiles;
        size_t maxPathLength;
    };
     */
    cad += "totalBytes: ";
    cad += fs_info.totalBytes;
    cad += "<BR>usedBytes: ";
    cad += fs_info.usedBytes;
    cad += "<BR>blockSize: ";
    cad += fs_info.blockSize;
    cad += "<BR>pageSize: ";
    cad += fs_info.pageSize;    
    cad += "<BR>maxOpenFiles: ";
    cad += fs_info.maxOpenFiles;
    cad += "<BR>maxPathLength: ";
    cad += fs_info.maxPathLength;
    }
  else cad += "Error al leer info";
  cad += "<BR>-----------------------------------------------<BR>"; 
  
  cad += pieHTML;
  server.send(200, "text/html", cad);     
  }

/*********************************************/
/*                                           */
/*  Crea un fichero a traves de una          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleCreaFichero(void)
  {
  String cad=cabeceraHTML;
  String nombreFichero="";
  String contenidoFichero="";
  boolean salvado=false;

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";

  if(server.hasArg("nombre") && server.hasArg("contenido")) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");
    contenidoFichero=server.arg("contenido");

    if(salvaFichero( nombreFichero, nombreFichero+".bak", contenidoFichero)) cad += "Fichero salvado con exito<br>";
    else cad += "No se pudo salvar el fichero<br>"; 
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTML;
  server.send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Borra un fichero a traves de una         */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleBorraFichero(void)
  {
  String cad=cabeceraHTML;
  String nombreFichero="";
  String contenidoFichero="";

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  
  if(server.hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");

    if(borraFichero(nombreFichero)) cad += "El fichero " + nombreFichero + " ha sido borrado.\n";
    else cad += "No sepudo borrar el fichero " + nombreFichero + ".\n"; 
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTML;
  server.send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Lee un fichero a traves de una           */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleLeeFichero(void)
  {
  String cad=cabeceraHTML;
  String nombreFichero="";

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  
  if(server.hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");

    //inicializo el sistema de ficheros
    if (SPIFFS.begin()) 
      {
      Serial.println("---------------------------------------------------------------\nmounted file system");  
      //file exists, reading and loading
      if(!SPIFFS.exists(nombreFichero)) cad += "El fichero " + nombreFichero + " no existe.\n";
      else
        {
         File f = SPIFFS.open(nombreFichero, "r");
         if (f) 
           {
           Serial.println("Fichero abierto");
           size_t tamano_fichero=f.size();
           Serial.printf("El fichero tiene un tamaño de %i bytes.\n",tamano_fichero);
           cad += "El fichero tiene un tamaño de ";
           cad += tamano_fichero;
           cad += " bytes.<BR>";
           char buff[tamano_fichero+1];
           f.readBytes(buff,tamano_fichero);
           buff[tamano_fichero+1]=0;
           Serial.printf("El contenido del fichero es:\n******************************************\n%s\n******************************************\n",buff);
           cad += "El contenido del fichero es:<BR>";
           cad += buff;
           cad += "<BR>";
           f.close();
           }
         else cad += "Error al abrir el fichero " + nombreFichero + "<BR>";
        }  
      Serial.println("unmounted file system\n---------------------------------------------------------------");
      }//La de abrir el sistema de ficheros
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTML;
  server.send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Lee info del FS                          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleInfoFS(void)
  {
  String cad=cabeceraHTML;

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  
  //inicializo el sistema de ficheros
  if (SPIFFS.begin()) 
    {
    Serial.println("---------------------------------------------------------------\nmounted file system");  
    FSInfo fs_info;
    if(SPIFFS.info(fs_info)) 
      {
      /*        
       struct FSInfo {
          size_t totalBytes;
          size_t usedBytes;
          size_t blockSize;
          size_t pageSize;
          size_t maxOpenFiles;
          size_t maxPathLength;
      };
       */
      cad += "totalBytes: ";
      cad += fs_info.totalBytes;
      cad += "<BR>usedBytes: ";
      cad += fs_info.usedBytes;
      cad += "<BR>blockSize: ";
      cad += fs_info.blockSize;
      cad += "<BR>pageSize: ";
      cad += fs_info.pageSize;    
      cad += "<BR>maxOpenFiles: ";
      cad += fs_info.maxOpenFiles;
      cad += "<BR>maxPathLength: ";
      cad += fs_info.maxPathLength;
      }
    else cad += "Error al leer info";

    Serial.println("unmounted file system\n---------------------------------------------------------------");
    }//La de abrir el sistema de ficheros

  cad += pieHTML;
  server.send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Pagina no encontrada                     */
/*                                           */
/*********************************************/
void handleNotFound()
  {
  String message = "";//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";

  message = "<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  message += "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i=0; i<server.args(); i++)
    {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
  server.send(404, "text/html", message);
  }

void inicializaWebServer(void)
  {
  //decalra las URIs a las que va a responder
  server.on("/", handleRoot); //Responde con la iodentificacion del modulo
  server.on("/web", handleWeb); //Servicio de estdo de reles en HTML
  server.on("/estado", handleEstadoReles); //Servicio de estdo de reles
  server.on("/activaRele", handleActivaRele); //Servicio de activacion de rele
  server.on("/desactivaRele", handleDesactivaRele);  //Servicio de desactivacion de rele
  server.on("/test", handleTest);  //URI de test

  server.on("/reset", handleReset);  //URI de test  
  server.on("/restart", handleRestart);  //URI de test
  server.on("/info", handleInfo);  //URI de test
  
  server.on("/creaFichero", handleCreaFichero);  //URI de crear fichero
  server.on("/borraFichero", handleBorraFichero);  //URI de borrar fichero
  server.on("/leeFichero", handleLeeFichero);  //URI de leer fichero
  server.on("/infoFS", handleInfoFS);  //URI de info del FS

  server.onNotFound(handleNotFound);//pagina no encontrada

  server.begin();
  Serial.println("HTTP server started");
  }

void webServer(int debug)
  {
  server.handleClient();
  }
