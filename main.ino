#define PUSHPIN_RED 2    // Bouton rouge
#define PUSHPIN_GREEN 3  // Bouton vert
#define NUM_LEDS  1      // Nombre de LEDs (ici 1 LED)
#define SEALEVELPRESSURE_HPA (1013.25)  // Pression atmosphérique au niveau de la mer en hPa
#define inactivityTimeout 500000
#define longPressDuration  5000
#include <ChainableLED.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <RTClib.h>
#include <EEPROM.h>


volatile unsigned long lastPushRB = 0;
volatile unsigned long lastPushGB = 0;
long lastActivityTime = 0;
volatile unsigned long lastLogTime = 0;
volatile bool RBPushed = false;
volatile bool GBPushed = false;


// Constantes pour les LED (PROGMEM)

const char msgModeStandard[] PROGMEM = "Mode Standard";
const char msgModeConfig[] PROGMEM = "Mode Configuration";
const char msgModeMaint[] PROGMEM = "Mode Maintenance";
const char msgModeEco[] PROGMEM = "Mode Économique";
const char msgRtcPerdu[] PROGMEM = "RTC a perdu l'heure, initialisation à l'heure par défaut.";
const char msgRtcPasTrouve[] PROGMEM = "RTC ne peut pas être trouvé.";
const char msgEntreCommande[] PROGMEM = "Mode de configuration prêt. Entrez une commande :";
const char msgVersion[] PROGMEM = "Version du programme: 1.0.0, Numéro de lot: 123456";
const char msgNonReconnue[] PROGMEM = "Commande non reconnue: ";
const char msgDateMaj[] PROGMEM = "Date mise à jour: ";
const char msgHeureMaj[] PROGMEM = "Heure mise à jour: ";
const char msgConfigReset[] PROGMEM = "Tous les paramètres ont été réinitialisés aux valeurs par défaut.";
const char msgStandardAfk[] PROGMEM = "Passage au mode Standard après inactivité.";
const char msgLogIntervalMaj[] PROGMEM = "Intervalle de journalisation mis à jour: ";
const char msgFichierMaxMaj[] PROGMEM = "Taille maximale du fichier de log mise à jour: ";
const char msgTimeoutMaj[] PROGMEM = "Timeout capteur mis à jour: ";
const char msgLuminMaj[] PROGMEM = "LUMIN mis à jour: ";
const char msgLuminLowMaj[] PROGMEM = "LUMIN_LOW mis à jour: ";
const char msgLuminHighMaj[] PROGMEM = "LUMIN_HIGH mis à jour: ";
const char msgTempAirMaj[] PROGMEM = "TEMP_AIR mis à jour: ";
const char msgMinTempAirMaj[] PROGMEM = "MIN_TEMP_AIR mis à jour: ";
const char msgMaxTempAirMaj[] PROGMEM = "MAX_TEMP_AIR mis à jour: ";
const char msgHygrMaj[] PROGMEM = "HYGR mis à jour: ";
const char msgHygrMinTMaj[] PROGMEM = "HYGR_MINT mis à jour: ";
const char msgHygrMaxTMaj[] PROGMEM = "HYGR_MAXT mis à jour: ";
const char msgPressureMaj[] PROGMEM = "PRESSURE mis à jour: ";
const char msgPressureMinMaj[] PROGMEM = "PRESSURE_MIN mis à jour: ";
const char msgPressureMaxMaj[] PROGMEM = "PRESSURE_MAX mis à jour: ";
const char msgLuminoSensor[] PROGMEM = "Valeur du capteur de luminosité : ";
const char msgBmeErreur[] PROGMEM = "Erreur de communication avec le capteur BME280 !";

const char msgBmeDonnee[] PROGMEM = "=== Données du capteur BME280 ===";
const char msgTemperature[] PROGMEM = "Température : ";
const char msgHumidite[] PROGMEM = "Humidité relative : ";
const char msgPression[] PROGMEM = "Pression atmosphérique : ";
const char msgAltitude[] PROGMEM = "Altitude estimée : ";
const char msgGpsDonnee[] PROGMEM = "=== Données du GPS ===";

