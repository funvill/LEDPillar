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

FASTLED_USING_NAMESPACE

// Game Settings
// ----------------------------------------------------------------------------
static const unsigned short SETTINGS_NUM_LEDS = (30*4); // The number of LEDs in this strip.
static const unsigned char SETTINGS_MAX_BEATS = 15; // The max number of beats that can be on the pillar at the same time.
static const unsigned char SETTINGS_FRAMES_PER_SECOND = 120;
static const unsigned char SETTING_BEAT_TAIL_LENGTH = 3; // The length of the fading tail.
static const unsigned long SETTING_SERIAL_BAUD_RATE = 115200; // The baud rate for the debug prints.
static const unsigned char SETTING_GLOBAL_BRIGHTNESS = 255; // Set the global brightness, this is useful when the LED strip is powered via USB. 0-254
static const unsigned char SETTINGS_GOAL_SIZE = 10; // The size of the goal at the bottom of the pillar.
static const unsigned char SETTINGS_BUTTON_STATUS_SIZE = 5; // The size of the current button down indicator at the top pf the pillar.
static CRGB SETTING_GOAL_COLOR = CRGB::Goldenrod; // DarkOrchid; // https://github.com/FastLED/FastLED/wiki/Pixel-reference
static const unsigned char SETTINGS_STARTING_LIVES = 5; // How many mistakes that they can make before the game is over.

static const unsigned short SETTING_CREATION_SPEED_START = (1000 * 2); // The starting time for how often to create a new beat.
static const unsigned short SETTING_CREATION_SPEED_END = 300; // The minimum time for how often to create a new beat.
static const unsigned short SETTING_CREATION_SPEED_INCREMENT = 200; // How much faster the creation beats get each time the user scores.

static const unsigned short SETTING_BEAT_SPEED_START = 50; // The starting speed for the movement of the beats.
static const unsigned short SETTING_BEAT_SPEED_END = 6; // The fastest speed for the movement of the beats.
static const unsigned short SETTING_BEAT_SPEED_INCREMENT = 1; // How much faster the movement of beats get each time the user scores.

static const unsigned short SETTING_SCORE_GOOD = 5; // How many points they get when they press the right button.
static const unsigned short SETTING_SCORE_BAD = 0; // How many points get removed from their score when they get it wrong.

// Pins
// ----------------------------------------------------------------------------
static const unsigned char SETTING_PIN_LED_DATA = 6;
static const unsigned char SETTING_SCORE_BOARD_CS = 10;
static const unsigned char SETTING_PIN_PLAYER_GREEN_BUTTON = 2;
static const unsigned char SETTING_PIN_PLAYER_BLUE_BUTTON = 3;
static const unsigned char SETTING_PIN_PLAYER_RED_BUTTON = 4;

#define NULL 0

// Globals
// ----------------------------------------------------------------------------

uint8_t gCurrentPatternNumber ; // Index number of which pattern is current

// Score display
static const unsigned char SETTING_SCORE_BOARD_DISPLAYS = 4;
LedMatrix ledMatrix = LedMatrix(SETTING_SCORE_BOARD_DISPLAYS, SETTING_SCORE_BOARD_CS);
char gameLives;

// Game states
static const unsigned char GAME_STATE_STARTUP = 0; // Demo mode, lots of intersting patters
static const unsigned char GAME_STATE_STARTING = 1; // The user has started the game, and the score board is counting down to start.
static const unsigned char GAME_STATE_RUNNING = 2;
static const unsigned char GAME_STATE_GAMEOVER = 3;
unsigned char gameState;

// Game score
short gameScore;

// Creation speed.
// The speed that new beats will be created on the pillar at a time.
short creationSpeed;
short beatsMovementSpeed;

// LEDs
// The current status of all the LEDS
CRGB leds[SETTINGS_NUM_LEDS];


