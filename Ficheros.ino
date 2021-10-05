/************************************************/
/*                                              */
/* Funciones para la gestion de ficheros en     */
/* memoria del modulo esp6288                   */
/*                                              */
/************************************************/

/************************************************/
/* Inicializa el sistema de ficheros del modulo */
/************************************************/
boolean inicializaFicheros(int debug)
{
  //inicializo el sistema de ficheros
  if (!SPIFFS.begin()) 
    {
    Serial.println("No se puede inicializar el sistema de ficheros");
    return (false);
    }
  return (true);
}

/************************************************/
/* Recupera los datos de                        */
/* de un archivo cualquiera                     */
/************************************************/
boolean leeFichero(String nombre, String &contenido)
  {
  boolean leido=false;
  
  Serial.println("Inicio de lectura de fichero " + nombre);

  if (SPIFFS.exists(nombre)) 
    {
    //file exists, reading and loading
    Serial.printf("Encontrado fichero de configuracion %s.\n",nombre.c_str());
    File configFile = SPIFFS.open(nombre, "r");
    if (configFile) 
      {
      Serial.printf("Abierto fichero de configuracion %s.\n",configFile.name());
      size_t size = configFile.size();

      // Allocate a buffer to store contents of the file.
      char *buff=NULL;
      buff=(char *)malloc(size+1);

      configFile.readBytes(buff, size);
      buff[size]=0;//pongo el fin de cadena
        
      contenido=String(buff);
      Serial.printf("Contenido del fichero: #%s#\n",contenido.c_str());
      free(buff);
      leido=true;
        
      configFile.close();//cierro el fichero
      Serial.println("Cierro el fichero");
      }//la de abrir el fichero de configuracion del WiFi
      else Serial.println("Fichero no se puede abrir");
    }//la de existe fichero
    else Serial.println("Fichero no existe");
  
  return leido;
  }
/**********************************************************************/
/* Salva la cadena pasada al fichero especificado                     */
/* Si ya existe lo sobreescribe                                       */
/**********************************************************************/  
boolean salvaFichero(String nombreFichero, String nombreFicheroBak, String contenidoFichero)
  {
  boolean salvado=false;

  //file exists, reading and loading
  if(SPIFFS.exists(nombreFichero.c_str())) 
    {
    if(nombreFicheroBak!="")
      {
      Serial.printf("El fichero %s ya existe, se copiara con el nombre %s.\n",nombreFichero.c_str(),nombreFicheroBak.c_str());
        
      if(SPIFFS.exists(nombreFicheroBak.c_str())) SPIFFS.remove(nombreFicheroBak.c_str());  
      SPIFFS.rename(nombreFichero.c_str(),nombreFicheroBak.c_str());  
      }
    else Serial.printf("El fichero %s ya existe, sera sobreescrito.\n",nombreFichero.c_str());
    }

  Serial.print("Nombre fichero: ");
  Serial.println(nombreFichero.c_str());
  Serial.print("Contenido fichero: ");
  Serial.println(contenidoFichero.c_str());
   
  File newFile = SPIFFS.open(nombreFichero.c_str(), "w");//abro el fichero, si existe lo borra
  if (newFile) 
    {
    Serial.printf("Abierto fichero %s.\nGuardo contenido:\n#%s#\n",newFile.name(),contenidoFichero.c_str());
  
    newFile.print(contenidoFichero);
    newFile.close();//cierro el fichero
    Serial.println("Cierro el fichero");
    salvado=true;
    }
  else Serial.println("El fichero no se pudo abrir para escritura.\n");
      
  return salvado;
  }


/****************************************************/
/* Borra el fichero especificado                    */
/****************************************************/  
boolean borraFichero(String nombreFichero)
  {
  boolean borrado=false;

  //file exists, reading and loading
  if(!SPIFFS.exists(nombreFichero)) Serial.println("El fichero " + nombreFichero + " no existe.");
  else
    {
    if (SPIFFS.remove(nombreFichero)) 
      {
      borrado=true;
      Serial.println("El fichero " + nombreFichero + " ha sido borrado.");
      }
    else Serial.println("No se pudo borrar el fichero " + nombreFichero + ".");
    }  

  return borrado;
  }  

