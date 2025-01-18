// #################################
// MOZZI RELATED
#include "MozziConfigValues.h"
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_EXTERNAL_TIMED
#define MOZZI_CONTROL_RATE 256
#include <Mozzi.h>
#include <Oscil.h>
#include <LowPassFilter.h>
#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/waveshape_chebyshev_6th_256_int8.h>
#include <EventDelay.h>
#include <DAC_MCP49xx.h>

// #################################
// DAC Related
#define DAC_CS_PIN 7 // CS PIN
DAC_MCP49xx dac(DAC_MCP49xx::MCP4921, DAC_CS_PIN);

// #################################
// Settings to adapt live
int sequencerSteps = 9; 
float baseFrequency = 45.0f;
int currentWaveType = 3; // 1 = sine, 2 = square, 3 = sawtooth, 4 = triangle, 5 = wave
int tempo = 140; // Default tempo in BPM

// ############################################
// Sequencer Variables
float acidHarmonics[16]; // Maximum steps for the sequencer
int currentStep = 0; // Current step in the sequencer
EventDelay stepDelay; // Step delay for controlling tempo

// #################################
// Oscillators and Filter
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> sinOsc(SIN2048_DATA); // SINE
Oscil<SAW2048_NUM_CELLS, MOZZI_AUDIO_RATE> sawOsc(SAW2048_DATA); // SAW
Oscil<SQUARE_NO_ALIAS_2048_NUM_CELLS, MOZZI_AUDIO_RATE> squareOsc(SQUARE_NO_ALIAS_2048_DATA); // SQUARE
Oscil<TRIANGLE2048_NUM_CELLS, MOZZI_AUDIO_RATE> triangleOsc(TRIANGLE2048_DATA); // TRIANGLE
Oscil<CHEBYSHEV_6TH_256_NUM_CELLS, MOZZI_AUDIO_RATE> waveOsc(CHEBYSHEV_6TH_256_DATA); // WAVE

LowPassFilter lpf; // Low-pass filter
float cutoffFreq = 300.0f; // Initial cutoff frequency
uint8_t resonance = 150; // Moderate resonance for smoother sound
int volume = 128; // Volume scaling (0-255)

// #################################
// INPUT CONTROLS
#define freqPot A5
#define tempPot A4

// #################################
// Helper Function to Convert BPM to Step Delay (ms)
int bpmToStepDelay(int bpm, int stepsPerBeat) {
  return (60000 / bpm) / stepsPerBeat; // Calculate delay in ms for one step
}

// #################################
// Audio Output Control
void audioOutput(const AudioOutput f) {
  uint16_t out = (f.l() >> 4) + 2048;
  dac.output(out);
}

// #################################
// Main Setup Code
void setup() {
  Serial.begin(9600);

  dac.init(); // Setup DAC
  startMozzi(); // Initialise Mozzi Start

  // PIN MODES 
  pinMode(freqPot, INPUT);
  pinMode(tempPot, INPUT);

  // Initialize harmonics with more musical intervals
  for (int i = 0; i < 16; i++) {
    acidHarmonics[i] = (1.0 + (float)i / 2.0); // Adjust spacing for harmonics
  }

  stepDelay.set(bpmToStepDelay(tempo, sequencerSteps)); // Set delay based on tempo and steps
  lpf.setCutoffFreqAndResonance(cutoffFreq, resonance); // Initialize filter
}

// #################################
// Update Control (Handles Sequencer and Modulations)
void updateControl() {

  // Read the frequency potentiometer for base frequency control
  int freqVal = mozziAnalogRead(freqPot);
  float mappedFreq = map(freqVal, 0, 1023, 20, 200); // Base Frequency Mapped to Analog Input

  // Read the tempo potentiometer and map it to a BPM range
  int tempVal = mozziAnalogRead(tempPot);
  tempo = map(tempVal, 0, 1023, 60, 180); // Map pot range to 60–180 BPM

  if (stepDelay.ready()) {
    currentStep = (currentStep + 1) % sequencerSteps; // Advance sequencer step
    float freq = acidHarmonics[currentStep] * mappedFreq; // Get current harmonic frequency

    switch (currentWaveType) {
      case 1:
        sinOsc.setFreq(freq);
        break;

      case 2:
        squareOsc.setFreq(freq);
        break;
      
      case 3:
        sawOsc.setFreq(freq);
        break;

      case 4:
        triangleOsc.setFreq(freq);
        break;

       case 5:
        waveOsc.setFreq(freq);
        break;
      
      default:
        sinOsc.setFreq(freq);
        break;
    }

    // Smooth filter modulation
    cutoffFreq = 200.0f + 100.0f * sin(2.0f * PI * currentStep / sequencerSteps);
    lpf.setCutoffFreqAndResonance((uint8_t)cutoffFreq, resonance);

    stepDelay.set(bpmToStepDelay(tempo, sequencerSteps)); // Update step delay dynamically
    stepDelay.start(); // Restart delay for the next step
  }
}

// #################################
// Update Audio (Handles Oscillator and Filter Output)
AudioOutput updateAudio() {
  int signal;

  switch (currentWaveType) {
      case 1:
        signal = sinOsc.next() / 2;
        break;

      case 2:
        signal = squareOsc.next() / 2;
        break;
      
      case 3:
        signal = sawOsc.next() / 2;
        break;

      case 4:
        signal = triangleOsc.next() / 2;
        break;

       case 5:
        signal = waveOsc.next() / 2;
        break;
      
      default:
        signal = sinOsc.next() / 2;
        break;
    }

  // Apply low-pass filter
  int filteredSignal = lpf.next(signal);

  // Apply volume scaling
  filteredSignal = (filteredSignal * volume) / 255;

  return MonoOutput::from8Bit(filteredSignal); // Return filtered signal
}

// #################################
// Main Loop Code
void loop() {
  audioHook(); // Required for Mozzi to work
}
