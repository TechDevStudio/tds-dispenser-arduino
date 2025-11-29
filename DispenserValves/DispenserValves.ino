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

//constantes de tiempo
#define TIMEOUT_NO_FLOW 10000 //segundos de inactividad para terminar proceso
#define WATER_TIME 2000 //tiempo de agua en limpieza
#define AIR_TIME 2000 //tiempo de aire en limpieza
#define SAFE_DELAY_INTER_PROCESS 200 //tiiempo de espera entre activaciones y desactivaciones
#define UPDATE_READ_INTERVAL 200 //milisegundos para lectura

//constantes de flujo
#define SAFE_LOWFLOW_THRESHOLD 0.3 //bajo esto asumimos no hay dispensado
#define CALIBRATION_FACTOR 4380.0

#define DEBUG true

#define VALVE_ON false
#define VALVE_OFF true

//flujo del proceso
enum FlowState{
  WAITING_COMMAND,
  DISPENSE_START,
  DISPENSING,
  DISPENSE_END,
  CLEAN_CYCLE_START,
  WATER_CLEAN,
  AIR_CLEAN,
  CLEAN_CYCLE_END
};

FlowState current_state = WAITING_COMMAND;

volatile unsigned long totalPulses = 0;
volatile unsigned long lastPulseTime = 0;

unsigned long lastUpdateTime = 0;
unsigned long lastPulseSnapshot = 0;
unsigned long currentMillis = 0;

bool firstRun=true;

void IRAM_ATTR flowSensorISR() {
  totalPulses++;
  lastPulseTime = millis();
}

void FnPrintSerial(String message, bool new_line){
  if(firstRun){
    return;
  }
  if(new_line){
    Serial.println(message);
    Serial2.println(message);
  }else{
    Serial.print(message);
  }
}

void FnStartDispensing(int valve_id){
  current_state = DISPENSE_START;

  //cerramos todo por seguridad
  FnCloseAllValves();

  //abrimos primero valvula de producto
  if (valve_id == 1){
    digitalWrite(VALVE_01,VALVE_OFF); 
  }else if(valve_id == 2){
    digitalWrite(VALVE_02,VALVE_OFF); 
  }else if(valve_id == 3){
    digitalWrite(VALVE_03,VALVE_OFF); 
  }else if(valve_id == 4){
    digitalWrite(VALVE_04,VALVE_OFF); 
  }else if(valve_id == 5){
    digitalWrite(VALVE_05,VALVE_OFF); 
  }else if(valve_id == 6){
    digitalWrite(VALVE_06,VALVE_OFF); 
  }else if(valve_id == 7){
    digitalWrite(VALVE_07,VALVE_OFF); 
  }else if(valve_id == 8){
    digitalWrite(VALVE_08,VALVE_OFF); 
  }
  delay(100);

  FnResetVariables();

  //abrimos valvula de salida
  pinMode(VALVE_SECOUT,OUTPUT);
  delay(100);

  //confirmacion de apertura
  Serial.println("CM_DISP_ON");
  
}

void FnReadFlowSensor(){
  unsigned long currentTime = millis();
    
  // Periodic updates for display/monitoring
  if (currentTime - lastUpdateTime >= UPDATE_READ_INTERVAL) {
    noInterrupts();
    unsigned long pulses = totalPulses;
    interrupts();
    
    // Calculate metrics
    float totalVolume = (pulses / CALIBRATION_FACTOR) * 1000.0; // mL
    
    // Calculate current flow rate
    unsigned long pulsesSinceLastUpdate = pulses - lastPulseSnapshot;
    unsigned long timeDelta = currentTime - lastUpdateTime;
    float flowRate = (pulsesSinceLastUpdate / CALIBRATION_FACTOR) * (60000.0 / timeDelta); // L/min
    
    FnPrintSerial("RSP_PARTIAL_VOLUME:" + String(totalVolume,0),true);
    //FnPrintSerial(String(flowRate,2),false);
    //FnPrintSerial(" L/min",true);
    
    lastPulseSnapshot = pulses;
    lastUpdateTime = currentTime;

    // Check for flow timeout
    if (currentTime - lastPulseTime > TIMEOUT_NO_FLOW) {
      current_state = DISPENSE_END;
      FnPrintSerial("RSP_TOTAL_VOLUME:" + String(totalVolume,0),true);
    }
  }
  

}

void FnFinishDispensing(){
  FnCloseAllValves();
  current_state = CLEAN_CYCLE_START;
  FnPrintSerial("RSP_DISPENSE_END",true);
}



