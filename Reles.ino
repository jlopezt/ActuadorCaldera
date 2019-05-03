/*****************************************/
/*                                       */
/*  Control de reles para el termostato  */
/*                                       */
/*****************************************/
typedef struct{
  int8_t id;
  String nombre;
  int8_t estado;
  }rele_t; 

rele_t reles[MAX_RELES];
String nombre_reles[MAX_RELES];

//tabla de pines GPIOs
int8_t pinGPIOS[9]={16,5,4,0,2,14,12,13,15}; //tiene 9 puertos digitales, el indice es el puerto Dx y el valor el GPIO

void inicializaReles()
  {  
  //preconfiguracion de fabrica de los reles
  //0 Calefaccion
  nombre_reles[0]="Calefaccion";
  //1 Otros
  nombre_reles[1]="Otros";  

  //for(int i;i<MAX_RELES;i++) Serial.printf("Nombre rele[%i]=%s\n",i,nombre_reles[i].c_str());

  for(int8_t i=0;i<MAX_RELES;i++)
    {
    //inicializo la parte fisica
    pinMode(pinGPIOS[RELES_PIN+i], OUTPUT); //es salida
    digitalWrite(pinGPIOS[RELES_PIN+i], !nivelActivo);  //lo inicializo a apagado
    pinMode(pinGPIOS[LEDS_PIN+i], OUTPUT); //es salida
    digitalWrite(pinGPIOS[LEDS_PIN+i], !nivelActivo);  //lo inicializo a apagado
    
    //inicializo la parte logica
    reles[i].id=i;
    reles[i].nombre=nombre_reles[i];
    reles[i].estado=0;    
    }

  //for(int i;i<MAX_RELES;i++) Serial.printf("Nombre rele[%i]=%s\n",i,reles[i].nombre.c_str());    
  }


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
  if(id <0 || id>=MAX_RELES) return -1; //Rele fuera de rango
  //Leo directamente los registros que guardan el estado

  //return reles[id].estado;
  if(digitalRead(pinGPIOS[RELES_PIN+id])==nivelActivo) return 1; 
  else return 0;
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
  if(id <0 || id>=MAX_RELES) return -1; //Rele fuera de rango

  //parte fisica
  digitalWrite(pinGPIOS[RELES_PIN+id], estado_final); //controlo el rele
  //controlo el led asociado
  if (nivelActivo) digitalWrite(pinGPIOS[LEDS_PIN+id], estado_final); 
  else digitalWrite(pinGPIOS[LEDS_PIN+id], !estado_final); 
  
  if(debug)
    {
    Serial.printf("id: %i; GPIO: %i; estado: ",(int)id,(int)pinGPIOS[RELES_PIN+id]);
    Serial.println(digitalRead(pinGPIOS[RELES_PIN+id]));
    }
    
  //parte logica
  if(estado_final) reles[id].estado=1;
  else reles[id].estado=0;

  return 1;
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
