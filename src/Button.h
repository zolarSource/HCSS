#ifndef BUTTON_H
#define BUTTON_H

class Button {
  public: Button(byte pin) {
      pinMode(pin, INPUT);
      buttonPin = pin;
    }

    bool isClicked() {
      tick();

      if (buttonClickFlag) {
        buttonClickFlag = 0;
        return 1;
      }

      return 0;
    };

    bool isHeld() {
      tick();

      if (buttonHoldFlag) {
        buttonHoldFlag = 0;
        return 1;
      }

      return 0;
    };

  private:
    void tick() {
      bool buttonState = !digitalRead(buttonPin);
      uint32_t currentMillis = millis();

      if (
        buttonState == 1
        && previousState == LOW
        && currentMillis - pressTime > SHORT_PRESS_TIME
        && isButtonClick == 0
      ) {
        previousState = HIGH;
        pressTime = currentMillis;
        isButtonClick = 1;
      }

      if (
        buttonState == 0
        && previousState == HIGH
        && isButtonClick == 1
      ) {
        previousState = LOW;
        buttonClickFlag = 1;
        pressTime = 0;
        isButtonClick = 0;
      }

      if (
        buttonState == 0
        && previousState == HIGH
        && isButtonHold == 1
      ) {
        previousState = LOW;
        isButtonHold = 0;
        pressTime = 0;
        holdTime = 0;
      }

      if (
        buttonState == 1
        && previousState == HIGH
        && currentMillis - pressTime > LONG_PRESS_TIME
        && currentMillis - holdTime > 1000
      ) {
        isButtonHold = 1;
        buttonHoldFlag = 1;
        holdTime = currentMillis;
        isButtonClick = 0;
        buttonClickFlag = 0;
      }
    }

    byte buttonPin;
    unsigned long pressTime = 0;
    unsigned long holdTime = 0;
    bool previousState = LOW;
    bool isButtonClick = 0;
    bool isButtonHold = 0;
    bool buttonClickFlag = 0;
    bool buttonHoldFlag = 0;
};

#endif