uint8_t INTERVAL = 5;
int FILE_MAX_SIZE = 4096;
int TIMEOUT = 30;

int LUMIN = 1, LUMIN_LOW = 255, LUMIN_HIGH = 768;
uint8_t TEMP_AIR = 1, MIN_TEMP_AIR = -10, MAX_TEMP_AIR = 60;
uint8_t HYGR = 1, HYGR_MINT = 0, HYGR_MAXT = 70;
int PRESSURE = 1, PRESSURE_MIN = 850, PRESSURE_MAX = 1080;

ChainableLED leds(7, 8, NUM_LEDS);
SoftwareSerial SoftSerial(4, 5);  // Port série logiciel
Adafruit_BME280 bme;              // Capteur BME280
RTC_DS3231 rtc;


enum Mode {
  MODE_STANDARD,
  MODE_CONFIGURATION,
  MODE_MAINTENANCE,
  MODE_ECONOMIQUE
};

Mode currentMode = MODE_STANDARD;

void printProgmemMessage(const char* message) {
  char buffer[128];              // Crée un tableau de 128 caractères en RAM pour stocker temporairement le message.
  strcpy_P(buffer, message);      // Copie le message depuis la mémoire flash (PROGMEM) dans le tampon RAM `buffer`.
  Serial.println(buffer);         // Affiche le message depuis le tampon RAM via le port série.
}


void checkInactivity() {
  if (currentMode == MODE_CONFIGURATION && millis() - lastPushRB >= inactivityTimeout) {
    currentMode = MODE_STANDARD;
    printProgmemMessage(msgModeStandard);
  }
}

void interruptRB() {
  if (digitalRead(PUSHPIN_RED) == LOW) {
    lastPushRB = millis();
    RBPushed = true;
  } else {
    RBPushed = false;
  }
}

void interruptGB() {
  if (digitalRead(PUSHPIN_GREEN) == LOW) {
    lastPushGB = millis();
    GBPushed = true;
  } else {
    GBPushed = false;
  }
}

bool longPushButton(volatile unsigned long &lastPush, volatile bool &pushButton) {
  if (pushButton && millis() - lastPush >= longPressDuration) {
    while (digitalRead(PUSHPIN_RED) == LOW || digitalRead(PUSHPIN_GREEN) == LOW) {
      delay(10);  // Petit délai pour éviter de bloquer
    }
    delay(500); // Délai pour éviter l'effet rebond
    pushButton = false;
    return true;
  }
  return false;
}

void changeLed() {
  switch (currentMode) {
    case MODE_STANDARD:
      leds.setColorHSL(0, 0.33, 1.0, 0.5);  // Vert
      break;
    case MODE_CONFIGURATION:
      leds.setColorHSL(0, 0.17, 1.0, 0.5);  // Jaune
      break;
    case MODE_MAINTENANCE:
      leds.setColorHSL(0, 0.06, 1.0, 0.5);  // Orange
      break;
    case MODE_ECONOMIQUE:
      leds.setColorHSL(0, 0.66, 1.0, 0.5);  // Bleu
      break;
  }
}

void setClock(char* time_str) {
  int h, m, s;
  sscanf(time_str, "%d:%d:%d", &h, &m, &s);
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
  
  Serial.print("Heure mise à jour: ");
  if (h < 10) Serial.print('0'); Serial.print(h); Serial.print(':');
  if (m < 10) Serial.print('0'); Serial.print(m); Serial.print(':');
  if (s < 10) Serial.println('0'); Serial.println(s);
}

void setDate(char* time_str) {
  int h, m, s;
  sscanf(time_str, "%d:%d:%d", &h, &m, &s);
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
  
  printProgmemMessage(msgHeureMaj);
  if (h < 10) Serial.print('0'); Serial.print(h); Serial.print(':');
  if (m < 10) Serial.print('0'); Serial.print(m); Serial.print(':');
  if (s < 10) Serial.println('0'); Serial.println(s);
}

