// LED Pillar
// https://github.com/funvill/LEDPillar
//
// ----------------------------------------------------------------------------
// Created by: Steven Smethurst
// Last updated: May 16, 2017
//
// ToDo: If they push the button and there is no beats  there. is that a game over?

#include "FastLED.h"
#include "LedMatrix.h"
#include <SPI.h>

#include <stdio.h>
#include <string.h> // memset

FASTLED_USING_NAMESPACE

// Game Settings
// ----------------------------------------------------------------------------
static const unsigned short SETTINGS_NUM_LEDS = (30 * 4); // The number of LEDs in this strip.
static const unsigned char SETTINGS_BLOCK_SIZE = 4; // Number of LEDs
static const unsigned long SETTING_BLOCK_FALL_SPEED = 70; // ms, lower numbers are faster. Starting speed.  

// LEDs
CRGB leds[SETTINGS_NUM_LEDS];
static const unsigned char SETTING_GLOBAL_BRIGHTNESS = 255; // Set the global brightness, this is useful when the LED strip is powered via USB. 0-254
static const unsigned char SETTINGS_FRAMES_PER_SECOND = 120;

// Debug
static const unsigned long SETTING_SERIAL_BAUD_RATE = 115200; // The baud rate for the debug prints.

// Block Types
static const char BLOCK_TYPE_NONE = '_';
static const char BLOCK_TYPE_RED = 'R';
static const char BLOCK_TYPE_GREEN = 'G';
static const char BLOCK_TYPE_BLUE = 'B';

// Pins
// ----------------------------------------------------------------------------
static const unsigned char SETTING_PIN_LED_DATA = 6;
static const unsigned char SETTING_SCORE_BOARD_CS = 10;
static const unsigned char SETTING_PIN_PLAYER_GREEN_BUTTON = 2;
static const unsigned char SETTING_PIN_PLAYER_BLUE_BUTTON = 3;
static const unsigned char SETTING_PIN_PLAYER_RED_BUTTON = 4;

// Buttons
// ----------------------------------------------------------------------------
class CButton {
private:
    unsigned int pin;
    bool lastRead;
    bool currentState;

    static const bool BUTTON_UP = true;
    static const bool BUTTON_DOWN = false;

public:
    bool currentRead;

    // If this button has been pressed in the last loop. This is only triggered once per press.
    // The button must come up before it is able to be set low again.
    bool hasButtonBeenPressed()
    {
        return currentState == BUTTON_DOWN;
    }

    // This returns the actual state of the button. This is useful to tell if the button is being held down.
    bool isButtonDown()
    {
        return currentRead == BUTTON_DOWN;
    }

    void loop()
    {
        // Debounce the button.
        this->currentRead = digitalRead(this->pin);
        if (this->currentRead == lastRead) {
            currentState = BUTTON_UP;
            return;
        }
        lastRead = this->currentRead;

        if (currentRead == BUTTON_DOWN) {
            Serial.println(String(millis()) + " FYI: Button was pressed, pin = " + String(pin));
        }
        currentState = currentRead;
    }
    void init(unsigned int pin)
    {
        this->lastRead = BUTTON_DOWN;
        this->currentRead = BUTTON_DOWN;
        this->currentState = BUTTON_DOWN;
        this->pin = pin;
        pinMode(this->pin, INPUT_PULLUP);
    }
    void reset()
    {
        this->currentState = BUTTON_DOWN;
    }
};

// CTetris
// ----------------------------------------------------------------------------
class CTetris {
private:
    char blockStack[SETTINGS_NUM_LEDS];
    unsigned short blockStackTop;

    char currentBlockType;
    unsigned short currentBlockLocation;
    unsigned char currentBlockSize;

    CButton & redButton;
    CButton & greenButton;
    CButton & blueButton;

    unsigned short gameScore;

    void CheckAndDoGameInput(bool red, bool green, bool blue);
    void CheckAndMoveBlocks();
    void MoveBlock();
    bool CheckForGameOver();
    void Draw();

    static const CRGB GetRGBForBlockType(char blockType);
    unsigned long GetBlockSpeed(); 

public:
    CTetris() {}; 
    unsigned short GetGameScore()
    {
        return this->gameScore;
    };
    void Setup();
    void Loop(bool red, bool green, bool blue);
};

void CTetris::Setup()
{
    // Reset block stack
    memset(this->blockStack, BLOCK_TYPE_NONE, SETTINGS_NUM_LEDS);

    // Last block
    this->blockStackTop = 0; // No blocks in the stack

    // Current block
    this->currentBlockType = BLOCK_TYPE_RED;
    this->currentBlockLocation = SETTINGS_NUM_LEDS;
    this->currentBlockSize = SETTINGS_BLOCK_SIZE;

    this->gameScore = 0;
}

