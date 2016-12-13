
// 6.301 Fall 2016
// Temp Sensor Dual-Slope ADC Firmware
// V1

#include <OneWire.h>
#include <DallasTemperature.h>

// Pinout
const byte sw_ptat = 6;       // IPTAT switch
const byte sw_ref = 4;        // IREF switch
const byte sw_int_short = 3;  // Integrator short/reset switch
const byte sw_startup = 5;    // PTAT startup bias switch
const byte pin_comp_out = 2;  // Comparator output (ensure interrupt capable)
const byte pin_temp_sensor = 12;  // DS18B20 one-wire pin

// One-Wire Temp Sensor Object
OneWire oneWire(pin_temp_sensor);
DallasTemperature sensors(&oneWire);
DeviceAddress tempsensor1 = { 0x28, 0x2E, 0x03, 0x10, 0x05, 0x00, 0x00, 0xF8 };

// Program Variables
int samplecnt = 0; // keep track of number of samples
unsigned long avg = 0; // to average collected data
int firstrun = 1; // to prevent garbage data from being printed when sampling was not run yet

// Manual Calibration Offset
const float calconstant = 25 + 21;

// Timing Values
const uint16_t ramp_up_delta_t_us = 5000;
unsigned long ramp_down_start_time_us = 0;
unsigned long ramp_down_end_time_us = 0;
unsigned long ramp_down_delta_t_us = 0;

// Setup Routine
void setup() {

  // Power Saving
  ADCSRA = ADCSRA & B01111111; // disable ADC
  ACSR = B10000000;            // disable analog comparator
  DIDR0 = DIDR0 | B00111111;   // disable input buffers on analog pins

  // Configure IO pins
  pinMode(sw_ptat, OUTPUT);
  pinMode(sw_ref, OUTPUT);
  pinMode(sw_int_short, OUTPUT);
  pinMode(sw_startup, OUTPUT);
  pinMode(pin_comp_out, INPUT);

  // Start serial terminal
  Serial.begin(115200);

  // Start One-Wire bus
  sensors.begin();
  sensors.setResolution(tempsensor1, 11);  // 11 bits, 0.125C resolution, 375ms conv time
   
  // Configure interrupt
  attachInterrupt( digitalPinToInterrupt( pin_comp_out ), isr_comp, RISING );

  // Default pin values
  digitalWrite( sw_ptat, LOW );
  digitalWrite( sw_ref, LOW );
  digitalWrite( sw_int_short, LOW );

  // Startup PTAT
  digitalWrite( sw_startup, HIGH );
  delay(30);
  digitalWrite( sw_startup, LOW );

  // Ramp the integrator all the way up so that the ADC can start
  // even if the integrator initial condition is negative
  digitalWrite( sw_ptat, HIGH );
  digitalWrite( sw_ref, LOW );
  
  delay(200);

  // Start the first ramp-down
  digitalWrite( sw_ptat, LOW );
  digitalWrite( sw_ref, HIGH );

}



void loop() {
  cli();  // Disable interrupts
  unsigned long ramp_down_delta_t_us_sample = ramp_down_delta_t_us;
  unsigned long ramp_down_start_time_us_sample = ramp_down_start_time_us;
  unsigned long ramp_down_end_time_us_sample = ramp_down_end_time_us;
  
  sei();  // Enable interrupts
  
  sensors.requestTemperatures();
  
  if (samplecnt == 0){
    // reset things, as temp sampling has stopped or cycle has been interrupted
    digitalWrite( sw_int_short, HIGH );
    digitalWrite( sw_startup, HIGH );
    delay(50);
    digitalWrite(sw_startup, LOW );
    digitalWrite(sw_int_short, LOW );
    Serial.println("Reset asserted by cycle watchdog");
    firstrun = 1;
    
  } else if(firstrun == 1) {

    firstrun = 0;
    avg = 0;
    samplecnt = 0;
    
  } else {

    cli();
    avg = avg / (samplecnt);
    unsigned long avg_sample = avg;
    avg = 0;
    samplecnt = 0;
    sei();

    float local_temp = calculate_temperature_calibrated(avg_sample);
    float cal_temp = sensors.getTempCByIndex(0);
    float error = local_temp - cal_temp;

    Serial.print(ramp_down_delta_t_us_sample);
    Serial.print(",");
    Serial.print(local_temp);
    Serial.print(",");
    Serial.println(cal_temp);

  }


  
  delay(100);
}

// sample cycle finished
void isr_comp() {
  cli();  // Disable interrupts

  // Stop the integrator ramp-down
  digitalWrite( sw_ref, LOW );

  // Figure out the ramp-down time
  ramp_down_end_time_us = micros();
  ramp_down_delta_t_us = ramp_down_end_time_us - ramp_down_start_time_us;

  samplecnt += 1;
  avg = (avg + ramp_down_delta_t_us);

  // State 3: Reset
  digitalWrite( sw_int_short, HIGH );
  delayMicroseconds(10);
  digitalWrite( sw_int_short, LOW );
  
  // Stage 1: Ramp-up

  // Start the integrator ramp-up
  digitalWrite(sw_ptat, HIGH);

  // Integrate the known current for a known amount of time
  delayMicroseconds(ramp_up_delta_t_us);

  // Stop the integrator ramp-up
  digitalWrite(sw_ptat, LOW);

  // State 2: Ramp-down

  // Start the integrator ramp-down
  digitalWrite(sw_ref, HIGH);
  ramp_down_start_time_us = micros();

  EIFR = 0x01; // Clear any pending interrupts on INT0

  sei(); // Enable interrupts
}

// Uses the calibration curve to calculate the temperature
float calculate_temperature( unsigned long ramp_down_delta_t_us ) {
  return (ramp_down_delta_t_us*171e-6*1.6e-19*1e3)/(ramp_up_delta_t_us*1.38e-23*3*1.792) - 273.15;
}

float calculate_temperature_calibrated( unsigned long ramp_down_delta_t_us ) {
  return calculate_temperature( ramp_down_delta_t_us ) + calconstant;
}