void lux(int LUMIN, int LUMIN_LOW, int LUMIN_HIGH) {
  if (LUMIN == 1){
    int sensorValue = analogRead(0);
    if (sensorValue <= LUMIN_HIGH && sensorValue >= LUMIN_LOW){
      printProgmemMessage(msgLuminoSensor);
      Serial.println(sensorValue);  // Afficher la valeur du capteur
    }
    else {
      while (1) {
      leds.setColorHSL(0, 0.0, 1.0, 0.5);  // Rouge (alerte)
      delay(1000);
      leds.setColorHSL(0, 0.33, 1.0, 0.5);  // Vert (normal)
      delay(1000);
    }
    }
  }
}

void bmesensor(int TEMP_AIR, int MIN_TEMP_AIR, int MAX_TEMP_AIR, int HYGR, int HYGR_MINT, int HYGR_MAXT, int PRESSURE, int PRESSURE_MIN, int PRESSURE_MAX) {
  if (!bme.begin(0x76)) {
    printProgmemMessage(msgBmeErreur);
    while (1) {
      leds.setColorHSL(0, 0.0, 1.0, 0.5);  // Rouge (alerte)
      delay(1000);
      leds.setColorHSL(0, 0.33, 1.0, 0.5);  // Vert (normal)
      delay(1000);
    }
  }
  if (TEMP_AIR == 1) {
      printProgmemMessage(msgBmeDonnee);
      printProgmemMessage(msgTemperature);
      Serial.print(bme.readTemperature());
      Serial.println(F(" °C"));
  }

  if (HYGR == 1) {
    printProgmemMessage(msgHumidite);
    Serial.print(bme.readHumidity());
    Serial.println(F(" %"));
    }
  
  if (PRESSURE == 1) {
    printProgmemMessage(msgPression);
    Serial.print(bme.readPressure()/ 100.0F);
    Serial.println(F(" hPa"));
    }


  printProgmemMessage(msgAltitude);
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(F(" m"));
  Serial.println(F("==============================="));
}

void gps() {
  if (SoftSerial.available()) {  // Si des données arrivent du GPS
    printProgmemMessage(msgGpsDonnee);
    while (SoftSerial.available()) {
      char c = SoftSerial.read();  // Lire un caractère depuis le GPS
      Serial.write(c);  // Afficher immédiatement le caractère
    }
    Serial.println();  // Nouvelle ligne après avoir affiché les données
    Serial.println("=====================");
  }

  if (Serial.available()) {  // Si des données arrivent du port série (PC)
    SoftSerial.write(Serial.read());  // Les transférer au GPS
  }
}

void writeIntToEEPROM(int address, int value) {
  EEPROM.put(address, value);
}

// Fonction pour lire un entier depuis l'EEPROM
int readIntFromEEPROM(int address) {
  int value;
  EEPROM.get(address, value);
  return value;
}

// Chargement des paramètres depuis l'EEPROM
void loadConfigFromEEPROM() {
  INTERVAL = readIntFromEEPROM(0);
  FILE_MAX_SIZE = readIntFromEEPROM(4);
  TIMEOUT = readIntFromEEPROM(8);

  LUMIN = readIntFromEEPROM(12);
  LUMIN_LOW = readIntFromEEPROM(16);
  LUMIN_HIGH = readIntFromEEPROM(20);

  TEMP_AIR = readIntFromEEPROM(24);
  MIN_TEMP_AIR = readIntFromEEPROM(28);
  MAX_TEMP_AIR = readIntFromEEPROM(32);

  HYGR = readIntFromEEPROM(36);
  HYGR_MINT = readIntFromEEPROM(40);
  HYGR_MAXT = readIntFromEEPROM(44);

  PRESSURE = readIntFromEEPROM(48);
  PRESSURE_MIN = readIntFromEEPROM(52);
  PRESSURE_MAX = readIntFromEEPROM(56);

  // Valeurs par défaut si EEPROM vide
  if (INTERVAL == 0xFFFF) INTERVAL = 10;
  if (FILE_MAX_SIZE == 0xFFFF) FILE_MAX_SIZE = 4096;
  if (TIMEOUT == 0xFFFF) TIMEOUT = 30;

  if (LUMIN == 0xFFFF) LUMIN = 1;
  if (LUMIN_LOW == 0xFFFF) LUMIN_LOW = 255;
  if (LUMIN_HIGH == 0xFFFF) LUMIN_HIGH = 768;

  if (TEMP_AIR == 0xFFFF) TEMP_AIR = 1;
  if (MIN_TEMP_AIR == 0xFFFF) MIN_TEMP_AIR = -10;
  if (MAX_TEMP_AIR == 0xFFFF) MAX_TEMP_AIR = 60;

  if (HYGR == 0xFFFF) HYGR = 1;
  if (HYGR_MINT == 0xFFFF) HYGR_MINT = 0;
  if (HYGR_MAXT == 0xFFFF) HYGR_MAXT = 50;

  if (PRESSURE == 0xFFFF) PRESSURE = 1;
  if (PRESSURE_MIN == 0xFFFF) PRESSURE_MIN = 850;
  if (PRESSURE_MAX == 0xFFFF) PRESSURE_MAX = 1080;
}

