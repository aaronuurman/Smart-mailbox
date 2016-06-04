#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <avr/sleep.h> 

//Seadistused
SoftwareSerial GSMModule(7, 8);

//Muutujad
int wakePin = 2;                 // Väline ärataja   
String incomingSMS = "";
String Message = "";
String SecretCode = "#S100nPN";           //Turvakood.
String PhoneNumber = "";
int redLED = 13;                          //Vigade märgutuli.
int greenLED = 12;                        //Töövalmiduse märgutuli.
String Validate = "";                     //Valideerimise muutuja numbri jaoks.
boolean errorfinder = false;              //Errorite otsija.
String ValidNumber = "";                  //Valideeritud telefoni number.
int address = 0;
boolean numberValidades = false;
boolean readytosend = false;
String sendnumber = "";

void setup()
{
  Serial.begin(19200);                    // Jadapordiga ühenduskiiruse defineerimine
  GSMModule.begin(19200);                 // GSM mooduli jadapordi ühenduskiiruse defineerimine
  delay(20000);                           // Ooteaeg, vajalik mooduli ühendamiseks GSM võrku.
  GSMModule.print("AT+CMGF=1\r");         //SMS tekstirežiimi seadistamine
  delay(100);
  GSMModule.print("AT+CNMI=2,2,0,0,0\r"); //Jadapordi  monitoorimisel  kuvab tekstina smsi sisu
  delay(100);
  pinMode(wakePin, INPUT_PULLUP);   
  attachInterrupt(0, wakeUp, HIGH);       //Äratab kui 2. viigu sisend on kõrge  
}
//Äratab ülesse
void wakeUp() {
  GSMModulePower();  
  delay(5000);
  Serial.println("Ärkasin!");
  if (GSMModule.available() > 0){           // Don't read unless
      incomingSMS = GSMModule.readString(); // Read a character
      Serial.println(incomingSMS);
      int i = incomingSMS.length();
      Serial.println(i);
      if (incomingSMS.length() > 0){
        AnalyzeMessage();
      } else{
        if (readytosend){
          //Saadab sõnumi
            SendSMS();
            do {} while (1);
            readytosend = false;      
        }
      }
  }
}  
//Puhkerežiim 
void sleep() {  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // Puhkerežiimi seadistamine 
  sleep_enable();          // mcucr registris muudab puhkerežiimi biti 1 
  attachInterrupt(0,wakeUp, HIGH); // Välise ärataja defineerimine
  GSMModulePower(); 
  sleep_mode();            // seade puhkereziimi   
  sleep_disable();         // Esimene asi peale puhkust lülita välja puhkereziim  
  detachInterrupt(0);  
}

//GSM Mooduli toide
void GSMModulePower()
{
  pinMode(9, OUTPUT); 
  digitalWrite(9,LOW);
  delay(1000);
  digitalWrite(9,HIGH);
  delay(2000);
  digitalWrite(9,LOW);
  delay(3000);
}

//Loeb EEPROM
void ReadEEPROM()
{
  EEPROM.get(address, sendnumber);
  Serial.println( sendnumber );  
}
//Salvestab EEPROM
void SaveEEPROM()
{
    if(numberValidades){
      ValidNumber.remove(0,1);
      Serial.println(ValidNumber);
      EEPROM.put(address, ValidNumber);
      numberValidades = false;
      Serial.println("Salvestatud EEPROM");
      readytosend = true;
    }
}
//Sõnumi saatmine
void SendSMS()
{
  GSMModule.print("AT+CMGF=1\r");                                                        // AT command to send SMS message
  delay(100);
  GSMModule.println("AT + CMGS = \"+37253018504\"");                                     // recipient's mobile number, in international format
  delay(100);
  GSMModule.println("Sulle on kiri");        // message to send
  delay(100);
  GSMModule.println((char)26);                       
  delay(100); 
  GSMModule.println();
  delay(5000);                                     //Saatmisaeg
}

//Kontrollib telefoninumbri valiidsust.
void ValidateNumber(){
  if (PhoneNumber.startsWith("+",0)){           //Kontrollib , et telefoni number algaks "+"iga.
    Validate = PhoneNumber;                     //Määrab valideeritava numbri.
    Validate.replace("+", "");                  //Eemaldab "+"i.
    Serial.println(Validate);
    //Tsükkel mis kestab täpselt nii kaua, kui pikk on saadetud number.
    int z = Validate.length();
    Serial.println(z);
    for (int x = 0; x < z;  x++){
      char c = Validate.charAt(x);              //Võtab x-numbri telefoninumbrist. 
      if ((c >= '0') && (c <= '9')) {           //Kontrollib kas x on number.
        Serial.print("Saadetud number on arv:");        //Jadapordi väljund
        Serial.println(c);
        delay(1000);
        Serial.println(x);
        if(x == (z-1)){
          ValidNumber = PhoneNumber;
          Serial.println(ValidNumber);
          Serial.println("OK");
          numberValidades = true;
          SaveEEPROM(); 
        }
      }else{
        Error2();                               //Veateade nr.2.
        errorfinder = true;                     //Leiti error
        readytosend = true;
        break;                                  //Väljub tsüklist, kuna saadetud number ei valideeru
      }
    }
  }
}

//Telefoni number sõnumi seest.
void AnalyzeMessage() {
  if (incomingSMS.length() > 0){                    //Kontrollib kas sõnumil on sisu.
    int x = incomingSMS.indexOf(SecretCode);
    Serial.println(x);
    String bufferstr = incomingSMS.substring(x);
    Serial.println(bufferstr);
    Message = bufferstr;
    Message.replace(SecretCode, "");                //Eemaldab sõnumi algusest turvakoodi ja tühiku.
    Message.replace(" ", ""); 
    PhoneNumber = Message;                          //Määrab telefoni numbri kuhu SMS saadetakse.
    Serial.println(PhoneNumber);
    PhoneNumber.trim();
    x = PhoneNumber.length();
    Serial.println(x);
    ValidateNumber();           
  }
}

void loop()
{
  sleep();     // Puhkereziim 
}

//Veateade nr.1.
void Error1(){
  Serial.println("Turvakood ei ühti");
}

//Veateade nr.2.
void Error2(){
  Serial.println("Number ei valideeru");
  digitalWrite(redLED, HIGH);
  delay(2000);
  digitalWrite(redLED, LOW);
  delay(1000);
  digitalWrite(redLED, HIGH);
  delay(2000);
  digitalWrite(redLED, LOW);
  delay(1000);
  digitalWrite(redLED, HIGH);
  delay(2000);
  digitalWrite(redLED, LOW);
}