String GetColorAsString(CRGB color)
{
    if (color.red > 100) {
        return String("Red");
    } else if (color.green > 100) {
        return String("Green");
    } else if (color.blue > 100) {
        return String("Blue");
    }

    return String("Unknown r=" + String(color.red) + ", g=" + String(color.green) + ", b=" + String(color.blue));
}


// Buttons
class CButtons {
private:
    unsigned int pin;
    bool lastRead;
    bool currentState;

    static const bool BUTTON_UP = true;
    static const bool BUTTON_DOWN = false;

public:
    CRGB color;
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
        return currentRead == BUTTON_DOWN;;
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
            Serial.println(String(millis()) + " FYI: Button was pressed, pin = " + String(pin) + ", Color = " + GetColorAsString(this->color));
        }
        currentState = currentRead;
    }
    void Init(unsigned int pin, CRGB color)
    {
        this->currentRead = BUTTON_DOWN;
        this->pin = pin;
        this->color = color;
        pinMode(this->pin, INPUT_PULLUP);
    }
    void reset() {
        currentState = BUTTON_DOWN;
    }
};

static const unsigned char BUTTON_MAX = 3;
CButtons inputsButtons[BUTTON_MAX];

class CBeats {
private:
    unsigned long lastMove;

public:
    bool isAlive;
    short location;
    short speed; // Lower is faster
    CRGB color;

    CBeats()
    {
        reset();
    }

    void reset()
    {
        // This beat is dead
        isAlive = false;
        lastMove = 0 ; 
        location = SETTINGS_NUM_LEDS ; 
        speed = SETTING_BEAT_SPEED_START ; 

        // Change any of the tail of the LEDs to black
        for (int offsetLED = location; offsetLED < SETTINGS_NUM_LEDS && (offsetLED - location) < SETTING_BEAT_TAIL_LENGTH; offsetLED++) {
            leds[offsetLED] = CRGB::Black;
        }
    }

    bool create(short movementSpeed = SETTING_BEAT_SPEED_START)
    {
        if (isAlive) {
            // This beats is alive. We can not create on it.
            return false;
        }

        lastMove = 0;
        speed = movementSpeed;
        location = SETTINGS_NUM_LEDS;

        isAlive = true;

        switch (random(0, 3)) {
            default:
            case 0:
                color = CRGB::Green;
                break;
            case 1:
                color = CRGB::Blue;
                break;
            case 2:
                color = CRGB::Red;
                break;
        }

        return true;
    }

    void draw()
    {
        if (!isAlive) {
            // Do nothing
            return;
        }
        leds[location] = color;
    }

    void move()
    {
        if (!isAlive) {
            // This beats is alive. We can not create on it.
            return ;
        }

        // Move the beats.
        location--;
        if( location < 0 ) {
            location = 0 ; // Range checking 
        }

        // Create a tail of fading LEDs.
        for (int offsetLED = location; offsetLED < SETTINGS_NUM_LEDS && (offsetLED - location) < SETTING_BEAT_TAIL_LENGTH; offsetLED++) {
            leds[offsetLED].fadeToBlackBy(220 - ((offsetLED - location) * 20));
        }

        // ensure that the last LED in the tail is black.
        if (location < SETTINGS_NUM_LEDS - SETTING_BEAT_TAIL_LENGTH) {
            leds[location + SETTING_BEAT_TAIL_LENGTH] = CRGB::Black;
        }
    }

    void loop()
    {
        if (!isAlive) {
            // Do nothing
            return;
        }

        // Check for dead beats.
        if (location == 0) {
            reset();
            return;
        }

        // See if we have to move the beat
        if (lastMove <= millis() - abs(speed)) {
            lastMove = millis();
            move();
        }

        draw();
    }
};

CBeats beats[SETTINGS_MAX_BEATS];
CBeats* nextBeat;