void CTetris::Loop(bool red, bool green, bool blue)
{
    if (this->CheckForGameOver()) {
        // Game over ! :(
        return;
    }

    this->CheckAndDoGameInput(red, green, blue);
    this->CheckAndMoveBlocks();

    // Display the blockStack to the strip.
    this->Draw();
}

unsigned long CTetris::GetBlockSpeed() {
    // Game gets faster as you core more points 
    unsigned long speed = SETTING_BLOCK_FALL_SPEED - this->GetGameScore() ; 
    return (speed < 10 ? 10 : speed) ; // Min 10 speed. 
}

void CTetris::CheckAndDoGameInput(bool red, bool green, bool blue)
{
    // Check to see if any of the buttons are pressed
    //      Check to see if the button that is pressed is the same as the bottom block.
    //      if there is no buttom block, check the current block.

    char buttonPressType = BLOCK_TYPE_NONE;
    if (red) {
        buttonPressType = BLOCK_TYPE_RED;
    } else if (green) {
        buttonPressType = BLOCK_TYPE_GREEN;
    } else if (blue) {
        buttonPressType = BLOCK_TYPE_BLUE;
    } else {
        // No button have been pressed
        return; // Nothing to do
    }

    Serial.print(String(millis()) + " Button [" + String(buttonPressType) + "] ");

    // Check to see if there are any blocks at the bottom
    if (this->blockStackTop > 0) {
        // There is at lest one block at the bottom of the stack.
        // check to see if this block is the same color as the one we just presses.
        if (buttonPressType == this->blockStack[0]) {
            // Good !
            Serial.print("Good Bottom [" + String(this->blockStack[0]) + "] ");
            // They pressed right button
            this->gameScore++;

            // We need to remove the block and shift the entire stack down.
            memset(this->blockStack, BLOCK_TYPE_NONE, SETTINGS_BLOCK_SIZE);

            // We are done
            return;
        } else {
            // BAD !
            Serial.print("Bad Bottom [" + String(this->blockStack[0]) + "] ");

            // They pushed the wrong button !
            // Move the block faster
            this->MoveBlock();
            return;
        }
    }

    // There are no blocks on the stack yet.
    // Check to see there is a block that is currently falling and if it matches
    if (this->currentBlockType == buttonPressType) {
        // Good !
        Serial.print("Good current [" + String(this->currentBlockType) + "] ");

        // They pressed the right button
        this->gameScore++;

        this->currentBlockType = BLOCK_TYPE_NONE;
        this->currentBlockLocation = 0;
        return;
    } else {
        // BAD !
        Serial.print("Bad current [" + String(this->currentBlockType) + "] ");
        // They pressed the wrong button.
        // Move the block faster
        this->MoveBlock();
        return;
    }
}

char GetRandomBlockColor() {
    static const char newTypeBlock[3] = { BLOCK_TYPE_RED, BLOCK_TYPE_GREEN, BLOCK_TYPE_BLUE };
    return newTypeBlock[random(0, 3)] ; 
}

void CTetris::MoveBlock()
{
    // Check to see if the last block is already at the top of the stack.
    if (this->currentBlockLocation <= this->blockStackTop || this->currentBlockLocation <= 0) {
        // The current block is on top of the block stack.

        // Add the current block to the block stack
        for (unsigned short offset = 0; offset < this->currentBlockSize; offset++) {
            this->blockStack[this->blockStackTop++] = this->currentBlockType;
        }

        // Create new block
        this->currentBlockLocation = SETTINGS_NUM_LEDS;
        this->currentBlockSize = SETTINGS_BLOCK_SIZE;

        // We need to choose a block color, but it can not match the same block color as the last one.
        // The block color should also be random.
        char newBlockColor = BLOCK_TYPE_RED ; 
        for( unsigned char attempts = 0 ; attempts < 10 ; attempts++) {
            newBlockColor = GetRandomBlockColor(); 
            if( newBlockColor != this->currentBlockType ) {
                break ; 
            }
            // We randomly selected the same color as before. Try again. 
        }
        this->currentBlockType = newBlockColor;

        // New block has been created.
    }

    // Move the block.
    this->currentBlockLocation--;

    // Check to see if this block is ontop of the
}

