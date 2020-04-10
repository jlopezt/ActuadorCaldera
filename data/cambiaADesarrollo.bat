move Config.json Config.json.sav
move MQTTConfig.json MQTTConfig.json.sav
move WiFiConfig.json WiFiConfig.json.sav
move RelesConfig.json RelesConfig.json.sav

copy Config.json.desarrollo Config.json
copy RelesConfig.json.desarrollo RelesConfig.json
copy WiFiConfig.json.desarrollo WiFiConfig.json
copy MQTTConfig.json.desarrollo MQTTConfig.json

del Config.json.sav
del WiFiConfig.json.sav
del MQTTConfig.json.sav
del RelesConfig.json.sav

date /T>AA_DESARROLLO
time /T>>AA_DESARROLLO 
del AA_PRODUCCION