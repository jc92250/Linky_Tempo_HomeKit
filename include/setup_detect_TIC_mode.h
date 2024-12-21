#ifndef setup_detect_TIC_mode_h
#define setup_detect_TIC_mode_h

#include <Arduino.h>
#include "LibTeleinfo.h"

// Debug: utilisé lors de la recherche du mode de la TIC (recherche si historique ou standard)
//#define DEBUG_TIC

// Utilisé pour forcer un mode
// plutôt que de laisser l'auto-détection se dérouler
#define FORCE_MODE_TIC		TINFO_MODE_HISTORIQUE
//#define FORCE_MODE_TIC		TINFO_MODE_STANDARD

// ---------------------------------------------------------------- 
// change_etat_led_teleinfo
// ---------------------------------------------------------------- 
void change_etat_led(uint8_t led)
{
  static uint8_t led_state;

  led_state = !led_state;
  digitalWrite(led, led_state);

}

// ---------------------------------------------------------------- 
// setup_detect_TIC_mode
//
// Exécuté au démarrage de l'Arduino
// Détermine le mode de fonctionnement de la TIC
// Et du coup, configure le Serial en conséquence:
// - 1200 bauds en mode historique
// ou
// - 9600 bauds en mode standard
// Le code commence par interpréter les infos en mode historique
// Si en 10s il n'a pas reconnu une trame valide, il considère
// que la TIC est en mode standard
//
// Les leds:
// On utilise la DED_BUILTIN
// Elle va clignoter à chaque caractère reçu de la TIC
// A la fin, si tou va bien (on a bien détecté le mode "historique"
// la LED va s'éteindre
// Sinon, elle va clignoter lentement indéfiniment
// ---------------------------------------------------------------- 
_Mode_e setup_detect_TIC_mode()
{
  _Mode_e mode;

  #ifdef FORCE_MODE_TIC
	mode = FORCE_MODE_TIC;
	if (mode == TINFO_MODE_HISTORIQUE) {
		Serial2.begin(1200);
		Serial.print(F(">> FORCE TIC mode historique <<"));
	}
	else {
		Serial2.begin(9600);
		Serial.print(F(">> FORCE TIC mode standard <<"));
	}
  return mode;
  #endif

  boolean flag_timeout = false;
  boolean flag_found_speed = false;
  uint32_t currentTime = millis();
  unsigned step = 0;
  int nbc_etiq=0;
  int nbc_val=0;

  // TODO: LED_DETECT_ON();
  // TODO: digitalWrite(LED_BUILTIN, HIGH);
  
  // Test en mode historique
  // Recherche des éléments de début, milieu et fin de donnée (0x0A, 0x20, 0x20, 0x0D)
  Serial2.begin(1200); // mode historique
  //Serial.println(F(">> Recherche mod TIC"));

  while (!flag_timeout && !flag_found_speed) {
    if (Serial2.available()>0) {
      char in = (char)Serial2.read() & 127;  // seulement sur 7 bits
      // TODO: change_etat_led(LED_BUILTIN);
	  
      #ifdef DEBUG_TIC
      if (isascii(in)) {
        Serial.print(in);
      } else {
        Serial.print("0x");
        Serial.println(in, HEX);
      }
      #endif
      // début trame
      if (in == 0x0A) {
        step = 1;
        nbc_etiq = -1;
        nbc_val = 0;
        #ifdef DEBUG_TIC
          Serial.println(F("0x0A: ceci est le début d'une donnée"));
        #endif
      }
	  
      // premier milieu de trame, étiquette
      if (step == 1) {
        if (in == 0x20) {
          #ifdef DEBUG_TIC
            Serial.print(F("0x20: ceci est une étiquette. nbchar="));
            Serial.println(nbc_etiq);
          #endif
          if (nbc_etiq > 3 && nbc_etiq < 10) step = 2; else step = 0;
        }
        else nbc_etiq++; // recupère nombre caractères de l'étiquette
      }
      else {
        // deuxième milieu de trame, valeur
        if (step == 2) {
          if (in == 0x20) {
            #ifdef DEBUG_TIC
              Serial.print(F("0x20: ceci est une valeur. nbchar="));
              Serial.println(nbc_val);
            #endif
            if (nbc_val > 0 && nbc_val < 13) step = 3; else step = 0;
          }
          else nbc_val++; // recupère nombre caractères de la valeur
        }
      }

      // fin trame
      if (step == 3 && in == 0x0D) {
        #ifdef DEBUG_TIC
          Serial.println(F("0x0D: ceci est une fin de donnée"));
        #endif
        flag_found_speed = true;
        step = 0;
      }
    }
    if (currentTime + 10000 <  millis()) {
      flag_timeout = true; // 10s de timeout
    }
  } // tant qu'on n'a pas identifié le mode & tant qu'on n'est pas en timeout

  //digitalWrite(LED_BLANC, LOW);

  if (flag_timeout == true && flag_found_speed == false) { // trame avec vitesse histo non trouvée donc passage en mode standard
    mode = TINFO_MODE_STANDARD;
    Serial2.end();
    Serial2.begin(9600); // mode standard
    //Serial.println(F("<< TIC en mode standard"));
  }
  else {
    mode = TINFO_MODE_HISTORIQUE;
    //Serial.println(F("<< TIC en mode historique"));
  }
  
  // TODO: digitalWrite(LED_BUILTIN, LOW);

  // TODO: LED_DETECT_OFF();
  return mode;
}

#endif