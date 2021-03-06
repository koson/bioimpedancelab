#include "Wire.h"
#define button 2

#define START_FREQ_R1 0x82
#define START_FREQ_R2 0x83
#define START_FREQ_R3 0x84

#define FREG_INCRE_R1 0x85
#define FREG_INCRE_R2 0x86
#define FREG_INCRE_R3 0x87

#define NUM_INCRE_R1 0x88
#define NUM_INCRE_R2 0x89

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

onButtonPressed();

}



void onButtonPressed(){
int flag;
boolean tempFlag;
flag = digitalRead(button);

if(flag){
	//runSweep();
	delay(3000);
	tempFlag = measureTemperature();
	Serial.println(tempFlag);
	}
}


void programReg(){

	// Set Range 2, PGA gain 1
	writeData(CTRL_REG,0x07,0);

	// Set settling cycles
	writeData(0x8a, 0x07, 0);
	writeData(0x8b, 0xff, 0);


	// Start frequency of 1kHz
	writeData(START_FREQ_R1, 0, 0);
	writeData(START_FREQ_R2, 0x83, 0);
	writeData(START_FREQ_R3, 0x12, 0);

	// Increment by 1 kHz
	writeData(FREG_INCRE_R1, 0, 0); 
	writeData(FREG_INCRE_R2, 0x83, 0); 
	writeData(FREG_INCRE_R3, 0x12, 0);


	// Points in frequency sweep (100), max 511
	writeData(NUM_INCRE_R1, 0, 0);
	writeData(NUM_INCRE_R2, 0x64, 0);

	// Standby '10110000'
	writeData(CTRL_REG, 0xb0, 1);

	// Initialize sweep
	writeData(CTRL_REG, 0x10, 1);

}


void runSweep() {
	int re[100];
	int img[100];
	int i=0;

	// Start sweep
	writeData(CTRL_REG, 0x20, 1);	

		while (true) {
			
			// Check for availability of data
			if ((readData(0x96) & 0x04)==4){
				writeData(CTRL_REG,0xa0,0);
				break;
			}		

			// Real Data
			re[i]=readData(0x94) + readData(0x95);
			img[i++]=readData(0x96) + readData(0x97);

			Serial.print("Real:");
			Serial.println(re[i-1]);

			Serial.print("Img:");
			Serial.println(img[i-1]);
			Serial.print("\n");	
		}


}






int readData(int addr){
	Wire.requestFrom(addr, 1);
	int data = Wire.read();
	Wire.endTransmission();
	return data;	
}


void writeData(int addr, int data,  int flag) {

 Wire.beginTransmission(addr);
 Wire.write(data); 
 Wire.endTransmission();

}


boolean measureTemperature() {
  
	// Measure temperature '10010000'
	writeData(CTRL_REG, 0x90, 1);

	delay(10); // wait for 10 ms

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
    
    Serial.print("Current temperature is ");
    Serial.print(temperatureData); 
    Serial.println(" degrees Celsius.");
    

    // Power Down '10100000'
    writeData(CTRL_REG,0xa0,1);

    
    return true;

  } else {
    return false;
  }
}