void CTetris::CheckAndMoveBlocks()
{
    // Remove any empty blocks if needed. 
    if( this->blockStackTop > 0 && this->blockStack[0] == BLOCK_TYPE_NONE ){
        // We need to remove the block and shift the entire stack down.
        memcpy(this->blockStack, this->blockStack + SETTINGS_BLOCK_SIZE, this->blockStackTop);
        memset(this->blockStack + (this->blockStackTop - SETTINGS_BLOCK_SIZE), BLOCK_TYPE_NONE, SETTINGS_BLOCK_SIZE);
        this->blockStackTop -= SETTINGS_BLOCK_SIZE;
    }

    static unsigned long nextBlockMove = 0;
    if (nextBlockMove > millis()) {
        // Its not time to move the block yet.
        return;
    }
    // Time to move the block down
    this->MoveBlock();

    // Update the timer.
    nextBlockMove = millis() + this->GetBlockSpeed();
}

bool CTetris::CheckForGameOver()
{
    if (this->blockStackTop > SETTINGS_NUM_LEDS || this->blockStackTop + this->currentBlockSize > SETTINGS_NUM_LEDS) {
        // Game over
        return true;
    }

    return false;
}

static const CRGB CTetris::GetRGBForBlockType(char blockType)
{
    switch (blockType) {
        case BLOCK_TYPE_RED: {
            return CRGB::Red;
            break;
        }
        case BLOCK_TYPE_GREEN: {
            return CRGB::Green;
            break;
        }
        case BLOCK_TYPE_BLUE: {
            return CRGB::Blue;
            break;
        }
        default: {
            return CRGB::Black;
            break;
        }
    }
}

void CTetris::Draw()
{

    for (unsigned short offset = 0; offset < SETTINGS_NUM_LEDS; offset++) {
        if (offset <= this->blockStackTop) {
            // Print the blocks in the stack
            // Serial.print(String(this->blockStack[offset]));
            leds[offset] = GetRGBForBlockType(this->blockStack[offset]);
        } else if (offset > this->currentBlockLocation && offset <= this->currentBlockLocation + this->currentBlockSize) {
            // Print the falling block
            // Serial.print(String(this->currentBlockType));
            leds[offset] = GetRGBForBlockType(this->currentBlockType);
        } else {
            // Empty
            // Serial.print(String(BLOCK_TYPE_NONE));
            leds[offset] = GetRGBForBlockType(BLOCK_TYPE_NONE);
        }
    }
    // Serial.print("\n");

    // Print the falling block.
}

// --------------------------------------------------

// Globals
// ----------------------------------------------------------------------------
// Game
CTetris game;
// Score display
static const unsigned char SETTING_SCORE_BOARD_DISPLAYS = 4;
LedMatrix ledMatrix = LedMatrix(SETTING_SCORE_BOARD_DISPLAYS, SETTING_SCORE_BOARD_CS);
// Buttons
static const unsigned char BUTTON_MAX = 3;
CButton inputsButtons[BUTTON_MAX];

void UpdateScoreBoard(String text)
{
    // Draw the tet.
    ledMatrix.clear();
    ledMatrix.setText(text);
    ledMatrix.drawText();

    /*
    for (unsigned char offsetLives = 0; offsetLives < gameLives; offsetLives++) {
        ledMatrix.setPixel(31 - offsetLives, 7);
    }
    */

    ledMatrix.commit();
}

void setup()
{
    // initialize serial communications
    Serial.begin(SETTING_SERIAL_BAUD_RATE);

    // Print version info
    Serial.println("LED Tetris v0.0.1, Last updated: 2018 Aug 25 ");
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
    game.Setup();
}

void loop()
{
    // Check the buttons
    for (int offsetButton = 0; offsetButton < BUTTON_MAX; offsetButton++) {
        inputsButtons[offsetButton].loop();
    }

/*
    for (int offsetButton = 0; offsetButton < BUTTON_MAX; offsetButton++) {
        if( inputsButtons[offsetButton].hasButtonBeenPressed() ) {
            Serial.println("Button ["+ String( offsetButton ) +"] hasButtonBeenPressed ");
        }
        if( inputsButtons[offsetButton].isButtonDown() ) {
            Serial.println("Button ["+ String( offsetButton ) +"] isButtonDown ");
        }
    }

    return ; 
    */

    // Do game loop
    game.Loop(inputsButtons[0].hasButtonBeenPressed(), inputsButtons[1].hasButtonBeenPressed(), inputsButtons[2].hasButtonBeenPressed() );

    // Update the LEDs
    FastLED.show();
    // FastLED.delay(1000 / SETTINGS_FRAMES_PER_SECOND);

    // Update the score board
    UpdateScoreBoard(String(game.GetGameScore()));
}
