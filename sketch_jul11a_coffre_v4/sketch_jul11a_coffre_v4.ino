#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Preferences.h> // Pour sauvegarder en mémoire ESP32

// ===== CONFIG HARDWARE =====
#define SS_PIN 5
#define RST_PIN 17
MFRC522 rfid(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo monServo;
Preferences preferences; // Pour stocker badges et PIN

const byte ROWS = 4; const byte COLS = 4;
char keys[ROWS] [COLS]= {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[] = {13, 12, 14, 27};
byte colPins[] = {26, 25, 33, 32};
Keypad clavier = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#define LED_VERTE 2
#define LED_ROUGE 4
#define BUZZER 16
#define SERVO_PIN 15

// ===== VARIABLES =====
String codePinAdmin = "1234";
String codeSaisi = "";
int nbBadges = 0;
String badges[10]; // Max 10 badges
unsigned long tempsBlocage = 0;
int tentatives = 0;
bool modeAdmin = false;

// ===== SETUP =====
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  lcd.init(); lcd.backlight();
  monServo.attach(SERVO_PIN); monServo.write(0);

  pinMode(LED_VERTE, OUTPUT); pinMode(LED_ROUGE, OUTPUT); pinMode(BUZZER, OUTPUT);

  preferences.begin("coffre", false);
  chargerDonnees(); // Charge badges et PIN sauvegardés

  bip(100);
  lcd.print("Coffre V4 Demarre");
  delay(2000);
}

// ===== LOOP =====
void loop() {
  if(millis() < tempsBlocage){
    lcd.clear(); lcd.print("BLOQUE 1 MIN");
    lcd.setCursor(0,1); lcd.print((tempsBlocage - millis())/1000);
    delay(1000); return;
  }

  lcd.clear(); lcd.print("Passez Badge ou ");
  lcd.setCursor(0,1); lcd.print("*Menu Admin");

  if(verifierBadge()) return;
  if(verifierToucheMenu()) return;
}

// ===== FONCTIONS PRINCIPALES =====
bool verifierBadge(){
  if(!rfid.PICC_IsNewCardPresent() ||!rfid.PICC_ReadCardSerial()) return false;

  String uid = lireUID();
  bip(50);

  if(estBadgeValide(uid)){
    demanderPIN(uid);
  } else {
    accesRefuse();
  }
  rfid.PICC_HaltA();
  return true;
}

bool verifierToucheMenu(){
  char touche = clavier.getKey();
  if(touche == '*'){
    modeAdmin = true;
    menuAdmin();
    modeAdmin = false;
    return true;
  }
  return false;
}

void demanderPIN(String uid){
  codeSaisi = "";
  lcd.clear(); lcd.print("Entrez PIN:");
  while(codeSaisi.length() < 4){
    char t = clavier.getKey();
    if(t && t!= '#' && t!= '*' && t!= 'A' && t!= 'B' && t!= 'C' && t!= 'D'){
      codeSaisi += t; lcd.print("*"); bip(50);
    }
    if(t == '#') return; // Annuler
  }

  if(codeSaisi == codePinAdmin){
    ouvrirCoffre();
  } else {
    accesRefuse();
  }
}

void ouvrirCoffre(){
  tentatives = 0;
  digitalWrite(LED_VERTE, HIGH);
  monServo.write(90); lcd.clear(); lcd.print("Coffre OUVERT");
  bip(200); delay(3000);
  monServo.write(0); digitalWrite(LED_VERTE, LOW);
}

void accesRefuse(){
  tentatives++;
  digitalWrite(LED_ROUGE, HIGH); bip(500);
  lcd.clear(); lcd.print("ACCES REFUSE");
  delay(1000); digitalWrite(LED_ROUGE, LOW);

  if(tentatives >= 3){
    tempsBlocage = millis() + 60000; // 1 min
    tentatives = 0;
  }
}

// ===== MENU ADMIN =====
void menuAdmin(){
  lcd.clear(); lcd.print("1:Chg PIN 2:Badge");
  lcd.setCursor(0,1); lcd.print("3:Suppr Badge #");

  while(true){
    char t = clavier.getKey();
    if(t == '1') changerPIN();
    if(t == '2') ajouterBadge();
    if(t == '3') supprimerBadge();
    if(t == '#') break;
  }
}

void changerPIN(){
  lcd.clear(); lcd.print("Ancien PIN:");
  String ancien = saisirPIN();
  if(ancien!= codePinAdmin){ accesRefuse(); return; }

  lcd.clear(); lcd.print("Nouveau PIN:");
  String nouveau = saisirPIN();
  codePinAdmin = nouveau;
  preferences.putString("pin", codePinAdmin);
  lcd.print("OK!"); bip(100); delay(1000);
}

void ajouterBadge(){
  lcd.clear(); lcd.print("Entrez PIN Admin:");
  if(saisirPIN()!= codePinAdmin){ accesRefuse(); return; }

  lcd.clear(); lcd.print("Passez nouveau");
  lcd.setCursor(0,1); lcd.print("Badge...");
  delay(500); // Laisse le temps d'enlever l'ancien badge

  while(!rfid.PICC_IsNewCardPresent()){} // Attend
  while(!rfid.PICC_ReadCardSerial()){} // Lit

  String newUID = lireUID(); // lireUID fait déjà HaltA

  // Vérifie si badge existe déjà
  if(estBadgeValide(newUID)){
    lcd.clear(); lcd.print("Badge deja existe");
    delay(1500); return;
  }

  if(nbBadges < 10){
    badges[nbBadges] = newUID; nbBadges++;
    sauvegarderBadges();
    lcd.clear(); lcd.print("Badge Ajoute!");
    bip(100); delay(1000);
  } else {
    lcd.clear(); lcd.print("Memoire pleine!");
    delay(1500);
  }
}

void supprimerBadge(){
  lcd.clear(); lcd.print("Entrez PIN Admin:");
  if(saisirPIN()!= codePinAdmin){ accesRefuse(); return; }

  lcd.clear(); lcd.print("Passez Badge a");
  lcd.setCursor(0,1); lcd.print("Supprimer");
  delay(500);

  while(!rfid.PICC_IsNewCardPresent()){}
  while(!rfid.PICC_ReadCardSerial()){}

  String delUID = lireUID(); // lireUID fait déjà HaltA

  if(delUID == badges[0]){ // Badge 0 = Admin principal
    lcd.clear(); lcd.print("Admin: Non Supp");
    delay(1500); return;
  }

  for(int i=0; i<nbBadges; i++){
    if(badges[i] == delUID){
      for(int j=i; j<nbBadges-1; j++) badges[j] = badges[j+1];
      nbBadges--; sauvegarderBadges();
      lcd.clear(); lcd.print("Badge Supprime!");
      bip(100); delay(1000); return;
    }
  }
  lcd.clear(); lcd.print("Badge introuvable");
  delay(1500);
}

// ===== FONCTIONS UTILES =====
String saisirPIN(){
  String pin = "";
  while(pin.length() < 4){
    char t = clavier.getKey();
    if(t && t >= '0' && t <= '9'){ pin += t; lcd.print("*"); bip(50); }
  }
  return pin;
}
String lireUID(){
  String uid = "";
  for(byte i = 0; i < rfid.uid.size; i++) {
    uid += (rfid.uid.uidByte[i] < 0x10? "0" : "") + String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  rfid.PICC_HaltA(); 
  rfid.PCD_StopCrypto1();
  return uid;
}

bool estBadgeValide(String uid){
  for(int i=0; i<nbBadges; i++){
    if(badges[i] == uid) return true;
  }
  return false;
}

void bip(int ms){ digitalWrite(BUZZER, HIGH); delay(ms); digitalWrite(BUZZER, LOW); }

void chargerDonnees(){
  codePinAdmin = preferences.getString("pin", "1234");
  nbBadges = preferences.getInt("nb", 1);
  badges[0] = preferences.getString("b0", "ADMIN"); // Mets ton UID admin ici la 1ere fois
  for(int i=1; i<nbBadges; i++){
    badges[i] = preferences.getString(("b" + String(i)).c_str(), "");
  }
}

void sauvegarderBadges(){
  preferences.putInt("nb", nbBadges);
  for(int i=0; i<nbBadges; i++){
    preferences.putString(("b" + String(i)).c_str(), badges[i]);
  }
}