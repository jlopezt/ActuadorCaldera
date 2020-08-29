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
#define IDENTIFICACION "Version " + String(VERSION) + "." + "<BR>"
#define FONDO     String("#DDDDDD")
#define TEXTO     String("#000000")
#define ENCENDIDO String("#FFFF00")
#define APAGADO   String("#DDDDDD")


#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <FS.h>

ESP8266WebServer server(PUERTO_WEBSERVER);

String cabeceraHTML="<!DOCTYPE html><HTML><HEAD><TITLE>" + nombre_dispositivo + " </TITLE></HEAD><BODY><h1><a href=\"../\" target=\"_self\">" + nombre_dispositivo + "</a><br></h1>";
String enlaces="<TABLE>\n<CAPTION>Enlaces</CAPTION>\n<TR><TD><a href=\"info\" target=\"_self\">Info</a></TD></TR>\n<TR><TD><a href=\"test\" target=\"_self\">Test</a></TD></TR>\n<TR><TD><a href=\"restart\" target=\"_self\">Restart</a></TD></TR>\n<TR><TD><a href=\"listaFicheros\" target=\"_self\">Lista ficheros</a></TD></TR>\n<TR><TD><a href=\"estado\" target=\"_self\">Estado</a></TD></TR>\n</TABLE>\n"; 
String pieHTML="</BODY></HTML>";

void inicializaWebServer(void)
  {
  //decalra las URIs a las que va a responder
  server.on("/", HTTP_ANY, handleRoot); //Responde con la iodentificacion del modulo
//  server.on("/web", HTTP_ANY, handleWeb); //Servicio de estdo de reles en HTML
//  server.on("/estado", HTTP_ANY, handleEstadoReles); //Servicio de estdo de reles
  server.on("/activaRele", HTTP_ANY, handleActivaRele); //Servicio de activacion de rele
  server.on("/desactivaRele", HTTP_ANY, handleDesactivaRele);  //Servicio de desactivacion de rele
  
  server.on("/test", HTTP_ANY, handleTest);  //URI de test
  server.on("/reset", HTTP_ANY, handleReset);  //URI de test  
  server.on("/restart", HTTP_ANY, handleRestart);  //URI de test
  server.on("/info", HTTP_ANY, handleInfo);  //URI de test
  
  server.on("/listaFicheros", HTTP_ANY, handleListaFicheros);  //URI de leer fichero
  server.on("/creaFichero", HTTP_ANY, handleCreaFichero);  //URI de crear fichero
  server.on("/borraFichero", HTTP_ANY, handleBorraFichero);  //URI de borrar fichero
  server.on("/leeFichero", HTTP_ANY, handleLeeFichero);  //URI de leer fichero
  server.on("/manageFichero", HTTP_ANY, handleManageFichero);  //URI de leer fichero  
  server.on("/infoFS", HTTP_ANY, handleInfoFS);  //URI de info del FS

  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);//pagina no encontrada

  server.begin();
  Serial.println("HTTP server started");
  }

void webServer(int debug)
  {
  server.handleClient();
  }
  
