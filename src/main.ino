/**
 * DMK (deemonoise's midi kontroller)
 * 
 * 4x4 midi controller by deemonoise https://github.com/deemonoise/DMK
 * 
 * Required library
 *  TimerOne https://playground.arduino.cc/Code/Timer1
 *  MIDI https://github.com/FortySevenEffects/arduino_midi_library
 *  GyverOLED https://github.com/GyverLibs/GyverOLED
 *  EncButton https://github.com/GyverLibs/EncButton
 *  
*/

#include "TimerOne.h"
#include "MIDI.h"
MIDI_CREATE_DEFAULT_INSTANCE();

#define USE_MICRO_WIRE
#include <GyverOLED.h>
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

#define CLK 3
#define DT 2
#define SW 4
#include <EncButton.h>
EncButton<EB_TICK, CLK, DT, SW> enc;

#include <SoftwareSerial.h>
SoftwareSerial keyboard(10, 11); // RX, TX

struct BtnStr {
  byte action;
  byte button;
};

BtnStr btnData;

// disable for debug
bool midiEnabled = true;

#define NOTE_MODE 0
#define BB_MODE 1
#define SEQ_MODE 2

byte midiChanel = 1;
byte baseNote = 48;
int mode = 0;
int bbMode = 0;
int octave = 0;
int scale = 0;
int rootNote = 0;
int selectedRow = 0;
int velocity = 127;
int bpm = 120;
bool play = false;
bool isEditing = false;
bool hold = false;
bool seqEdit = false;

String modes[3] = {"note", "blackbox", "seq"};

String bbModes[2] = {"PAD", "SEQ"};

byte bbNotes[2][16] = {
  {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51},
  {52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67}
};

String scales[15] = {
  "chromatic", "major", "minor", "maj.penta", "min.penta", "blues", "phrygian",
  "bebop", "enigmatic", "gypsy", "hirajoshi", "locrian", "mixolidian", "ukrainian", "yo"
  };

