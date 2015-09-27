/*

Author: RTB  9/27/2015
This code is intended to be a "quick start" to getting the HMC5983
Initializes and grabs data over SPI. Then sends out data over Serial
in Comma Separated format

HARDWARE SETUP FOR THE ARDUINO UNO
  * CS - to digital pin 10  (SS pin)
  * SDI - to digital pin 11 (MOSI pin)
  * SDO - to digital pin 12 (MISO)
  * CLK - to digital pin 13 (SCK pin)

From datasheet for the HMC5983
Below is an example of a (power-on) initialization process for “continuous-measurement mode” via I2C interface:
1. Write CRA (00) – send 0x3C 0x00 0x70 (8-average, 15 Hz default or any other rate, normal measurement)
2. Write CRB (01) – send 0x3C 0x01 0xA0 (Gain=5, or any other desired gain)
3. For each measurement query:
  Write Mode (02) – send 0x3C 0x02 0x01 (Single-measurement mode)
  Wait 6 ms or monitor status register or DRDY hardware interrupt pin
  Send 0x3D 0x06 (Read all 6 bytes. If gain is changed then this data set is using previous gain)
  Convert three 16-bit 2’s compliment hex values to decimal values and assign to X, Z, Y, respectively.
(Self addition:)
4. Convert the magnetic information into a compass value
REGARDING THE CALCULATION OF THE ACTUAL HEADING VALUE
From AN-203 http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/Magnetic__Literature_Application_notes-documents/AN203_Compass_Heading_Using_Magnetometers.pdf
The magnetic compass heading can be determined (in degrees) from the magnetometer's x and y readings by using the
following set of equations:
  Direction (y>0) = 90 - [arcTAN(x/y)]*180/PI
  Direction (y<0) = 270 - [arcTAN(x/y)]*180/PI
  Direction (y=0, x<0) = 180.0
  Direction (y=0, x>0) = 0.0
MISSING : EARTH DECLINATION ANGLE
In other words, we are not making any compensation for the earth's north pole location vs the magnetic measurement


// MEMORY MAPPING
/*
Address Location  Name    Access
---------------------------------------------------
0x00  Configuration Register A  Read/Write
0x01  Configuration Register B  Read/Write
0x02  Mode Register     Read/Write
0x03  Data Output X MSB Register  Read
0x04  Data Output X LSB Register  Read
0x05  Data Output Z MSB Register  Read
0x06  Data Output Z LSB Register  Read
0x07  Data Output Y MSB Register  Read
0x08  Data Output Y LSB Register  Read
0x09  Status Register     Read
0x0A  Identification Register A Read
0x0B  Identification Register B Read
0x0C  Identification Register C Read
0x31  Temperature Output MSB Register Read
0x32  Temperature Output LSB Register Read

*/

#define HMC5983_CONF_A    0x00
#define HMC5983_CONF_B    0x01
#define HMC5983_MODE    0x02
#define HMC5983_OUT_X_MSB 0x03
#define HMC5983_OUT_X_LSB 0x04
#define HMC5983_OUT_Z_MSB 0x05
#define HMC5983_OUT_Z_LSB 0x06
#define HMC5983_OUT_Y_MSB 0x07
#define HMC5983_OUT_Y_LSB 0x08
#define HMC5983_STATUS    0x09
#define HMC5983_ID_A    0x0A
#define HMC5983_ID_B    0x0B
#define HMC5983_ID_C    0x0C
#define HMC5983_TEMP_OUT_MSB  0x31
#define HMC5983_TEMP_OUT_LSB  0x32
#define HMC5983_ID

// NOTE THAT FOR SPI, the first Bit designates Read or Write
// 0 = Write and 1 = Read
//                    the second bit (M/S) sets whether to increment address or not (0 does not, 1 does)
//                    for multiple byte reading        
//  The other 6 bits (of the 8 bit byte that needs to be sent first before reading OR writing) is he address.               

#include <SPI.h>

// set pin 10 as the slave select 
const int ssPin = 10;
byte val = 0;
byte receivedVal = 0;
byte idA = 0;
byte idB = 0;
byte idC = 0;
byte XH = 0;
byte XL = 0;
byte YH = 0;
byte YL = 0;
byte ZH = 0;
byte ZL = 0;
int X = 0;
int Y = 0;
int Z = 0;

void setup() {
  // put your setup code here, to run once:
 // set the slaveSelectPin as an output:
  pinMode (ssPin, OUTPUT);
  // initialize SPI:
  SPI.begin();
  //  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
  SPI.setClockDivider(128) ;
  SPI.setBitOrder(MSBFIRST);  // data delivered MSB first as in MPU-6000 Product Specification
  SPI.setDataMode(SPI_MODE0); // latched on rising edge, transitioned on falling edge, active low 
}