void reset()
{
    UpdateScoreBoard("Push");
    gameLives = SETTINGS_STARTING_LIVES ; 
    gameState = GAME_STATE_STARTUP;
    gameScore = 0;
    creationSpeed = SETTING_CREATION_SPEED_START;
    beatsMovementSpeed = SETTING_BEAT_SPEED_START;
    gCurrentPatternNumber = 0 ;


    // Set all the LEDS to black
    for (unsigned short offsetLED = 0; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
        leds[offsetLED] = CRGB::Black;
    } 

    // Reset the buttons
    for (unsigned short offsetButton = 0; offsetButton < BUTTON_MAX; offsetButton++) {
        inputsButtons[offsetButton].reset();
    }

    // Reset all the beats back to their starting locations.
    for (unsigned short offsetBeat = 0; offsetBeat < SETTINGS_MAX_BEATS; offsetBeat++) {
        beats[offsetBeat].reset();
    }
    nextBeat = NULL;
}

void setup()
{
    // initialize serial communications
    Serial.begin(SETTING_SERIAL_BAUD_RATE);

    // Print version info
    Serial.println("LED Pillar v0.1.1, Last updated: 2017 May 24 ");
    Serial.println("More information: https://blog.abluestar.com/projects/2017-led-pillar");
    
    // LEDs
    Serial.println(String(millis()) + " FYI: Setting up the LEDS. LED Count = " + String(SETTINGS_NUM_LEDS));
    FastLED.addLeds<NEOPIXEL, SETTING_PIN_LED_DATA>(leds, SETTINGS_NUM_LEDS);
    // set master brightness control
    FastLED.setBrightness(SETTING_GLOBAL_BRIGHTNESS);

    // Buttons.
    inputsButtons[0].Init(SETTING_PIN_PLAYER_GREEN_BUTTON, CRGB::Green);
    inputsButtons[1].Init(SETTING_PIN_PLAYER_BLUE_BUTTON, CRGB::Blue);
    inputsButtons[2].Init(SETTING_PIN_PLAYER_RED_BUTTON, CRGB::Red);

    // LED Matrix / Score board.
    ledMatrix.init();
    ledMatrix.setRotation(true);
    ledMatrix.setTextAlignment(TEXT_ALIGN_LEFT);
    ledMatrix.setCharWidth(8);

    // Reset the game back to defaults.
    reset();
}

void createbeats()
{
    // Search thought the list of beats and find one that we can create.
    for (unsigned char offsetBeat = 0; offsetBeat < SETTINGS_MAX_BEATS; offsetBeat++) {
        if (beats[offsetBeat].create(beatsMovementSpeed)) {
            // We were able to create a new beats
            Serial.println(String(millis()) + " FYI: Created a beat, offsetBeat="+String(offsetBeat)+", beatsMovementSpeed="+String(beatsMovementSpeed));
            return; 
        } 
    }
    Serial.println(String(millis()) + " Error: No more beats to create");
}

void levelUp()
{
    // Only level up after each 5 scores. 
    if( gameScore % (5 * SETTING_SCORE_GOOD) != 0 ) {
        return ; 
    }

    // Increase the creation speed of new beats.
    creationSpeed -= SETTING_CREATION_SPEED_INCREMENT;
    if (creationSpeed < SETTING_CREATION_SPEED_END) {
        // We are already creating them as fast we want to.
        creationSpeed = SETTING_CREATION_SPEED_END;

        // Lets increase the speed that the beats move instead.
        beatsMovementSpeed -= SETTING_BEAT_SPEED_INCREMENT;
        if (beatsMovementSpeed < SETTING_BEAT_SPEED_END) {
            beatsMovementSpeed = SETTING_BEAT_SPEED_END;
        }
        
    }
    Serial.println(String(millis()) + " FYI: levelUp. beatsMovementSpeed = " + String(beatsMovementSpeed) + ", creationSpeed = " + String(creationSpeed));
}
void gameScored(CRGB color)
{
    gameScore += SETTING_SCORE_GOOD;
    if (gameScore > 9999) {
        gameScore = 9999;
    }
    Serial.println(String(millis()) + " FYI: Game Scored. Color = " + GetColorAsString(color) + ", Score = " + String(gameScore));

    // Update score board
    UpdateScoreBoard(String(gameScore));
    levelUp();
        
}

