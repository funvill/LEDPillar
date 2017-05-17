// LED Pillar
// https://github.com/funvill/LEDPillar
//
// ----------------------------------------------------------------------------
// Created by: Steven Smethurst
// Last updated: May 16, 2017
//
// ToDo: If they push the button and there is no cursor  there. is that a game over?

#include "FastLED.h"

FASTLED_USING_NAMESPACE

// Game Settings
// ----------------------------------------------------------------------------
static const unsigned char SETTINGS_NUM_LEDS = 30; // The number of LEDs in this strip.
static const unsigned char SETTINGS_MAX_CURSORS = 30;
#define SETTINGS_FRAMES_PER_SECOND 120

// Pins
static const unsigned char SETTING_PIN_LED_DATA = 12;
static const unsigned char SETTING_PIN_PLAYER_GREEN_BUTTON = 2;
static const unsigned char SETTING_PIN_PLAYER_BLUE_BUTTON = 3;
static const unsigned char SETTING_PIN_PLAYER_RED_BUTTON = 4;
static const unsigned int SETTING_SERIAL_BAUD_RATE = 9600;

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

class CCursors {
private:
    unsigned long lastMove;

public:
    bool isAlive;
    unsigned short location;

    unsigned short speed; // Lower is faster

    CRGB color;

    CCursors()
    {
        isAlive = false;
    }

    bool create()
    {
        if (isAlive) {
            // This cursor is alive. We can not create on it.
            return false;
        }

        lastMove = 0;
        speed = 200;
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
        // ToDo: Remove the old color
        leds[location] = color;
    }

    void move()
    {
        location--;
    }

    void loop()
    {
        if (!isAlive) {
            // Do nothing
            return;
        }

        // Check for dead cursors.
        if (location == 0) {
            isAlive = false;
            return;
        }

        if (lastMove < millis() - speed) {
            move();
        }

        draw();
    }
};

CCursors cursor[SETTINGS_MAX_CURSORS];

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

    reset();
}

void reset()
{
    gameScore = 0;
    creationSpeed = 1000 * 1;
    gameState = GAME_STATE_STARTUP;

    // Set all the LEDS to black
    for (unsigned short ledOffset = 0; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CRGB::Black;
    }
}

void createCursor()
{
    // Search thought the list of cursors and find one that we can create.
    for (int i = 0; i < SETTINGS_MAX_CURSORS; i++) {
        if (cursor[i].create()) {
            Serial.print(" FYI: Cursor was created at offset = " + String(i));
            return; // We were able to create a new cursor
        }
    }

    Serial.print(" Error: No more cursors to create.");
}

void gameStart()
{
    Serial.print(" | gameState = Start");

    // Set all the LEDS to black
    for (unsigned short ledOffset = 0; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CRGB::Black;
    }

    // Set up the goal
    leds[0] = CRGB::Yellow;
    leds[1] = CRGB::Yellow;
    leds[2] = CRGB::Yellow;

    // Set up the cursor
    leds[SETTINGS_NUM_LEDS - 1] = CRGB::Green;

    // If any button is pressed start the game.
    for (int i = 0; i < BUTTON_MAX; i++) {
        if (inputsButtons[i].isButtonDown()) {
            // A button has been pressed.
            gameState = GAME_STATE_RUNNING;
            createCursor();
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

    // Set all the LEDS to black
    for (unsigned short ledOffset = 0; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CRGB::Black;
    }

    // Set up the goal
    leds[0] = CRGB::Yellow;
    leds[1] = CRGB::Yellow;
    leds[2] = CRGB::Yellow;

    // Move the cursors.
    for (int currsorOffset = 0; currsorOffset < SETTINGS_MAX_CURSORS; currsorOffset++) {
        if (!cursor[currsorOffset].isAlive) {
            continue;
        }
        cursor[currsorOffset].loop();

        // Check to see if this is in the cursor offset area.
        if (cursor[currsorOffset].location < 3) {
            // Check to see if the corresponding button has been pressed.
            for (int buttonOffset = 0; buttonOffset < BUTTON_MAX; buttonOffset++) {
                if (inputsButtons[buttonOffset].color == cursor[currsorOffset].color) {
                    // We found the right button
                    if (inputsButtons[buttonOffset].isButtonDown()) {
                        // The right button and the right color has been pressed.
                        gameScore++;
                        cursor[currsorOffset].isAlive = false;
                        break;
                    }
                }
            }

            if (cursor[currsorOffset].location == 0) {
                // Game over
                Serial.print(" FYI: Game over currsorOffset = " + String(currsorOffset));
                cursor[currsorOffset].isAlive = false;
            }
        }
    }

    // Create new cursors if needed.
    EVERY_N_MILLISECONDS(creationSpeed) { createCursor(); } // change patterns periodically
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

    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(1000 / SETTINGS_FRAMES_PER_SECOND);
}