#include <Grove_LED_Bar.h>              //Library for LED bar
#include <SoftwareSerial.h>             //Library for Serial Communication
#include <math.h>                       //Library for mathematical computations
#include <LiquidCrystal.h>              //Library for LCD Display
Grove_LED_Bar bullet_bar(9, 8, 0);      //Bullet LED bar is attached to D8 of Base shield
Grove_LED_Bar health_bar(5, 4, 0);      //Health LED bar is attached to D4 of Base Shield
Grove_LED_Bar armour_bar(3, 2, 0);      //Bullet LED bar is attached to D2 of Base shield
LiquidCrystal lcd(12, 7, 6, 25, 24, 23, 22);     // Initialized the library with number of interface pins
#define BULLET_PIN  A4           	//LDR for Bullet Count detection is attached to A4 pin of arduino
#define SENSOR_PIN  A5         		//Chest LDR for Health detection is attached to A5 pin of arduino
#define SENSOR_PIN2  A3       		//Head LDR for Health detection is attached to A3 pin of arduino
#define BUZZER_PIN  A0         		//Buzzer is attached to A0 pin of Base Shield
#define led  9                          //LED is attached to A9 pin of Arduino
int LDRValue, LDRValue2;        	//LDR values for Arm and head sensor
int LDRDiff, LDRDiff2;                  //Difference in readings
int LSB = 150, LSH = 200, LSH2 = 150;   //Threshold values
int Bullet, Health,  Armour;       	//Health, Armour and Bullet count
int dead = 1;                           //integer to declare if it is dead
String responseIs = "", content = "", add = "";
char incoming_char = 0;
boolean pr = false;
int rssi = 0;                     	//Current RSSI value is stored in it
int count = 0;
int arssi[50];                   	//Last 50 RSSI values to average it out
int head = 0;
float avrssi = 0;              		//Average RSSI value
int dist;                           	//Distance calculated through RSSI
int bomb;                         	//Current distance of bomb
float dist_fb = 0;              	//Distance from bomb
double ratio = 1, glow = 0;              
//Decides how the intensity of LED based on ratio of distance of bomb from playerto distance of bomb from server
int healthdiff = 0;             	//diff of health readings from LDR
int armourdiff = 0;           		//diff of armour readings from LDR 
int bulletdiff = 0;             	//diff of bullet readings from LDR
SoftwareSerial XBee(10, 11);            //serial port for xbee communication
int backLight = 13;         		//Port through which backlight power will be provided                     


void setup()
{
    Serial.begin(9600);
    //Initialization of both LED bar with health=100 and bullet=10
    bullet_bar.begin();
    health_bar.begin();
    armour_bar.begin();

    pinMode(BUZZER_PIN, OUTPUT);
    Bullet = 10;
    Health = 100;
    for(int i=1;i<=10;i++){
      bullet_bar.setLed(i, 1);
    }
    for(int i=1;i<=10;i++){
      health_bar.setLed(i, 1);
    }
    for(int i=1;i<=10;i++){
      armour_bar.setLed(i, 1);
    }
    pinMode(backLight, OUTPUT);           //set pin 13 as output
    analogWrite(backLight, 100);          //give 100 backlight power
    lcd.begin(16, 4);                     //initialize with 16x4 array
    lcd.setCursor(0, 0);                  //Set cursor to column 0 line 0
    lcd.print("Health: ");                //Print “Health”
    lcd.setCursor(0, 1);
    lcd.print("Armour: ");
    lcd.setCursor(8, 0);
    lcd.print(Health);
    lcd.setCursor(8, 1);
    lcd.print(Armour);
    
    //Begin ibeacon communication
    //Serial1 correponds to BLE 4.0
    Serial1.begin(9600);
    delay(100);
    //Sending all the commands to setup BLE in recieving mode
    Serial1.write("AT");
    delay(100);
    Serial1.write("AT+IBEA1");
    delay(100);
    Serial1.write("AT+IMME1");
    delay(100);
    Serial1.write("AT+ROLE1");
    delay(100);/**/
    XBee.begin(9600);
    
    //Reading initial value of LDRs
    LDRValue = analogRead(SENSOR_PIN);
    LDRValue2 = analogRead(SENSOR_PIN2);
    LDRValue = analogRead(BULLET_PIN);
   
}

