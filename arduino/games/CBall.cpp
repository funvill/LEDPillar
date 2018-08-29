#include "CBall.h"

void CBall::Reset(CRGB *leds, unsigned short count) {
    Serial.println("CBall:Reset ledCount=" + String(count) );

    this->leds = leds; 
    this->ledCount = ledCount ; 
    if( this->ledCount > SETTINGS_BALL_MAX_LED_COUNT) {
        this->ledCount = SETTINGS_BALL_MAX_LED_COUNT ; 
    }

}
bool CBall::Loop(CButton& red, CButton& green, CButton& blue) {
    return false; 
}