void FnStartCleanCycle(){
  FnPrintSerial("CLEAN_STARTED",true);

  //noInterrupts();

  //abrir valvula de exhaust
  digitalWrite(VALVE_EXHAUST,VALVE_OFF); 
  
  current_state = WATER_CLEAN;
  currentMillis = millis();
}

void FnWaterCleanCycle(){
  //abrir agua
  digitalWrite(VALVE_WATER,VALVE_OFF);
  //FnPrintSerial(String(currentMillis),false);
  //FnPrintSerial("FnWaterCleanCycle",false);
  //FnPrintSerial(String(millis()),true);

  if(millis() > currentMillis + WATER_TIME){
    //cerrar agua
    digitalWrite(VALVE_WATER,VALVE_ON);
    current_state = AIR_CLEAN;
    currentMillis = millis();
  }
}

void FnAirCleanCycle(){
  //abrir aire
  digitalWrite(VALVE_AIR,VALVE_OFF);
  //FnPrintSerial(String(currentMillis),false);
  //FnPrintSerial("FnAirCleanCycle",false);
  //FnPrintSerial(String(millis()),true);
  if(millis() > currentMillis + AIR_TIME){
    //cerrar aire
    digitalWrite(VALVE_AIR,VALVE_ON);
    current_state = CLEAN_CYCLE_END;
    currentMillis = millis();
  }
}

void FnEndCleanCycle(){
  //cerrar exhaust
  digitalWrite(VALVE_EXHAUST,VALVE_ON); 
  //FnPrintSerial("CLEAN_FINISHED",true);
  if(millis() > currentMillis + SAFE_DELAY_INTER_PROCESS){
    //interrupts();
    FnPrintSerial("CM_READY",true);
    current_state = WAITING_COMMAND;
    currentMillis = millis();
    firstRun = false;
  }
}



void FnResetVariables(){
  totalPulses = 0;
  lastPulseSnapshot = 0;
  lastPulseTime = millis();
  lastUpdateTime = millis();
}

void FnCloseAllValves(){
  digitalWrite(VALVE_01,VALVE_ON);  
  digitalWrite(VALVE_02,VALVE_ON);
  digitalWrite(VALVE_03,VALVE_ON);  
  digitalWrite(VALVE_04,VALVE_ON);
  digitalWrite(VALVE_05,VALVE_ON);  
  digitalWrite(VALVE_06,VALVE_ON);
  digitalWrite(VALVE_07,VALVE_ON);  
  digitalWrite(VALVE_08,VALVE_ON);
  
  digitalWrite(VALVE_EXHAUST,VALVE_ON);  
  digitalWrite(VALVE_AIR,VALVE_ON);
  digitalWrite(VALVE_WATER,VALVE_ON);  
  
  digitalWrite(VALVE_SECOUT,VALVE_ON);

  delay(300);
}

void FnReadSerial(){
  if (Serial2.available() > 0) {
    String command = Serial2.readStringUntil('\n');
    command.trim();

    if (command == "CM_VALVE01") {
      // Dispensar 1
      FnStartDispensing(1);
    } else if (command == "CM_VALVE02") {
      // Dispensar 2
      FnStartDispensing(2);
    } else if (command == "CM_VALVE03") {
      // Dispensar 3
      FnStartDispensing(3);
    } else if (command == "CM_VALVE04") {
      // Dispensar 4
      FnStartDispensing(4);
    } else if (command == "CM_VALVE05") {
      // Dispensar 5
      FnStartDispensing(5);
    } else if (command == "CM_VALVE06") {
      // Dispensar 6
      FnStartDispensing(6);
    } else if (command == "CM_VALVE07") {
      // Dispensar 7
      FnStartDispensing(7);
    } else if (command == "CM_VALVE08") {
      // Dispensar 8
      FnStartDispensing(8);
    } else if (command == "CM_FORCEEND") {
      // Dispensar 8
      current_state = DISPENSE_END;
    }else {
      Serial.print("Unknown command: ");
      Serial.println(command);
    }
  }
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

  current_state = CLEAN_CYCLE_START;

  currentMillis = millis();
}



void loop() {

  switch(current_state){
    case WAITING_COMMAND:
      FnReadSerial();
    break;
    case DISPENSE_START:
      current_state = DISPENSING;
      currentMillis = millis();
    break;
    case DISPENSING:
      FnReadFlowSensor();
    break;
    case DISPENSE_END:
      FnFinishDispensing();
    break;
    case CLEAN_CYCLE_START:
      FnStartCleanCycle();
    break;
    case WATER_CLEAN:
      FnWaterCleanCycle();
    break;
    case AIR_CLEAN:
      FnAirCleanCycle();
    break;
    case CLEAN_CYCLE_END:
      FnEndCleanCycle();
    break;
  } 
}





