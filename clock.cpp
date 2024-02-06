#include <stdio.h>
#include <wiringPi.h>
#include <math.h>
#include <ADCDevice.hpp>
#include <stdlib.h>
#include <wiringPiI2C.h>
#include <pcf8574.h>
#include <lcd.h>
#include <time.h>

//variables to turn on/off backlight with button
const int buttonPin = 1;
int backlightState = LOW;
int buttonState = HIGH;  
int lastbuttonState = HIGH;
long lastChangeTime;
long captureTime = 20; //time button needs to be pressed to change state
int currentButtonState;


int pcf8574_address = 0x27;// PCF8574T:0x27, PCF8574AT:0x3F
const int BASE = 64;// any number above 64
//Define the output pins of the PCF8574, which are directly connected to the LCD1602 pin.
const int RS = BASE+0;
const int RW = BASE+1;
const int EN = BASE+2;
const int LED = BASE+3;
const int D4 = BASE+4;
const int D5 = BASE+5;
const int D6 = BASE+6;
const int D7 = BASE+7;
int lcdhd;

char weekdays[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

ADCDevice *adc; 

ADCDevice* getAdc(){
	adc = new ADCDevice();
	if(adc->detectI2C(0x48)){    
        delete adc; 
        adc = new PCF8591(); 
    }
    else if(adc->detectI2C(0x4b)){
        delete adc;
        adc = new ADS7830();
    }
    else{
		adc = NULL;
	}
	return adc;
}

void printTemperature(){
	int adcValue = adc->analogRead(0);  //read analog value A0 pin    
	float voltage = (float)adcValue / 255.0 * 3.3;   
	float thermistorResistance = 10 * voltage / (3.3 - voltage);
	float kelvinTemp = 1/(1/(273.15 + 25) + log(thermistorResistance/10)/3950.0); 
	float celciusTemp = kelvinTemp -273.15; 
	float fahrenheitTemp = celciusTemp * 1.8 + 32;
	printf("ADC value : %d  ,\tVoltage : %.2fV, \tTemperature : %.2fC\n",adcValue,voltage,celciusTemp);
	lcdPosition(lcdhd,0,0); //set lcd position to top row first col
	lcdPrintf(lcdhd,"Temp:%.0fF, %.0fC", fahrenheitTemp, celciusTemp);
}

void printDataTime(){
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    printf("%s \n",asctime(timeinfo));
    lcdPosition(lcdhd,0,1); //set lcd position to bottom row first col
    printf("tm_wday = %d, day = %s\n",timeinfo->tm_wday, weekdays[timeinfo->tm_wday]);
    lcdPrintf(lcdhd,"%s, %02d:%02d:%02d",weekdays[timeinfo->tm_wday],timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec); //Display system time on LCD
	
}

int detectI2C(int addr){
    int _fd = wiringPiI2CSetup (addr);   
    if (_fd < 0){		
        printf("Error address : 0x%x \n",addr);
        return 0 ;
    } 
    else{	
        if(wiringPiI2CWrite(_fd,0) < 0){
            printf("Not found device in address 0x%x \n",addr);
            return 0;
        }
        else{
            printf("Found device in address 0x%x \n",addr);
            return 1 ;
        }
    }
}

void setUpPcf8574(){
	pcf8574Setup(BASE,pcf8574_address);//initialize PCF8574
    for(int i=0;i<8;i++){
        pinMode(BASE+i,OUTPUT);     //set PCF8574 port to output mode
    }
}

void toggleBacklight(){
	
	currentButtonState = digitalRead(buttonPin);
	if (currentButtonState != lastbuttonState){
		lastChangeTime = millis();
	}
	//if changing-state of the button lasts longer than captureTime, we consider that 
		//the current button state is an effective change rather than a buffeting
		if(millis() - lastChangeTime > captureTime){
			
			if(currentButtonState != buttonState){
				buttonState = currentButtonState;
				//if the state is low, it means the action is pressing
				if(buttonState == LOW){
					printf("Button is pressed\n");
					backlightState = !backlightState;
					if(backlightState){
						printf("Backlight is ON\n");
					}
					else {
						printf("Bakclight is OFF\n");
					}
				}
				//if the state is high, it means the action is releasing
				else {
					printf("Button is released\n");
				}
			}
		} else{
			printf("Button not pressed for long enough or buffeting !\n");
		}
		digitalWrite(LED,backlightState);
		lastbuttonState = currentButtonState;
		
}


int main(void){
    
    printf("This program is a clock with temperature \n"
    "and a button to toggle lcd screen backlight on and off \n");
    
    adc = getAdc();
    if (adc == NULL){
        printf("No correct I2C address found, \n"
        "Please use command 'i2cdetect -y 1' to check the I2C address! \n"
        "Program Exit. \n");
        return -1;
    }
    
    if(detectI2C(0x27)){
        pcf8574_address = 0x27;
    }else if(detectI2C(0x3F)){
        pcf8574_address = 0x3F;
    }else{
        printf("No correct I2C address found, \n"
        "Please use command 'i2cdetect -y 1' to check the I2C address! \n"
        "Program Exit. \n");
        return -1;
    }
     
    setUpPcf8574();
    
	lcdhd = lcdInit(2,16,4,RS,EN,D4,D5,D6,D7,0,0,0,0);// initialize LCD and return “handle” used to handle LCD
    if(lcdhd == -1){
        printf("lcdInit failed !");
        return 1;
    }
    
    digitalWrite(LED,backlightState);
    digitalWrite(RW,LOW); //allow writing to LCD
    pinMode(buttonPin, INPUT);
    pullUpDnControl(buttonPin, PUD_UP); 
    
    while(1){
		toggleBacklight();
		printTemperature();
		printDataTime();       
        delay(1000);
    }
    return 0;
}
