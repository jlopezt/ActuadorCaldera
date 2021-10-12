#define IDENTIFICACION "Version " + String(VERSION) + "." + "<BR>"
#define FONDO     String("#DDDDDD")
#define TEXTO     String("#000000")
#define ENCENDIDO String("#FFFF00")
#define APAGADO   String("#DDDDDD")


#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <FS.h>

ESP8266WebServer server(PUERTO_WEBSERVER);

const String cabeceraHTMLlight = "<!DOCTYPE html>\n<head>\n<meta charset=\"UTF-8\" />\n<TITLE>Domoticae</TITLE><link rel=\"stylesheet\" type=\"text/css\" href=\"css.css\"></HEAD><html lang=\"es\">\n<BODY>\n"; 
const String pieHTMLlight="</body>\n</HTML>\n";

void inicializaWebServer(void)
  {
  //decalra las URIs a las que va a responder
  server.on("/", HTTP_ANY, handleMain); //layout principal
  server.on("/estado", HTTP_ANY, handleEstado); //Servicio de estdo de reles
  server.on("/nombre", handleNombre); //devuelve un JSON con las medidas, reles y modo para actualizar la pagina de datos  
  server.on("/root", HTTP_ANY, handleRoot); //devuleve el frame con la informacion principal
    
  server.on("/activaRele", HTTP_ANY, handleActivaRele); //Servicio de activacion de rele
  server.on("/desactivaRele", HTTP_ANY, handleDesactivaRele);  //Servicio de desactivacion de rele
  server.on("/bloquear", HTTP_ANY, handleBloquear); //Servicio de bloqueo de los mensajes MQTT (modo panico)
  
  server.on("/restart", HTTP_ANY, handleRestart);  //URI de test
  server.on("/info", HTTP_ANY, handleInfo);  //URI de test
  
  server.on("/ficheros", HTTP_ANY, handleFicheros);  //URI de leer fichero      
  server.on("/listaFicheros", HTTP_ANY, handleListaFicheros);  //URI de leer fichero
  server.on("/creaFichero", HTTP_ANY, handleCreaFichero);  //URI de crear fichero
  server.on("/borraFichero", HTTP_ANY, handleBorraFichero);  //URI de borrar fichero
  server.on("/leeFichero", HTTP_ANY, handleLeeFichero);  //URI de leer fichero
  server.on("/manageFichero", HTTP_ANY, handleManageFichero);  //URI de leer fichero  
  server.on("/infoFS", HTTP_ANY, handleInfoFS);  //URI de info del FS

 //Uploader
  server.on("/upload", HTTP_GET, []() {
    if (!handleFileReadChunked("/upload.html")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  
  //first callback is called after the request has ended with all parsed arguments, second callback handles file uploads at that location  
  server.on("/upload",  HTTP_POST, []() {  // If a POST request is sent to the /upload.html address,
    server.send(200, "text/plain", "Subiendo..."); 
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);//pagina no encontrada

  server.begin();
  Serial.println("HTTP server started");
  }

void webServer(int debug)
  {
  server.handleClient();
  }

/********************************************************/
/*  Servicios comunes para actualizar y cargar la web   */
/********************************************************/    
void handleMain(){handleFileReadChunked("main.html");}

void handleRoot(){handleFileReadChunked("root.html");}

void handleNombre()
  {
  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonBuffer jsonBuffer(capacity);
  
  JsonObject& root = jsonBuffer.createObject();
  root["nombreFamilia"] = NOMBRE_FAMILIA;
  root["nombreDispositivo"] = nombre_dispositivo;
  root["version"] = VERSION;
  
  String cad="";
  root.printTo(cad);
  server.send(200,"text/json",cad);
  }

void handleEstado(void)
  {
  String cad=generaJson();
  
  server.send(200, "application/json", cad);   
   
  //Serial.println("Medidas requeridas ok");
  }
  
/*********************************************/
/*                                           */
/*    activa el bloqueo de mensajes MQTT     */
/*    para la activacion desacticion de      */
/*    los reles                              */
/*                                           */
/*********************************************/
void handleBloquear() 
  {
  if(server.hasArg("accion"))
    {    
    if (server.arg("accion")=="desbloquear") 
      {
      Serial.printf("la accion es desbloquear\n");  
      desactivaBloqueoMQTT();
      }
    else if (server.arg("accion")=="bloquear") 
      {
      Serial.printf("la accion es bloquear\n");  
      activaBloqueoMQTT();
      }
    }
    
  handleRoot();
  }
    
/*********************************************/
/*                                           */
/*    desactiva el bloqueo de mensajes MQTT  */
/*    para la activacion desacticion de      */
/*    los reles                              */
/*                                           */
/*********************************************/
void xxxhandledesbloquear() 
  {
  desactivaBloqueoMQTT();
  handleRoot();
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

    handleRoot();
    return;
    
    cad += id;
    cad += SEPARADOR;
    cad += estadoRele(id);
      
    //server.send(200, "text/plain", cad); 
    handleRoot();
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

    handleRoot();
    return;
      
    cad += id;
    cad += SEPARADOR;
    cad += estadoRele(id);  
      
    //server.send(200, "text/plain", cad); 
    handleRoot();
    }
  else server.send(404, "text/plain", cad); 
  }

/*********************************************/
/*                                           */
/*  Reinicia el dispositivo mediante         */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleRestart(void)
  {
  handleFileReadChunked("restart.html"); 
  delay(1000);
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
  String cad="";

  cad += cabeceraHTMLlight;
  cad += IDENTIFICACION;

  cad += "<BR>----------------- uptime --------------------<BR>";
  unsigned long milis=millis();

  cad += "uptime (ms): " + String(milis);
  cad += "<BR>";

  int8_t segundos=(milis/1000)%60;
  int8_t minutos=(milis/60000)%60;
  int8_t horas=milis/3600000;

  char horaText[9]="";
  sprintf(horaText,"%02i:%02i:%02i",horas,minutos,segundos);
  cad += String(horaText);
  cad += "<BR>---------------------------------------------<BR>";

  cad += "<BR>-----------------info logica-----------------<BR>";
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
  
  cad += "<BR>-----------------Contadores info-----------------<BR>";
  cad += "multiplicadorAnchoIntervalo: ";
  cad += String(multiplicadorAnchoIntervalo);
  cad += "<BR>";     
  cad += "anchoIntervalo: ";
  cad += String(anchoIntervalo);
  cad += "<BR>";   
  cad += "frecuenciaOTA: ";
  cad += String(frecuenciaOTA);
  cad += "<BR>";   
  cad += "frecuenciaServidorWeb: ";
  cad += String(frecuenciaServidorWeb);
  cad += "<BR>";   
  cad += "frecuenciaOrdenes: ";
  cad += String(frecuenciaOrdenes);
  cad += "<BR>"; 
  cad += "frecuenciaMQTT: ";
  cad += String(frecuenciaMQTT);
  cad += "<BR>";
  cad += "frecuenciaEnvioDatos: ";
  cad += String(frecuenciaEnvioDatos);
  cad += "<BR>";  
  cad += "frecuenciaWifiWatchdog: ";
  cad += String(frecuenciaWifiWatchdog); 
  cad += "<BR>";  
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
  
  cad += pieHTMLlight;
  server.send(200, "text/html", cad);     
  }
/************************* FICHEROS *********************************************/
/*********************************************/
/*                                           */
/*  Crea un fichero a traves de una          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleCreaFichero(void)
  {
  String cad="";
  String nombreFichero="";
  String contenidoFichero="";

  if(server.hasArg("nombre") && server.hasArg("contenido")) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");
    contenidoFichero=server.arg("contenido");

    if(salvaFichero( nombreFichero, nombreFichero+".bak", contenidoFichero)) 
      {
      String cad=directorioFichero(nombreFichero);
      server.sendHeader("Location", "ficheros?dir=" + cad,true); 
      server.send(302, "text/html","");        
      return;
      }  
    else cad += "No se pudo salvar el fichero<br>"; 
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

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
  String nombreFichero="";
  String contenidoFichero="";
  String cad="";

  if(server.hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");

    if(borraFichero(nombreFichero)) 
      {
      String cad=directorioFichero(nombreFichero);
      server.sendHeader("Location", "ficheros?dir=" + cad,true); 
      server.send(302, "text/html","");       
      return;
      }
    else cad += "No sepudo borrar el fichero " + nombreFichero + ".\n"; 
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

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
  String cad="";
  String nombreFichero="";
  String contenido="";
   
  if(server.hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");

    if(leeFichero(nombreFichero, contenido))
      {
      cad += "El fichero tiene un tama&ntilde;o de ";
      cad += contenido.length();
      cad += " bytes.<BR>";           
      cad += "El contenido del fichero es:<BR>";
      cad += "<textarea readonly=true cols=75 rows=20 name=\"contenido\">";
      cad += contenido;
      cad += "</textarea>";
      cad += "<BR>";
      }
    else cad += "Error al abrir el fichero " + nombreFichero + "<BR>";   
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  server.send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Habilita la edicion y borrado del        */
/*  fichero indicado, a traves de una        */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/ 
void handleManageFichero(void)
  {
  String nombreFichero="";
  String contenido="";
  String cad=cabeceraHTMLlight;

  if(server.hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");
          
    if (!server.chunkedResponseModeStart(200, "text/html")) {
      server.send(505, F("text/html"), F("HTTP1.1 required"));
      return;
      }     

    cad += "<form id=\"borrarFichero\" action=\"/borraFichero\">\n";
    cad += "  <input type=\"hidden\" name=\"nombre\" value=\"" + nombreFichero + "\">\n";
    cad += "</form>\n";

    cad += "<form id=\"salvarFichero\" action=\"creaFichero\" target=\"_self\">\n";
    cad += "  <input type=\"hidden\" name=\"nombre\" value=\"" + nombreFichero + "\">\n";
    cad += "</form>\n";

    cad += "<form id=\"volver\" action=\"ficheros\" target=\"_self\">\n";
    cad += "  <input type=\"hidden\" name=\"dir\" value=\"" + directorioFichero(nombreFichero) + "\">\n";
    cad += "</form>\n";

    cad += "<div id=\"contenedor\" style=\"width:900px;\">\n";
    cad += "  <p align=\"center\" style=\"margin-top: 0px;font-size: 16px; background-color: #83aec0; background-repeat: repeat-x; color: #FFFFFF; font-family: Trebuchet MS, Arial; text-transform: uppercase;\">Fichero: " + nombreFichero + "(" + contenido.length() + ")</p>\n";
    cad += "  <BR>\n";
    cad += "  <table width='100%'><tr>\n"; 
    cad += "  <td align='left'><button form=\"salvarFichero\" type=\"submit\" value=\"Submit\">Salvar</button></td>\n";  
    cad += "  <td align='center'><button form=\"borrarFichero\" type=\"submit\" value=\"Submit\">Borrar</button></td>\n";        
    cad += "  <td align='right'><button form=\"volver\" type=\"submit\" value=\"Submit\">Atras</button></td>\n";  
    cad += "  </tr></table>\n";       
    cad += "  <BR><BR>\n";
    cad += "  <textarea form=\"salvarFichero\" cols=120 rows=45 name=\"contenido\">\n";
    server.sendContent(cad);

    if (SPIFFS.exists(nombreFichero))
      {
      const uint16_t buffSize=1000;  
      File file = SPIFFS.open(nombreFichero, "r");    
      Serial.printf("El fichero %s existe\n",nombreFichero.c_str());

      char *buff=(char *)malloc(buffSize);      
      if(buff==NULL){
          Serial.printf("Error en chunk\n"); 
          server.sendContent(String("Error al reservar memoria"));
          }
      else{
        Serial.printf("Iniciando While...\n");
        uint16_t tamano=file.size();
        uint16_t leido=0;
        while(leido<tamano){
          uint16_t tam=tamano-leido;
          if(tam>buffSize) tam=buffSize;
          leido+=file.readBytes(buff,tam);
          server.sendContent(buff,tam);   
          Serial.printf("tamaño: %i | leido: %i | tam: %i\n",tamano,leido, tam);   
          }          
        free(buff);
        }
      file.close();
      }
    else server.sendContent(String("Error al abrir el fichero " + nombreFichero));

    cad  = "\n</textarea>\n";
    cad += "</div>\n";
    cad += pieHTMLlight;
    server.sendContent(cad);
    server.chunkedResponseFinalize();
    return;
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTMLlight;
  server.send(200, "text/html", cad);
  }

/*********************************************/
/*                                           */
/*  Lista los ficheros en el sistema a       */
/*  traves de una peticion HTTP              */ 
/*                                           */
/*********************************************/  
void handleListaFicheros(void)
  {
  String prefix="/";  

  if(server.hasArg("dir")) prefix=server.arg("dir");

  server.send(200,"text/json",listadoFicheros(prefix));
  }

void handleFicheros(void)
  {
  String prefix="/";  

  if(server.hasArg("dir")) prefix=server.arg("dir");

  server.sendHeader("Location","ficheros.html?dir=" + prefix, true);      
  server.send(302);  
  }
/**********************************************************************/

/*********************************************/
/*                                           */
/*  Lee info del FS                          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleInfoFS(void)
  {
  String cad=cabeceraHTMLlight;

  cad += IDENTIFICACION;
  
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

  cad += pieHTMLlight;
  server.send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Pagina no encontrada                     */
/*                                           */
/*********************************************/
void handleNotFound()
  {
  if(handleFileReadChunked(server.uri()))return;
    
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

/**********************************************************************/
String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) 
  { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  
  if (!path.startsWith("/")) path = "/" + path;
  path = "/www" + path; //busco los ficheros en el SPIFFS en la carpeta www
  //if (!path.endsWith("/")) path += "/";
  
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) 
    { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
    }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
  }

bool handleFileReadChunked(String path) 
  { // send the right file to the client (if it exists)
  const uint16_t buffSize=1000;
  Serial.println("handleFileReadChunked: " + path);
  
  if (!path.startsWith("/")) path = "/" + path;
  path = "/www" + path; //busco los ficheros en el SPIFFS en la carpeta www
  //if (!path.endsWith("/")) path += "/";
  
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) 
    { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file

    //Inicio el chunked************************
    //size_t sent = server.streamFile(file, contentType);    // Send it to the client
    uint16_t tamano=file.size();
    uint16_t leido=0;
    char *buff=(char *)malloc(buffSize);
    
    if(buff==NULL){printf("Error en chunk\n"); return false;}
    if (!server.chunkedResponseModeStart(200, contentType)) {
      server.send(505, F("text/html"), F("HTTP1.1 required"));
      return false;
    }    
    
    while(leido<tamano){
      uint16_t tam=tamano-leido;
      if(tam>buffSize) tam=buffSize;
      leido+=file.readBytes(buff,tam);
      server.sendContent(buff,tam);   
      Serial.printf("tamaño: %i | leido: %i | tam: %i\n",tamano,leido, tam);   
    }
    server.chunkedResponseFinalize();
    free(buff);
    //Fin del chunked*************************
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
    }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
  }

void handleFileUpload()
  {
  String path = "";
  static File fsUploadFile;
  HTTPUpload& upload = server.upload();

  Serial.printf("Vamos a subir un fichero...");
  if(upload.status == UPLOAD_FILE_START)
    {
    if(server.hasArg("directorio"))path=server.arg("directorio");
    if(!path.startsWith("/")) path = "/" + path;
    if(!upload.filename.startsWith("/")) path = path + "/";
    path += upload.filename;    
    
    Serial.printf("handleFileUpload Name: %s",path.c_str());
    fsUploadFile = SPIFFS.open(path.c_str(), "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    if(!fsUploadFile) Serial.printf("Error al crear el fichero\n");
    }
  else if(upload.status == UPLOAD_FILE_WRITE)
    {
    if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
    else Serial.printf("Error al escribir en el fichero\n");
    } 
  else if(upload.status == UPLOAD_FILE_END)
    {
    String mensaje="";  

    if(fsUploadFile) // If the file was successfully created
      {                                    
      fsUploadFile.close();                               // Close the file again
      Serial.printf("handleFileUpload Size: %i", upload.totalSize);
      mensaje="Fichero subido con exito (" + String(upload.totalSize) + "bytes)";  
      }
    else mensaje="Se produjo un error al subir el fichero [" + path + "]";  

    server.sendHeader("Location","resultadoUpload.html?mensaje=" + mensaje, true);      // Redirect the client to the success page
    server.send(302);  
    }
  }