// Sauvegarde des paramètres dans l'EEPROM
void saveConfigToEEPROM() {
  writeIntToEEPROM(0, INTERVAL);
  writeIntToEEPROM(4, FILE_MAX_SIZE);
  writeIntToEEPROM(8, TIMEOUT);

  writeIntToEEPROM(12, LUMIN);
  writeIntToEEPROM(16, LUMIN_LOW);
  writeIntToEEPROM(20, LUMIN_HIGH);

  writeIntToEEPROM(24, TEMP_AIR);
  writeIntToEEPROM(28, MIN_TEMP_AIR);
  writeIntToEEPROM(32, MAX_TEMP_AIR);

  writeIntToEEPROM(36, HYGR);
  writeIntToEEPROM(40, HYGR_MINT);
  writeIntToEEPROM(44, HYGR_MAXT);

  writeIntToEEPROM(48, PRESSURE);
  writeIntToEEPROM(52, PRESSURE_MIN);
  writeIntToEEPROM(56, PRESSURE_MAX);
}

void processCommand(char* command) {
  if (strncmp(command, "CLOCK=", 6) == 0) {
    setClock(command + 6);
    saveConfigToEEPROM();
  } else if (strncmp(command, "DATE=", 5) == 0) {
    setDate(command + 5);
    saveConfigToEEPROM();
  } else if (strncmp(command, "INTERVAL=", 14) == 0) {
    INTERVAL = atoi(command + 14);
    saveConfigToEEPROM();  // Sauvegarder dans l'EEPROM
    printProgmemMessage(msgLogIntervalMaj);
    Serial.println(INTERVAL);
  } else if (strncmp(command, "FILE_MAX_SIZE=", 14) == 0) {
    FILE_MAX_SIZE = atoi(command + 14);
    saveConfigToEEPROM();  // Sauvegarder dans l'EEPROM
    printProgmemMessage(msgFichierMaxMaj);
    Serial.println(FILE_MAX_SIZE);
  } else if (strncmp(command, "TIMEOUT=", 8) == 0) {
    TIMEOUT = atoi(command + 8);
    saveConfigToEEPROM();  // Sauvegarder dans l'EEPROM
    printProgmemMessage(msgTimeoutMaj);
    Serial.println(TIMEOUT);
  } else if (strncmp(command, "LUMIN=", 6) == 0) {
    LUMIN = atoi(command + 6);
    saveConfigToEEPROM();
    printProgmemMessage(msgLuminMaj);
    Serial.println(LUMIN);
  } else if (strncmp(command, "LUMIN_LOW=", 10) == 0) {
    if (atoi(command + 10)>LUMIN_HIGH){
        Serial.println("Erreur saisie");
    } else {
        LUMIN_LOW = atoi(command + 13);
        saveConfigToEEPROM();
        printProgmemMessage(msgLuminLowMaj);
        Serial.println(LUMIN_LOW);
    }
  } else if (strncmp(command, "LUMIN_HIGH=", 11) == 0) {
    if (atoi(command + 11)<LUMIN_LOW){
        Serial.println("Erreur saisie");
    } else {
        LUMIN_HIGH = atoi(command + 13);
        saveConfigToEEPROM();
        printProgmemMessage(msgLuminHighMaj);
        Serial.println(LUMIN_HIGH);
    }
  } else if (strncmp(command, "TEMP_AIR=", 9) == 0) {
    TEMP_AIR = atoi(command + 9);
    saveConfigToEEPROM();
    printProgmemMessage(msgTempAirMaj);
    Serial.println(TEMP_AIR);
  } else if (strncmp(command, "MIN_TEMP_AIR=", 13) == 0) {
    if (atoi(command + 13)>MAX_TEMP_AIR){
        Serial.println("Erreur saisie");
    } else {
        MIN_TEMP_AIR = atoi(command + 13);
        saveConfigToEEPROM();
        printProgmemMessage(msgMinTempAirMaj);
        Serial.println(MIN_TEMP_AIR);
    }
  } else if (strncmp(command, "MAX_TEMP_AIR=", 13) == 0) {
    if (atoi(command + 13)<MIN_TEMP_AIR){
        Serial.println("Erreur saisie");
    } else {
        MAX_TEMP_AIR = atoi(command + 13);
        saveConfigToEEPROM();
        printProgmemMessage(msgMaxTempAirMaj);
        Serial.println(MAX_TEMP_AIR);
    }
  } else if (strncmp(command, "HYGR=", 5) == 0) {
    HYGR = atoi(command + 5);
    saveConfigToEEPROM();
    printProgmemMessage(msgHygrMaj);
    Serial.println(HYGR);
  } else if (strncmp(command, "HYGR_MINT=", 10) == 0) {
    if (atoi(command + 10)>HYGR_MAXT){
        Serial.println("Erreur saisie");
    } else {
        HYGR_MINT = atoi(command + 10);
        saveConfigToEEPROM();
        printProgmemMessage(msgHygrMinTMaj);
        Serial.println(HYGR_MINT);
    }
  } else if (strncmp(command, "HYGR_MAXT=", 10) == 0) {
    if (atoi(command + 10)<HYGR_MINT){
        Serial.println("Erreur saisie");
    } else {
        HYGR_MAXT = atoi(command + 10);
        saveConfigToEEPROM();
        printProgmemMessage(msgHygrMaxTMaj);
        Serial.println(HYGR_MAXT);
    }
  } else if (strncmp(command, "PRESSURE=", 9) == 0) {
    PRESSURE = atoi(command + 9);
    saveConfigToEEPROM();
    printProgmemMessage(msgPressureMaj);
    Serial.println(PRESSURE);
  } else if (strncmp(command, "PRESSURE_MIN=", 13) == 0) {
    if (atoi(command + 13)>PRESSURE_MAX){
        Serial.println("Erreur saisie");
    } else {
        PRESSURE_MIN = atoi(command + 13);  // Correction ici
        saveConfigToEEPROM();
        printProgmemMessage(msgPressureMinMaj);  // Assurez-vous que ce message existe
        Serial.println(PRESSURE_MIN);  // Assurez-vous d'afficher la bonne variable
    }
  } else if (strncmp(command, "PRESSURE_MAX=", 13) == 0) {
    if (atoi(command + 13)<PRESSURE_MIN){
        Serial.println("Erreur saisie");
    } else {
        PRESSURE_MAX = atoi(command + 13);
        saveConfigToEEPROM();
        printProgmemMessage(msgPressureMaxMaj);
        Serial.println(PRESSURE_MAX);
    }
  } else if (strncmp(command, "RESET", 5) == 0) {
    INTERVAL = 10;
    FILE_MAX_SIZE = 4096;
    TIMEOUT = 30;
    LUMIN = 1; LUMIN_LOW = 255; LUMIN_HIGH = 768;
    TEMP_AIR = 1; MIN_TEMP_AIR = -10; MAX_TEMP_AIR = 60;
    HYGR = 1; HYGR_MINT = 0; HYGR_MAXT = 70;
    PRESSURE = 1; PRESSURE_MIN = 850; PRESSURE_MAX = 1080;
    saveConfigToEEPROM();  // Sauvegarder les valeurs par défaut dans l'EEPROM
    printProgmemMessage(msgConfigReset);
  } else if (strncmp(command, "VERSION", 7) == 0) {
    printProgmemMessage(msgVersion);
  } else {
    printProgmemMessage(msgNonReconnue);
    Serial.println(command);
  }
}


