//
// ----------------------------------------------------------------
// Les différents Pins de l'ESP
// ----------------------------------------------------------------

// builtin led pour homeSpan
#define PIN_LED           2

// bouton poussoir pour homeSpan (pour faire le setup du device)
#define PIN_SETUP         5

// Sortie qui commande le fil pilote
#define PIN_PILOTE       19

// bouton poussoir pour annuler le délestage
// note: on ne peut annuler le délestage qu'en HPJW (heure pleine jour blanc)
#define PIN_ANNUL_DELEST 27

// LED jaune allumée si le delestage a été annulé
#define LED_ANNUL        32

// La TIC du linky est reçue sur Serial2()