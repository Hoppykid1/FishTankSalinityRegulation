  #include <movingAvg.h> //this implements a moving average library
  #include <SoftwareSerial.h> //sets up LCD
  SoftwareSerial LCD = SoftwareSerial(255, 2);
  
  //initialize global variables
  float salinityLL = 0.090;//This is the Salinity Lower Limit Raw value is 592
  float salinitySP = 0.10;//This is the Salinity SetPoint
  float salinityUL = 0.120;//This is the Salinity Upper Limit Raw value is 606
  float salinityV = 0;//This is the Salinity's Actual Value
  float salinityRAW = 0;
 
  
  float tempUL = 25.1;//This is the Temperature Lower Limit
  float tempSP = 25;//This is the Temperature SetPoint
  float tempLL = 24.9;//This is the Temperature Lower Limit
  float tempV = 1;//This is the Temperature Lower Limit
  float tempRAW = 0;
  float tempDS = 0;

  int counterSal;
  int counterTemp;// this is a counter variable for the data smoothing for temperature
  
  float mOfSoln = 86; // mass of solution
  float mOfSalt = mOfSoln*salinityV; //mass of salt
  float mOfWater = mOfSoln - mOfSalt; //mass of water

  float waterNeeded;
  float saltWNeeded;

  float gainNACL; //these are the gain variables
  float gainH2O;

  movingAvg salinity(3);

//THIS IS THE START OF THE ACTUAL CODE


void setup() {
  //initilize digital pins
  pinMode(2,OUTPUT); // this pin is for the LCD
  pinMode(3,OUTPUT); // this pin is for the left Solenoid
  pinMode(4,OUTPUT); // this pin is for the right Solenoid
  pinMode(5,OUTPUT); //this pin is for the Heater
  pinMode(11, OUTPUT); //this is for salinity
  Serial.begin(9600); // use a baud rate of 9600 bps
  
  salinity.begin(); 

  getSalinity();delay(1000);
  getSalinity();delay(1000);
  getSalinity();delay(1000);
}


//Main Code goes here
void loop() {
  getSalinity();//Gets Salinity and prints the values 
  getTemp();//Gets temperature and prints the values 
  printLCD();//updates LCD Screen
  
  calculateGain();
 // Serial.print("water needed   ");
 // Serial.println(gainH2O);
 // Serial.print("Salt Water Needed   ");
 // Serial.println(gainNACL);
  delay(1000);//test

  
  //corrects oversalinity
  if (salinityV > salinityUL){ //corrects oversalinity
    diWaterOn();
    delay(1000*(gainH2O/3.025));
    diWaterOff();
    //DEADTIME
  for (int i=0; i <= 6; i++){// this is the dead time
    getSalinity();
    getTemp();
    printLCD();
    delay(1000);
    Serial.println("We are in Dead Time");
  }
  }
  
  //corrects undersalinity
  if (salinityV < salinityLL){
    naclWaterOn();
    for(int i = 0; i <= gainNACL/4.2; i++){
      delay(1000);
      printLCD(); 
    }
    //delay(1000*(gainNACL/4.2));
    naclWaterOff();
    printLCD();
     for (int i=0; i <= 6; i++){// this is the dead time
    getSalinity(); //this updates rolling average in deadtime
    getTemp();
    printLCD();
    delay(1000);
    Serial.println("We are in Dead Time");
       }
    }
  }














//This Function was written by Caleb Hopkins 2/23/2019
void printLCD(){
  LCD.begin(9600); // use a baud rate of 9600 bps
  

  LCD.write(12); // clear screen & move to top left position
  
  LCD.write(132); //this puts LCL in the top left corner and sets the Lower Control Limit Column
  LCD.write("LCL");
  
  LCD.write(139);  //this prints SP and sets the Set Point column
  LCD.write("SP"); 
  
  LCD.write(144);  // this prints UCL and sets the Upper Control Limit Column
  LCD.write("UCL");

  LCD.write(148);   // this prints S:
  LCD.write("S:");

  LCD.write(151);   // this prints the salinity Lower Limit to the LCD
  LCD.print(salinityLL,3);

  LCD.write(157);   // this prints the Salinity setpoint to the LCD
  LCD.print(salinitySP,3);

  LCD.write(163);   // this prints the salinity Upper Limit to the LCD
  LCD.print(salinityUL,3);

  LCD.write(168);   // this prints T:
  LCD.write("T:");

  LCD.write(172);   // this prints the temperature Lower Limit
  LCD.print(tempLL,1);
 
  LCD.write(178);   // this prints the temperature Set Point
  LCD.print(tempSP,1);

  LCD.write(183);   // this prints the temperature Upper Limit
  LCD.print(tempUL,1);

  LCD.write(188);   // this prints S= to the lcd
  LCD.write("S=");

  LCD.write(190);   // this prints the actual salinity
  LCD.print(salinityV,3);

  LCD.write(196);   // this prints the T=
  LCD.write("T=");

  LCD.write(198);   // this prints the actual temperature value
  LCD.print(tempV,1);

  LCD.write(203);   // this prints H=
  LCD.write("H=");

  if (tempV < tempLL) //if the actual temperature value is less than the Lower Limit then the LCD will indicate if the heater is on or off.
  {
  LCD.write(205);
  LCD.write("on"); //prints on
  heaterOn(); //turns the heater on
  }
  else
  {
  
  LCD.write(205);
  LCD.write("off"); // prints off
  heaterOff();//turns the heater off
  }
  LCD.write(22);
}

