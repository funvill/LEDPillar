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
static const unsigned short SETTINGS_NUM_LEDS = (30 * 4); // The number of LEDs in this strip. 30 per meter, and 4 meters of LEDs.
static const unsigned short SETTING_GAME_OVER_FLASHING_SPEED = 300; 
static const unsigned short SETTING_CREATE_NEW_BLOCKS_SPEED = 500 ; 
static const unsigned char SETTINGS_BLOCK_SIZE = 4; // Number of LEDs
static const unsigned long SETTING_BLOCK_FALL_SPEED = 70; // ms, lower numbers are faster. Starting speed.
const unsigned char SETTINGS_MAX_BLOCKS = 10;

// LEDs
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

CRGB leds[SETTINGS_NUM_LEDS];

// CTetris
// ----------------------------------------------------------------------------

class CTetrisBlock {
public:
    char type; // _, R, G, B
    unsigned short location; // The location of the bottom led in this block
    unsigned char size; // the amount of LEDs in the block
    unsigned char speed; // The amount of block to move by when moving

    void Reset()
    {
        this->type = BLOCK_TYPE_NONE;
        this->location = SETTINGS_NUM_LEDS;
        this->size = SETTINGS_BLOCK_SIZE;
        this->speed = SETTING_BLOCK_FALL_SPEED;
    }
};



// CTetris
// ----------------------------------------------------------------------------
class CTetris {
private:

    // Block stacks 
    char blockStack[SETTINGS_NUM_LEDS];
    unsigned short blockStackTop;

    // Container to hold potential blocks. 
    CTetrisBlock blocks[SETTINGS_MAX_BLOCKS];

    // Buttons 
    CButton& redButton;
    CButton& greenButton;
    CButton& blueButton;

    unsigned short gameScore;

    void CheckAndDoGameInput(CButton& red, CButton& green, CButton& blue);
    void CheckAndMoveBlocks();
    void MoveBlock();
    bool CheckForGameOver();
    void Draw();
    void GameOver();
    void CreateNewBlock();
    void CheckAndCreateBlocks();

    static const CRGB GetRGBForBlockType(char blockType);
    unsigned long GetBlockSpeed();

public:
    CTetris(){this->Reset();};
    unsigned short GetGameScore()
    {
        return this->gameScore;
    };
    void Reset();
    bool Loop(CButton& red, CButton& green, CButton& blue);
};

void CTetris::Reset()
{
    // Reset block stack
    memset(this->blockStack, BLOCK_TYPE_NONE, SETTINGS_NUM_LEDS);
    this->blockStackTop = 0; // No blocks in the stack

    // Reset blocks
    for (unsigned char blocksOffset = 0; blocksOffset < SETTINGS_MAX_BLOCKS; blocksOffset++) {
        this->blocks[blocksOffset].Reset();
    }

    // Reset the score 
    this->gameScore = 0;
}

bool CTetris::Loop(CButton& red, CButton& green, CButton& blue)
{
    // Do buttonloop
    red.loop();
    green.loop();
    blue.loop();

    if (this->CheckForGameOver()) {
        // Game over ! :(
        this->GameOver();

        // Check for reset
        if (red.isButtonDown() && green.isButtonDown() && blue.isButtonDown()) {
            // All buttons pressed. Reset
            this->Setup();
            this->Draw();
            return false;
        }
        return true;
    }

    this->CheckAndDoGameInput(red, green, blue);
    this->CheckAndMoveBlocks();
    this->CheckAndCreateBlocks();

    // Display the blockStack to the strip.
    this->Draw();
    return true;
}

void CTetris::CheckAndCreateBlocks()
{
    static unsigned long nextCreateBlock = 0;
    if (nextCreateBlock < millis()) {
        // Time to create a new block.
        nextCreateBlock = millis() + SETTING_CREATE_NEW_BLOCKS_SPEED;
        this->CreateNewBlock();
    }
}

void CTetris::GameOver()
{
    // Flash on and off with the current colors.
    static unsigned long nextFlash = 0;
    static bool flashState = false;

    if (nextFlash < millis()) {
        nextFlash = millis() + SETTING_GAME_OVER_FLASHING_SPEED; // ms
        flashState = !flashState;
        if (flashState) {
            for (unsigned short offset = 0; offset < SETTINGS_NUM_LEDS; offset++) {
                leds[offset] = CRGB::Black;
            }
        } else {
            this->Draw();
        }
    }
}