void gameUserFail(CRGB color)
{
    // gameScored( color ) ; 
    // return ;

    if (gameLives > 0) {
        gameLives--;
    }
    Serial.println(String(millis()) + " FYI: gameUserFail. Color = " + GetColorAsString(color) + ", gameLives = " + String(gameLives, DEC));


    if (gameLives == 0) {
        gameLives = 0;
        Serial.println(String(millis()) + " FYI: Game over, Score = " + String(gameScore) );
        gameState = GAME_STATE_GAMEOVER;
        UpdateScoreBoard(String(gameScore));
        return ; 
    }

    // Update score board to show the lives. 
    UpdateScoreBoard(String(gameScore));
}

void gameOver()
{
    // Check to see if the user has pressed TWO button at the same time.
    unsigned char buttonDown = 0;
    for (int offsetButton = 0; offsetButton < BUTTON_MAX; offsetButton++) {
        if (inputsButtons[offsetButton].isButtonDown()) {
            buttonDown++;
        }
    }
    
    if (buttonDown >= 3) {
        reset();
    }
}

void gameLoop()
{
    // Draw the goal.
    // We want to draw the goal first so that it can be overwritten by the moving beats.
    for (unsigned char offsetLED = 0; offsetLED < SETTINGS_GOAL_SIZE; offsetLED++) {
        leds[offsetLED] = SETTING_GOAL_COLOR;
    }

    // Move the beats.
    for (int offsetBeats = 0; offsetBeats < SETTINGS_MAX_BEATS; offsetBeats++) {
        if (!beats[offsetBeats].isAlive) {
            continue;
        }
        beats[offsetBeats].loop();

        // Serial.print("@" + String(offsetBeats) + "=" + String(beats[offsetBeats].location) + "[" + GetColorAsString(beats[offsetBeats].color) + "], ");

        // Check to see if the beat has gone out of bounds
        if (beats[offsetBeats].location == 0) {
            if (nextBeat == &beats[offsetBeats]) {
                nextBeat = NULL;
            }
            Serial.println(String(millis()) + " FYI: beat out of bounds, offsetBeats = " + String(offsetBeats) + ", Color = " + GetColorAsString(beats[offsetBeats].color));
            gameUserFail(beats[offsetBeats].color); // Debug: Show the progression of the game.
            beats[offsetBeats].reset();
        } else {
            // Find the next beat.
            if (nextBeat == NULL) {
                nextBeat = &beats[offsetBeats];
            } else {
                if (nextBeat->location > beats[offsetBeats].location) {
                    nextBeat = &beats[offsetBeats];
                }
            }
        }
    }

    // Check to see if the user has pressed a button.
    bool buttonWasDown = false;
    for (int offsetButton = 0; offsetButton < BUTTON_MAX; offsetButton++) {
        if (inputsButtons[offsetButton].hasButtonBeenPressed()) {
            bool foundBeat = false;
            // The user has pressed a button.
            if (nextBeat != NULL) {
                if (nextBeat->isAlive) {
                    if (nextBeat->location < SETTINGS_GOAL_SIZE) {
                        // We found a beat in the goal location.
                        // Check to see if it matches the color of the button that was just pressed.
                        if (inputsButtons[offsetButton].color == nextBeat->color) {
                            // The right button and the right color has been pressed.
                            gameScored(inputsButtons[offsetButton].color);
                            nextBeat->reset();
                            nextBeat = NULL;
                            foundBeat = true;
                            break;
                        }
                    }
                }
            }

            if (!foundBeat) {
                // The user pressed the button early or the wrong button!
                gameUserFail(inputsButtons[offsetButton].color);
                // If there is a color at the bottom kill it
                if (nextBeat != NULL) {
                    if (nextBeat->location < SETTINGS_GOAL_SIZE) {
                        // Kill this beat.
                        nextBeat->reset();
                        nextBeat = NULL;
                    }
                }
            }
        }

        // Depending on what buttons are down flash the top of the pillar
        if (inputsButtons[offsetButton].isButtonDown()) {
            buttonWasDown = true;
            for (unsigned char offsetLED = SETTINGS_NUM_LEDS - SETTINGS_BUTTON_STATUS_SIZE; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
                leds[offsetLED] = inputsButtons[offsetButton].color;
            }
        }
    }

    if (!buttonWasDown) {
        for (unsigned char offsetLED = SETTINGS_NUM_LEDS - SETTINGS_BUTTON_STATUS_SIZE; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
            leds[offsetLED] = CRGB::Black;
        }
    }

    // Create a new beat as needed.
    static unsigned long lastCreation = 0;
    if (lastCreation <= millis() - creationSpeed) {
        lastCreation = millis();
        createbeats();
    }
}

