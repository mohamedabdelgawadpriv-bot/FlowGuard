#define NUM_LIGHTS 4

// Pin configurations
const int RED_PINS[NUM_LIGHTS]   = {2, 4, 6, 8};
const int YELLOW_PINS[NUM_LIGHTS] = {10, 11, 12, 13};  // Optional: add yellow LEDs
const int GREEN_PINS[NUM_LIGHTS] = {3, 5, 7, 9};

// Traffic control data
int road_order[NUM_LIGHTS] = {0, 1, 2, 3};
int green_time[NUM_LIGHTS] = {0};
bool has_ambulance[NUM_LIGHTS] = {false};  // Track emergency status

// Timing constants
const int YELLOW_TIME = 3000;  // 3 seconds yellow warning
const int ALL_RED_TIME = 2000; // 2 seconds all-red safety buffer

// Status tracking
bool systemActive = false;
unsigned long lastActivityTime = 0;
const unsigned long IDLE_TIMEOUT = 300000; // 5 minutes idle timeout

void setup() {
    Serial.begin(9600);
    
    // Initialize all pins
    for (int i = 0; i < NUM_LIGHTS; i++) {
        pinMode(RED_PINS[i], OUTPUT);
        pinMode(GREEN_PINS[i], OUTPUT);
        pinMode(YELLOW_PINS[i], OUTPUT);
        
        // Start with all red
        digitalWrite(RED_PINS[i], HIGH);
        digitalWrite(GREEN_PINS[i], LOW);
        digitalWrite(YELLOW_PINS[i], LOW);
    }
    
    Serial.println("========================================");
    Serial.println("SMART TRAFFIC PRIORITY SYSTEM");
    Serial.println("========================================");
    Serial.println("Status: READY");
    Serial.println("Waiting for traffic data...");
    Serial.println("========================================");
}

void loop() {
    if (Serial.available()) {
        String received = Serial.readStringUntil('\n');
        received.trim();
        
        int ampIndex = received.indexOf('&');
        
        if (ampIndex > 0) {
            lastActivityTime = millis();
            
            String order_str = received.substring(0, ampIndex);
            String green_str = received.substring(ampIndex + 1);
            
            // Parse road order
            parseRoadOrder(order_str);
            
            // Parse green times
            parseGreenTimes(green_str);
            
            // Display received data
            displayReceivedData();
            
            // Execute priority-based traffic control
            executePrioritySequence();
            
            // Reset system for next cycle
            resetSystem();
        }
    }
    
    // Check for idle timeout
    checkIdleTimeout();
    
    delay(50);
}

void parseRoadOrder(String order_str) {
    int lastIndex = 0;
    for (int i = 0; i < NUM_LIGHTS; i++) {
        int commaIndex = order_str.indexOf(',', lastIndex);
        if (commaIndex < 0) commaIndex = order_str.length();
        
        road_order[i] = order_str.substring(lastIndex, commaIndex).toInt();
        lastIndex = commaIndex + 1;
    }
}

void parseGreenTimes(String green_str) {
    int lastIndex = 0;
    for (int i = 0; i < NUM_LIGHTS; i++) {
        int commaIndex = green_str.indexOf(',', lastIndex);
        if (commaIndex < 0) commaIndex = green_str.length();
        
        green_time[i] = green_str.substring(lastIndex, commaIndex).toInt() * 1000;
        
        // Detect emergency (longer green times indicate ambulances)
        has_ambulance[i] = (green_time[i] > 15000); // More than 15 seconds suggests emergency
        
        lastIndex = commaIndex + 1;
    }
}

void displayReceivedData() {
    Serial.println("\n========================================");
    Serial.println("NEW PRIORITY SEQUENCE RECEIVED");
    Serial.println("========================================");
    Serial.println("Processing Order:");
    
    for (int i = 0; i < NUM_LIGHTS; i++) {
        int road = road_order[i];
        int timeInSeconds = green_time[i] / 1000;
        
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(". Road ");
        Serial.print(road);
        Serial.print(" - ");
        Serial.print(timeInSeconds);
        Serial.print("s");
        
        if (has_ambulance[i]) {
            Serial.print(" [EMERGENCY - AMBULANCE DETECTED]");
        }
        
        Serial.println();
    }
    Serial.println("========================================\n");
}

