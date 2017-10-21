/*
  Model Rocket dual pyro timer triggered by rocket lift-off Ver 1.0
  This is an alternative firmware for the AltiDuo altimeter board to 
  take advantage of the pressure sensor to do a programmable timer 
  triggered by the rocket lift-off.
  The timer of the first pyro can be 1000, 3000, 5000 or 7000ms
  The timer of the second pyro can be 2000, 4000, 6000 or 8000ms
  This is set by a combination of 2 jumpers.
 
 Copyright Boris du Reau 2012-2017
 
 This is using a BMP085 or BMP180 pressure sensor and an Attiny 84
  
 The following should fire one pyro output x ms after liftoff and 
 the second pyro 1000ms after the first pyro. 
 If it is at least 20m above ground of the launch site
 
 
 For the BMP085 pressure sensor
 Connect VCC of the BMP085 sensor to 5.0V! make sure that you are using the 5V sensor (GY-65 model)
 Connect GND to Ground
 Connect SCL to i2c clock - pin 9 (PA4) of the Attiny 84
 Connect SDA to i2c data - pin 7 (PA6) of the Attiny 84
  EOC is not used, it signifies an end of conversion
 XCLR is a reset pin, also not used here
 If you prefer you can also use a BMP180 sensor which is software compatible. Do not use the BMP180
 library otherwise your Attiny 84 will run out of memory.
 
 The micro switches are connected on pin 5(PA8) and pin 6 (PA7)
 The first pyro is connected to pin 13 (PA0)
 The second pyro is connected to pin 2 (PA10)
 The first pyro continuity test is connected to pin 11 (PA2)
 The second pyro continuity test is connected to pin 3 (PA9)
 The speaker/buzzer is connected to pin 12 (PA1)
 
 ***Important*** whenever you load that firmware to your altimeter make sure that you label it. 
 */

#include <TinyWireM.h>
#include <tinyBMP085.h>

#define DEBUG //=true

BMP085 bmp;

//ground level altitude
long initialAltitude;
//current altitude
long currAltitude;
//Apogee altitude
long apogeeAltitude;
long mainAltitude;
long liftoffAltitude;
long lastAltitude;
//Our drogue has been ejected i.e: apogee has been detected
boolean apogeeHasFired =false;
boolean mainHasFired=false;
//nbr of measures to do so that we are sure that apogee has been reached 
unsigned long measures=5;
unsigned long mainTempo;

/* TODO: those need to be changed for the attiny 84*/
const int pinTempo1 = 3;
const int pinTempo2 = 2;

const int pinApogee = 0;//10;
const int pinMain = 10;//0;
const int pinApogeeContinuity = 1;
const int pinMainContinuity = 8;
const int pinSpeaker = 9;
boolean liftOff= false;
unsigned long initialTime;

int nbrLongBeep=0;
int nbrShortBeep=0;

boolean NoBeep=false;
//Kalman Variables
float f_1=1.00000;  //cast as float
float kalman_x;
float kalman_x_last;
float kalman_p;
float kalman_p_last;
float kalman_k;
float kalman_q;
float kalman_r;
float kalman_x_temp;
float kalman_p_temp;
//float KAlt;
//end of Kalman Variables

void setup()
{
  int val = 0;     // variable to store the read value
  int val1 = 0;     // variable to store the read value


  //init Kalman filter
  KalmanInit();
  //Presure Sensor Initialisation
  bmp.begin( BMP085_STANDARD);
  //our drogue has not been fired
  apogeeHasFired=false;
  mainHasFired=false;

  //Initialise the output pin
  pinMode(pinApogee, OUTPUT);
  pinMode(pinMain, OUTPUT);
  pinMode(pinSpeaker, OUTPUT);

  pinMode(pinTempo1, INPUT);
  pinMode(pinTempo2, INPUT);

  pinMode(pinApogeeContinuity, INPUT);
  pinMode(pinMainContinuity, INPUT);
  //Make sure that the output are turned off
  digitalWrite(pinApogee, LOW);
  digitalWrite(pinMain, LOW);
  digitalWrite(pinSpeaker, LOW);

  //initialisation give the version of the altimeter
  //One long beep per major number and One short beep per minor revision
  //For example version 1.2 would be one long beep and 2 short beep
  beepAltiVersion(1,3);

  //number of measures to do to detect Apogee
  measures = 5;

  //initialise the deployment altitude for the main 
  //mainDeployAltitude = 2;

  // On the Alti duo when you close the jumper you set it to 0
  // val is the left jumper and val1 is the right jumper

  val = digitalRead(pinTempo1); 
  val1 = digitalRead(pinTempo2);  
  if(val == 0 && val1 ==0)
  {
    mainTempo = 2000;
  }
  if(val == 0 && val1 ==1)
  {
    mainTempo = 4000;
  }
  if(val == 1 && val1 ==0)
  {
    mainTempo = 6000;
  }
  if(val == 1 && val1 ==1)
  {
    mainTempo = 8000;
  }

  // let's do some dummy altitude reading
  // to initialise the Kalman filter
  for (int i=0; i<50; i++){
    KalmanCalc(bmp.readAltitude());
  }
  //let's read the lauch site altitude
  long sum = 0;
  for (int i=0; i<10; i++){
    sum += KalmanCalc(bmp.readAltitude());
    delay(50); 
  }
  initialAltitude = (sum / 10.0);
  lastAltitude = 0; 

  liftoffAltitude =  20;
}

