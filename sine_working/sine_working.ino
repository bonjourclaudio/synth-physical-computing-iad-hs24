#include "MozziConfigValues.h"  // for named option values
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_EXTERNAL_TIMED
#define MOZZI_CONTROL_RATE 256  // Control rate for Mozzi (Hz)

#include <Mozzi.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>  // Sine wave table
#include <EventDelay.h>
#include <DAC_MCP49xx.h>  // DAC MCP4921 library

#define SS_PIN 7  // Chip select pin for DAC MCP4921
DAC_MCP49xx dac(DAC_MCP49xx::MCP4921, SS_PIN);

//int potPin = A0;  // Potentiometer for frequency control
int tempoPotPin = A0;  // Optional potentiometer for sequencer tempo

// Sine wave oscillator
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);

// Sequencer variables
EventDelay stepDelay;  // Delay to control time between sequencer steps
const int sequence[] = {220, 330, 440, 550, 660, 440, 330, 220};  // 8-step note sequence (frequencies in Hz)
const int sequenceLength = sizeof(sequence) / sizeof(sequence[0]);
int currentStep = 0;  // Index of the current step in the sequence

void audioOutput(const AudioOutput f) {
  uint16_t out = (f.l() >> 4) + 2048;  // Convert to 12-bit DAC output
  dac.output(out);
}

void setup() {
  dac.init();  // Initialize DAC
  dac.setPortWrite(true);  // Fast writes for AVR platforms
  startMozzi();
  aSin.setFreq(sequence[0]);  // Start with the first frequency in the sequence
  stepDelay.set(500);  // Initial sequencer step delay (in ms)
}

void updateControl() {
  // Adjust the sequencer tempo based on the potentiometer
  int tempoVal = analogRead(tempoPotPin);
  int stepDuration = map(tempoVal, 0, 1023, 100, 1000);  // Map pot value to step duration (100ms to 1000ms)
  stepDelay.set(stepDuration);

  // Advance the sequencer step if the delay is ready
  if (stepDelay.ready()) {
    currentStep = (currentStep + 1) % sequenceLength;  // Move to the next step, loop after 8 steps
    aSin.setFreq(sequence[currentStep]);  // Set oscillator frequency to the current step
    stepDelay.start();  // Restart the delay
  }
}

AudioOutput updateAudio() {
  return MonoOutput::from8Bit(aSin.next());  // Output mono sine wave
}

void loop() {
  audioHook();  // Required for Mozzi
}
