#include <SPI.h>       
#include <MFRC522.h>   
#include <Servo.h>     

// Pins
#define RST_PIN           9      
#define SS_PIN            10      
#define TRIG_PIN_1        2   
#define ECHO_PIN_1        19  
#define TRIG_PIN_2        3   
#define ECHO_PIN_2        18  
#define FLAME_PIN         6    
#define GAS_PIN           4  
#define SOUND_PIN         5    
#define BUZZER_SIGNAL     A3 
#define BUZZER_GND        A2    
#define RGB_R             8        
#define RGB_G             7        
#define SERVO_PIN         A0   
#define RELAY_PIN         A1   

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo radarServo;      

// Alarm status flags
bool flameDetected = false, gasDetected = false, soundDetected = false, intrusionDetected = false, alarmActive = false;

// Timers for 10 seconds cooldowns
unsigned long flameClearTime = 0, gasClearTime = 0, soundClearTime = 0, intrusionClearTime = 0;

// System states
bool flameWaitingToClear = false, gasWaitingToClear = false, soundWaitingToClear = false, intrusionWaitingToClear = false;
bool greenLedActive = false, waitingForRFID = false, ignoreEntryLogic = false, blinkState = false, toneToggleState = false;      

// Timers for events, LEDs and servo motor
unsigned long entryTime = 0, lastBlinkTime = 0, exitIgnoreStartTime = 0, greenLedStartTime = 0, lastServoTime = 0, lastSirenTime = 0, lastToneSwitchTime = 0;
unsigned long sensor1TriggerTime = 0, sensor2TriggerTime = 0;

bool sensor1First = false, sensor2First = false;
int servoStep = 0, servoAngleTarget = 0, sirenFrequency = 600;
unsigned long servoDelay = 15; 
const unsigned long rfidTimeout = 10000; // 10 seconds to scan RFID

// Gate status
enum Sequence { NONE, ENTRY, EXIT };
Sequence currentSequence = NONE;

// Get ultrasonic distance (15ms timeout)
int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2); 
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 15000); 
  return (duration == 0) ? 999 : duration * 0.034 / 2;                   
}

void triggerGreenLed5Sec() { 
  if (!flameDetected && !gasDetected && !soundDetected && !intrusionDetected) {
    digitalWrite(RGB_G, HIGH); 
    greenLedActive = true; 
    greenLedStartTime = millis(); 
  }
}

void handleServoAndBuzzer() {
  // Sound for drone syren
  static unsigned long localLastSirenTime = 0;
  static int localSirenFrequency = 600;
  static bool localSirenGoingUp = true;

  bool anyAlarmActive = (flameDetected || gasDetected || soundDetected || intrusionDetected);
  if (anyAlarmActive) digitalWrite(RGB_R, HIGH); else if (!waitingForRFID) digitalWrite(RGB_R, LOW);

  // Drone detected: spotlight begins searching
  if (soundDetected) { 
    digitalWrite(RELAY_PIN, HIGH);
    if (millis() - lastServoTime >= servoDelay) {
      lastServoTime = millis(); 
      int currentPos = radarServo.read();
      if (currentPos <= 0) servoAngleTarget = 180; 
      if (currentPos >= 180) servoAngleTarget = 0;
      if (servoAngleTarget == 180 && currentPos < 180) radarServo.write(currentPos + 1);
      else if (servoAngleTarget == 0 && currentPos > 0) radarServo.write(currentPos - 1);
    }
  } else {
    // Return spotlight to sleeping condition
    digitalWrite(RELAY_PIN, LOW); 
    int currentPos = radarServo.read();
    if (currentPos != 90) {
      if (millis() - lastServoTime >= 10) { 
        lastServoTime = millis(); 
        if (currentPos < 90) radarServo.write(currentPos + 1); 
        if (currentPos > 90) radarServo.write(currentPos - 1); 
      }
    }
  }

  // Siren sound
  if (soundDetected) { 
    if (millis() - localLastSirenTime >= 10) { 
      localLastSirenTime = millis();
      if (localSirenGoingUp) { 
        localSirenFrequency += 15; 
        if (localSirenFrequency >= 1200) localSirenGoingUp = false; 
      } else { 
        localSirenFrequency -= 15; 
        if (localSirenFrequency <= 500) localSirenGoingUp = true; 
      }
      tone(BUZZER_SIGNAL, localSirenFrequency); 
    }
  } 
  else if (flameDetected) { 
    // Fire alarm sound
    if (millis() - lastToneSwitchTime >= 250) { 
      lastToneSwitchTime = millis(); 
      toneToggleState = !toneToggleState; 
    }
    tone(BUZZER_SIGNAL, toneToggleState ? 1100 : 800);
  } 
  else if (gasDetected) { 
    // Gas leak alarm sound
    if (millis() - lastToneSwitchTime >= 70) { 
      lastToneSwitchTime = millis(); 
      toneToggleState = !toneToggleState; 
    }
    if (toneToggleState) tone(BUZZER_SIGNAL, 1900); else noTone(BUZZER_SIGNAL);
  } 
  else if (intrusionDetected) { 
    // Illegal enterance alarm sound
    if (millis() - lastToneSwitchTime >= 400) { 
      lastToneSwitchTime = millis(); 
      toneToggleState = !toneToggleState; 
    }
    if (toneToggleState) tone(BUZZER_SIGNAL, 600); else noTone(BUZZER_SIGNAL);
  }
  else { 
    noTone(BUZZER_SIGNAL); 
    if (!waitingForRFID) { 
      digitalWrite(BUZZER_SIGNAL, LOW); 
      digitalWrite(RGB_R, LOW); 
    } 
  }
}

