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

    // on ne peut pas modifier les valeurs dans la méthode update()
    // on va donc les mémoriser et ils seront appliqués dans la loop()
    if (bNewModeKnown) {
      if (bNewModeAuto) {
        LOG1("update->passer en AUTO\n");
        newCurrentMode = Characteristic::CurrentHeatingCoolingState::IDLE;
        newTargetMode = Characteristic::TargetHeatingCoolingState::AUTO;
        newTargetTemp = 10.0;
        bNeedToUpdateTarget = true;
      } else {
        LOG1("update->passer en HEAT\n");
        newCurrentMode = Characteristic::CurrentHeatingCoolingState::HEATING;
        newTargetMode = Characteristic::TargetHeatingCoolingState::HEAT;
        newTargetTemp = 21.0;
        bNeedToUpdateTarget = true;
      }
    }
    return true;
  }

  // Il est interdit de modifier des caractéristiques dans update car cela cause ce message:
  // *** WARNING:  Attempt to set value of Characteristic::TargetTemperature within update() while it is being simultaneously updated by Home App.  This may cause device to become non-responsive!
  // On se propose donc de "recadrer" ces valeurs dans la loop:
  // Cela corresponds au cas où l'utilisateur a modifié la température target via le slider
  // Comme l'objectif est de définir si on est en mode 'chauffage' ou 'auto', on ne gère que deux températures target:
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
      LOG1("loop->changer de mode vers:\n");
      if (newTargetMode == Characteristic::TargetHeatingCoolingState::AUTO) LOG1("targetMode=AUTO\n");
      else if (newTargetMode == Characteristic::TargetHeatingCoolingState::HEAT) LOG1("targetMode=HEAT\n");
      LOG1("targetTemp=%f\n", newTargetTemp);
      if (newCurrentMode == Characteristic::CurrentHeatingCoolingState::HEATING) LOG1("currentMode=HEATING\n");
      else if (newCurrentMode == Characteristic::CurrentHeatingCoolingState::IDLE) LOG1("currentMode=IDLE\n");
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
      LOG1("button->passer en mode AUTO\n");
      newCurrentMode = Characteristic::CurrentHeatingCoolingState::IDLE;
      newTargetMode = Characteristic::TargetHeatingCoolingState::AUTO;
      newTargetTemp = 10.0;
      bNeedToUpdateTarget = true;
    }else if (c == Characteristic::CurrentHeatingCoolingState::IDLE) {
      LOG1("button->passer en mode HEAT\n");
      newCurrentMode = Characteristic::CurrentHeatingCoolingState::HEATING;
      newTargetMode = Characteristic::TargetHeatingCoolingState::HEAT;
      newTargetTemp = 21.0;
      bNeedToUpdateTarget = true;
    }
  }

  boolean heatEnforced() {
    if (bNeedToUpdateTarget) {
      if (newTargetMode == Characteristic::TargetHeatingCoolingState::HEAT) return true; else return false;
    } else {
      boolean val = current->getVal() == Characteristic::CurrentHeatingCoolingState::HEATING;
      //if (val) LOG1("heatEnforced? YES\n"); else LOG1("heatEnforced? NO\n");
      return val;
    }
  }

  void enforceAuto() {
    if (bNeedToUpdateTarget) {
      LOG1("enforce AUTO (in var)\n");
      newCurrentMode = Characteristic::CurrentHeatingCoolingState::IDLE;
      newTargetMode = Characteristic::TargetHeatingCoolingState::AUTO;
      newTargetTemp = 10.0;
    } else if (current->getVal() != Characteristic::CurrentHeatingCoolingState::IDLE ||
        target->getVal() != Characteristic::TargetHeatingCoolingState::AUTO ||
        tTemp->getVal() < 9.9 || tTemp->getVal() > 10.1) {
      LOG1("enforce AUTO (in characteristics)\n");
      newCurrentMode = Characteristic::CurrentHeatingCoolingState::IDLE;
      newTargetMode = Characteristic::TargetHeatingCoolingState::AUTO;
      newTargetTemp = 10.0;
      bNeedToUpdateTarget = true;
    }
  }
};
