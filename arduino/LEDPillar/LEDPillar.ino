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
static const unsigned char SETTINGS_NUM_LEDS = 30; // The number of LEDs in this strip.
static const unsigned char SETTINGS_MAX_BEATS = 30;
static const unsigned char SETTINGS_FRAMES_PER_SECOND = 120;
static const unsigned char SETTINGS_GOAL_SIZE = 5;
static const unsigned char SETTING_BEAT_TAIL_LENGTH = 4; // The length of the fading tail. 

// Pins
static const unsigned char SETTING_PIN_LED_DATA = 6;
static const unsigned char SETTING_SCORE_BOARD_CS = 10;
static const unsigned char SETTING_PIN_PLAYER_GREEN_BUTTON = 2;
static const unsigned char SETTING_PIN_PLAYER_BLUE_BUTTON = 3;
static const unsigned char SETTING_PIN_PLAYER_RED_BUTTON = 4;
static const unsigned int SETTING_SERIAL_BAUD_RATE = 9600;

// Score display
static const unsigned char SETTING_SCORE_BOARD_DISPLAYS = 4;
LedMatrix ledMatrix = LedMatrix(SETTING_SCORE_BOARD_DISPLAYS, SETTING_SCORE_BOARD_CS);

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

    bool isButtonDown()
    {
        return currentState == BUTTON_DOWN;
    }

    void loop()
    {
        // Debounce the button.
        bool currentRead = digitalRead(this->pin);
        if (currentRead == lastRead) {
            currentState = BUTTON_UP;
            return;
        }
        lastRead = currentRead;

        if (currentRead == BUTTON_DOWN) {
            Serial.print(" FYI: Button was pressed, pin = " + String(pin));
        }
        currentState = currentRead;
    }
    void Init(unsigned int pin, CRGB color)
    {
        this->pin = pin;
        this->color = color;
        pinMode(this->pin, INPUT_PULLUP);
    }
};

static const unsigned char BUTTON_MAX = 3;
CButtons inputsButtons[BUTTON_MAX];

// Game states
static const unsigned char GAME_STATE_STARTUP = 0;
static const unsigned char GAME_STATE_RUNNING = 1;
static const unsigned char GAME_STATE_GAMEOVER = 2;
unsigned char gameState;

unsigned int gameScore;

//
unsigned short creationSpeed;

// The current status of all the LEDS
CRGB leds[SETTINGS_NUM_LEDS];

class CBeats {
private:
    unsigned long lastMove;

public:
    bool isAlive;
    unsigned short location;
    unsigned short speed; // Lower is faster
    CRGB color;


    Cbeatss()
    {
        reset();
    }

    void reset()
    {
        // This beat is dead 
        isAlive = false;

        // Change any of the tail of the LEDs to black
        for (int offsetLED = location; offsetLED < SETTINGS_NUM_LEDS && (offsetLED - location) < SETTING_BEAT_TAIL_LENGTH; offsetLED++) {
            leds[offsetLED] = CRGB::Black;
        }
    }