//this function was written by Caleb Hopkins on 2/23/2019
void heaterOn(){
  digitalWrite(5, HIGH);//this turns the heater on
}

//this function was written by Caleb Hopkins on 2/23/2019
void heaterOff(){
  digitalWrite(5, LOW);//this turns the heater off
}

//this function was written by Caleb Hopkins on 2/25/2019
void getTemp() {
  tempRAW = analogRead(2);
  tempV = (tempRAW-259)/10.2;
  Serial.print("This is the temp:   ");
  Serial.println(tempV);
}

void getSalinity() {
  digitalWrite(11,HIGH); //this gets the raw data from analog pin zero for the salinity sensor
  float salinityRAW = analogRead(0);
  delay(100);
  digitalWrite(11,LOW);
  int sensorMovingAvg = salinity.reading(salinityRAW);
  
  salinityV = 2.07*pow(10,-24)*pow(salinity.getAvg(),7.05)*100;//this calculates the percent salinity in the system.
  Serial.print("salinity value: ");
  Serial.println(salinityV);
}



void calculateGain(){
  
  if (salinityV > salinityUL) //if the water is too salty
  {
    
    //This Code deals with mass balance by finding the amount
    //of water needed to correct the system
    
    mOfSalt = salinityV/100*mOfSoln;
    mOfWater = mOfSoln - mOfSalt;
    waterNeeded = (mOfSalt/.001) - mOfSalt;//finds mass of water in system
    waterNeeded = waterNeeded - mOfWater;//calculates the water needed in the system
    gainH2O = waterNeeded*.8;
  }
  
  if (salinityV < salinityLL)//if water is not salty enough
  {
    
    //This Code deals with mass balance by finding the amount 
    //of salt in the system and calculating how much salt is needed
    
    mOfSalt = salinityV/100*mOfSoln;
    mOfWater = mOfSoln - mOfSalt;
    saltWNeeded = (.001*mOfSoln) - mOfSoln*(salinityV/100);//Salt Needed
    saltWNeeded = saltWNeeded/.01;//Actual amount of Salt Water Needed
    gainNACL = saltWNeeded*.8;
  }
}


//This function turns the DI Water Solenoid on
void diWaterOn(){
  digitalWrite(4, HIGH);
}

//This function turns the DI Water Solenoid off
void diWaterOff(){
  digitalWrite(4, LOW);
}

//This function turns the NACL Water Solenoid On
void naclWaterOn(){
  digitalWrite(3, HIGH);
}

//This function turns the NACL Water Solenoid Off
void naclWaterOff(){
  digitalWrite(3, LOW);
}

//DATA ROUNDING BELOW
/*
void salinityRound()
{
  
  float rawSalinity = analogRead(0);
 
  arrayOfSalinity[counterSal++] = rawSalinity;
  if (counterSal >= runningAverageCount)
  {
    counterSal = 0; 
  }
  runningAverageSalinity = 0;
  for(int i=0; i< runningAverageCount; ++i)
  {
    runningAverageSalinity += arrayOfSalinity[i];
  }
  runningAverageSalinity /= runningAverageCount;
}



void salinityRound()  //Salinity rounding data
{
int valueSalV1; //first value
int valueSalV2; //second value
int valueSalV3; //third value
counterSal ++; //makes the things cycle between the three values we need
if (counterSal == 1) 
{
  valueSalV1 = salinityRAW; //sets the first value to the new value
}
if (counterSal == 2)
{
  valueSalV2 = salinityRAW; //sets the second value to the new value
}
if (counterSal == 3)
{
  valueSalV3 = salinityRAW; //sets the third value to the new value
  counterSal = 0;  //resets the counter for the data
}

salinityDS = ((valueSalV1+valueSalV2+valueSalV3)/3); //data smoothing part
}

void tempRound()  //Salinity rounding data
{
int valueTempV1; //first value
int valueTempV2; //second value
int valueTempV3; //third value
counterTemp ++; //makes the things cycle between the three values we need
if (counterTemp == 1) 
{
  valueTempV1 = tempRAW; //sets the first value to the new value
}
if (counterTemp == 2)
{
  valueTempV2 = tempRAW; //sets the second value to the new value
}
if (counterTemp == 3)
{
  valueTempV3 = tempRAW; //sets the third value to the new value
  counterTemp = 0;  //resets the counter for the data
}

tempDS = ((valueTempV1+valueTempV2+valueTempV3)/3); //data smoothing part
}
void correctValues(){
  if (salinityV > salinityUL){ //corrects oversalinity
    diWaterOn();
    delay(1000*(gainH2O/3.025));
    diWaterOff();
  }

  if (salinityV < salinityLL){
    naclWaterOn();
    delay(1000*(gainNACL/4.2));
    naclWaterOff();
  }
}
*/