void gameCountDown()
{
    UpdateScoreBoard(String(3));
    for (unsigned short offsetLED = 0; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
        leds[offsetLED] = CRGB::Red;
    }
    FastLED.show();
    delay(1000);

    UpdateScoreBoard(String(2));
    for (unsigned short offsetLED = 0; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
        leds[offsetLED] = CRGB::Blue;
    }
    FastLED.show();
    delay(1000);

    UpdateScoreBoard(String(1));
    for (unsigned short offsetLED = 0; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
        leds[offsetLED] = CRGB::Green;
    }
    FastLED.show();
    delay(1000);

    for (unsigned short offsetLED = 0; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
        leds[offsetLED] = CRGB::Black;
    }

    gameState = GAME_STATE_RUNNING;
}

void UpdateScoreBoard(String text)
{
    // Draw the tet.
    ledMatrix.clear();
    ledMatrix.setText(text);
    ledMatrix.drawText();

    for (unsigned char offsetLives = 0; offsetLives < gameLives; offsetLives++) {
        ledMatrix.setPixel(31 - offsetLives, 7);
    }

    ledMatrix.commit();
}

void loop()
{
    // delay(200); 
/*
    static unsigned long lastFrame = 0;
    Serial.println("");
    Serial.print(millis());
    Serial.print(" (" + String(millis() - lastFrame) + ") ");
    lastFrame = millis();
*/
    // Check the buttons
    for (int offsetButton = 0; offsetButton < BUTTON_MAX; offsetButton++) {
        inputsButtons[offsetButton].loop();
    }

    if (gameState == GAME_STATE_STARTUP) {
        gameStart();
    } else if (gameState == GAME_STATE_STARTING) {
        gameCountDown();
    } else if (gameState == GAME_STATE_RUNNING) {
        gameLoop();
    } else if (gameState == GAME_STATE_GAMEOVER) {
        gameOver();
    } else {
        reset();  
        Serial.println("Error: Unknown gameState = " + String(gameState));
    }

    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(1000 / SETTINGS_FRAMES_PER_SECOND);
}

// Demo mode
// ----------------------------------------------------------------------------

// Used for demo patterns
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbowWithGlitter, confetti, sinelon, juggle, bpm };
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
    Serial.println(String(millis()) + " FYI: gameState = Start | Pattern = " + String(gCurrentPatternNumber));
}

void gameStart()
{
    
    UpdateScoreBoard("Push me");

    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();

    // do some periodic updates
    EVERY_N_MILLISECONDS(10) { gHue += 3; } // slowly cycle the "base color" through the rainbow
    EVERY_N_SECONDS(30) { nextPattern(); } // change patterns periodically

    // If any button is pressed start the game.
    for (int offsetButton = 0; offsetButton < BUTTON_MAX; offsetButton++) {
        if (inputsButtons[offsetButton].hasButtonBeenPressed()) {
            // A button has been pressed. Start the game
            // Set all the LEDS to black
            for (unsigned short offsetLED = 0; offsetLED < SETTINGS_NUM_LEDS; offsetLED++) {
                leds[offsetLED] = CRGB::Black;
            }
            gameState = GAME_STATE_STARTING;
            return;
        }
    }
}

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
