#include "homeSpan.h"
#include "pinout.h"

struct DEV_Thermostat: Service::Thermostat {

  SpanCharacteristic *current;
  SpanCharacteristic *target;
  SpanCharacteristic *cTemp;
  SpanCharacteristic *tTemp;
  SpanCharacteristic *cHumid;
  Adafruit_BME280 *bme;

  DEV_Thermostat(Adafruit_BME280 *bme280) {
    bme = bme280;
    current = new Characteristic::CurrentHeatingCoolingState(Characteristic::CurrentHeatingCoolingState::IDLE); // PR+EV    0(default)=IDLE, 1=HEATING, 2=COOLING
    current->setValidValues(2, Characteristic::CurrentHeatingCoolingState::IDLE, Characteristic::CurrentHeatingCoolingState::HEATING);
    target  = new Characteristic::TargetHeatingCoolingState(Characteristic::TargetHeatingCoolingState::AUTO);  // PW+PR+EV 0(default)=OFF, 1=HEAT, 2=COOL, 3=AUTO
    target->setValidValues(2, Characteristic::TargetHeatingCoolingState::HEAT, Characteristic::TargetHeatingCoolingState::AUTO);                             // valeurs supportées: HEAT & AUTO
    cTemp = new Characteristic::CurrentTemperature(0);           // PR+EV    default = 0°C
    tTemp = new Characteristic::TargetTemperature(11);           // PW+PR+EV default = 16°C
    new Characteristic::TemperatureDisplayUnits(0);              // 0(default)=CELSIUS
    cHumid = new Characteristic::CurrentRelativeHumidity(0);     // PR+EV    default = 0%

    new SpanButton(PIN_ANNUL_DELEST);                            // bouton pour annuler le délestage
  }

  // Certaines caractéristiques ne peuvent pas être modifiées dans la méthode update()
  // On les capture donc ici et elles seront updatées dans la méthode loop()
  // TODO: trouver une façon plus élégante de traiter ce point
  boolean bNeedToUpdateTarget = false;
  float newTargetTemp;
  int newTargetMode;
  int newCurrentMode;

  boolean update() override {
    bool bNewModeKnown = false;
    bool bNewModeAuto; // false = on annule le délestage

    // est-ce qu'on a joué avec la température target ?
    if (tTemp->updated()) {
      if (tTemp->getNewVal() >= 21) {
        bNewModeAuto = false;
        bNewModeKnown = true;
      } else if (tTemp->getNewVal() <= 16) {
        bNewModeAuto = true;
        bNewModeKnown = true;
      }
    }

    // est-ce qu'on a joué avec les boutons
    if (!bNewModeKnown && target->updated()) {
      if (target->getNewVal() == Characteristic::TargetHeatingCoolingState::HEAT) {
        bNewModeAuto = false;
        bNewModeKnown = true;
      } else if (target->getNewVal() == Characteristic::TargetHeatingCoolingState::AUTO) {
        bNewModeAuto = true;
        bNewModeKnown = true;
      }
    }

    // on applique les changements qu'on peut
    // en l'occurence, uniquement 'current'
    // on ne peut pas modifier 'target' et 'tTemp' immédiatement
    // TODO: faut-il checker les valeurs avant d'appliquer ?
    if (bNewModeKnown) {
      if (bNewModeAuto) {
        newCurrentMode = Characteristic::CurrentHeatingCoolingState::IDLE;
        newTargetMode = Characteristic::TargetHeatingCoolingState::AUTO;
        newTargetTemp = 10.0;
        bNeedToUpdateTarget = true;
      } else {
        newCurrentMode = Characteristic::CurrentHeatingCoolingState::HEATING;
        newTargetMode = Characteristic::TargetHeatingCoolingState::HEAT;
        newTargetTemp = 21.0;
        bNeedToUpdateTarget = true;
      }
    }
    return true;
  }

  // Il est interdit de modifier 'target' et 'tTemp' dans update car cela cause ce message:
  // *** WARNING:  Attempt to set value of Characteristic::TargetTemperature within update() while it is being simultaneously updated by Home App.  This may cause device to become non-responsive!
  // On se propose donc de "recadrer" sa valeur dans la loop:
  // Cela corresponds au cas où l'utilisateur a modifié la température target via le slider
  // Comme l'objectif est de définit si on est en mode 'chauffage' ou 'auto', on ne gère que deux températures target:
  // 21°C pour le mode 'chauffage' et 10°C pour le mode 'auto'
  uint32_t lastBMEread; // quand le BME280 a été lu la dernière fois
  void loop() override {
    // Propagation des infos du BME280 (température et humidité)
    if (bme != nullptr) {
      uint32_t currentTime = millis();
      if (currentTime - lastBMEread > ELAPSE_READ_BME) {
        cTemp->setVal(bme->readTemperature()); // valeur en °C
        cHumid->setVal(bme->readHumidity());   // valeur en %
        lastBMEread = currentTime;
      }
    }
    // Propagation des infos de délestage ou non
    if (bNeedToUpdateTarget) {
      target->setVal(newTargetMode);
      tTemp->setVal(newTargetTemp);
      current->setVal(newCurrentMode);
      bNeedToUpdateTarget = false;
    }
  }

  void button(int pin, int pressType) override {
    if (pin != PIN_ANNUL_DELEST) return;
    if (pressType != SpanButton::SINGLE) return;
    // On bascule entre le mode HEATING et IDLE
    int c = current->getVal();
    if (c == Characteristic::CurrentHeatingCoolingState::HEATING) {
      newCurrentMode = Characteristic::CurrentHeatingCoolingState::IDLE;
      newTargetMode = Characteristic::TargetHeatingCoolingState::AUTO;
      newTargetTemp = 10.0;
      bNeedToUpdateTarget = true;
    }else if (c == Characteristic::CurrentHeatingCoolingState::IDLE) {
      newCurrentMode = Characteristic::CurrentHeatingCoolingState::HEATING;
      newTargetMode = Characteristic::TargetHeatingCoolingState::HEAT;
      newTargetTemp = 21.0;
      bNeedToUpdateTarget = true;
    }
  }

  boolean heatEnforced() {
    return current->getVal() == Characteristic::CurrentHeatingCoolingState::HEATING;
  }

  void enforceAuto() {
    if (current->getVal() != Characteristic::CurrentHeatingCoolingState::IDLE ||
        target->getVal() != Characteristic::TargetHeatingCoolingState::AUTO ||
        tTemp->getVal() < 9.9 || tTemp->getVal() > 10.1) {
      newCurrentMode = Characteristic::CurrentHeatingCoolingState::IDLE;
      newTargetMode = Characteristic::TargetHeatingCoolingState::AUTO;
      newTargetTemp = 10.0;
      bNeedToUpdateTarget = true;
    }
  }
};
