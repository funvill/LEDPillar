#include "CButtons.h"
#include "FastLED.h"


static const unsigned short SETTINGS_BALL_MAX_LED_COUNT = (30*5);

class CBall
{
    public:

        unsigned short gameScore ;
        unsigned short ledCount;
        CRGB *leds;         

        CBall() { this->Reset(NULL, SETTINGS_BALL_MAX_LED_COUNT); };
        unsigned short GetGameScore()
        {
            return this->gameScore;
        };
        void Reset(CRGB *leds, unsigned short count);
        bool Loop(CButton& red, CButton& green, CButton& blue);        


};