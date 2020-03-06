/*-------------------------------
INFO:
Coded by jjavieralv 06/03/2020
Version 1.0
Details: This program uses an arduino and a PN532 to read UID of NFC card
     and if that UID is in the granted list, allows you to access. All
     is indicated with a led.
To do this program, I have used iso_14443a uid example of the PN532 v1.0.4 library
---------------------------------
TESTS:
  -Arduino Uno and PN532 over I2C 
    Using pins:
      SDA   A4
      SCL   A5
      IRQ   2
      RESET 3
      VCC   5V
      GND   GND
*/

//Libraries 
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h> //tested with version 1.0.4 of librarie PN532

//pins used to I2C
//SDA A4
//SCL A5

//This is used with I2C to allow breakout, the IRQ and RESET (reset is not usually used)
#define PN532_IRQ   (2)
#define PN532_RESET (3) 

//Define other pins used. 
//  Door activates the relay
//  green_led indicates you have access
//  red_led indicates that you have not access or you have to wait anti-bruteforce delay
#define green_led   (4)
#define red_led     (5)
#define door        (6)

//Define anti-bruteforce values
//  bruteforce_delay sets in seconds the delay to activate anti-bruteforce system
//  errors is used to count consecutive errors to activate anti-bruteforce system
//  max_errors sets the maxium retries before activate anti-bruteforce system
byte bruteforce_delay=10;
byte errors=0;
byte max_errors=4;

//Initialices PN532 using I2C
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

//If you are not able to see Serial console with Zero, remove this if (and also change
//Adafruit_PN532.cpp library file)
#if defined(ARDUINO_ARCH_SAMD)
   #define Serial SerialUSB
#endif


//--------------------------------------------------------------------------
//Setup function to starts nfc system and set some pins and variables
void setup(void) {
  //for Leonardo/Micro/Zero
  #ifndef ESP8266
    while (!Serial);
  #endif

  //sets baudrate to be used in serial monitor
  Serial.begin(115200);
  //Indicator to the setup is starting
  Serial.println("Setup is starting");
  //first config pins 

  //conects to nfc 
  nfc.begin();
  //get, save and print PN53* type and verson
  uint32_t versiondata = nfc.getFirmwareVersion();
  //check if was able to connect with PN532, if not able, software halts
    if (! versiondata) {
      Serial.print("Didn't find PN53x board");
      while (1); // halt if not able to connect with board
    }else{
      //here enters if are able to connect with board, now prints and configure nfc
      Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
      Serial.print("Firmware version: "); Serial.print((versiondata>>16) & 0xFF, DEC); 
      Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
      //set max number of retry attemps no read from tag
      nfc.setPassiveActivationRetries(0xFF);
      //sets board to be able to read RFID tags
      nfc.SAMConfig();
      //all has been configurated right, now wait for a tag :)
      Serial.println("All has been configurated right, now wait for a tag :)");
    }

  //set up extra pins
  pinMode(door, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(red_led, OUTPUT);

}

//--------------------------------------------------------------------------
//This is the blink system used to indicates you have access (ok=true) or not 
//granted (ok=false)
void blink(boolean ok){
  //set up the number of blinks
  byte num_of_blinks=3;
  //set up time between blinks in milliseconds
  byte time_blinks=100;

  if(ok){
    while(num_of_blinks!=0){
      digitalWrite(green_led, HIGH);
      delay(time_blinks);
      digitalWrite(green_led, LOW);
        delay(time_blinks);
        num_of_blinks--;
    }
  }else{
    while(num_of_blinks!=0){
      digitalWrite(red_led, HIGH);
      delay(time_blinks);
      digitalWrite(red_led, LOW);
        delay(time_blinks);
        num_of_blinks--;
      }
  }
}

//--------------------------------------------------------------------------
//Opens the door and turn on green led while the door is possible to open
void door_opened(){
  //set the number of milliseconds that the open door will be
  uint8_t door_time=2000;
  digitalWrite(green_led, HIGH);
  digitalWrite(door, HIGH);
  delay(door_time);
  digitalWrite(green_led, LOW);
  digitalWrite(door, LOW);
}

//--------------------------------------------------------------------------
//This option takes an UID and find it in a database(checking 4 first values).
//If UID is in granted database, returns true, else, returns false
boolean access(uint8_t uid[]){
  byte position_in_tag=0;
  //this is the database (should be upgrade using a hash function)
  //If you want to add a tag, check the monitor serie while you are using
  //the tag. Take and add 4 first UID values in decimal(uint8_t)
  uint8_t granted[][4] = {
  //{1,1,1,1}           //example

  };

  //calculate the number of cards. tag_number=num_of_values_in_array/length_of_each_card(4)
  int8_t tag_number=sizeof(granted)/4;
  Serial.print("Number of cards stored: ");Serial.println(tag_number);
  tag_number--;

  //this "while" takes each card(starting with the last until the first), the "while" inside checks each value, and if all
  //of 4 values are same, return a true, else, returns false
  while(tag_number>=0){
    Serial.print("Checking card number");Serial.println(tag_number);
    //reset possition to be able to check all UID values starting in 0 to 3 position(4 values)
    position_in_tag=0;

    //this is the part that compares uid and stores values one at time unil one value failes
    //or already checks right 4 values
    while(position_in_tag<4 && granted[tag_number][position_in_tag]==uid[position_in_tag]){
      Serial.print("Value in uid: ");Serial.println(uid[position_in_tag]);
      Serial.print("Value stored: ");Serial.println(granted[tag_number][position_in_tag]);
      position_in_tag++;
      //if position_in_tag is 4, that means that the uid is allowed, exit while
      if(position_in_tag==4){
        tag_number=-1;
      }
    }
    tag_number--;
    Serial.println("");
  }

  if(position_in_tag==4){
    return true;
  }else{
    return false;
  }
}

//--------------------------------------------------------------------------
//Now comes the infinite loop
void loop(void) {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  //detects if there is a card near and readable with the NFC reader
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    Serial.println("Card detected! ");
    //Prints uid length 
    Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    //prints UID card value
    Serial.print("UID Value: ");
    for (short i=0; i < uidLength; i++){Serial.print(" ");Serial.print(uid[i]);}

    //checking if the door should be opened
    Serial.println("Checking if the UID is in the granted database ");
    //Using function "access" you check if the UID is granted, in that case, "access"
    //returns true
    if(access(uid)){
      Serial.println("Tag granted, opening door :)");
      Serial.println("\n\n");
      door_opened;
    }else{
      Serial.print("Tag is not allowed :(");
      Serial.println("\n\n");
      blink(false);
      //each time that a bad tag is readed, errors increments. 
      errors++;
    }
    
    //Anti-bruteforcing function, if 4 bad tags are consecutivelly readed, it wait extra seconds   
      if(errors>max_errors){
        delay(bruteforce_delay*1000);
        //after this, reset errors counter
        errors=0;
      }else{
        //time between be able to read another tag
        delay(1000);
      }
  }else{
    Serial.println("Timed out waiting for a card :(");
  }
}




