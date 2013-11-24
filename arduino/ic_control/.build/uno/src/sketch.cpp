#include <Arduino.h>
#include "Wire.h"
void setup();
void loop();
void programReg();
void runSweep();
void writeData(int addr, int data);
int readData(int addr);
boolean measureTemperature();
#line 1 "src/sketch.ino"
//#include "Wire.h"
#define button 2
#define SLAVE_ADDR 0x0D
#define ADDR_PTR 0xB0

#define START_FREQ_R1 0x82
#define START_FREQ_R2 0x83
#define START_FREQ_R3 0x84

#define FREG_INCRE_R1 0x85
#define FREG_INCRE_R2 0x86
#define FREG_INCRE_R3 0x87

#define NUM_INCRE_R1 0x88
#define NUM_INCRE_R2 0x89

#define NUM_SCYCLES_R1 0x8A 
#define NUM_SCYCLES_R2 0x8B 

#define RE_DATA_R1 0x94
#define RE_DATA_R2 0x95

#define IMG_DATA_R1 0x96
#define IMG_DATA_R2 0x97

#define TEMP_R1 0x92
#define TEMP_R2 0x93

#define CTRL_REG 0x80
#define STATUS_REG 0x8F




void setup() {
	Wire.begin();
	Serial.begin(9600);
	pinMode(button, INPUT); 
	programReg();
}


void loop(){

  delay(5000);
  measureTemperature();
  runSweep();
}



void programReg(){

	// Set Range 2, PGA gain 1
	writeData(CTRL_REG,0x07);

	// Set settling cycles
	writeData(NUM_SCYCLES_R1, 0x07);
	writeData(NUM_SCYCLES_R2, 0xFF);

	// Start frequency of 1kHz
	writeData(START_FREQ_R1, 0);
	writeData(START_FREQ_R2, 0x83);
	writeData(START_FREQ_R3, 0x12);

	// Increment by 1 kHz
	writeData(FREG_INCRE_R1, 0); 
	writeData(FREG_INCRE_R2, 0x83); 
	writeData(FREG_INCRE_R3, 0x12);

	// Points in frequency sweep (100), max 511
	writeData(NUM_INCRE_R1, 0);
	writeData(NUM_INCRE_R2, 0x64);

	// Standby '10110000'
	writeData(CTRL_REG, 0xB0);

	// Initialize sweep
	writeData(CTRL_REG, 0x10);

}


void runSweep() {
	int re;
	int img;
	double mag;
	int i=0;

	// Start sweep
	writeData(CTRL_REG, 0x20);	


	while((readData(STATUS_REG) & 7) < 4 ) {  // Check that status reg != 4, sweep not complete
		delay(100); // delay between measurements

		re  = readData(RE_DATA_R1) << 8;
		re |= readData(RE_DATA_R2);

		// for negative re value
		if(re > 0x7FFF){
			re &= 0x7FFF;
		 	re -= 0x10000;
		}

		img  = readData(IMG_DATA_R1) << 8;
		img |= readData(IMG_DATA_R2);

		// for negative img value
		if(img > 0x7FFF){
			img &= 0x7FFF;
			img -= 0x10000;
		}


		mag = sqrt(pow(double(re),2)+pow(double(img),2));
		//gain, impedance, phase

		Serial.print("Real:");
		Serial.println(re);

		Serial.print("Imag:");
		Serial.println(img);
		Serial.println(" ");
		
		//Increment frequency
		if((readData(STATUS_REG) & 7) < 4 )
			writeData(CTRL_REG,0x30);

	}


	//Power down
	writeData(CTRL_REG,0xA0);


}



void writeData(int addr, int data) {

 Wire.beginTransmission(SLAVE_ADDR);
 Wire.write(addr);
 Wire.write(data);
 Wire.endTransmission();
 delay(1);

}


int readData(int addr){
	int data;

	Wire.beginTransmission(SLAVE_ADDR);
	Wire.write(ADDR_PTR);
	Wire.write(addr);
	Wire.endTransmission();

	delay(1);

	Wire.requestFrom(SLAVE_ADDR,1);

	if (Wire.available() >= 1){
		data = Wire.read();
	}
	else {
		data = -1;
	}

	delay(1);
	return data;	
}





boolean measureTemperature() {
  
	// Measure temperature '10010000'
	writeData(CTRL_REG, 0x90);
	//TODO: necessary to write to second control register?

	delay(10); // wait for 10 ms

	

	//Check status reg for temp measurement available
	int flag = readData(STATUS_REG)& 7;

  if (flag == 1) {

    // Temperature is available
    int temperatureData = readData(TEMP_R1) << 8;
    temperatureData |= readData(TEMP_R2);
    temperatureData &= 0x3FFF; // remove first two bits
    
    if (temperatureData & 0x2000 == 1) { // negative temperature
      
      temperatureData -= 0x4000;
    }
    
    temperatureData /= 32;
    
    Serial.print("Temperature: ");
    Serial.print(temperatureData);
    Serial.write(176);
    Serial.println("C.");
    

    // Power Down '10100000'
    writeData(CTRL_REG,0xA0);

    
    return true;

  } else {
    return false;
  }
}