unsigned long CTetris::GetBlockSpeed()
{
    // Game gets faster as you core more points
    unsigned long speed = SETTING_BLOCK_FALL_SPEED - (this->GetGameScore() * 2);
    return (speed < 10 ? 10 : speed); // Min 10 speed.
}

void CTetris::CheckAndDoGameInput(CButton& red, CButton& green, CButton& blue)
{
    // Check to see if any of the buttons are pressed
    //      Check to see if the button that is pressed is the same as the bottom block.
    //      if there is no buttom block, check the current block.

    char buttonPressType = BLOCK_TYPE_NONE;
    if (red.hasButtonBeenPressed()) {
        buttonPressType = BLOCK_TYPE_RED;
    } else if (green.hasButtonBeenPressed()) {
        buttonPressType = BLOCK_TYPE_GREEN;
    } else if (blue.hasButtonBeenPressed()) {
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
    // Find offset for the lowest block
    unsigned char lowestBlockOffset = SETTINGS_MAX_BLOCKS;
    unsigned short lowersBlockLocation = SETTINGS_NUM_LEDS;
    for (unsigned char offsetBlock = 0; offsetBlock < SETTINGS_MAX_BLOCKS; offsetBlock++) {
        if (this->blocks[offsetBlock].type == BLOCK_TYPE_NONE) {
            continue; // Skip
        }
        if (this->blocks[offsetBlock].location < lowersBlockLocation) {
            lowersBlockLocation = this->blocks[offsetBlock].location;
            lowestBlockOffset = offsetBlock;
        }
    }

    if (lowestBlockOffset != SETTINGS_MAX_BLOCKS) {
        // There is an active block.
        if (this->blocks[lowestBlockOffset].type == buttonPressType) {
            // Good !
            Serial.print("Good current(" + String(lowestBlockOffset) + ")=[" + String(this->blocks[lowestBlockOffset].type) + "] ");

            // They pressed the right button
            this->gameScore++;

            this->blocks[lowestBlockOffset].Reset();
            CreateNewBlock();
            return;
        } else {
            // BAD !
            Serial.print("Bad current(" + String(lowestBlockOffset) + ")=[" + String(this->blocks[lowestBlockOffset].type) + "] ");
            // They pressed the wrong button.
            // Move the block faster
            this->MoveBlock();
            return;
        }
    }
}

char GetRandomBlockColor()
{
    static const char newTypeBlock[3] = { BLOCK_TYPE_RED, BLOCK_TYPE_GREEN, BLOCK_TYPE_BLUE };
    return newTypeBlock[random(0, 3)];
}

void CTetris::CreateNewBlock()
{
    // We don't want two blocks ontop of each other.
    // First check to see if there is already an active block at the top.
    char highestBlockType = BLOCK_TYPE_NONE;
    unsigned highestBlockLocation = 0;
    for (unsigned char offsetBlock = 0; offsetBlock < SETTINGS_MAX_BLOCKS; offsetBlock++) {
        if (this->blocks[offsetBlock].type == BLOCK_TYPE_NONE) {
            continue; // Skip
        }

        if (this->blocks[offsetBlock].location + this->blocks[offsetBlock].size > SETTINGS_NUM_LEDS - SETTINGS_BLOCK_SIZE) {
            // We found a freshly created block already at the top of the LEDs. nothing to do.
            return;
        }

        // Find the highest blocks color.
        // When we are creating the next block we don't want to double up on the colors.
        if (this->blocks[offsetBlock].location > highestBlockLocation) {
            highestBlockLocation = this->blocks[offsetBlock].location;
            highestBlockType = this->blocks[offsetBlock].type;
        }
    }

    for (unsigned char offsetBlock = 0; offsetBlock < SETTINGS_MAX_BLOCKS; offsetBlock++) {
        if (this->blocks[offsetBlock].type != BLOCK_TYPE_NONE) {
            continue; // Skip
        }

        // Found an empty spot
        // Create new block
        this->blocks[offsetBlock].location = SETTINGS_NUM_LEDS;
        this->blocks[offsetBlock].size = SETTINGS_BLOCK_SIZE;

        // We need to choose a block color, but it can not match the same block color as the last one.
        // The block color should also be random.
        char newBlockColor = BLOCK_TYPE_RED;
        for (unsigned char attempts = 0; attempts < 10; attempts++) {
            newBlockColor = GetRandomBlockColor();
            if (newBlockColor != highestBlockType) {
                break;
            }
            // We randomly selected the same color as before. Try again.
        }
        this->blocks[offsetBlock].type = newBlockColor;
        return;
    }
}

void CTetris::MoveBlock()
{
    bool foundActiveBlock = false;
    for (unsigned char offsetBlock = 0; offsetBlock < SETTINGS_MAX_BLOCKS; offsetBlock++) {
        if (this->blocks[offsetBlock].type == BLOCK_TYPE_NONE) {
            continue; // Skip
        }

        // Check to see if the last block is already at the top of the stack.
        if (this->blocks[offsetBlock].location <= this->blockStackTop || this->blocks[offsetBlock].location <= 0) {
            // The current block is on top of the block stack.

            // Add the current block to the block stack
            for (unsigned short offset = 0; offset < this->blocks[offsetBlock].size; offset++) {
                this->blockStack[this->blockStackTop++] = this->blocks[offsetBlock].type;
            }

            // Kill this block
            this->blocks[offsetBlock].type = BLOCK_TYPE_NONE;
            continue;
        }

        // Move the block.
        this->blocks[offsetBlock].location--;
        foundActiveBlock = true;
    }

    // Check to see if there are any active blocks. If there are none then create one.
    if (!foundActiveBlock) {
        this->CreateNewBlock();
    }
}

void CTetris::CheckAndMoveBlocks()
{
    // Remove any empty blocks if needed.
    if (this->blockStackTop > 0 && this->blockStack[0] == BLOCK_TYPE_NONE) {
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
    if (this->blockStackTop > SETTINGS_NUM_LEDS) {
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
        } else {
            // Empty
            // Serial.print(String(BLOCK_TYPE_NONE));
            leds[offset] = GetRGBForBlockType(BLOCK_TYPE_NONE);
        }

        for (unsigned char offsetBlock = 0; offsetBlock < SETTINGS_MAX_BLOCKS; offsetBlock++) {
            if (this->blocks[offsetBlock].type == BLOCK_TYPE_NONE) {
                continue; // Skip
            }

            if (offset > this->blocks[offsetBlock].location && offset <= this->blocks[offsetBlock].location + this->blocks[offsetBlock].size) {
                // Print the falling block
                // Serial.print(String(this->currentBlockType));
                leds[offset] = GetRGBForBlockType(this->blocks[offsetBlock].type);
            }
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

// Used for demo patterns
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbowWithGlitter, confetti, sinelon, juggle, bpm };
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
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
    game.Reset();
}

void loop()
{
    static bool gameState = false; // false = demo, true = game
    static unsigned long gameStageChangeTimeout = 0; // How long before you can change the game state again.

    if (gameState) {
        // Do game loop
        if (!game.Loop(inputsButtons[0], inputsButtons[1], inputsButtons[2])) {
            // Game over.
            gameState = false;
            gameStageChangeTimeout = millis() + 1000;
        }

        // Update the score board
        UpdateScoreBoard(String(game.GetGameScore()));

    } else {
        // Do demo patterns
        UpdateScoreBoard("Push me");

        // Call the current pattern function once, updating the 'leds' array
        gPatterns[gCurrentPatternNumber]();

        // Do some periodic updates
        EVERY_N_MILLISECONDS(10) { gHue += 3; } // slowly cycle the "base color" through the rainbow
        EVERY_N_SECONDS(30) { nextPattern(); } // change patterns periodically

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
                    game.Reset();
                    return;
                }
            }
        }
    }

    // Update the LEDs
    FastLED.show();
    // FastLED.delay(1000 / SETTINGS_FRAMES_PER_SECOND);
}

// ----------------------------------------------------------------------------
// Demo patterns
// ----------------------------------------------------------------------------

void rainbow()
{
    // FastLED's built-in rainbow generator
    fill_rainbow(leds, SETTINGS_NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter()
{
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter)
{
    if (random8() < chanceOfGlitter) {
        leds[random16(SETTINGS_NUM_LEDS)] += CRGB::White;
    }
}

void confetti()
{
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(leds, SETTINGS_NUM_LEDS, 10);
    int pos = random16(SETTINGS_NUM_LEDS);
    leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon()
{
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(leds, SETTINGS_NUM_LEDS, 20);
    int pos = beatsin16(13, 0, SETTINGS_NUM_LEDS);
    leds[pos] += CHSV(gHue, 255, 192);
}

void bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 100;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < SETTINGS_NUM_LEDS; i++) { //9948
        leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
}

void juggle()
{
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(leds, SETTINGS_NUM_LEDS, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++) {
        leds[beatsin16(i + 7, 0, SETTINGS_NUM_LEDS)] |= CHSV(dothue, 200, 255);
        dothue += 32;
    }
}