void setup() {
  Serial.begin(9600);
  SoftSerial.begin(9600);
  pinMode(PUSHPIN_RED, INPUT_PULLUP);
  pinMode(PUSHPIN_GREEN, INPUT_PULLUP);
  leds.init();
  attachInterrupt(digitalPinToInterrupt(PUSHPIN_RED), interruptRB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PUSHPIN_GREEN), interruptGB, CHANGE);
  if (digitalRead(PUSHPIN_RED) == LOW) {
    lastActivityTime = millis();
    printProgmemMessage(msgModeConfig);
    currentMode = MODE_CONFIGURATION;
  // Initialiser en mode Standard
  }else {
  printProgmemMessage(msgModeStandard);
  currentMode = MODE_STANDARD;
  }
}

void loop() {
  changeLed();
  unsigned long currentTime = millis();
  
  if (currentTime - lastLogTime >= INTERVAL * 1000) {
    lastLogTime = currentTime;  // Update lastLogTime here
    
    switch (currentMode) {
      case MODE_STANDARD:
        gps();
        lux(LUMIN,LUMIN_LOW,LUMIN_HIGH);
        bmesensor(TEMP_AIR,MIN_TEMP_AIR,MAX_TEMP_AIR,HYGR,HYGR_MINT,HYGR_MAXT,PRESSURE,PRESSURE_MIN,PRESSURE_MAX);
        if (longPushButton(lastPushRB, RBPushed)) {
          printProgmemMessage(msgModeMaint);
          currentMode = MODE_MAINTENANCE;
        } else if (longPushButton(lastPushGB, GBPushed)) {
          printProgmemMessage(msgModeEco);
          currentMode = MODE_ECONOMIQUE;
        }
        break;

      case MODE_CONFIGURATION:
        printProgmemMessage(msgEntreCommande);
        if (Serial.available() > 0) {
          char command[100];
          int len = Serial.readBytesUntil('\n', command, sizeof(command) - 1);
          command[len] = '\0';
          processCommand(command);
          lastActivityTime = millis();
        }
        if (millis() - lastActivityTime >= inactivityTimeout) {
          printProgmemMessage(msgStandardAfk);
          currentMode = MODE_STANDARD;
        }
        break;

      case MODE_MAINTENANCE:
      delay(2000);
        gps();
        lux(LUMIN,LUMIN_LOW,LUMIN_HIGH);
        bmesensor(TEMP_AIR,MIN_TEMP_AIR,MAX_TEMP_AIR,HYGR,HYGR_MINT,HYGR_MAXT,PRESSURE,PRESSURE_MIN,PRESSURE_MAX);
        if (longPushButton(lastPushRB, RBPushed)) {
          printProgmemMessage(msgModeStandard);
          currentMode = MODE_STANDARD;
        }
        break;

      case MODE_ECONOMIQUE:
        bmesensor(TEMP_AIR,MIN_TEMP_AIR,MAX_TEMP_AIR,HYGR,HYGR_MINT,HYGR_MAXT,PRESSURE,PRESSURE_MIN,PRESSURE_MAX);
        if (longPushButton(lastPushRB, RBPushed)) {
          printProgmemMessage(msgModeMaint);
          currentMode = MODE_MAINTENANCE;
        } else if (longPushButton(lastPushGB, GBPushed)) {
          printProgmemMessage(msgModeStandard);
          currentMode = MODE_STANDARD;
        }
        break;
    }
  }
}
