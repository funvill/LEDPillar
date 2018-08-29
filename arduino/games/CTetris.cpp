#include "CTetris.h"

void CTetrisBlock::Reset()
{
    this->type = BLOCK_TYPE_NONE;
    this->location = 0;
}

void CTetris::Reset(CRGB *leds, unsigned short ledCount)
{
    Serial.println("Tetris:Reset ledCount=" + String(this->ledCount) );

    this->leds = leds; 
    this->ledCount = ledCount ; 
    if( this->ledCount > SETTINGS_MAX_LED_COUNT) {
        this->ledCount = SETTINGS_MAX_LED_COUNT ; 
    }

    // Reset block stack
    memset(this->blockStack, BLOCK_TYPE_NONE, this->ledCount);
    this->blockStackTop = 0; // No blocks in the stack

    // Reset blocks
    for (unsigned char blocksOffset = 0; blocksOffset < SETTINGS_MAX_BLOCKS; blocksOffset++) {
        this->blocks[blocksOffset].Reset();
    }

    // Reset the score
    this->gameScore = 0;
    this->gameOverTimer = 0;
}

bool CTetris::Loop(CButton& red, CButton& green, CButton& blue)
{
    // Do buttonloop
    red.loop();
    green.loop();
    blue.loop();

    if (this->CheckForGameOver()) {
        // Game over ! :(
        // Check to see if this is the first time we have entered this function since starting
        if (this->gameOverTimer == 0) {
            // First time. Set a timeout counter to exit the game over sequence.
            gameOverTimer = millis() + SETTING_GAME_OVER_RESET_TIMEOUT;
        }

        // Check for reset
        if (gameOverTimer < millis() || (red.isButtonDown() && green.isButtonDown() && blue.isButtonDown())) {
            // All buttons pressed. Reset
            this->Reset(this->leds, this->ledCount);
            this->Draw();
            return false;
        }

        this->GameOver();
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
            if( this->leds != NULL ) {
                for (unsigned short offset = 0; offset < this->ledCount; offset++) {
                    this->leds[offset] = CRGB::Black;
                }
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
            for( unsigned short moveOffset = 0 ; moveOffset < SETTINGS_BLOCK_SIZE ; moveOffset++ ) {
                this->MoveBlock();
            }
            CreateNewBlock();
            return;
        }
    }

    // There are no blocks on the stack yet.
    // Check to see there is a block that is currently falling and if it matches
    // Find offset for the lowest block
    unsigned char lowestBlockOffset = SETTINGS_MAX_BLOCKS;
    unsigned short lowersBlockLocation = this->ledCount;
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
            for( unsigned short moveOffset = 0 ; moveOffset < SETTINGS_BLOCK_SIZE ; moveOffset++ ) {
                this->MoveBlock();
            }
            CreateNewBlock();
            
            return;
        }
    }
}

char CTetris::GetRandomBlockColor()
{
    static const char newTypeBlock[3] = { BLOCK_TYPE_RED, BLOCK_TYPE_GREEN, BLOCK_TYPE_BLUE };
    return newTypeBlock[random(0, 3)];
}

void CTetris::CreateNewBlock()
{
    
    // We don't want two blocks ontop of each other.
    // First check to see if there is already an active block at the top.
    char highestBlockType = BLOCK_TYPE_NONE;
    unsigned short highestBlockLocation = 0;
    for (unsigned char offsetBlock = 0; offsetBlock < SETTINGS_MAX_BLOCKS; offsetBlock++) {
        if (this->blocks[offsetBlock].type == BLOCK_TYPE_NONE) {
            continue; // Skip
        }

        if (this->blocks[offsetBlock].location + SETTINGS_BLOCK_SIZE > this->ledCount - SETTINGS_BLOCK_SIZE) {
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
        this->blocks[offsetBlock].location = this->ledCount;

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
        Serial.println("CreateNewBlock type=" + String(this->blocks[offsetBlock].type) + "ledCount=" + String(this->ledCount) +", blockSize="+ String(SETTINGS_BLOCK_SIZE) );
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
            for (unsigned short offset = 0; offset < SETTINGS_BLOCK_SIZE; offset++) {
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
    if (this->blockStackTop > this->ledCount) {
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
    if( this->leds == NULL ) {
        return ; 
    }

    for (unsigned short offset = 0; offset < this->ledCount; offset++) {
        if (offset <= this->blockStackTop) {
            // Print the blocks in the stack
            // Serial.print(String(this->blockStack[offset]));
            this->leds[offset] = GetRGBForBlockType(this->blockStack[offset]);
        } else {
            // Empty
            // Serial.print(String(BLOCK_TYPE_NONE));
            this->leds[offset] = GetRGBForBlockType(BLOCK_TYPE_NONE);
        }

        for (unsigned char offsetBlock = 0; offsetBlock < SETTINGS_MAX_BLOCKS; offsetBlock++) {
            if (this->blocks[offsetBlock].type == BLOCK_TYPE_NONE) {
                continue; // Skip
            }

            if (offset > this->blocks[offsetBlock].location && offset <= this->blocks[offsetBlock].location + SETTINGS_BLOCK_SIZE) {
                // Print the falling block
                // Serial.print(String(this->currentBlockType));
                this->leds[offset] = GetRGBForBlockType(this->blocks[offsetBlock].type);
            }
        }
    }
    // Serial.print("\n");

    // Print the falling block.
}