void handleRoot() 
  {
  String cad="";

  //Revisa si se pide bloquear las activaciones por mensajes MQTT
  if(server.hasArg("accion"))
    {    
    if (server.arg("accion")=="desbloquear") desactivaBloqueoMQTT();
    else if (server.arg("accion")=="bloquear") activaBloqueoMQTT();
    }
    
  cad += cabeceraHTML;
  //genero la respuesta por defecto
  cad +="<BR>";  

  //Salidas
  cad += "\n<TABLE style=\"border: 2px solid grey\">\n";
  cad += "<CAPTION>Salidas</CAPTION>\n";  
  for(int8_t i=0;i<MAX_RELES;i++)
    {
    cad += "<TR>\n";
    //Enlace para activar o desactivar
    String orden="";
    String color="";
    if (estadoRele(i)==1) 
      {
      orden="desactiva"; 
      color=APAGADO;
      }
    else 
      {
      orden="activa";
      color=ENCENDIDO;
      }
    cad += "<TD STYLE=\"color: " + TEXTO + "; text-align: center; background-color: " + FONDO + "\">" + nombreRele(i) + "</TD>";
    cad += "<TD STYLE=\"color: " + TEXTO + "; text-align: center; background-color: " + String((estadoRele(i)?ENCENDIDO:APAGADO)) + "; width: 100px\">" + nombreEstadoRele(i) + "</TD>";
    cad += "<td>\n";
    cad += "<form action=\"" + orden + "Rele\">\n";
    cad += "<input  type=\"hidden\" id=\"id\" name=\"id\" value=\"" + String(i) + "\" >\n";
    cad += "<input STYLE=\"color: " + TEXTO + "; text-align: center; background-color: " + color + "; width: 80px\" type=\"submit\" value=\"" + orden + "r\">\n";
    cad += "</form>\n";
    cad += "</td>\n";    
    cad += "</TR>\n";        
    }
  cad += "</TABLE>\n";
  cad += "<BR><BR>";  

  //Bloquear actuador
  cad += "\n<TABLE style=\"border: 2px solid grey\">\n";
  cad += "<CAPTION>Estado de bloqueo</CAPTION>\n";  
  cad += "<tr>";
  cad += "<TD STYLE=\"color: " + TEXTO + "; text-align: center; background-color: " + (bloqueoMQTT()?ENCENDIDO:APAGADO) + "; width: 100px\">" + (bloqueoMQTT()?String("Bloqueado"):String("Desbloqueado")) + "</TD>\n";  
  cad += "<td>";
  cad += "<form action=\"./\" meth=\"post\">\n";
  String accionBloqueo;
  String color="";
  if(bloqueoMQTT()) 
    {
    accionBloqueo = "desbloquear";
    color = APAGADO;;
    }
  else 
    {
    accionBloqueo = "bloquear";
    color = ENCENDIDO;
    }
  cad += "<input type=\"hidden\" id=\"accion\" name=\"accion\" value=\"" + accionBloqueo +"\">";
  cad += "<input STYLE=\"color: " + TEXTO + "; text-align: center; background-color: " + color + "; width: 100px\" type=\"submit\" value=\"" + accionBloqueo +"\">";
  cad += "</form>";
  cad += "</td>";
  cad += "</tr>";  
  cad += "</table>";  
  cad += "<BR><BR>";  
 
  cad += enlaces;
  cad += "<BR>";
  cad += "vueltas= " + String(vuelta) + " / " + String(UINT16_MAX);
  cad += "<BR><BR>";
  cad += IDENTIFICACION;

  cad += pieHTML;
  
  server.send(200, "text/html", cad);
  }

