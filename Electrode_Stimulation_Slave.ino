/*
  Electrode_Stimulation_Slave.ino
  Code run by peripheral slave Arduino Nano that controls electrode stimulation
  Kevin Wu 
  BME 349
*/
#include <CurieBLE.h>
#include <EEPROM.h>
 
BLEPeripheral blePeripheral;  
BLEService StimService("0000181a-0000-1000-8000-00805f9b34fb");
 
// Custom 128-bit UUIDs, read and writable by master/central
BLECharacteristic StimCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic CalibrationCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214")
const int ADDR_EEPROM_USER_VOLT 0;   // byte address of user calibrated voltage limit in EEPROM
 
const int electrode = A0; // analog output pin 0 for electrical stimulation, built in DAC
const int potentiometer = A1; // analog input pin 1 for reading potentiometer
int stim_state;  // State of current electrical stimulation: 0=Ramp Down, 1=GradualRampUp, 2=RapidRampUp
int MAX_V_OUT;   // Maximum voltage output, as calibrated by user
// Maximum current output is 3.3V and will be split to two electrodes; base voltage output from Arduino is 1.65V to each electrode 
int v_out;       // Current voltage delivered via each electrode, will be value between [0,255] corresponding to [0,3.3] V
 
void setup() {
    Serial.begin(9600);
 
    // set electrode pins to output mode
    pinMode(electrode, OUTPUT);
 
    // set advertised local name and service UUID:
    blePeripheral.setLocalName("Electrode Stimulation");
    blePeripheral.setAdvertisedServiceUuid(StimService.uuid());
    
    blePeripheral.addAttribute(StimService);    // add service  
    blePeripheral.addAttribute(StimCharacteristic); // add characteristic
 
    StimCharacteristic.setValue(0); // set the initial value for the characeristic:
    blePeripheral.begin();  // begin advertising BLE service:
    stim_state = 0; // default to off
    v_out = 0;      // default to 0 voltage output
    MAX_V_OUT = EEPROM.read(ADDR_EEPROM_USER_VOLT);
}
 
void loop() {
    // listen for BLE peripherals to connect:
    BLECentral central = blePeripheral.central();
 
    // If a central is connected to peripheral slave 
    if (central) {
        while (central.connected()) {
            // If calibration button pressed and central/master recently wrote to characteristic, update MAX_V_OUT
            if(CalibrationCharacteristic.written()) {
                long starttime = millis();
                long endtime = starttime;
                while((endtime - starttime) <= 60000){  // User calibration for 60 seconds
                    int user_input = analogRead(potentiometer);
                    analogWrite(A0, user_input/4);  // 0-1023 values to 0-255 values, output to electrodes
                    endtime = millis();
                }
                MAX_V_OUT = user_input/4;
                EEPROM.update(ADDR_EEPROM_USER_VOLT, MAX_V_OUT);      // store current SPO2 as constant in EEPROM memory to be retrieved upon startup
                CalibrationCharacteristic.write(0x01);
            }
            // If central/master recently wrote to the characteristic, update electrode stimulation
            if (StimCharacteristic.written()) {
                if (!StimCharacteristic.value() == 1) {   // 1 written = Gradual Ramp Up
                    stim_state = 1;
                    GradualRampUp();
                } else if(StimCharacteristic.value() == 2) {    // 2 written = Rapid Ramp Up
                    stim_state = 2;
                    GradualRampUp();
                } else{ // 0 or anything else written = Ramp Down
                    stim_state = 0;
                    RampDown();
                }
            }
        }
  }
 
    // State check code that will be always run 
    if(stim_state == 1){
        GradualRampUp();
    } else if(stim_state == 2){
        RapidRampUp();
    } else {
        RampDown();    
    }
}
 
void GradualRampUp(){
    // Output voltage to electrodes while less than user calibrated voltage limit 
    analogWrite(A0, v_out); 
    if(v_out < MAX_V_OUT){
        v_out++;
    }
    delay(10/MAX_V_OUT);  // Total of 10 seconds to reach user calibrated voltage limit  
}
 
void RapidRampUp(){
    // Output voltage to electrodes while less than user calibrated voltage limit
    analogWrite(A0, v_out); 
    if(v_out < MAX_V_OUT){
        v_out++;
    }
    delay(5/MAX_V_OUT);   // Total of 5 seconds to reach user calibrated voltage limit 
}
 
void RampDown(){
    if(v_out != 0){
        analogWrite(A0, v_out); 
        v_out--;
        delay(10/MAX_V_OUT) // Total of 10 seconds to ramp down from existing voltage
    }
}