void loop()
{
  //read current altitude
  currAltitude = (KalmanCalc(bmp.readAltitude())- initialAltitude);
  if (( currAltitude > liftoffAltitude) != true)
  {
    continuityCheck(pinApogeeContinuity);
    continuityCheck(pinMainContinuity);
  }

  if (( currAltitude > liftoffAltitude) == true && liftOff == false && mainHasFired == false)
  {
    liftOff = true;
    // save the time
    initialTime =millis();

  }  

  while(liftOff ==true)
  {
    unsigned long currentTime;
    //unsigned long diffTime;

    currAltitude = (KalmanCalc(bmp.readAltitude())- initialAltitude);

    currentTime = millis()- initialTime;
    if (currentTime > mainTempo && apogeeHasFired ==false)
    {
      //fire drogue
      digitalWrite(pinApogee, HIGH);
      delay (1000);
      digitalWrite(pinApogee, LOW);
      digitalWrite(pinMain, HIGH);
      delay (1000);
      digitalWrite(pinMain, LOW);
      apogeeHasFired=true;
    }



    if(apogeeHasFired == true )
    {
      beginBeepSeq();

      beepAltitude(apogeeAltitude);

    }
  }
}

void continuityCheck(int pin)
{
  int val = 0;     // variable to store the read value
  // read the input pin to check the continuity if apogee has not fired
  if (apogeeHasFired == false )
  {
    val = digitalRead(pin);   
    if (val == 0)
    {
      //no continuity long beep
      longBeep();
    }
    else
    {
      //continuity short beep
      shortBeep();
    }
  }
}



void beepAltitude(long altitude)
{
  int i;
  // this is the last thing that I need to write, some code to beep the altitude
  //altitude is in meters
  //find how many digits
  if(altitude > 99)
  {
    // 1 long beep per hundred meter
    nbrLongBeep= int(altitude /100);
    //then calculate the number of short beep
    nbrShortBeep = (altitude - (nbrLongBeep * 100)) / 10;
  } 
  else
  {
    nbrLongBeep = 0;
    nbrShortBeep = (altitude/10); 
  }

  if (nbrLongBeep > 0)
    for (i = 1; i <  nbrLongBeep +1 ; i++)
    {
      longBeep();
      delay(50);
    } 

  if (nbrShortBeep > 0)
    for (i = 1; i <  nbrShortBeep +1 ; i++)
    {
      shortBeep();
      delay(50);
    } 

  delay(5000);

}

void beginBeepSeq()
{
  int i=0;
  if (NoBeep == false)
  {
    for (i=0; i<10;i++)
    {
      tone(pinSpeaker, 1600,1000);
      delay(50);
      noTone(pinSpeaker);
    }
    delay(1000);
  }
}
void longBeep()
{
  if (NoBeep == false)
  {
    tone(pinSpeaker, 600,1000);
    delay(1500);
    noTone(pinSpeaker);
  }
}
void shortBeep()
{
  if (NoBeep == false)
  {
    tone(pinSpeaker, 600,25);
    delay(300);
    noTone(pinSpeaker);
  }
}

//================================================================
// Kalman functions in your code
//================================================================

//Call KalmanInit() once.  

//KalmanInit() - Call before any iterations of KalmanCalc()
void KalmanInit()
{
  kalman_q=4.0001;  //filter parameters, you can play around with them
  kalman_r=.20001;  // but these values appear to be fairly optimal

  kalman_x = 0;
  kalman_p = 0;
  kalman_x_temp = 0;
  kalman_p_temp = 0;

  kalman_x_last = 0;
  kalman_p_last = 0;

}

//KalmanCalc() - Calculates new Kalman values from float value "altitude"
// This will be the ASL altitude during the flight, and the AGL altitude during dumps
float KalmanCalc (float altitude)
{

  //Predict kalman_x_temp, kalman_p_temp
  kalman_x_temp = kalman_x_last;
  kalman_p_temp = kalman_p_last + kalman_r;

  //Update kalman values
  kalman_k = (f_1/(kalman_p_temp + kalman_q)) * kalman_p_temp;
  kalman_x = kalman_x_temp + (kalman_k * (altitude - kalman_x_temp));
  kalman_p = (f_1 - kalman_k) * kalman_p_temp;

  //Save this state for next time
  kalman_x_last = kalman_x;
  kalman_p_last = kalman_p;

  //Assign current Kalman filtered altitude to working variables
  //KAlt = kalman_x; //FLOAT Kalman-filtered altitude value
  return kalman_x;
}  

void beepAltiVersion (int majorNbr, int minorNbr)
{
  int i;
  for (i=0; i<majorNbr;i++)
  {
    longBeep();
  }
  for (i=0; i<minorNbr;i++)
  {
    shortBeep();
  }  
}


