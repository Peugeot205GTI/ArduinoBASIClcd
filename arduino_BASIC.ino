#include <font.h>
#include <LiquidCrystal.h>
// ^ - modified for faster SPI
#include <PS2Keyboard.h>
#include <EEPROM.h>

#include "basic.h"
#include "host.h"

// Define in host.h if using an external EEPROM e.g. 24LC256
// Should be connected to the I2C pins
// SDA -> Analog Pin 4, SCL -> Analog Pin 5
// See e.g. http://www.hobbytronics.co.uk/arduino-external-eeprom

// If using an external EEPROM, you'll also have to initialise it by
// running once with the appropriate lines enabled in setup() - see below

#if EXTERNAL_EEPROM
#include <I2cMaster.h>
// Instance of class for hardware master with pullups enabled
TwiMaster rtc(true);
#endif

// Keyboard
const int DataPin = 8;
const int IRQpin =  9;
PS2Keyboard keyboard;

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;

LiquidCrystal oled(rs, en, d4, d5, d6, d7);


// NB Keyboard needs a seperate ground from the OLED

// buzzer pin, 0 = disabled/not present
#define BUZZER_PIN    13

// BASIC
unsigned char mem[MEMORY_SIZE];
#define TOKEN_BUF_SIZE    64
unsigned char tokenBuf[TOKEN_BUF_SIZE];

const char welcomeStr[] PROGMEM = "Arduino BASIC";
char autorun = 0;

void setup() {
    keyboard.begin(DataPin, IRQpin);
    oled.begin(16,2);

    reset();
    host_init(BUZZER_PIN);
    host_cls();
    host_outputProgMemString(welcomeStr);
    // show memory size
    host_outputFreeMem(sysVARSTART - sysPROGEND);
    host_showBuffer();
    
    // IF USING EXTERNAL EEPROM
    // The following line 'wipes' the external EEPROM and prepares
    // it for use. Uncomment it, upload the sketch, then comment it back
    // in again and upload again, if you use a new EEPROM.
    // writeExtEEPROM(0,0); writeExtEEPROM(1,0);

    if (EEPROM.read(0) == MAGIC_AUTORUN_NUMBER)
        autorun = 1;
    else
        host_startupTone();
}

void loop() {
    int ret = ERROR_NONE;

    if (!autorun) {
        // get a line from the user
        char *input = host_readLine();
        // special editor commands
        if (input[0] == '?' && input[1] == 0) {
            host_outputFreeMem(sysVARSTART - sysPROGEND);
            host_showBuffer();
            return;
        }
        // otherwise tokenize
        ret = tokenize((unsigned char*)input, tokenBuf, TOKEN_BUF_SIZE);
    }
    else {
        host_loadProgram();
        tokenBuf[0] = TOKEN_RUN;
        tokenBuf[1] = 0;
        autorun = 0;
    }
    // execute the token buffer
    if (ret == ERROR_NONE) {
        host_newLine();
        ret = processInput(tokenBuf);
    }
    if (ret != ERROR_NONE) {
        host_newLine();
        if (lineNumber !=0) {
            host_outputInt(lineNumber);
            host_outputChar('-');
        }
        host_outputProgMemString((char *)pgm_read_word(&(errorTable[ret])));
    }
}