void loop()
{
  /*Serial1 module for distance communication*/
  delay(1);
  //This will read the response by bluetooth and concatenate it
  if(Serial1.available())
  {
    incoming_char = Serial1.read();
    content.concat(incoming_char);
    pr = true;
  }
  //it will print it when no more response character is available
  else if(pr){
    Serial.println(content);
    /*If response corresponds to DISI(i.e. response length = 86) command then
    it will check the address of the other BLE and then extract it's RSSI
    from the response*/
    if(content.length() == 86){
      add = content.substring(69,81);
      if(add = "74DAEAAFEBC1"){
        String x = content.substring(83);
        rssi = x.toInt();
        /**averaging over last 50 RSSI and then calculating the Distance
        RSSI = 10n*log(Dist) + TxPower
        We have set TxPower = 0 and taken n = 2 which is the case of ideal condition
        Ideal condition refer to no obstacle around in between both BLEs*/
        arssi[head] = rssi;
        head = (head+1)%50;
        avrssi = 0;
        for(int i=0;i<50;i++){
          avrssi += arssi[i];
        }
        avrssi = avrssi/50;
        dist = pow(10, avrssi/20);
        dist_fb = abs(dist - bomb);
        ratio = dist_fb/bomb;
        Serial.println(ratio);
        /*Base on the ratio the LED glow is maintained*/
        if(ratio < 0.05){
          /*If the ratio is very less => player is near to bomb
          and the health decreases*/
          glow = 200;
          Health -= 20;
          healthdiff = -1;
          
          XBee.println("Health Reduce "+String(Health));
          delay(100); 
          digitalWrite(BUZZER_PIN, HIGH);
          delay(200);
          digitalWrite(BUZZER_PIN, LOW);
        }
        else if(ratio < 0.2)
          glow = 250;
        else if(ratio < 0.5)
          glow = 253;
        else
          glow = 255;
        analogWrite(led, glow);/**/
      }
    }
    pr = false;
  }
  /*If we do not get any response for 1 sec
  we resend "AT+DISI?" command*/
  else{
    if(content == "OK+DISIS" && count < 1000){
      count++;
    }
    else{
      count = 0;
      content = "";
      Serial1.write("AT+DISI?");
      delay(100);
    }
  }/**/
  /*This is for manually sending the command*/
  if(Serial.available())
  {
    content = "";
    char s = Serial.read();
    Serial1.write(s);
  }/**/

/*This is for keeping the timer and making armour = 100 when 100 sec are done*/
if((millis()/1000)%100 == 0){
      Armour = 100;
      armourdiff = -1;
      lcd.setCursor(8, 2);
      lcd.print("    ");
  }
  lcd.setCursor(8, 2);
  lcd.print((millis()/1000)%100);
  
  /*Health Module*/
  //Taking the difference and then adding difference to the previous value
    LDRDiff = analogRead(SENSOR_PIN) - LDRValue;
    LDRValue += LDRDiff;
    LDRDiff2 = analogRead(SENSOR_PIN2) - LDRValue2;
    LDRValue2 += LDRDiff2;
    //If Health is greater than 0 and LDR difference 
    //is greater than given threshold then health is decreased
    //Simultaneously buzzer beeps when health is decreased 
    if (Health > 0){
      if (LDRDiff2 > LSH2)
      {
          Health -= 50;
          healthdiff = -1;
          
          XBee.println("Health Reduce "+String(Health));
          delay(100); 
          digitalWrite(BUZZER_PIN, HIGH);
          delay(50);
          digitalWrite(BUZZER_PIN, LOW);
      }
      else if (LDRDiff > LSH)
      {
          if(Armour <= 0){
              Health -= 10;
              healthdiff = -1;
              
              XBee.println("Health Reduce "+String(Health));
              delay(100); 
              digitalWrite(BUZZER_PIN, HIGH);
              delay(30);
              digitalWrite(BUZZER_PIN, LOW);
          }
          else{
              Armour -= 20;
              armourdiff = -1;
          }
      }
      else
      {
          digitalWrite(BUZZER_PIN, LOW);
      }
    }/*When Health is 0 then there is a long beep indicating the person is dead*/
else{
      Health = 0;
      if(dead == 1){
        digitalWrite(BUZZER_PIN, HIGH);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("You're Dead.");
        lcd.setCursor(0,1);
        lcd.print("Game over");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("----------");
        lcd.setCursor(0, 1);
        lcd.print("----------");
        dead = 0;
      }
      digitalWrite(BUZZER_PIN, LOW);
   }
    
    /*Health LED bar is decreased as well as reflected on LCD display according to the health*/
    if(healthdiff != 0){
      for(int i=0; i<10; i++){
       if(Health/10>i)
         health_bar.setLed(i+1, 1);
       else
         health_bar.setLed(i+1, 0);
      }
      lcd.setCursor(8, 0);
      lcd.print(Health);
      lcd.print(" ");
    }/**/
    healthdiff = 0;

   /*Armour LED bar is decreased as well as reflected on LCD display according to the armour. */
  if(armourdiff != 0){
      for(int i=0; i<10; i++){
       if(Armour/10>i)
         armour_bar.setLed(i+1, 1);
       else
         armour_bar.setLed(i+1, 0);
      }
      lcd.setCursor(8, 1);
      lcd.print(Armour);
      lcd.print(" ");
      if(Armour==0){
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Armour is over.");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Health: ");
        lcd.setCursor(0, 1);
        lcd.print("----------");
        lcd.setCursor(8, 0);
        lcd.print(Health);
      }
    }
    armourdiff = 0;
    
    /*The decrease of health is indicated at server through server*/
    if(XBee.available())
    {
      char temp = XBee.read();
      responseIs += temp;
    }/**/
    if(responseIs != "" && responseIs.indexOf("\n") != -1 )
    {
      if(responseIs.substring(0,4) == "bomb")
      {
        bomb = responseIs.substring(8).toInt();
        Serial.print(bomb);        
        responseIs = "";
      }
    }/**/
    
    /*Bullet Module*/
    Bulletdiff = analogRead(BULLET_PIN) - LDRValue;
    LDRValue += Bulletdiff;
    
    /*If the Ammo is +ve then we check if the LDR reading has a significant change or not.
    If it has change more than LSB then Bullet Count is Decreased by 1 and corresponding LEDs change*/
    if (Bullet > 0){
      if (Bulletdiff > LSB)
      {
          Serial.println(Bullet);
          Bullet -= 1;
          for(int i=0; i<10; i++){
           if(Bullet>i)
             bullet_bar.setLed(i+1, 1);
           else
             bullet_bar.setLed(i+1, 0);
        }
      }
    }/**/
}
