#define resonanceFreqPin    13
#define negResonanceFreqPin 12
#define CLKIO 16000000           // System Clock frequency
#define PreScalerMask0 0xF8      // Pre-Scaler divider = 0 (Timer/Counter0 stop)
#define PreScalerMask1 0xF8      // Pre-Scaler divider = 0 (Timer/Counter1 stop)

unsigned int preScalerOptions[] = {0, 1, 64, 1024};
boolean resonanceFreqIsOn = true;
unsigned long microsInterval = 4000000;   // Default to 4 seconds
unsigned long previousMicros = 0;

void setOnOFFResonanceFreq(float onOffResonanceFreq)
{
  if (onOffResonanceFreq > 0)
  {
    microsInterval = 1000000 / onOffResonanceFreq;  // Convert frequency to microseconds
  }
  else
  {
    Serial.println("Invalid on/off frequency. No changes have been done.");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(resonanceFreqPin, OUTPUT);
  pinMode(negResonanceFreqPin, OUTPUT);

  digitalWrite(resonanceFreqPin, LOW);   // Start with the LED pin turned off
  digitalWrite(negResonanceFreqPin, HIGH);   // Opposite value of LED pin

  noInterrupts();           // Disable all interrupts before initilizing timers

  // Initialize timer1
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Compare match register 16MHz / 1024 / Desired Freq / 2
  OCR1A = CLKIO / preScalerOptions[3] / 1 / 2;  // Default resonace frequeny 1 Hz
  
  TCCR1B |= (1 << WGM12);         // CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10);   // Set Pre-Scaler divider to 1024
  TIMSK1 |= (1 << OCIE1A);        // enable timer compare interrupt
  
  interrupts();   // Timers initialization done, enable all interrupts

  setOnOFFResonanceFreq(0.25);
  Serial.print("On/off frequency set to "); Serial.print(0.25, 4); Serial.println(" Hz");
  Serial.print("Resonance frequency set to 1.0000 Hz");
  Serial.print("     OCR1A = "); Serial.println(OCR1A);

/* Debug line */
//  Serial.print("            TCCR1B: "); Serial.println(TCCR1B, BIN);
}

void setFreq (float freq, byte ocrSize)
{
  unsigned int preScalerDivider = preScalerOptions[0];
  
  if (freq < (CLKIO / preScalerOptions[3] / pow(2, ocrSize * 8))) {   // Requested frequency is too slow
    Serial.println("");
    Serial.print("Frequency must be greater than ");
    Serial.print(CLKIO / preScalerOptions[3] / pow(2, ocrSize * 8), 4);
    Serial.println(" Hz.");
    Serial.println("No changes have been done.");
  }
  else if (freq < (CLKIO / pow(2,6) / pow(2, ocrSize * 8))) {   // Very low frequency
    preScalerDivider = preScalerOptions[3];
    TCCR1B = (TCCR1B & PreScalerMask1) | 1 << CS12 | 1 << CS10;   // Set Pre-Scaler divider to 1024
  }
  else if (freq < (CLKIO / pow(2, ocrSize * 8))) {   // Low frequency
    preScalerDivider = preScalerOptions[2];
    TCCR1B = (TCCR1B & PreScalerMask1) | 1 << CS11 | 1 << CS10;   // Set Pre-Scaler divider to 64
  }
  else if (freq <= 36363) {   // Maximum supported frequency
    preScalerDivider = preScalerOptions[1];
    TCCR1B = (TCCR1B & PreScalerMask1) | 1 << CS10;   // Set Pre-Scaler divider to 1 (practically no divider)
  }
  else {
    Serial.println(""); Serial.print("Frequency can be maximum 36363.0000 Hz.");
    Serial.println(""); Serial.print("The timer has been stopped.");
    Serial.println(""); Serial.println("Enter a new value to restart the timer.");
    TCCR1B &= PreScalerMask1;   // Set the Clock Select to No Clock (Timer/Counter stopped)
    digitalWrite(resonanceFreqPin, LOW);
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
  if (resonanceFreqIsOn)
  {
    digitalWrite(resonanceFreqPin, digitalRead(resonanceFreqPin) ^ 1);   // Toggle LED pin
    digitalWrite(negResonanceFreqPin, digitalRead(resonanceFreqPin) ^ 1);   // Opposite value of LED pin
  }
  else
  {
    // The resonance frequency output is off, don't change anything on the output.
  }
}

void toggleOnOffResonanceFreq()
{
  if ((micros() - previousMicros) >= microsInterval)
  {
    previousMicros = micros();
    resonanceFreqIsOn = !resonanceFreqIsOn;

    if (!resonanceFreqIsOn)
    {
      // The resonance frequency has been disabled, turn off the LED pin
      digitalWrite(resonanceFreqPin, LOW);
      digitalWrite(negResonanceFreqPin, HIGH);   // Opposite value of the LED pin
    }
  }
  else
  {
    // Nothing to do, it's still not time to turn on/off the resonance frequency
  }
}

void loop() {
  while (Serial.available() > 0)
  {
    if (Serial.peek() == 'a')
    {
      // Read the desired on/off frequency
      float freq = Serial.parseFloat();
      
      setOnOFFResonanceFreq(freq);
      Serial.print("On/off frequency set to "); Serial.print(freq, 4); Serial.println(" Hz");
    }
    else
    {
      // Read the desired resonance frequency
      float freq = Serial.parseFloat();
      
      setFreq(freq, sizeof(OCR1A));
      Serial.print("Resonance frequency set to "); Serial.print(freq, 4); Serial.print(" Hz");
      Serial.print("     OCR1A = "); Serial.println(OCR1A);
    }
  }

  toggleOnOffResonanceFreq();
}