    bool create()
    {
        if (isAlive) {
            // This beats is alive. We can not create on it.
            return false;
        }

        lastMove = 0;
        speed = 50;
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
        // Move the beats.
        location--;
       
       // Create a tail of fading LEDs. 
        for (int offsetLED = location; offsetLED < SETTINGS_NUM_LEDS && (offsetLED - location) < SETTING_BEAT_TAIL_LENGTH; offsetLED++) {
            leds[offsetLED].fadeToBlackBy( 220 - ((offsetLED - location)*20) );
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

        // Check for dead beatss.
        if (location == 0) {
            reset();
            return;
        }

        if (lastMove < millis() - speed) {
            lastMove = millis() ; 
            move();
        }

        draw();
    }
};

CBeats beats[SETTINGS_MAX_BEATS];

void setup()
{
    // LEDs
    FastLED.addLeds<NEOPIXEL, SETTING_PIN_LED_DATA>(leds, SETTINGS_NUM_LEDS);

    // Buttons.
    inputsButtons[0].Init(SETTING_PIN_PLAYER_GREEN_BUTTON, CRGB::Green);
    inputsButtons[1].Init(SETTING_PIN_PLAYER_BLUE_BUTTON, CRGB::Blue);
    inputsButtons[2].Init(SETTING_PIN_PLAYER_RED_BUTTON, CRGB::Red);

    // initialize serial communications
    Serial.begin(SETTING_SERIAL_BAUD_RATE);

    // led Matrix
    ledMatrix.init();
    ledMatrix.setRotation(true);
    ledMatrix.setTextAlignment(TEXT_ALIGN_LEFT);
    ledMatrix.setCharWidth(8);
    ledMatrix.setText("0000");

    reset();
}

void reset()
{
    gameScore = 0;
    creationSpeed = 1000 * 5;
    gameState = GAME_STATE_STARTUP;

    // Set all the LEDS to black
    for (unsigned short ledOffset = 0; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CRGB::Black;
    }

    //
    for (unsigned short beatsOffset = 0; beatsOffset < SETTINGS_MAX_BEATS; beatsOffset++) {
        beats[beatsOffset].reset();
    }
}

void createbeats()
{
    // Search thought the list of beatss and find one that we can create.
    for (int i = 0; i < SETTINGS_MAX_BEATS; i++) {
        if (beats[i].create()) {
            Serial.print(" FYI: beats was created at offset = " + String(i));
            return; // We were able to create a new beats
        }
    }

    Serial.print(" Error: No more beatss to create.");
}

void drawGoal()
{
    // Set up the goal
    for (unsigned char ledOffset = 0; ledOffset < SETTINGS_GOAL_SIZE; ledOffset++) {
        leds[ledOffset] = CRGB::Yellow;
    }
}

void gameStart()
{
    Serial.print(" | gameState = Start");

    // Set all the LEDS to black
    for (unsigned short ledOffset = 0; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CRGB::Black;
    }

    drawGoal();

    // If any button is pressed start the game.
    for (int i = 0; i < BUTTON_MAX; i++) {
        if (inputsButtons[i].isButtonDown()) {
            // A button has been pressed.
            gameState = GAME_STATE_RUNNING;
            createbeats();
        }
    }
}

void gameOver()
{
    Serial.print(" | gameState = Over");
}

void gameLoop()
{
    Serial.print(" | gameState = Loop");

    drawGoal();

    // Move the beatss.
    for (int beatsOffset = 0; beatsOffset < SETTINGS_MAX_BEATS; beatsOffset++) {
        if (!beats[beatsOffset].isAlive) {
            continue;
        }
        beats[beatsOffset].loop();

        // Check to see if this is in the beats offset area.
        if (beats[beatsOffset].location < SETTINGS_GOAL_SIZE) {
            // Check to see if the corresponding button has been pressed.
            for (int buttonOffset = 0; buttonOffset < BUTTON_MAX; buttonOffset++) {
                if (inputsButtons[buttonOffset].color == beats[beatsOffset].color) {
                    // We found the right button
                    if (inputsButtons[buttonOffset].isButtonDown()) {
                        // The right button and the right color has been pressed.
                        gameScore++;
                        beats[beatsOffset].reset(); 
                        break;
                    }
                }
            }

            if (beats[beatsOffset].location == 0) {
                // Game over
                Serial.print(" FYI: Game over beatsOffset = " + String(beatsOffset));
                beats[beatsOffset].isAlive = false;
            }
        }
    }

    // Create new beatss if needed.
    EVERY_N_MILLISECONDS(creationSpeed) { createbeats(); } // change patterns periodically
}

void UpdateScoreBoard()
{
    ledMatrix.clear();
    ledMatrix.setText(String(gameScore));
    ledMatrix.drawText();
    ledMatrix.commit();
}

void loop()
{
    Serial.println("");
    Serial.print(millis());
    Serial.print(" | gameScore = " + String(gameScore));

    // Check the buttons
    for (int buttonOffset = 0; buttonOffset < BUTTON_MAX; buttonOffset++) {
        inputsButtons[buttonOffset].loop();
    }

    if (gameState == GAME_STATE_STARTUP) {
        gameStart();
    } else if (gameState == GAME_STATE_RUNNING) {
        gameLoop();
    } else if (gameState == GAME_STATE_GAMEOVER) {
        gameOver();
    } else {
        Serial.print(" Error: Unknown gameState = " + String(gameState));
    }

    // Update score board
    UpdateScoreBoard();

    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(1000 / SETTINGS_FRAMES_PER_SECOND);
}