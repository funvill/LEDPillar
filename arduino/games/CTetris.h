#include "CButtons.h"
#include "FastLED.h"


static const unsigned short SETTING_GAME_OVER_FLASHING_SPEED = 300; 
static const unsigned short SETTING_GAME_OVER_RESET_TIMEOUT = 1000 * 60 ; 

static const unsigned short SETTING_CREATE_NEW_BLOCKS_SPEED = 500;
static const unsigned char SETTINGS_BLOCK_SIZE = 4; // Number of LEDs
static const unsigned long SETTING_BLOCK_FALL_SPEED = 70; // ms, lower numbers are faster. Starting speed.
static const unsigned char SETTINGS_MAX_BLOCKS = 10;
static const unsigned short SETTINGS_MAX_LED_COUNT = (30*5);

// Block Types
static const char BLOCK_TYPE_NONE = '_';
static const char BLOCK_TYPE_RED = 'R';
static const char BLOCK_TYPE_GREEN = 'G';
static const char BLOCK_TYPE_BLUE = 'B';

// CTetrisBlock
// ----------------------------------------------------------------------------

class CTetrisBlock {
public:
    char type; // _, R, G, B
    unsigned short location; // The location of the bottom led in this block

    void Reset();
};

// CTetris
// ----------------------------------------------------------------------------
class CTetris {
private:
    // Block stacks
    char blockStack[SETTINGS_MAX_LED_COUNT];
    unsigned short blockStackTop;

    // Container to hold potential blocks.
    CTetrisBlock blocks[SETTINGS_MAX_BLOCKS];

    unsigned short ledCount;
    CRGB *leds; 

    unsigned short gameScore;
    unsigned long gameOverTimer;

    void CheckAndDoGameInput(CButton& red, CButton& green, CButton& blue);
    void CheckAndMoveBlocks();
    void MoveBlock();
    bool CheckForGameOver();
    void Draw();
    void GameOver();
    void CreateNewBlock();
    void CheckAndCreateBlocks();
    char GetRandomBlockColor();

    const CRGB GetRGBForBlockType(char blockType);
    unsigned long GetBlockSpeed();

public:
    CTetris() { this->Reset(NULL, SETTINGS_MAX_LED_COUNT); };
    unsigned short GetGameScore()
    {
        return this->gameScore;
    };
    void Reset(CRGB *leds, unsigned short count);
    bool Loop(CButton& red, CButton& green, CButton& blue);
};

// --------------------------------------------------
