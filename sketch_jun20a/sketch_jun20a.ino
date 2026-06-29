const int MOSFET_GATE_PIN = 1;

void setup() {
// Initialize the pin as an output
  pinMode(MOSFET_GATE_PIN, OUTPUT);
  
  // Start with the MOSFET turned OFF (HIGH = OFF for P-channel)
  digitalWrite(MOSFET_GATE_PIN, HIGH);
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("AO3401 P-Channel MOSFET Test Starting...");
}

void loop() {

  // Turn ON the MOSFET (LOW pulls gate to GND, completing the circuit)
  Serial.println("MOSFET: ON (Load should be running)");
  digitalWrite(MOSFET_GATE_PIN, LOW); 
  delay(10000); // Wait 2 seconds

  // Turn OFF the MOSFET (HIGH pulls gate to 3.3V, breaking the circuit)
  Serial.println("MOSFET: OFF (Load should be stopped)");
  digitalWrite(MOSFET_GATE_PIN, HIGH); 
  delay(10000); // Wait 2 seconds

}

