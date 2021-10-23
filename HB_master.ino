/*
  Pulse_Ox_Master.ino
  Code run by master Arduino that receives SpO2 data and relays electrostimulation commands to slave Arduino
  Kevin Wu
  BME 349
  
  Sensor setup and data collection is based off SparkFun Electronic's open source MAX3010X Library
  Expected Peripheral (Slave) Service UUID: 0000181a-0000-1000-8000-00805f9b34fb
*/
 
#include <Wire.h>       // I2C comms with sensor
#include <CurieBLE.h> // Bluetooth comms between Arduinos
#include "MAX30105.h"   // SparkFun library written for 30105 sensor but is compatible with 30102 version
#include <EEPROM.h>   // ROM, nonvolatile memory
#include "spo2_algorithm.h" //  SparkFun module for SpO2 calculation
 
MAX30105 spo2_sensor;
 
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 50 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t IRBuffer[50]; //infrared LED sensor data measuring oxygenated hemoglobin
uint16_t redBuffer[50];  //red LED sensor data measuring deoxygenated hemoglobin
#else
uint32_t IRBuffer[50]; //infrared LED sensor data measuring oxygenated hemoglobin
uint32_t redBuffer[50];  //red LED sensor data measuring deoxygenated hemoglobin
#endif
 
const int ADDR_EEPROM_NORM_SPO2 0;   // byte address of patient specific normal SPO2 in EEPROM
const int ADDR_EEPROM_CALIBRATION 1; // byte address of boolean checking if device has been calibrated or not
int32_t NORM_SPO2; // Normal SpO2 value unique to patient  
int32_t spo2; // current SPO2 value
int8_t validSPO2; // Indicator to show if the SPO2 calculation is valid
int stim_state;  // State of current electrical stimulation
bool initialized; // Power-up flag, fill sensor data on first pass only
 
#define MAX_BRIGHTNESS 255
#define byte ledBrightness = 0; // Off 
#define byte sampleAverage = 4; 
#define byte ledMode = 2; // Red and IR light
#define byte sampleRate = 100; 
#define int pulseWidth = 411; 
#define int adcRange = 4096; 
#define StimServiceUUID = "0000181a-0000-1000-8000-00805f9b34fb";
#define StimCharacteristicUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
#define CalibrationCharacteristicUUID = "19B10002-E8F2-537E-4F6C-D104768A1214";
 
void setup()
{
    Serial.begin(115200); // initialize serial communication at 115200 bits per second:
    // Initialize SpO2 sensor
    if (!spo2_sensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
    {
    while (1);
    }
    spo2_sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); 
 
    calibrated = EEPROM.read(ADDR_EEPROM_CALIBRATION);
    if(calibrated){
        NORM_SPO2 = EEPROM.read(ADDR_EEPROM_NORM_SPO2);
    }
    // Initialize Bluetooth connection hardware  
    BLE.begin();
    initialized = false;    // First pass force fill sensor data 
    stim_state = 0;         // Electrical stimulation default off
    attachInterrupt(digitalPinToInterrupt(2), Calibrate, CHANGE); // Listener for calibration button    @@
}
 
void loop()
{
    if(!calibrated){    // Wait for calibration button to be pressed upon initial use
        delay(5000);
        continue;
    }
    if(!initialized){   // SpO2 computation only upon powerup 
        ComputeSPO2();
        initialized = true; 
    }
   
    // Continuously taking samples from MAX30102 and calculate SpO2 every second
    // Overwrite oldest 25 samples with newest 25 samples to open space for new data
    for (byte i = 25; i < 50; i++)
    {
        redBuffer[i - 25] = redBuffer[i];
        IRBuffer[i - 25] = IRBuffer[i];
    }
    for (byte i = 25; i < 50; i++)  // New sensor data
    {
        while (spo2_sensor.available() == false) // False when no new input received from sensor
        spo2_sensor.check(); // Check the sensor for new data
 
        redBuffer[i] = spo2_sensor.getRed();
        IRBuffer[i] = spo2_sensor.getIR();
        spo2_sensor.nextSample(); 
    }
 
    maxim_heart_rate_and_oxygen_saturation(IRBuffer, 50, redBuffer, &spo2, &validSPO2, 0, 0);
    BLE.scanForUuid(StimServiceUUID); // Start scanning for peripheral Slave Arduino
    BLEDevice peripheral = BLE.available(); // check if peripheral device has been discovered
    if(peripheral){
        BLE.stopScan(); 
    }
 
    // Electrical Stimulation Logic
    if(spo2 < NORM_SPO2 - 3){   // Current SpO2 under normal SpO2 level by over 3%
        ControlStimulation(1);  // Gradual ramp up electrical stimulation
    }
    else if(spo2 <= 88){        // Current SpO2 under universal safe SpO2 limit
        ControlStimulation(2);  // Rapid ramp up electrical stimulation
    }
    else(){                     // Current SpO2 is in normal range 
        ControlStimulation(0);  // Ramp down electrical stimulation
    }
    
}
 
void ComputeSPO2(){
    // Read the first 50 samples and calculate signal range
    for (byte i = 0 ; i < 50 ; i++)
    {
    while (spo2_sensor.available() == false) // False when no new input received from sensor
        spo2_sensor.check(); // Check the sensor for new data
 
    redBuffer[i] = spo2_sensor.getRed();
    IRBuffer[i] = spo2_sensor.getIR();
    spo2_sensor.nextSample();
    }
    //calculate SpO2 after first 50 samples (first 2 seconds of samples) Note: there is no built in function to calculate SpO2 separately
    maxim_heart_rate_and_oxygen_saturation(IRBuffer, 50, redBuffer, &spo2, &validSPO2, 0, 0);
}
 
// Calibration function to be run when first operating device, and whenever calibration button on device is pressed 
void Calibrate(){
    ComputeSPO2();
    // store current SPO2 as constant in EEPROM memory to be retrieved upon startup     
    EEPROM.update(ADDR_EEPROM_NORM_SPO2, spo2);   
    // update device status to calibrated   
    EEPROM.update(ADDR_EEPROM_CALIBRATION, true);
    
    peripheral.connect();
    BLECharacteristic CalibrationCharacteristic = peripheral characteristic(CalibrationCharacteristicUUID);
    CalibrationCharacteristic.writeByte(0x00);  // Set to not calibrated
    delay(1000);
    while(!CalibrationCharacteristic.readByte()){   // Wait for peripheral/slave to finish calibrating 
        delay(1000);
    }
    calibrated = true;
}
 
void ControlStimulation(int command){
    if (command != stim_state){ // State changed
        peripheral.connect();
        peripheral.discoverAttributes();    // read attributes of peripheral slave
        BLECharacteristic StimCharacteristic = peripheral characteristic(StimCharacteristicUUID);
        stim_state = command;
        while(peripheral.connected()){
            if(stim_state = 0){
                StimCharacteristic.writeByte(0x00); // Ramp down stimulation         
            }
            if(stim_state = 1){
                StimCharacteristic.writeByte(0x01); // Gradually ramp up stimulation         
            }
            if(stim_state = 2){
                StimCharacteristic.writeByte(0x02); // Rapidly ramp up stimulation         
            }
        peripheral.disconnect();
    } 
}
