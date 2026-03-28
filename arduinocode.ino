// ===== Pins =====
#define IR_LEFT 4
#define IR_RIGHT 5
#define ROOM_IR 6
#define ULTRA_TRIG 7
#define ULTRA_ECHO 8

#define MOTOR_IN1 9
#define MOTOR_IN2 10
#define MOTOR_IN3 11
#define MOTOR_IN4 12

#define BUZZER 3
#define LED_GREEN 2
#define SWITCH_REQUEST A2

// ===== Variables =====
#define OBSTACLE_DISTANCE 15 // cm
#define ROOM_WAIT_TIME 2000  // ms wait at room if no request
#define LINE_SEARCH_DELAY 200
#define SEARCH_TURN_DELAY 300

int requestsCount = 0;
bool atRoom = false;
bool journeyComplete = false;

// ===== Setup =====
void setup() {
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(ROOM_IR, INPUT);

  pinMode(ULTRA_TRIG, OUTPUT);
  pinMode(ULTRA_ECHO, INPUT);

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  pinMode(SWITCH_REQUEST, INPUT_PULLUP); // rocker switch

  Serial.begin(9600);
}

// ===== Ultrasonic distance =====
long measureDistance() {
  digitalWrite(ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRA_TRIG, LOW);
  long duration = pulseIn(ULTRA_ECHO, HIGH);
  long distance = duration * 0.034 / 2; // cm
  return distance;
}

// ===== Motor control =====
void moveForward() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
}

void turnLeft() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
}

void turnRight() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
}

void stopMotors() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
}

// ===== LED blink helper =====
void blinkLED(int pin, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    delay(500);
  }
}

// ===== Search for line if lost =====
bool searchLine() {
  // try left
  turnLeft();
  delay(SEARCH_TURN_DELAY);
  if(digitalRead(IR_LEFT) == HIGH || digitalRead(IR_RIGHT) == HIGH) return true;

  // try right
  turnRight();
  turnRight();
  delay(SEARCH_TURN_DELAY);
  if(digitalRead(IR_LEFT) == HIGH || digitalRead(IR_RIGHT) == HIGH) return true;

  // return to original direction
  turnLeft();
  delay(SEARCH_TURN_DELAY);
  
  // line not found
  return false;
}

// ===== Main Loop =====
void loop() {

  if(journeyComplete) {
    // Blink LEDs after journey
    if(requestsCount > 0) blinkLED(LED_GREEN, requestsCount);
    
    // Buzzer beep 1 sec per request with 1 sec gap
    for(int i=0;i<requestsCount;i++){
      tone(BUZZER,1000,1000);
      delay(2000); // 1 sec beep + 1 sec gap
    }

    // Reset for next journey
    requestsCount = 0;
    journeyComplete = false;
    return;
  }

  int leftSensor = digitalRead(IR_LEFT);
  int rightSensor = digitalRead(IR_RIGHT);
  int roomSensor = digitalRead(ROOM_IR);
  long distance = measureDistance();

  // ===== Obstacle detection =====
  if(distance < OBSTACLE_DISTANCE) {
    stopMotors();
    // simple obstacle avoid: turn right and continue
    turnRight();
    delay(SEARCH_TURN_DELAY);
    return;
  }

  // ===== Line following =====
  if(leftSensor == HIGH && rightSensor == HIGH) {
    moveForward();
  } else if(leftSensor == LOW && rightSensor == HIGH) {
    turnLeft();
  } else if(leftSensor == HIGH && rightSensor == LOW) {
    turnRight();
  } else {
    // Line lost: search in all directions
    stopMotors();
    bool found = searchLine();
    if(!found) {
      // No line in any direction → reached end (pharmacy)
      stopMotors();
      Serial.println("Reached pharmacy, reporting LEDs");
      journeyComplete = true;
      return;
    }
    return;
  }

  // ===== Room detection =====
  if(roomSensor == HIGH && !atRoom) {
    atRoom = true; // detected new room
    stopMotors();
    delay(500); // small pause before switch check

    if(digitalRead(SWITCH_REQUEST) == LOW) {
      // Request taken
      requestsCount++;
      tone(BUZZER, 1000, 1000); // beep 1 sec
      delay(1000); // wait for beep
    } else {
      // No request → wait fixed time and continue
      delay(ROOM_WAIT_TIME);
    }
  }

  // Reset room detection once line appears again
  if(roomSensor == LOW && atRoom) {
    atRoom = false;
  }
}