void executePrioritySequence() {
    Serial.println("Starting traffic light sequence...\n");
    
    for (int i = 0; i < NUM_LIGHTS; i++) {
        int currentRoad = road_order[i];
        int duration = green_time[i];
        
        // Display current action
        displayCurrentLight(currentRoad, i);
        
        // Transition to green for current road
        transitionToGreen(currentRoad);
        
        // Keep green for calculated duration
        delay(duration);
        
        // Transition through yellow to red
        transitionToRed(currentRoad);
        
        // All-red safety period between lights
        if (i < NUM_LIGHTS - 1) {
            allRedSafetyPeriod();
        }
    }
    
    Serial.println("\n========================================");
    Serial.println("Traffic sequence completed successfully");
    Serial.println("========================================\n");
}

void displayCurrentLight(int road, int position) {
    Serial.print("[");
    Serial.print(position + 1);
    Serial.print("/");
    Serial.print(NUM_LIGHTS);
    Serial.print("] Road ");
    Serial.print(road);
    Serial.print(" - GREEN for ");
    Serial.print(green_time[position] / 1000);
    Serial.print(" seconds");
    
    if (has_ambulance[position]) {
        Serial.print(" 🚑 PRIORITY: AMBULANCE");
    }
    
    Serial.println();
}

void transitionToGreen(int road) {
    // Turn all others red, current road green
    for (int j = 0; j < NUM_LIGHTS; j++) {
        if (j == road) {
            digitalWrite(RED_PINS[j], LOW);
            digitalWrite(YELLOW_PINS[j], LOW);
            digitalWrite(GREEN_PINS[j], HIGH);
        } else {
            digitalWrite(RED_PINS[j], HIGH);
            digitalWrite(YELLOW_PINS[j], LOW);
            digitalWrite(GREEN_PINS[j], LOW);
        }
    }
}

void transitionToRed(int road) {
    // Yellow warning phase
    digitalWrite(GREEN_PINS[road], LOW);
    digitalWrite(YELLOW_PINS[road], HIGH);
    
    Serial.print("  → Yellow warning (");
    Serial.print(YELLOW_TIME / 1000);
    Serial.println("s)");
    
    delay(YELLOW_TIME);
    
    // Turn to red
    digitalWrite(YELLOW_PINS[road], LOW);
    digitalWrite(RED_PINS[road], HIGH);
    
    Serial.println("  → Red");
}

void allRedSafetyPeriod() {
    Serial.print("  → All-red safety period (");
    Serial.print(ALL_RED_TIME / 1000);
    Serial.println("s)\n");
    
    // Ensure all lights are red
    for (int i = 0; i < NUM_LIGHTS; i++) {
        digitalWrite(RED_PINS[i], HIGH);
        digitalWrite(GREEN_PINS[i], LOW);
        digitalWrite(YELLOW_PINS[i], LOW);
    }
    
    delay(ALL_RED_TIME);
}

void resetSystem() {
    // Set all lights to red
    for (int i = 0; i < NUM_LIGHTS; i++) {
        digitalWrite(GREEN_PINS[i], LOW);
        digitalWrite(YELLOW_PINS[i], LOW);
        digitalWrite(RED_PINS[i], HIGH);
        
        road_order[i] = i;
        green_time[i] = 0;
        has_ambulance[i] = false;
    }
    
    Serial.println("System reset. Ready for next cycle.");
    Serial.println("Waiting for traffic data...\n");
}

void checkIdleTimeout() {
    if (lastActivityTime > 0 && (millis() - lastActivityTime > IDLE_TIMEOUT)) {
        Serial.println("\n⚠ IDLE TIMEOUT - System has been inactive");
        Serial.println("Resetting to default state...\n");
        resetSystem();
        lastActivityTime = 0;
    }
}