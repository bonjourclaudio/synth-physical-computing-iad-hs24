/*  Example of simple FM with the phase modulation technique,
    using Mozzi sonification library and an external DAC MCP4921 (original library by Thomas Backman - https://github.com/exscape/electronics/tree/master/Arduino/Libraries/DAC_MCP49xx)
    using an user-defined audioOutput() function.
    Based on Mozzi's example: FMsynth.

    Circuit: (see the DAC library README for details)

    MCP4921   //  Connect to:
    -------       -----------
    Vdd           V+
    CS            any digital pin defined by SS_PIN (see after), or pin 7 on UNO / 38 on Mega if you are using Portwrite
    SCK           SCK of Arduino
    SDI           MOSI of Arduino
    VoutA         to headphones/loudspeaker
    Vss           to GND
    VrefA         to V+ or a clean tension ref between V+ and GND
    LDAC          to GND


		Mozzi documentation/API
		https://sensorium.github.io/Mozzi/doc/html/index.html

		Mozzi help/discussion/announcements:
    https://groups.google.com/forum/#!forum/mozzi-users

   Mozzi documentation/API
   https://sensorium.github.io/Mozzi/doc/html/index.html

   Mozzi help/discussion/announcements:
   https://groups.google.com/forum/#!forum/mozzi-users

   Copyright 2020-2024 T. Combriat and the Mozzi Team

   Mozzi is licensed under the GNU Lesser General Public Licence (LGPL) Version 2.1 or later.
*/

// before including Mozzi.h, configure external audio output mode:
#include "MozziConfigValues.h"  // for named option values
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_EXTERNAL_TIMED
// Note: For demonstration purposes, this sketch does *not* set the following (although it would make sense):
//#define MOZZI_AUDIO_BITS 12  // the default value of 16 for external audio is thus used, instead
#define MOZZI_CONTROL_RATE 256 // Hz, powers of 2 are most reliable

#include <Mozzi.h>
#include <Oscil.h>
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <mozzi_midi.h>
#include <mozzi_fixmath.h>
#include <EventDelay.h>
#include <Smooth.h>
#include <DAC_MCP49xx.h>  // https://github.com/tomcombriat/DAC_MCP49XX 
                          // which is an adapted fork from https://github.com/exscape/electronics/tree/master/Arduino/Libraries/DAC_MCP49xx  (Thomas Backman)

// Synthesis part
Oscil <SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);

// External audio output parameters and DAC declaration
#define SS_PIN 7  // if you are on AVR and using PortWrite you need still need to put the pin you are actually using: 7 on Uno, 38 on Mega
DAC_MCP49xx dac(DAC_MCP49xx::MCP4921, SS_PIN);

int potPin = A0;



void audioOutput(const AudioOutput f)
{
  // signal is passed as 16 bit, zero-centered, internally. This DAC expects 12 bits unsigned,
  // so shift back four bits, and add a bias of 2^(12-1)=2048
  uint16_t out = (f.l() >> 4) + 2048;
  dac.output(out);
}



void setup() {
  dac.init();
  dac.setPortWrite(true);  //comment this line if you do not want to use PortWrite (for non-AVR platforms)
  startMozzi();
  aSin.setFreq(440);
}
void updateControl() {

  int freqVal = map(analogRead(potPin),0,2023, 200, 600);

  aSin.setFreq(freqVal);
}


AudioOutput updateAudio(){
  return MonoOutput::from8Bit(aSin.next()); // return an int signal centred around 0
}


void loop() {
  audioHook();
}