/*********************************************/
/*                                           */
/*    activa el bloqueo de mensajes MQTT     */
/*    para la activacion desacticion de      */
/*    los reles                              */
/*                                           */
/*********************************************/
void xxxhandleBloquear() 
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
/*    Web de consulta del estado de          */
/*    los reles                              */
/*                                           */
/*********************************************/
void handleWeb() 
  {
  String cad="";

  cad += cabeceraHTML;
  
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

    handleRoot();
    return;
    
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

    handleRoot();
    return;
      
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
  String cad="";

  cad += cabeceraHTML;
  cad += IDENTIFICACION;
  
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
  String cad="";

  cad += cabeceraHTML;
  cad += IDENTIFICACION;
  
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
  String cad="";

  cad += cabeceraHTML;
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
  String cad="";

  cad += cabeceraHTML;
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
  String cad="";
  String nombreFichero="";
  String contenidoFichero="";
  boolean salvado=false;

  cad += cabeceraHTML;
  cad += IDENTIFICACION;

  if(server.hasArg("nombre") && server.hasArg("contenido")) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");
    contenidoFichero=server.arg("contenido");

    if(salvaFichero( nombreFichero, nombreFichero+".bak", contenidoFichero)) 
    {
    //cad += "Fichero salvado con exito<br>";
    handleListaFicheros();
    return;
    }    
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
  String nombreFichero="";
  String contenidoFichero="";
  String cad="";

  cad += cabeceraHTML;
  cad += IDENTIFICACION;
  
  if(server.hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");

    if(borraFichero(nombreFichero)) 
      {
      //cad += "El fichero " + nombreFichero + " ha sido borrado.\n";
      handleListaFicheros();
      return;
      }
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
  String contenido="";
  
  cad += "<h1>" + String(NOMBRE_FAMILIA) + "</h1>";
  
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
  if(handleFileRead(server.uri()))return;
    
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

/*********************************************/
/*                                           */
/*  Habilita la edicion y borrado de los     */
/*  ficheros en el sistema a traves de una   */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/ 
void handleManageFichero(void)
  {
  String nombreFichero="";
  String contenido="";
  String cad="";

  cad += cabeceraHTML;
  cad += IDENTIFICACION;
  
  if(server.hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server.arg("nombre");
    cad += "<h2>Fichero: " + nombreFichero + "</h2><BR>";  

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

      cad += "<table><tr><td>";
      cad += "<form action=\"borraFichero\" target=\"_self\">";
      cad += "  <p>";
      cad += "    <input type=\"hidden\" name=\"nombre\" value=\"" + nombreFichero + "\">";
      cad += "    <input type=\"submit\" value=\"borrar\">";
      cad += "  </p>";
      cad += "</form>";
      cad += "</td></tr></table>";
      
      cad += "<table>Modificar fichero<tr><td>";      
      cad += "<form action=\"creaFichero\" target=\"_self\">";
      cad += "  <p>";
      cad += "    <input type=\"hidden\" name=\"nombre\" value=\"" + nombreFichero + "\">";
      cad += "    contenido del fichero: <br><textarea cols=75 rows=20 name=\"contenido\">" + contenido + "</textarea>";
      cad += "    <BR>";
      cad += "    <input type=\"submit\" value=\"salvar\">";
      cad += "  </p>";
      cad += "</td></tr></table>";
      }
    else cad += "Error al abrir el fichero " + nombreFichero + "<BR>";
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTML;
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
  String nombreFichero="";
  String contenidoFichero="";
  boolean salvado=false;
  String cad="";

  cad += cabeceraHTML;
  cad += IDENTIFICACION;
  
  //Variables para manejar la lista de ficheros
  String contenido="";
  String fichero="";  
  int16_t to=0;
  
  if(listaFicheros(contenido)) 
    {
    Serial.printf("contenido inicial= %s\n",contenido.c_str());      
    //busco el primer separador
    to=contenido.indexOf(SEPARADOR); 

    cad +="<style> table{border-collapse: collapse;} th, td{border: 1px solid black; padding: 10px; text-align: left;}</style>";
    cad += "<TABLE>";
    while(to!=-1)
      {
      fichero=contenido.substring(0, to);//cojo el principio como el fichero
      contenido=contenido.substring(to+1); //la cadena ahora es desde el separador al final del fichero anterior
      to=contenido.indexOf(SEPARADOR); //busco el siguiente separador

      cad += "<TR><TD>" + fichero + "</TD>";           
      cad += "<TD>";
      cad += "<form action=\"manageFichero\" target=\"_self\">";
      cad += "    <input type=\"hidden\" name=\"nombre\" value=\"" + fichero + "\">";
      cad += "    <input type=\"submit\" value=\"editar\">";
      cad += "</form>";
      cad += "</TD><TD>";
      cad += "<form action=\"borraFichero\" target=\"_self\">";
      cad += "    <input type=\"hidden\" name=\"nombre\" value=\"" + fichero + "\">";
      cad += "    <input type=\"submit\" value=\"borrar\">";
      cad += "</form>";
      cad += "</TD></TR>";
      }
    cad += "</TABLE>\n";
    cad += "<BR>";
    
    //Para crear un fichero nuevo
    cad += "<h2>Crear un fichero nuevo:</h2>";
    cad += "<table><tr><td>";      
    cad += "<form action=\"creaFichero\" target=\"_self\">";
    cad += "  <p>";
    cad += "    Nombre:<input type=\"text\" name=\"nombre\" value=\"\">";
    cad += "    <BR>";
    cad += "    Contenido:<br><textarea cols=75 rows=20 name=\"contenido\"></textarea>";
    cad += "    <BR>";
    cad += "    <input type=\"submit\" value=\"salvar\">";
    cad += "  </p>";
    cad += "</td></tr></table>";      
    }
  else cad += "<TR><TD>No se pudo recuperar la lista de ficheros</TD></TR>"; 

  cad += pieHTML;
  server.send(200, "text/html", cad); 
  }
  
/**********************************************************************/
String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) 
  { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  
  if (!path.startsWith("/")) path += "/";
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

void handleFileUpload()
  {
  File fsUploadFile;
  HTTPUpload& upload = server.upload();
  String path;
 
  if(upload.status == UPLOAD_FILE_START)
    {
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.startsWith("/www")) path = "/www"+path;
    if(!path.endsWith(".gz")) 
      {                          // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
      }
      
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
    }
  else if(upload.status == UPLOAD_FILE_WRITE)
    {
    if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
    } 
  else if(upload.status == UPLOAD_FILE_END)
    {
    if(fsUploadFile) 
      {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
      }
    else 
      {
      server.send(500, "text/plain", "500: couldn't create file");
      }
    }
  }
