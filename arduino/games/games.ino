// LED Pillar
// https://github.com/funvill/LEDPillar
//
// ----------------------------------------------------------------------------
// Created by: Steven Smethurst
// Last updated: Aug 28, 2018
//
// There are two games 
// 1) A 1D tetris
// 2) Ball stack game 
// 
// A 1D tetris
// ----------------------
// ToDo: 
// 
// Ball stack game 
// ----------------------
// A ball (LED cursor) starts at the top and falls towards the bottom. 
// The user must hit a button before the ball hits the top of the "stack" of LEDs at the bottom of the pillar. 
// When the user hits the button the ball "bounces" and heads back up to the top of the pillar, then falls back down. 
// The space below the bounce fills in and becomes the bew "top of the stack". 
// The game continues until the user runs out of space or the ball hits the top of the "stack" of leds. 
// The user gets points on how many seconds the game has been going for. 
//


#include "FastLED.h"
#include "LedMatrix.h"
#include "CButtons.h"

#include <SPI.h>

#include <stdio.h>
#include <string.h> // memset

FASTLED_USING_NAMESPACE

// Game Settings
// ----------------------------------------------------------------------------
static const unsigned short SETTINGS_NUM_LEDS = (30 * 6); // The number of LEDs in this strip. 30 per meter, and 5 meters of LEDs.


// LEDs
static const unsigned char SETTING_GLOBAL_BRIGHTNESS = 200; // Set the global brightness, this is useful when the LED strip is powered via USB. 0-254
static const unsigned char SETTINGS_FRAMES_PER_SECOND = 120;

// Debug
static const unsigned long SETTING_SERIAL_BAUD_RATE = 115200; // The baud rate for the debug prints.


// Pins
// ----------------------------------------------------------------------------
static const unsigned char SETTING_PIN_LED_DATA = 6;
static const unsigned char SETTING_SCORE_BOARD_CS = 10;
static const unsigned char SETTING_PIN_PLAYER_GREEN_BUTTON = 2;
static const unsigned char SETTING_PIN_PLAYER_BLUE_BUTTON = 3;
static const unsigned char SETTING_PIN_PLAYER_RED_BUTTON = 4;



// Globals
// ----------------------------------------------------------------------------
CRGB leds[SETTINGS_NUM_LEDS];
uint8_t gHue = 0; // rotating "base color" used by many of the patterns


#include "demo.h"

// Games
#include "CTetris.h"
CTetris gameTetris;

// Score display
static const unsigned char SETTING_SCORE_BOARD_DISPLAYS = 4;
LedMatrix ledMatrix = LedMatrix(SETTING_SCORE_BOARD_DISPLAYS, SETTING_SCORE_BOARD_CS);
// Buttons
static const unsigned char BUTTON_MAX = 3;
CButton inputsButtons[BUTTON_MAX];

// Used for demo patterns
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber; // Index number of which pattern is current

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
    Serial.println(String(millis()) + " FYI: gameState = Start | Pattern = " + String(gCurrentPatternNumber));
}

void UpdateScoreBoard(String text)
{
    // Draw the tet.
    ledMatrix.clear();
    ledMatrix.setText(text);
    ledMatrix.drawText();
    ledMatrix.commit();
}

void setup()
{
    // initialize serial communications
    Serial.begin(SETTING_SERIAL_BAUD_RATE);

    // Print version info
    Serial.println("LED Tetris v0.0.2, Last updated: 2018 Aug 25 ");
    Serial.println("More information: https://blog.abluestar.com/projects/2017-led-pillar");

    for (unsigned char offset = 0; offset < 3; offset++) {
        Serial.println("waiting " + String(3 - offset) + " seconds");
        delay(1000);
    }

    // LEDs
    Serial.println(String(millis()) + " FYI: Setting up the LEDS. LED Count = " + String(SETTINGS_NUM_LEDS));
    FastLED.addLeds<NEOPIXEL, SETTING_PIN_LED_DATA>(leds, SETTINGS_NUM_LEDS);
    
    // set master brightness control
    FastLED.setBrightness(SETTING_GLOBAL_BRIGHTNESS);

    // LED Matrix / Score board.
    ledMatrix.init();
    ledMatrix.setRotation(true);
    ledMatrix.setTextAlignment(TEXT_ALIGN_LEFT);
    ledMatrix.setCharWidth(8);
    UpdateScoreBoard("Push");

    // Buttons.
    inputsButtons[0].init(SETTING_PIN_PLAYER_RED_BUTTON);
    inputsButtons[1].init(SETTING_PIN_PLAYER_GREEN_BUTTON);
    inputsButtons[2].init(SETTING_PIN_PLAYER_BLUE_BUTTON);

    // Set up the game
    gameTetris.Reset(leds, SETTINGS_NUM_LEDS);
}

void loop()
{
    static bool gameState = false; // false = demo, true = game
    static unsigned long gameStageChangeTimeout = 0; // How long before you can change the game state again.

    if (gameState) {
       
        // Do game loop
        if (!gameTetris.Loop(inputsButtons[0], inputsButtons[1], inputsButtons[2])) {
            // Game over.
            gameState = false;
            gameStageChangeTimeout = millis() + 1000;
        }

        // Update the score board
        UpdateScoreBoard(String(gameTetris.GetGameScore()));
        

    } else {
        // Do demo patterns
        UpdateScoreBoard("Push me");

        // Call the current pattern function once, updating the 'leds' array
        gPatterns[gCurrentPatternNumber]();

        // Do some periodic updates
        EVERY_N_MILLISECONDS(10) { gHue += 3; } // slowly cycle the "base color" through the rainbow
        EVERY_N_SECONDS(60) { nextPattern(); } // change patterns periodically

        // Check to see if any button has been pressed.
        for (unsigned char buttonOffset = 0; buttonOffset < BUTTON_MAX; buttonOffset++) {
            inputsButtons[buttonOffset].loop();
        }
        // We added a timeout to the game state change so people don't accidentally start the game
        // immediately after reset.
        if (gameStageChangeTimeout < millis()) {
            for (unsigned char buttonOffset = 0; buttonOffset < BUTTON_MAX; buttonOffset++) {
                if (inputsButtons[buttonOffset].hasButtonBeenPressed()) {
                    // The user wants to start the game
                    gameState = true;
                    gameTetris.Reset(leds, SETTINGS_NUM_LEDS);
                    return;
                }
            }
        }
    }

    // Update the LEDs
    FastLED.show();
    FastLED.delay(1000 / SETTINGS_FRAMES_PER_SECOND);
}