void checkAlarmTimeout() {
  if (greenLedActive && (millis() - greenLedStartTime >= 5000)) { 
    digitalWrite(RGB_G, LOW); 
    greenLedActive = false; 
  }
  if (ignoreEntryLogic && (millis() - exitIgnoreStartTime >= 8000)) { 
    ignoreEntryLogic = false; 
    currentSequence = NONE; 
  }
}

void checkFlameSensor() {
  bool currentFlameState = digitalRead(FLAME_PIN); 
  if (currentFlameState == LOW) { 
    flameWaitingToClear = false; 
    if (!flameDetected) { 
      flameDetected = true; 
      alarmActive = true; 
      Serial.println(F("[ALERT] Fire detected in the sector!")); 
    }
  } else if (currentFlameState == HIGH && flameDetected) { 
    if (!flameWaitingToClear) { 
      flameClearTime = millis(); 
      flameWaitingToClear = true; 
      Serial.println(F("[STATUS] Fire was partly localized")); 
    }
    if (flameWaitingToClear && (millis() - flameClearTime >= 10000)) { 
      flameDetected = false; 
      flameWaitingToClear = false; 
      if (!gasDetected && !soundDetected && !intrusionDetected) alarmActive = false; 
      Serial.flush(); 
      mfrc522.PCD_Init(); // SPI reset
      Serial.println(F("[OK] Fire completely extinguished.")); 
    }
  }
}
void checkGasSensor() {
  bool currentGasState = digitalRead(GAS_PIN);     
  if (currentGasState == LOW) { 
    gasWaitingToClear = false; 
    if (!gasDetected) { 
      gasDetected = true; 
      alarmActive = true; 
      Serial.println(F("[ALERT] Gas leak detected!")); 
    }
  } else if (currentGasState == HIGH && gasDetected) { 
    if (!gasWaitingToClear) { 
      gasClearTime = millis(); 
      gasWaitingToClear = true; 
      Serial.println(F("[STATUS] Gas level decreased.")); 
    }
    if (gasWaitingToClear && (millis() - gasClearTime >= 10000)) { 
      gasDetected = false; 
      gasWaitingToClear = false; 
      if (!flameDetected && !soundDetected && !intrusionDetected) alarmActive = false; 
      Serial.flush(); 
      mfrc522.PCD_Init(); 
      Serial.println(F("[OK] Gas completely gone. Air quality normal.")); 
    }
  }
}

void checkSoundSensor() {
  bool isNoiseDetected = digitalRead(SOUND_PIN);
  if (isNoiseDetected == LOW) { 
    soundWaitingToClear = false; 
    if (!soundDetected) { 
      soundDetected = true; 
      alarmActive = true; 
      Serial.println(F("[ALERT] Unauthorized drone detected above the object!")); 
    }
  } else if (isNoiseDetected == HIGH && soundDetected) { 
    if (!soundWaitingToClear) { 
      soundClearTime = millis(); 
      soundWaitingToClear = true; 
      Serial.println(F("[STATUS] Drone target lost. Monitoring airspace...")); 
    }
    if (soundWaitingToClear && (millis() - soundClearTime >= 10000)) { 
      soundDetected = false; 
      soundWaitingToClear = false; 
      if (!flameDetected && !gasDetected && !intrusionDetected) alarmActive = false; 
      Serial.flush(); 
      mfrc522.PCD_Init(); 
      Serial.println(F("[OK] Airspace is clear. Drone was neutralized.")); 
    }
  }
}