// first element - number of scale notes
byte scaleNotes[15][17] = {
  // chromatic
  {12, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
  // major
  {7, 0, 2, 4, 5, 7, 9, 11},
  // minor
  {7, 0, 2, 3, 5, 7, 9, 10},
  // maj.penta
  {5, 0, 2, 4, 7, 9},
  // min.penta
  {5, 0, 3, 5, 7, 10},
  // blues
  {6, 0, 3, 5, 6, 7, 10},
  // phrygian
  {7, 0, 1, 3, 5, 7, 8, 10},
  // Bebop
  {8, 0, 2, 4, 5, 7, 9, 10, 11},
  // Enigmatic
  {7, 0, 1, 4, 6, 8, 10, 11},
  // Gypsy
  {7, 0, 2, 3, 6, 7, 8, 10},
  // Hirajōshi
  {5, 0, 4, 6, 7, 11},
  // Locrian
  {7, 0, 1, 3, 5, 6, 8, 10},
  // Mixolidian
  {7, 0, 2, 4, 5, 7, 9, 10},
  // Ukrainian
  {7, 0, 2, 3, 6, 7, 9, 10},
  // Yo
  {5, 0, 3, 5, 7, 10},
};
String rootNotes[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

bool heldKeys[16];

typedef struct {
  int note;
  byte dur;
  byte vel;
} noteStr;

// {note, dur, vel}
noteStr sequence[32];

void displayMode() {
  oled.home();
  oled.setCursor(13, 0);
  oled.print("         ");

  oled.home();
  oled.setCursor(13, 0);
  oled.print(modes[mode]);
}

void displayScale() {
  oled.home();
  oled.setCursor(13, 1);
  oled.print("             ");

  oled.home();
  oled.setCursor(13, 1);
  if (mode == NOTE_MODE) {
      oled.print(scales[scale]);
  } else if (mode == BB_MODE) {
    oled.print(bbModes[bbMode]);
  } else {
    oled.print("edit");
  }
}

void displayRoot() {
  oled.home();
  oled.setCursor(13, 2);
  oled.print("         ");

  if (mode != NOTE_MODE) return;

  oled.home();
  oled.setCursor(13, 2);
  oled.print("root: ");
  oled.print(rootNotes[rootNote]);
}

void displayOctave() {
  oled.home();
  oled.setCursor(13, 3);
  oled.print("         ");

  if (mode != NOTE_MODE) return;

  oled.home();
  oled.setCursor(13, 3);
  oled.print("tsp.: ");
  oled.print(octave);
}

void displayHold() {
  oled.home();
  oled.setCursor(13, 4);
  oled.print("         ");

  if (mode != NOTE_MODE) return;

  oled.home();
  oled.setCursor(13, 4);
  oled.print("hold: ");
  if (hold) {
    oled.invertText(true);
    oled.print("ON");
    oled.invertText(false);
  }

  if (!hold) {
    oled.invertText(false);
    oled.print("OFF");
  }
}

void displayChanel() {
  oled.home();
  oled.setCursor(13, 5);
  oled.print("         ");

  oled.home();
  oled.setCursor(13, 5);
  oled.print("ch. : ");
  oled.print(midiChanel);
}

void displayBpm() {
  oled.home();
  oled.setCursor(13, 6);
  oled.print("         ");

  oled.home();
  oled.setCursor(13, 6);
  oled.print("bpm : ");
  oled.print(bpm);
}


void displayPlay() {
  oled.home();
  oled.setCursor(13, 7);
  oled.print("         ");

  oled.home();
  oled.setCursor(13, 7);
  if (play) {
    oled.invertText(true);
    oled.print("play");
    oled.invertText(false);
  } else {
    oled.invertText(false);
    oled.print("stop");
  }
}

void displayCursor() {
  if (!isEditing) {
    for (int i = 0; i <= 7; i++) {
      oled.setCursor(0, i);
      oled.print(" ");
      oled.setCursor(119, i);
      oled.print(" ");
    }
    oled.setCursor(0, selectedRow);
    oled.print(">");
    oled.setCursor(119, selectedRow);
    oled.print("<");
  } else {
    oled.setCursor(0, selectedRow);
    oled.invertText(true);
    oled.print("*");
    oled.setCursor(119, selectedRow);
    oled.print("*");
    oled.invertText(false);
  }
}

void clearOled() {
  // clear oled. looks ugly but it works
  oled.setScale(2);
  oled.setCursor(0, 0);
  oled.print("                    ");
  oled.setCursor(0, 2);
  oled.print("                    ");
  oled.setCursor(0, 4);
  oled.print("                    ");
  oled.setCursor(0, 6);
  oled.print("                    ");
  oled.setCursor(0, 8);
  oled.print("                    ");
  oled.setScale(1);
}

void printDot(int i, bool active) {
  int x = 0;
  int y = 0;
  if (i < 4) {x = 25+i*20; y = 6;}
  if (i >= 4 && i < 8) {x = 25+20*(i-4); y = 4;}
  if (i >= 8 && i < 12) {x = 25+20*(i-8); y = 2;}
  if (i >= 12) {x = 25 + 20*(i-12); y = 0;}

  oled.setScale(2);
  oled.invertText(active);
  oled.setCursor(x, y);
  oled.print("o");
}

void displayGrid() {
  for (int i = 0; i < 16; i++) {
    printDot(i, heldKeys[i]);
  }
}

void displaySeq() {
  oled.home();
  oled.setCursor(13, 0);
  oled.print("SEQ");
  oled.setCursor(0, 6);
  oled.print("pos:32");
  oled.setCursor(50, 6);
  oled.print("note:C#");

  
  oled.fastLineH(40, 0, 127);
  for (int i = 0; i<32; i++) {
    oled.fastLineV(1+4*i, 40, 10);
  }
}

void displayAll() {
  clearOled();
  oled.setScale(1);
  displayMode();
  displayScale();
  displayRoot();
  displayChanel();
  displayOctave();
  displayHold();
  displayBpm();
  displayPlay();
  displayCursor();
}

byte getNote(byte key) {
  if (mode == 1) {
    return bbNotes[bbMode][key - 1];
  }

  if (key <= scaleNotes[scale][0]) return baseNote + rootNote + scaleNotes[scale][key];
  if (key > scaleNotes[scale][0] && key <= 2*scaleNotes[scale][0]) return baseNote + rootNote + scaleNotes[scale][key - scaleNotes[scale][0]] + 12;
  if (key > 2*scaleNotes[scale][0]) return baseNote + rootNote + scaleNotes[scale][key - 2*scaleNotes[scale][0]] + 24;

  return baseNote;
}

void processEncoder() {
  enc.tick();
  if (enc.turn()) {
    if (!hold) {
      // normal mode
      if (!isEditing && !seqEdit) {
        selectedRow += enc.dir();
        if (selectedRow < 0) selectedRow = 7;
        if (selectedRow > 7) selectedRow = 0;
        if (mode != NOTE_MODE) {
          if (selectedRow == 2 && enc.dir() > 0) selectedRow = 5;
          if (selectedRow == 4 && enc.dir() < 0) selectedRow = 1;
        }
        displayCursor();
        return;
      } 

      // editing mode  
      switch (selectedRow)
      {
      case 0:
        mode += enc.dir();
        if (mode > 2) mode = 0;
        if (mode < 0) mode = 2;
        displayAll();
        break;

      case 1:
        if (mode == NOTE_MODE) {
          scale += enc.dir();
          if (scale >= 14) scale = 0;
          if (scale < 0) scale = 14;
        } else if (mode == BB_MODE) {
          bbMode += enc.dir();
          if (bbMode > 1) bbMode = 0;
          if (bbMode < 0) bbMode = 1;
        }
        if (mode != SEQ_MODE) displayScale();
        break;

      case 2:
        rootNote += enc.dir();
        if (rootNote > 11) rootNote = 0;
        if (rootNote < 0) rootNote = 11;
        displayRoot();
        break;    

      case 3:
        octave += enc.dir();
        if (octave < -3) {
          octave = -3;
          break;
        } 
        if (octave > 4) {
          octave = 4;
          break;
        } 

        baseNote +=  12 * enc.dir();
        if (baseNote < 0) baseNote = 0;
        if(baseNote > 108) baseNote = 108;
        
        displayOctave();
        break;      

      case 5:
        midiChanel += enc.dir();
        if (midiChanel > 16) midiChanel = 1;
        if (midiChanel <= 0) midiChanel = 16;
        displayChanel();
        break;        

      case 6:
        bpm += enc.dir();
        if (bpm > 300) bpm = 300;
        if (bpm < 20) bpm = 20;

        long interval = 60L * 1000 * 1000 / bpm / 24;  
        Timer1.setPeriod(interval);

        displayBpm();
        break; 
      }
      
    }
  }

  if (enc.press()) {
    if (selectedRow == 1 && mode == SEQ_MODE) {
      seqEdit = !seqEdit;
      clearOled();
      if (seqEdit) {
        displaySeq();
      } else {
        displayAll();
      }
      return;
    } 

    if (selectedRow == 7) {
      play = !play;
      if (play) MIDI.sendRealTime(midi::Start);
      if (!play) MIDI.sendRealTime(midi::Stop);
      displayPlay();
      return;
    }
    
    if (selectedRow == 4) {
      hold = !hold;
      if (!hold) {
        for (int i = 0; i <= 16; i++) {
          heldKeys[i] = false;
          MIDI.sendNoteOff(getNote(i + 1), velocity, midiChanel);
        };
      }
      if (hold) {
        clearOled();
        displayGrid();
      } else {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
        clearOled();
        displayAll();
      }
      return;      
    }

    isEditing = !isEditing;
    displayCursor();
  }
}

void sendClockPulse() {
  if (midiEnabled && play) MIDI.sendRealTime(midi::Clock);
}

void recieveButtons() {
  keyboard.setTimeout(10);
  if (keyboard.readBytes((byte*)&btnData, sizeof(btnData))) {
    if (midiEnabled) {
      if (!hold) {
        // regular mode
        if (btnData.action) {
          heldKeys[btnData.button] = true;
          MIDI.sendNoteOn(getNote(btnData.button + 1), velocity, midiChanel);
        } else {
          heldKeys[btnData.button] = false;
          MIDI.sendNoteOff(getNote(btnData.button + 1), velocity, midiChanel);
        }
      } else {
        // HOLD mode
        if (btnData.action) {
          if (!heldKeys[btnData.button]) {
            heldKeys[btnData.button] = true;
            MIDI.sendNoteOn(getNote(btnData.button + 1), velocity, midiChanel);
          } else {
            heldKeys[btnData.button] = false;
            MIDI.sendNoteOff(getNote(btnData.button + 1), velocity, midiChanel);
          }
          printDot(btnData.button, heldKeys[btnData.button]);
        }
      }
    } else {
      Serial.print("note ");
      Serial.print(getNote(btnData.button + 1));
      if (btnData.action) {
        Serial.println(" ON");
      } else {
        Serial.println(" OFF");
      }
    }
  }
}

void setup() {
  if (midiEnabled) {
    MIDI.begin();
    MIDI.turnThruOff();
  } else {
    Serial.begin(9600);
  }

  keyboard.begin(4800);

  for (int i = 0; i <= 16; i++) heldKeys[i] = false;
  for (int i = 0; i <= 32; i++) sequence[i] = (noteStr) {0, 16, 127};

  oled.init();
  displayAll();

  Timer1.initialize();
  Timer1.setPeriod(60L * 1000 * 1000 / 120 / 24);
  Timer1.attachInterrupt(sendClockPulse); 
}

void loop() {
  processEncoder();
  recieveButtons();
}