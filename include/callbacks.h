#ifndef callbacks_h
#define callbacks_h

#include <Arduino.h>
#include "LibTeleinfo.h"
#include "constants.h"
//#include "config.h"

// ---------------------------------------------------------------- 
// Gestion du délestage
// On se met en délestage lorsqu'on recoit la trame PETC=HPJR
// ou HPJW
// On se met en délestage lorsque IINST > 80% de ISOUSC
// On a donc deux raisons différentes de passer en mode délestage
// Une fois en délestage, on y reste au moins 5mn
// ---------------------------------------------------------------- 

// Délestage ?
bool delestage_HPJR = false;
bool delestage_HPJW = false;
bool delestage_CONSO = false;
// Si délestage, c'est depuis quand, basé sur milli()
//uint32_t startDelestage;

// Certaines valeurs de la TIC sont des nombres
bool isNumber(char *val) {
  char *p = val;
  while (*p) {
    if ( *p < '0' || *p > '9' ) return false;
    p++;
  }
  return true;
}

// ---------------------------------------------------------------- 
// Callback appelé si détection d'un ADPS
// On ne va pas utiliser cette information car on veut être alerté
// plus tôt
// Donc, au lieu de surveiller le message ADPS, on va surveiller le
// message PAPP ce qui nous permettra de définir plus finement à
// quel niveau on déclenche le délestage
// Note: PAPP = Puissance apparente
// TODO: ou devrait-on se baser sur IINST = Intensité Instantanée ?
// ---------------------------------------------------------------- 
void AttachADPSCallback(uint8_t phase) {
  //printUptime();

  // Monophasé
  if (phase == 0 ) {
    //Serial.println("MESSAGE DE ADPS (Avertissement de Dépassement De Puissance Souscrite)");
  }
  else {
    //Serial.print(F("ADPS PHASE #"));
    //Serial.println('0' + phase);
  }
}

// ---------------------------------------------------------------- 
// Callback appelé lorsque la TIC a envoyé un frame complet
// TODO: décider quel callback utiliser. A priori il est recommendé d'utiliser ce callback
//       cf: https://hallard.me/libteleinfo/
// On utilise ce callback pour détecter un risque de dépassement de
// la puissance souscrite
// ---------------------------------------------------------------- 
void UpdatedFrameCallback(ValueList * one) {
  // protection
  if (nullptr == one) return;

  unsigned long isousc = 0;
  unsigned long iinst = 0;
  // on passe en revue toutes les valeurs
  // On s'arrête lorsqu'on a récupéré ISOUSC et IINST
  while(one) {
    if (!strcmp(one->name, "ISOUSC") && isNumber(one->value)) {
      isousc = atol(one->value);
      if (isousc > 0 && iinst > 0) break;
    }
    if (!strcmp(one->name, "IINST") && isNumber(one->value)) {
      iinst = atol(one->value);
      if (isousc > 0 && iinst > 0) break;
    }
    one = one->next;
  }
  // On ne peut rien faire si on n'a pas récupéré les deux valeurs
  if (isousc == 0 || iinst == 0) return;

  if (iinst > isousc*PERCENT_DELESTAGE/100) {
    delestage_CONSO = true;
    //startDelestage = millis();
    //Serial.println("PASSAGE EN MODE DELESTAGE de surconsommation");
    return;
  } else if (delestage_CONSO) {
    delestage_CONSO = false;
    //Serial.println("ANNULATION DELESTAGE de surconsommation");
  }
}

// ---------------------------------------------------------------- 
// Callback appelé lorsque la TIC a envoyé une nouvelle valeur
// On utilise ce callback pour détecter au plus tôt le passage
// aux heures pleines d'un jour blanc ou rouge
// Par sécurité (au cas où on reçoit des valeurs inconnues), on
// ne supprime le délestage que si on a reçu un PTEC reconnu
// ---------------------------------------------------------------- 
void AttachDataCallback(ValueList * me, uint8_t flags) {
  // protection
  if (nullptr == me) return;
  // réception d'un PTEC ?
  if (!strcmp(me->name, "PTEC")) {
    // deux cas nous intéressent: HPJW & HPJR (heure pleine, jour blanc ou rouge)
    // ou l'un des 4 autres cas (HCJW, HCJR, HPJB, HCJB)
    if (!strcmp(me->value, PERIODE_DELESTAGE_W)) {
      // On est en Heure Pleine Jour Blanc. Il faut délester
      delestage_HPJW = true;
      //startDelestage = millis();
      //println("PASSAGE EN MODE DELESTAGE d'heure pleine jour blanc");
    } else if (!strcmp(me->value, PERIODE_DELESTAGE_R)) {
      // On est en Heure Pleine Jour Rouge. Il faut délester
      delestage_HPJR = true;
      //startDelestage = millis();
      //println("PASSAGE EN MODE DELESTAGE d'heure pleine jour rouge");
    } else if (!strcmp(me->value, "HCJR") ||
               !strcmp(me->value, "HCJW") ||
               !strcmp(me->value, "HPJB") || !strcmp(me->value, "HCJB")) {
      // On n'est pas en Heure Pleine Jour Blanc ou Rouge. Pas besoin de délester
      if (delestage_HPJW) {
        delestage_HPJW = false;
        //Serial.println("ANNULATION DELESTAGE d'heure pleine jour blanc");
      }
      if (delestage_HPJR) {
        delestage_HPJR = false;
        //Serial.println("ANNULATION DELESTAGE d'heure pleine jour rouge");
      }
    }
  }
}

#endif