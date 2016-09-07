#define ledPin 13
#define CLKIO 16000000           // System Clock frequency
#define PreScalerMask0 0xF8      // Pre-Scaler divider = 0 (Timer/Counter stop)

unsigned int preScalerOptions[] = {0, 1, 64, 1024};

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 7812;                   // compare match register 16MHz /(1024 * 1Hz) / 2
  TCCR1B |= (1 << WGM12);         // CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10);   // Set Pre-Scaler divider to 1024
  TIMSK1 |= (1 << OCIE1A);        // enable timer compare interrupt
  interrupts();                   // enable all interrupts
  Serial.print("Frequency set to 1.0000 Hz");
  Serial.print("     OCR1A = "); Serial.println(OCR1A);

/* Debug line */
//  Serial.print("            TCCR1B: "); Serial.println(TCCR1B, BIN);
}

void setFreq (float freq)
{
  unsigned int preScalerDivider = preScalerOptions[0];
  
  if (freq < (CLKIO / preScalerOptions[3] / pow(2,16))) {   // Requested frequency is too slow
    Serial.println("");
    Serial.print("Frequency must be greater than ");
    Serial.print(CLKIO / preScalerOptions[3] / pow(2,16), 4);
    Serial.println(" Hz.");
    Serial.printf("The timer has been stopped.\nEnter a new value to restart the timer.\n");
    TCCR1B &= PreScalerMask0;   // Set the Clock Select to No Clock (Timer/Counter stopped)
    digitalWrite(ledPin, LOW);
  }
  else if (freq < (CLKIO / pow(2,6) / pow(2,16))) {   // Very low frequency
    preScalerDivider = preScalerOptions[3];
    TCCR1B = (TCCR1B & PreScalerMask0) | 1 << CS12 | 1 << CS10;   // Set Pre-Scaler divider to 1024
  }
  else if (freq < (CLKIO / pow(2,16))) {   // Low frequency
    preScalerDivider = preScalerOptions[2];
    TCCR1B = (TCCR1B & PreScalerMask0) | 1 << CS11 | 1 << CS10;   // Set Pre-Scaler divider to 64
  }
  else if (freq <= 36363) {   // Maximum supported frequency
    preScalerDivider = preScalerOptions[1];
    TCCR1B = (TCCR1B & PreScalerMask0) | 1 << CS10;   // Set Pre-Scaler divider to 1 (practically no divider)
  }
  else {
    Serial.printf("\nFrequency can be maximum 36363.0000 Hz.");
    Serial.printf("\nThe timer has been stopped.");
    Serial.printf("\nEnter a new value to restart the timer.\n");
    TCCR1B &= PreScalerMask0;   // Set the Clock Select to No Clock (Timer/Counter stopped)
    digitalWrite(ledPin, LOW);
  }

  if (preScalerDivider > 0) {
    OCR1A = CLKIO / preScalerDivider / freq / 2;   // Set the new frequency
  }

/* Debug lines */
//  Serial.print("            TCCR1B: "); Serial.println(TCCR1B, BIN);
//  Serial.printf("Pre-scaler divider: %d\n", preScalerDivider);
//  Serial.printf("             OCR1A: %d\n", OCR1A);
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  digitalWrite(ledPin, digitalRead(ledPin) ^ 1);   // toggle LED pin
}


void loop() {
  while (Serial.available() > 0) {
    // Read the desired frequency
    float freq = Serial.parseFloat();
    if (freq > 0) {
//      Serial.println("");
      Serial.print("Frequency set to "); Serial.print(freq, 4); Serial.print(" Hz");
      setFreq(freq);
      Serial.print("     OCR1A = "); Serial.println(OCR1A);
    }
  }
}

