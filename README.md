
# 🔒 coffre_v4_rfid_and_keypad_esp32

cofre fort ESP32 + RFID + keypad double securité blocage 1min apres passage de la mauvaise badge 
3 fois et du code pine  + code arduino IDE

Projet complet : Code Arduino + Démo.

### 1. Code
Dans `/Code` : `sketch_jul11a_coffre_v4` à ouvrir avec Arduino IDE.


### 2.Démo
 OUVRIR demo_photo_video pour voir image du montage et video demo

### 3.Composants
 ESP32-WROOM-32, LCD I2C, Servo SG90, Buzzer, 2 LED(rouge,vert) , 2 Résistance, lecteur RFID-RC522
 keypad une plaquette d'éssaie et des fils de connexion

### FONTIONNEMENT
 ## 1. 
    A l'allumage du dispositif le lcd affiche passer votre badge et * pour un menu
    et aussi a chaque passage d'une badge le buzzer et les LEDS réagissent selon si c'est la bonne ou mauvaise
  
 ## 2.
    Bonne badge la led verte s'allume et il demande le code pin et si cest correct le servo tourne la led verte s'allume
    mauvaise badge la LED rouge s'allume et LCD affiche acces refuser essai 1/3 
 
 ## 3.
    Le menu a trois option 
      1.changer code pine 
      2.ajouter nouveau badge(il enregistre j'usqua 10 badge différent)
      3.supprimer badge
TOUT LES DEMO SON DANS LA VIDEO 

      
