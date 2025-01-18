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
#include <EventDelay.h>
#include <DAC_MCP49xx.h>

// #################################
// DAC Related
#define DAC_CS_PIN 7 // CS PIN
DAC_MCP49xx dac(DAC_MCP49xx::MCP4921, DAC_CS_PIN);

// #################################
// Acid Techno Sequencer Settings
int sequencerSteps = 8; // Set the step count (e.g., 8 for techno loops)
float baseFrequency = 45.0f; // Default base frequency (low for bass-heavy acid sound)
int currentWaveType = 2; // 0 = sine, 1 = square, 2 = sawtooth, 3 = triangle

// Frequencies for harmonics (nonlinear for acid sound)
float acidHarmonics[16]; // Maximum steps for the sequencer
int currentStep = 0; // Current step in the sequencer
EventDelay stepDelay; // Step delay for controlling tempo

// #################################
// Oscillators and Filter
Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> sinOsc(SIN2048_DATA); // SINE
Oscil<SAW2048_NUM_CELLS, MOZZI_AUDIO_RATE> sawOsc(SAW2048_DATA); // SAW
Oscil<SQUARE_NO_ALIAS_2048_NUM_CELLS, MOZZI_AUDIO_RATE> squareOsc(SQUARE_NO_ALIAS_2048_DATA); // SQUARE
Oscil<TRIANGLE2048_NUM_CELLS, MOZZI_AUDIO_RATE> triangleOsc(TRIANGLE2048_DATA); // TRIANGLE
LowPassFilter lpf; // Low-pass filter
float cutoffFreq = 300.0f; // Initial cutoff frequency
uint8_t resonance = 150; // Moderate resonance for smoother sound
int volume = 128; // Volume scaling (0-255)

// #################################
// Audio Output Control
void audioOutput(const AudioOutput f) {
  uint16_t out = (f.l() >> 4) + 2048;
  dac.output(out);
}

// #################################
// Main Setup Code
void setup() {
  dac.init(); // Setup DAC
  startMozzi(); // Initialise Mozzi Start

  // Initialize acid harmonics with more musical intervals
  for (int i = 0; i < 16; i++) {
    acidHarmonics[i] = baseFrequency * (1.0 + (float)i / 2.0); // Adjust spacing for harmonics
  }

  stepDelay.set(150); // Default step delay (150ms per step for ~140 BPM)
  lpf.setCutoffFreqAndResonance(cutoffFreq, resonance); // Initialize filter
}

// #################################
// Update Control (Handles Sequencer and Modulations)
void updateControl() {
  if (stepDelay.ready()) {
    currentStep = (currentStep + 1) % sequencerSteps; // Advance sequencer step
    float freq = acidHarmonics[currentStep]; // Get current harmonic frequency

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

      case 4;
        triangleOsc.setFreq(freq);
        break;
      
      default:
        sinOsc.setFreq(freq);
        break;
    }

    // Smooth filter modulation
    cutoffFreq = 200.0f + 100.0f * sin(2.0f * PI * currentStep / sequencerSteps); // Reduce modulation depth
    lpf.setCutoffFreqAndResonance((uint8_t)cutoffFreq, resonance);

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
        signal = squareOsc.next() / 2; // Scale amplitude
        break;
      
      case 3:
        signal = sawOsc.next() / 2; // Scale amplitude
        break;

      case 4;
        signal = triangleOsc.next() / 2;
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
