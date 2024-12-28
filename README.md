# Linky_Tempo_HomeKit

** work in progress **  
Status actuel: démontrateur technique de la partie HomeKit sur platine d'essai

## Présentation

Ceci est la "V2" du projet Linky_Tempo.  
L'objectif est de pouvoir accéder au status du délestage ainsi qu'à la possibilité d'annuler ce délestage depuis Internet.

L'approche suivie sera de:
- remplacer l'Arduino par un ESP32
- installer HomeSpan sur l'ESP32 pour qu'il soit vu comme un device HomeKit
- le configurer comme un "Thermostat"

Pourquoi "Thermostat" ?  
Car cela semble être le device dont les fonctionnalités sont les plus proches de celles qu'on veut obtenir.  
Voici ses différents modes de fonctionnement (CurrentHeatingCoolingState) et leur usage prévu:
- IDLE: utilisé, délestage possible en fonction des infos du Linky
- HEATING: délestage annulé, chauffage actif
- COOLING: pas utilisé

Concernant la commande (TargetHeatingCoolingState) on a:
- OFF: pas utilisé
- HEAT: annulation du délestage, activer le chauffage (sauf si surconsommation)
- COOL: pas utilisé
- AUTO: mode automatique (délestage HPJR, HPJW et surconsommation)

Le circuit a également un bouton poussoir permettant de basculer entre le mode automatique et le mode annulation du délestage

Note: "Heater-Cooler" aurait peut-être pu être utilisé à la place de "Thermostat". Mais cette possibilité n'a pas été étudiée.

## Configuration

Factory reset:  
Faire un appui long (> 10 secondes) sur le bouton reset. Au bout de 3 secondes, la led interne va clignoter rapidement, continuer l'appui sur le bouton jusqu'à l'extinction de la led.

A ce moment, le device est configuré comme un point d'accès wifi sur lequel on peut
se connecter avec un ordinateur ou un smartphone. Voici les paramètres du point d'accès:
- nom du wifi: JCDELESTAGE
- mot de passe: 123456789

Via ce point d'accès, il est possible de:
- définit le réseau wifi sur lequel le device doit se connecter
- définir le setup-code homeKit du device

Une fois ces informations saisies, le device va redémarrer et ce connecteur à ce
réseau wifi. Une fois ceci effectué, la led bleue va flasher 2 fois toutes les 3 secondes
pour indiquer qu'il n'est pas pairé dans Maison.

## Les LEDs

LED rouge: toujours allumée, indique que l'ESP32 est alimenté  
LED bleue:  
- allumée fixe: le device est en fonctionnement et pairé dans Maison  
- allumée et clignotante (2 flashs brefs toutes les 3 secondes): le device n'est pas pairé dans Maison
