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
