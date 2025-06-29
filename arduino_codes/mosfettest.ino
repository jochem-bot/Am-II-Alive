const int mosfetGatePin = 3;  


int brightness = 0;



void setup() {
  
  pinMode(mosfetGatePin, OUTPUT);
  
  
  Serial.begin(9600);
  Serial.println("MOSFET LED Control Initialized.");
}

void loop() {
  analogWrite(mosfetGatePin, 30);
  delay(30); 
}