//////////////////////////////////
void loop() {
  // put your main code here, to run repeatedly:
// Set up the HMC5983 for basic operation
Serial.begin(230400);
delay(500);
//Serial.print("START");
//Serial.print('\n');
//Serial.print(idA);
//Serial.print('\n');
//read_HMC5983_ID();

delay(50);
// SET UP HMC5983
configA(0xFC);  /// Look at data sheet for options (TS, AVG, ODR, MS)
configB(0x00);  /// Gain Setting 000 = highest , 111 = lowest
mode_conf(0x01);   // Mode configuration  


while(1){   //// Loop forever 
mode_conf(0x01);  // 0x01 Single Read mode
read_all_mags();  // Sequentially Read X,Y,and Z Channels 
//readStatus();
delay(10);
        }
}

//////////////////////////////////////////////////
////// FUNCTIONS  //////////////////////////////
/////////////////////////////////////////////////

void HMC_init() {   /////NOTE NOT USED RIGHT NOW>>>
  digitalWrite(ssPin, LOW);
 // SPI.transfer(HMC5983_ADDRESS);
//  SPI.transfer(HMC5983_WRITE);
//  SPI.transfer(HMC5983_CONF_A, );
//  SPI.transfer(HMC5983_CONF_B);
  // take the SS pin high to de-select the chip:
  digitalWrite(ssPin, HIGH);
                }

//  NOW READ THE ID 
 void read_HMC5983_ID() {
 digitalWrite(ssPin, LOW);
 SPI.transfer(0x8A);                //  Reads the ID (there are 3 bytes)
 idA = SPI.transfer(0x00);  // Send dummy bytes so that the clock toggles and we can read data
 digitalWrite(ssPin, HIGH);
 delay(1);

 digitalWrite(ssPin, LOW);
 SPI.transfer(0x8B);
 idB = SPI.transfer(0x00);
 digitalWrite(ssPin, HIGH);
 delay(1);

 digitalWrite(ssPin, LOW);
 SPI.transfer(0x8C);
 idC = SPI.transfer(0x00);
 digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
 delay(1);
 
Serial.print("The address of the HMC5983 is:");
Serial.print('\n');
Serial.print(char(idA));
Serial.print('\n');
Serial.print(char(idB));
Serial.print('\n');
Serial.print(char(idC));
Serial.print('\n');
Serial.print('\n');
                    }

//Configures the A register  (Temp, # samples averaged, Data rate, Bias)
void configA(int Aconfig) {
  digitalWrite(ssPin, LOW);
  SPI.transfer(HMC5983_CONF_A);      // Setup Write to A configuration reg   
  SPI.transfer(Aconfig);  // Send configuration 
  digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
 }

void mode_conf(int mode) {
  digitalWrite(ssPin, LOW);
  SPI.transfer(HMC5983_MODE);                //  Writes the mose register  1000 1001
  SPI.transfer(mode);               // 
  digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
 }

// Configure the B register
void configB(int Bconfig) {
 digitalWrite(ssPin, LOW);
 SPI.transfer(HMC5983_CONF_B);                //  Write the configuration B register  1000 1001
 SPI.transfer(Bconfig);                // Set highest gain values
 digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
  }

void readX() {
 digitalWrite(ssPin, LOW);
 SPI.transfer(0x84);                //  Reads the LSB of the X register
 receivedVal = SPI.transfer(0x00);  // Send dummy bytes so that the clock toggles and we can read data
 digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
Serial.print("X_Mag = ");
Serial.print(~receivedVal+1);
Serial.print('\n');
delay(30);
}

void readT() {
 digitalWrite(ssPin, LOW);
 SPI.transfer(0xB1);                //  Reads the LSB of the Temp register
 receivedVal = SPI.transfer(0x00);  // Send dummy bytes so that the clock toggles and we can read data
 digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
Serial.print("T = ");
Serial.print(receivedVal);
Serial.print('\n');
delay(300);
}

void readStatus() {
 digitalWrite(ssPin, LOW);
 SPI.transfer(0x89);                //  Reads the LSB of the status register
 receivedVal = SPI.transfer(0x0);  // Send dummy bytes so that the clock toggles and we can read data
 digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
Serial.print("Status = ");
Serial.print(receivedVal);
Serial.print('\n');
delay(300);
}

void read_all_mags() {
  digitalWrite(ssPin, LOW);
  SPI.transfer(0xC3);                //  Reads the LSB of the X register
  //delay(1000);
  XH = SPI.transfer(0x00);  // Send dummy bytes so that the clock toggles and we can read data
  XL = SPI.transfer(0x00);
  ZH = SPI.transfer(0x00);
  ZL = SPI.transfer(0x00);
  YH = SPI.transfer(0x00);
  YL = SPI.transfer(0x00);
  digitalWrite(ssPin, HIGH);          // Done with read cycle, so pull SS high
///// NOW CALCULATE FULL TWOS COMP AND SEND VALS TO SERIAL PORT
X = float((XH<<8) + XL);
Y = float((YH<<8) + YL);
Z = float((ZH<<8) + ZL);
Serial.print(X);
Serial.print(" , ");
Serial.print(Y);
Serial.print(" , ");
Serial.println(Z);
}
