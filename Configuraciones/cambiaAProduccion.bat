move Config.json Config.json.sav
move MQTTConfig.json MQTTConfig.json.sav
move WiFiConfig.json WiFiConfig.json.sav
move RelesConfig.json RelesConfig.json.sav

copy Config.json.produccion Config.json
copy RelesConfig.json.produccion RelesConfig.json
copy WiFiConfig.json.produccion WiFiConfig.json
copy MQTTConfig.json.produccion MQTTConfig.json

del Config.json.sav
del WiFiConfig.json.sav
del MQTTConfig.json.sav
del RelesConfig.json.sav
 
date /T>AA_PRODUCCION
time /t>>AA_PRODUCCION
del AA_DESARROLLO 