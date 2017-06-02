#include <Adafruit_NFCShield_I2C.h>
#include <Wire.h>

#define IRQ   (2)
#define RESET (3) 


Adafruit_NFCShield_I2C nfc(IRQ, RESET);

String strid;

void setup(){
  Serial.begin(115200);
  //Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Obtenir les données et les affichées!
  //Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  //Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  //Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Fixer le nombre d'essais MAS pour lire les carte
  // Cela empeche d'attente infiniement après la lecture d'une carte (qui est
  // le fonctionnement par défaut du PN532).
  nfc.setPassiveActivationRetries(0xFF);
  
  // configurer la carte pour lire des tags RFID
  nfc.SAMConfig();
    
  //Serial.println("Attendre apres une carte ISO14443A");
}

void loop(void) {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer/Tampon pour stocker le UID retourné
  uint8_t uidLength;                        // Longueur de l'UID (4 ou 7 bytes/octets en fonction du type de carte ISO14443A)
  
  // Attendre après une carte de type ISO14443A (Mifare, etc.).  Lorsqu'une carte est trouvée
  // 'uid' est remplis avec l'UID, et uidLength indiquera si nous avons un
  // uid en 4 bytes/octets (Mifare Classic) ou en 7 bytes/octets (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    strid = "";
    //Serial.println("Une carte trouvee!");
    //Serial.print("Longueur UID: ");Serial.print(uidLength, DEC);Serial.println(" bytes / octets");
    //Serial.print("Valeur UID: ");
    for (uint8_t i=0; i < uidLength; i++) 
    {
      strid = strid + uid[i]; 
    }
    Serial.print(strid);
    //Serial.println("");
    // Attendre une seconde avant de continuer
    delay(1000);
  }
  else
  {
    // PN532 probablement en timeout en attendant une carte
    Serial.println("Timeout pendant l attente d une carte");
  }
   
    
    // Uncomment just _one_ line below depending on how your breakout or shield
    // is connected to the Arduino:
     
    // Use this line for a breakout with a SPI connection:
    //Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
     
    // Use this line for a breakout with a hardware SPI connection.  Note that
    // the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
    // hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
    // SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
    //Adafruit_PN532 nfc(PN532_SS);
     
    // Or use this line for a breakout or shield with an I2C connection:
    //Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

}
