#ifndef constants_h
#define constants_h

// En mode setup, l'ESP32 active un point d'accès wifi
// Dont les paramètres sont les suivants:
#define APSSID     "JCDELESTAGE"
#define APPASSWORD "123456789"

// Code d'apairage HomeKit
#define PAIRING_CODE "46637726" // 4663 7726

// HomeSpan display name
#define DISPLAY_NAME "Chauffage-Délestage"
#define HOST_NAME_BASE "HomeSpan"
#define MODEL_NAME "HomeSpan-ESP32"

// A quelle fréquence doit-on lire les valeurs de température
// et humidité du BME280 ?
#define ELAPSE_READ_BME 5000 // 1000 = 1000ms = 1s

//
// Paramètres de l'application
//
// ---------------------------------------------------------------- 
// A quel pourcentage de la puissance souscrite faut-il délester
// Pour les tests, on peut utiliser 5 (5%) qui permet de déclencher
// le délestage via l'allumage d'une plaque électrique
// En production, ce sera probablement 80%
// ---------------------------------------------------------------- 
//
const unsigned long PERCENT_DELESTAGE = 80;

// ---------------------------------------------------------------- 
// Quelle période Tempo déclenche le délestage
// Voici les périodes tempo:        heures pleines heures creuses
//                   jour Bleu          HPJB           HCJB
//                   jour Blanc         HPJW           HCJW
//                   jour Rouge         HPJR           HCJR
// On se propose de délester aussi bien les HP jour blanc que
// les HP jour rouge
// ---------------------------------------------------------------- 
const char *PERIODE_DELESTAGE_W = "HPJW";
const char *PERIODE_DELESTAGE_R = "HPJR";

// ---------------------------------------------------------------- 
// Durée minimale d'un délestage (1000 = 1sec)
// L'objectif est de ne pas rallumer le chauffage trop souvent
// pour éviter des alternances de délestages trop fréquents
// 300000 = 5mn
// 180000 = 3mn
// ---------------------------------------------------------------- 
const unsigned long TEMPS_DELESTAGE  =  240000; // 4mn

#endif