// Logic of entrance and exit
void processPassageLogic(bool s1, bool s2) {
  unsigned long now = millis(); 
  if (ignoreEntryLogic) return;
  
  if (!s1 && !s2) { sensor1First = false; sensor2First = false; }
  if (s1 && !s2 && !sensor1First && !sensor2First && !intrusionDetected) { sensor1First = true; sensor1TriggerTime = now; }
  if (s2 && !s1 && !sensor1First && !sensor2First) { sensor2First = true; sensor2TriggerTime = now; }
  
  // 3 seconds to trigger the second sensor
  if (sensor1First && (now - sensor1TriggerTime > 3000)) sensor1First = false;
  if (sensor2First && (now - sensor2TriggerTime > 3000)) sensor2First = false;

  // Person enters
  if (sensor1First && s2 && (now - sensor1TriggerTime < 3000) && !intrusionDetected) {
    sensor1First = false; 
    if (flameDetected || gasDetected || soundDetected) {
      // Trigger alarm if someone enters during emergency
      Serial.println(F("[ALERT] Attention! Illegal entry detected during emergency."));
      intrusionWaitingToClear = false; 
      intrusionDetected = true; 
      alarmActive = true; 
      waitingForRFID = false;
    } else {
      currentSequence = ENTRY; 
      entryTime = now; 
      waitingForRFID = true; 
      Serial.println(F("[STATUS] Entry detected. The authorisation card was required..."));
    }
  }
  
  // Person exits
  if (sensor2First && s1 && (now - sensor2TriggerTime < 3000)) {
    sensor2First = false; 
    currentSequence = EXIT; 
    waitingForRFID = false; 
    ignoreEntryLogic = true; 
    exitIgnoreStartTime = now; 
    Serial.println(F("[STATUS] Exit detected. Free passage allowed.")); 
    triggerGreenLed5Sec();
  }
  
  // Cooldown for perimeter alarm reset
  if (intrusionDetected) {
    if (!intrusionWaitingToClear) { intrusionClearTime = now; intrusionWaitingToClear = true; }
    if (intrusionWaitingToClear && (now - intrusionClearTime >= 10000)) { 
      intrusionDetected = false; 
      intrusionWaitingToClear = false; 
      if (!flameDetected && !gasDetected && !soundDetected) alarmActive = false;
      sensor1First = false; 
      sensor2First = false; 
      Serial.flush(); 
      mfrc522.PCD_Init(); 
      Serial.println(F("[OK] Perimeter alarm reset..")); 
    }
  }
}

// Logic for the card verification
void handleRFIDWait() {
  if (ignoreEntryLogic || currentSequence == EXIT) { waitingForRFID = false; return; }
  
  // Flash red LED while waiting for the card
  if (millis() - lastBlinkTime >= 250) { 
    lastBlinkTime = millis(); 
    blinkState = !blinkState; 
    digitalWrite(RGB_R, blinkState ? HIGH : LOW); 
  }
  
  // Trigger alarm if user hasn't put the authorisation card in 10 seconds
  if (millis() - entryTime > rfidTimeout) {
    waitingForRFID = false; 
    currentSequence = NONE;
    if (flameDetected || gasDetected || soundDetected) { 
      Serial.println(F("[ALERT] Attention! Illegal entry detected.")); 
    } else { 
      Serial.println(F("[ALERT] Attention! Authorisation card was not put.")); 
      intrusionWaitingToClear = false; 
      intrusionDetected = true; 
      alarmActive = true; 
    }
    return;
  }
  
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return; 
  
  // Card is put successfully
  digitalWrite(RGB_R, LOW); 
  Serial.println(F("[OK] RFID sucessfully put. Welcome to the facility."));
  tone(BUZZER_SIGNAL, 1500, 300); 
  triggerGreenLed5Sec(); 
  waitingForRFID = false; 
  currentSequence = NONE; 
  mfrc522.PICC_HaltA();           
}

void setup() {
  Serial.begin(9600); // Bluetooth
  SPI.begin(); 
  mfrc522.PCD_Init();          
  radarServo.attach(SERVO_PIN); 
  radarServo.write(90);         
  
  pinMode(TRIG_PIN_1, OUTPUT); pinMode(ECHO_PIN_1, INPUT); 
  pinMode(TRIG_PIN_2, OUTPUT); pinMode(ECHO_PIN_2, INPUT);  
  pinMode(FLAME_PIN, INPUT); pinMode(GAS_PIN, INPUT); pinMode(SOUND_PIN, INPUT);
  pinMode(RGB_R, OUTPUT); pinMode(RGB_G, OUTPUT); 
  digitalWrite(RGB_R, LOW); digitalWrite(RGB_G, LOW);    
  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW); 
  pinMode(BUZZER_GND, OUTPUT); digitalWrite(BUZZER_GND, LOW); 
  pinMode(BUZZER_SIGNAL, OUTPUT); digitalWrite(BUZZER_SIGNAL, LOW); 
  
  // Greeting sound
  tone(BUZZER_SIGNAL, 1000, 100); delay(150); 
  tone(BUZZER_SIGNAL, 1500, 100); delay(150); 
  tone(BUZZER_SIGNAL, 2000, 100); delay(150); 
  noTone(BUZZER_SIGNAL);
  
  Serial.println(F("[SYSTEM] Initialization successful. Security system is online."));
}

void loop() {
  checkFlameSensor(); 
  checkGasSensor(); 
  checkSoundSensor(); 
  checkAlarmTimeout(); 
  handleServoAndBuzzer(); 
  
  if (waitingForRFID) { handleRFIDWait(); return; }
  
  int dist1 = getDistance(TRIG_PIN_1, ECHO_PIN_1); 
  int dist2 = getDistance(TRIG_PIN_2, ECHO_PIN_2);
  
  // Keeping the distance between 1cm and 15cm for the ultrasonic sensors
  processPassageLogic((dist1 >= 1 && dist1 <= 15), (dist2 >= 1 && dist2 <= 15));
}