/************************************************/
/* Recupera los ficheros almacenados en el      */
/* dispositivo. Devuelve una cadena separada    */
/* por SEPARADOR                                */
/************************************************/
boolean listaFicheros(String &contenido)
  {   
  contenido="";

  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) 
    {
    Serial.print("FILE: ");
    Serial.println(dir.fileName());

    contenido += String(dir.fileName());
    contenido += SEPARADOR;
    }
    
  return true;
  }  

/************************************************/
/* Devuelve el nombre del direcotrio del        */
/* fichro que se pasa como parametro            */
/************************************************/
String directorioFichero(String nombreFichero)
  {
  if (!nombreFichero.startsWith("/")) nombreFichero="/" + nombreFichero;
  String cad=nombreFichero.substring(0,nombreFichero.lastIndexOf("/"));
  return(cad);
  }

/************************************************/
/* Devuelve si un nombre de fichero incluye     */
/* un directorio por que encuentre mas de una / */
/************************************************/
boolean esDirectorio(String nombre)
  {
  if(nombre.startsWith("/")) nombre=nombre.substring(1);//si empieza por / se lo quito

  if(nombre.indexOf("/")!=-1) return true;
  return false;
  }

/************************************************/
/* Recupera los ficheros almacenados en el      */
/* dispositivo. Devuelve una cadena separada    */
/* por SEPARADOR                                */
/************************************************/
String listadoFicheros(String prefix)
  {   
  String salida="";

  if(!prefix.startsWith("/")) prefix="/" + prefix;

  const size_t capacity = 2*JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(31);
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& json = jsonBuffer.createObject();
  json["padre"] = prefix;

  JsonArray& subdirectorios = json.createNestedArray("subdirectorios");
  JsonArray& ficheros = json.createNestedArray("ficheros");

  Dir root = SPIFFS.openDir(prefix);

  while(root.next())
    {
    String fichero=String(root.fileName());
    //Si el nombre incluye el prefix, se lo quito
    uint8_t inicio=(fichero.indexOf(prefix)==-1?0:fichero.indexOf(prefix));
    fichero=fichero.substring(inicio+prefix.length());

    if(esDirectorio(fichero)) 
      {
      //verifico que el directorio no este ya en la lista
      boolean existe=false;
      String subdir=fichero.substring(0,fichero.indexOf("/"));
      for(uint8_t i=0;i<subdirectorios.size();i++)
        {
        if(subdir==subdirectorios[i]) 
          {
          existe=true;
          break;
          }
        }
      if(!existe) subdirectorios.add(fichero.substring(0,fichero.indexOf("/")));
      }
    else 
      {
      JsonObject& fichero_nuevo = ficheros.createNestedObject();
      fichero_nuevo["nombre"] = fichero;
      fichero_nuevo["tamano"] = root.fileSize();
      fichero_nuevo["fechaEdicion"] = horaYfecha(root.fileTime());
      }
    }   
  json.printTo(salida);
  return (salida);
  }  

/************************************************/
/* Devuelve si existe o no un fichero en el     */
/* dispositivo                                  */
/************************************************/
boolean existeFichero(String nombre)
  {  
  return (SPIFFS.exists(nombre));
  }


/************************************************/
/* Formatea el sistemas de ficheros del         */
/* dispositivo                                  */
/************************************************/
boolean formatearFS(void)
  {  
  return (SPIFFS.format());
  }

/***************************************************************/
/*                                                             */
/*  Genera una cadena con la hora en formato dd-mm-yy HH:MM:SS */
/*  a partir de la estrucutura time_t que se le pasa           */
/*                                                             */
/***************************************************************/
String horaYfecha(time_t entrada)
  {
  String cad="";  
  const char formato[]="%d-%m-%Y %H:%M:%S";
  const uint8_t longitud=20;
  char buf[longitud];

  struct tm* ts = localtime(&entrada);
  strftime(buf, sizeof(buf), formato, ts);
  buf[longitud-1]=0;

  return (String(buf));    
  }
