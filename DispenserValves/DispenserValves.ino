//SENSOR PIN
#define FLOW_SENSOR_PIN 34
//COMMON VALVES PINS
#define VALVE_EXHAUST 14
#define VALVE_AIR 15
#define VALVE_WATER 18
#define VALVE_SECOUT 19
//BEVERAGES VALVES PINS
#define VALVE_01 21
#define VALVE_02 22
#define VALVE_03 23
#define VALVE_04 25
#define VALVE_05 26
#define VALVE_06 27
#define VALVE_07 32
#define VALVE_08 33
//COMMUNICATION WITH DISPLAY PINS
#define COMM_RX_02 16
#define COMM_TX_02 17

const int noInteractionTime = 3000; //segundos de inactividad para terminar proceso
const int waterTime = 2000;
const int airTime = 2000;

volatile int pulseCount = 0;
float flowRate = 0.0;
float totalVolume = 0.0;



void IRAM_ATTR flowSensorISR() {
  pulseCount++;
}

void FnCleanCycle(){
  //guitar interrupst y cerrar todas las valvulas
  noInterrupts();
  digitalWrite(VALVE_01,HIGH);  
  digitalWrite(VALVE_02,HIGH);
  digitalWrite(VALVE_03,HIGH);  
  digitalWrite(VALVE_04,HIGH);
  digitalWrite(VALVE_05,HIGH);  
  digitalWrite(VALVE_06,HIGH);
  digitalWrite(VALVE_07,HIGH);  
  digitalWrite(VALVE_08,HIGH);
  
  digitalWrite(VALVE_EXHAUST,HIGH);  
  digitalWrite(VALVE_AIR,HIGH);
  digitalWrite(VALVE_WATER,HIGH);  
  
  digitalWrite(VALVE_SECOUT,HIGH);

  delay(300);
  
  //abrir valvula de exhaust
  digitalWrite(VALVE_EXHAUST,LOW); 
  delay(100);
  //abrir agua
  digitalWrite(VALVE_WATER,LOW);
  delay(waterTime);
  //cerrar agua
  digitalWrite(VALVE_WATER,HIGH);
  delay(100);
  
  //abrir aire
  digitalWrite(VALVE_AIR,LOW);
  delay(airTime);
  //cerrar aire
  digitalWrite(VALVE_AIR,HIGH);
  delay(100);

  //cerrar exhaust
  digitalWrite(VALVE_EXHAUST,HIGH); 
  delay(100);

  //habilitar interrupts
  interrupts();
}

void setup() {

  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  pinMode(VALVE_01,OUTPUT);  
  pinMode(VALVE_02,OUTPUT);
  pinMode(VALVE_03,OUTPUT);  
  pinMode(VALVE_04,OUTPUT);
  pinMode(VALVE_05,OUTPUT);  
  pinMode(VALVE_06,OUTPUT);
  pinMode(VALVE_07,OUTPUT);  
  pinMode(VALVE_08,OUTPUT);
  pinMode(VALVE_EXHAUST,OUTPUT);  
  pinMode(VALVE_AIR,OUTPUT);
  pinMode(VALVE_WATER,OUTPUT);  
  pinMode(VALVE_SECOUT,OUTPUT);

  // Attach interrupt on rising edge
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowSensorISR, RISING);

  Serial.begin(115200); // Debug
  Serial2.begin(115200, SERIAL_8N1, COMM_RX_02, COMM_TX_02); // display

  FnCleanCycle();
}

void loop() {
  // Calculate flow every second
  static unsigned long lastTime = 0;
  
  if (millis() - lastTime >= 100) {
    // Disable interrupts while reading
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();
    
    // Calculate flow rate (adjust calibration factor for your sensor)
    // Most sensors: pulses per second / calibration factor = L/min
    flowRate = (pulses / 7.5); // Example: YF-S201 uses 7.5 pulses/L
    
    // Add to total volume
    totalVolume += (flowRate / 60.0); // Convert to liters
    
    Serial.print("Flow: ");
    Serial.print(flowRate);
    Serial.print(" L/min | Total: ");
    Serial.print(totalVolume);
    Serial.println(" L");
    
    lastTime = millis();
  }
}