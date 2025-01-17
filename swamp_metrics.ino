/*--------------------------------------------------
Author:   Aidan Vancil + Andrew Gorum
Project:  Creating A Embedded System (Swamp Cooler)
Comments: N/A
Date:     11/09/22
---------------------------------------------------*/
#include <LiquidCrystal.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Stepper.h>
//#include <Wire.h>
//#include "DS1307.h"

#define RDA 0x80
#define TBE 0x20

// Temperature / Humidity
float temp;
#define DHTPIN 6       // DHT Pin
#define DHTTYPE DHT11  // DHT Type

DHT dht(DHTPIN, DHTTYPE);

// Stepper Motor
const int stepsPerRevolution = 200;

// Initialize the stepper library on pins 2 through 5:
const float STEPS_PER_REV = 32;
const float GEAR_RED = 64;
const float STEPS_PER_OUT_REV = STEPS_PER_REV * GEAR_RED;
int StepsRequired;
int Pval;
int potVal;
Stepper myStepper(STEPS_PER_REV, 2, 4, 3, 5);

// Define Port B Register Pointers
volatile unsigned char *port_b = (unsigned char *)0x25;
volatile unsigned char *ddr_b = (unsigned char *)0x24;
volatile unsigned char *pin_b = (unsigned char *)0x23;

// Define Port D Register Pointers
volatile unsigned char *port_d = (unsigned char *)0x2B;
volatile unsigned char *ddr_d = (unsigned char *)0x2A;
volatile unsigned char *pin_d = (unsigned char *)0x29;

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int *myUBRR0 = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char *)0x00C6;

volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;

#define WRITE_HIGH_PD(pin_num) *port_d |= (0x01 << pin_num);
#define WRITE_LOW_PD(pin_num) *port_d &= ~(0x01 << pin_num);
// LCD
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

void setup() {
  // setup the UART
  //U0init(9600);
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);
  myStepper.setSpeed(200);
  lcd.clear();
  
  /*
  //RTC setup
  clock.begin();
  clock.fillByYMD(2022,11,23);//Nov 23,2022
  clock.fillByHMS(09,01,00);//09:01 00
  clock.fillDayOfWeek(WED);//
  clock.setTime();//write time to the RTC chip
  */
  
  //lcd.print("Water Level: ");

  //adc_init();

  // PD0

}

void loop() {
  lcd.clear();
  bool button_1 = *pin_d & 0b00000010;
  bool button_2 = *pin_d & 0b00000001;   
  if (button_1){
    myStepper.step(50);
    //printTime();

  }
  if (button_2){
    myStepper.step(-50);
    //printTime();

  }
  float humidity = dht.readHumidity();
  float temp_celsius = dht.readTemperature();
  float temp_farenheit = dht.readTemperature(true);
  lcd.print("Temp   : ");
  lcd.print(temp_farenheit);
  lcd.print("\337F");
  lcd.setCursor(0,1);
  lcd.print("Humid %: ");
  lcd.print(humidity);
  delay(1000);


  // for (int pos = 0; pos < 13; pos++){
  //   lcd.scrollDisplayLeft();
  //   delay(10000);
  // }
  // for (int pos = 0; pos < 13; pos++){
  //   lcd.scrollDisplayRight();
  //   delay(10000);
  // }


  // get the reading from the ADC
  // unsigned int adc_reading = adc_read(0);
  // print_int(adc_reading);
  // if (adc_reading > 200){
  //   WRITE_HIGH_PD(7);
  // } else {
  //   WRITE_LOW_PD(7);
  // }
}

void adc_init() {
  // setup the A register
  *my_ADCSRA |= 0b10000000;  // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111;  // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111;  // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000;  // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111;  // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000;  // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX &= 0b01111111;  // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX |= 0b01000000;  // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX &= 0b11011111;  // clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11100000;  // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num) {
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if (adc_channel_num > 7) {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while ((*my_ADCSRA & 0x40) != 0)
    ;
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

void print_int(unsigned int out_num) {
  // clear a flag (for printing 0's in the middle of numbers)
  unsigned char print_flag = 0;
  // if its greater than 1000
  if (out_num >= 1000) {
    // get the 1000's digit, add to '0'
    U0putchar(out_num / 1000 + '0');
    // set the print flag
    print_flag = 1;
    // mod the out num by 1000
    out_num = out_num % 1000;
  }
  // if its greater than 100 or we've already printed the 1000's
  if (out_num >= 100 || print_flag) {
    // get the 100's digit, add to '0'
    U0putchar(out_num / 100 + '0');
    // set the print flag
    print_flag = 1;
    // mod the output num by 100
    out_num = out_num % 100;
  }
  // if its greater than 10, or we've already printed the 10's
  if (out_num >= 10 || print_flag) {
    U0putchar(out_num / 10 + '0');
    print_flag = 1;
    out_num = out_num % 10;
  }
  // always print the last digit (in case it's 0)
  U0putchar(out_num + '0');
  // print a newline
  U0putchar('\n');
}

void U0init(int U0baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  // Same as (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

/*
void printTime(){

    clock.getTime();
    Serial.print(clock.hour, DEC);
    Serial.print(":");
    Serial.print(clock.minute, DEC);
    Serial.print(":");
    Serial.print(clock.second, DEC);
    Serial.print("  ");
    Serial.print(clock.month, DEC);
    Serial.print("/");
    Serial.print(clock.dayOfMonth, DEC);
    Serial.print("/");
    Serial.print(clock.year+2000, DEC);
    Serial.print(" ");
    Serial.print(clock.dayOfMonth);
    Serial.println(" ");
}
*/
unsigned char U0kbhit() {
  return *myUCSR0A & RDA;
}
unsigned char U0getchar() {
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata) {
  while ((*myUCSR0A & TBE) == 0)
    ;
  *myUDR0 = U0pdata;
}
