#include "MozziConfigValues.h"  // for named option values
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_EXTERNAL_TIMED
#define MOZZI_CONTROL_RATE 256  // Control rate for Mozzi (Hz)

#include <Mozzi.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <tables/cos2048_int8.h>
#include <tables/saw2048_int8.h>
#include <EventDelay.h>
#include <DAC_MCP49xx.h>  // DAC MCP4921 library


// DAC

#define SS_PIN 7  // Chip select pin for DAC MCP4921
DAC_MCP49xx dac(DAC_MCP49xx::MCP4921, SS_PIN);

//int potPin = A0;  // Potentiometer for frequency control
int tempoPotPin = A1;  // Potentiometer for sequencer tempo
int modeSelectPin = A0;  // Potentiometer to select scale/mode
int oscSelectPin = A2;  // Potentiometer to switch oscillators

// Oscillators (sine, square, sawtooth)
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> sineOsc(SIN2048_DATA);
Oscil<COS2048_NUM_CELLS, MOZZI_AUDIO_RATE> cosOsc(COS2048_DATA);
Oscil<SAW2048_NUM_CELLS, MOZZI_AUDIO_RATE> sawOsc(SAW2048_DATA);

// Sequencer variables
EventDelay stepDelay;  // Delay to control time between sequencer steps
const int majorScale[] = {261, 293, 329, 349, 392, 440, 493, 523};  // Major scale (C4 to C5)
const int minorScale[] = {261, 293, 311, 349, 392, 415, 466, 523};  // Minor scale
const int pentatonicScale[] = {261, 293, 329, 392, 440, 523};  // Pentatonic scale
const int* scales[] = {majorScale, minorScale, pentatonicScale};  // Array of scales
const int scaleLengths[] = {8, 8, 6};  // Lengths of the scales
int currentScale = 0;  // Default to major scale
int currentStep = 0;  // Index of the current step in the sequence

int currentOscillator = 0;  // 0 = sine, 1 = square, 2 = saw

void audioOutput(const AudioOutput f) {
  uint16_t out = (f.l() >> 4) + 2048;  // Convert to 12-bit DAC output
  dac.output(out);
}

void setup() {
  dac.init();  // Initialize DAC
  dac.setPortWrite(true);  // Fast writes for AVR platforms
  startMozzi();
  sineOsc.setFreq(261);  // Default frequency (C4 note)
  cosOsc.setFreq(261);
  sawOsc.setFreq(261);
  stepDelay.set(500);  // Initial sequencer step delay (in ms)
}

void updateControl() {
  // Adjust sequencer tempo based on the potentiometer
  int tempoVal = analogRead(tempoPotPin);
  int stepDuration = map(tempoVal, 0, 1023, 100, 1000);  // Map pot value to step duration (100ms to 1000ms)
  stepDelay.set(stepDuration);

  // Select scale/mode based on the mode select potentiometer
  int modeVal = analogRead(modeSelectPin);
  currentScale = map(modeVal, 0, 1023, 0, 2);  // 0 = Major, 1 = Minor, 2 = Pentatonic

  // Select oscillator based on the oscillator select potentiometer
  int oscVal = analogRead(oscSelectPin);
  currentOscillator = map(oscVal, 0, 1023, 0, 2);  // 0 = Sine, 1 = Square, 2 = Saw

  // Advance the sequencer step if the delay is ready
  if (stepDelay.ready()) {
    const int* currentScaleArray = scales[currentScale];  // Get the current scale array
    int scaleLength = scaleLengths[currentScale];  // Get the length of the current scale
    int freq = currentScaleArray[currentStep];  // Get the current note frequency

    // Set frequency for all oscillators
    sineOsc.setFreq(freq);
    cosOsc.setFreq(freq);
    sawOsc.setFreq(freq);

    currentStep = (currentStep + 1) % scaleLength;  // Move to the next step, loop after the scale length
    stepDelay.start();  // Restart the delay
  }
}

AudioOutput updateAudio() {
  int signal;
  if (currentOscillator == 0) {
    signal = sineOsc.next();  // Sine wave
  } else if (currentOscillator == 1) {
    signal = cosOsc.next();  // Square wave
  } else {
    signal = sawOsc.next();  // Sawtooth wave
  }

  return MonoOutput::from8Bit(signal);  // Output mono audio
}

void loop() {
  audioHook();  // Required for Mozzi to process audio

 
}
