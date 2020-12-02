// CONSTANTS
// ----------------------------------
#define LEFT_BUTTON_PIN 5
#define CENTRAL_BUTTON_PIN 4
#define RIGHT_BUTTON_PIN 3

#define LONG_PRESS_TIME 2000
#define SHORT_PRESS_TIME 50

#define INIT_ADDR 1023  // backup cell address
#define INIT_KEY 50     // first launch key

#define UPDATE_INCREMENT 1
#define UPDATE_DECREMENT 0

#define STATIC_ELEMENTS_COUNT 5

#define POWER_RELAY_PIN 10
#define ROOM_RELAY_PIN 9
#define LIVING_ROOM_RELAY_PIN 8

#define MAIN_SCREEN_NUM 0
#define TIMERS_SCREEN_NUM 1
#define MANUAL_MODE_SCREEN_NUM 2

// #define TOGGLE_RELAY_TIME 10000

// LIBRARIES
// ----------------------------------
#include <Arduino.h>
#include <DS3231.h>
#include <EEPROM.h>
#include <microWire.h>
#include <microLiquidCrystal_I2C.h>

#include "Button.h"
#include "MenuSystem.h"

DS3231 rtc(SDA, SCL);
Time time;
LiquidCrystal_I2C lcd(0x27, 20, 4);

Button leftButton(LEFT_BUTTON_PIN);
Button centralButton(CENTRAL_BUTTON_PIN);
Button rightButton(RIGHT_BUTTON_PIN);

// GLOBAl VARIABLES
// ----------------------------------
const char room[6] PROGMEM = "Room:";
const char delimiter[2] PROGMEM = ":";
const char livingRoom[5] PROGMEM = "Zal:";
const char weekDay[6] PROGMEM = "Wekd:";
const char weekend[6] PROGMEM = "Wknd:";

const char* const staticMenuText[STATIC_ELEMENTS_COUNT] PROGMEM = {
  room, delimiter, livingRoom, weekDay, weekend
};

const char* daysOfTheWeekArr[7] = { "Mon", "Tues",  "Wed", "Thurs", "Fri", "Sat", "Sun" };

struct SettingsStruct{
  uint8_t hours = 0;
  uint8_t minutes = 0;

  uint8_t dayOfWeek = 5;

  uint8_t livingRoomState = 0;
  uint8_t roomState = 0;

  uint8_t wekdOnHours = 0;
  uint8_t wekdOnMinutes = 0;
  uint8_t wekdOffHours = 0;
  uint8_t wekdOffMinutes = 0;

  uint8_t wkndOnHours = 0;
  uint8_t wkndOnMinutes = 0;
  uint8_t wkndOffHours = 0;
  uint8_t wkndOffMinutes = 0;

  uint8_t livingRoomRelayState = 0;
  uint8_t roomRelayState = 0;

  bool isManualMode = 0;
} Settings;

bool toggleLivingRoomRelay = 0;
bool toggleRoomRelay = 0;
unsigned long toggleRelayTimer = 0;

// Functions declarations
char* getStaticMenuItemFromPGM(int staticMenuTextIndex, uint8_t bufferLength);
char* numToTimeFormat(uint8_t num, uint8_t bufferLength);
char* numToWeekFormate(uint8_t num, uint8_t bufferLength);
char* numToOnOffState(uint8_t num, uint8_t bufferLength);

uint8_t updateHours(uint8_t value, bool operationType);
uint8_t updateMinutes(uint8_t value, bool operationType);
uint8_t updateDayOfWeek(uint8_t value, bool operationType);
uint8_t updateState(uint8_t value, bool operationType);

void updateTime(Time const&time);
void handleLivingRoomRelay(Time const&time);
bool isCurrentTimeInInterval(
  int currentTimeSum, 
  int onTimeSum, 
  int offTimeSum
);

class MainMenuRenderer: public MenuRenderer {
  public:
    void renderMenu(Menu const&menu) const override {
      for(byte i = 0; i < menu.getItemsCount(); i++) {
        MenuItem const* item = menu.getItem(i);
        renderMenuItem(*item);
      }
    };

    void renderMenuItem(MenuItem const&item) const override {
      if(item.staticMenuTextIndex >= 0) {
        renderDynamicMenuItemWithText(item);
      } else {
        renderDynamicMenuItem(item);
      }
    };

    void renderDynamicMenuItem(MenuItem const&item) const {
      char *buffer = (*(item.formatValue))(*(item.value), item.dynamicTextbufferSize);

      lcd.setCursor(item.x, item.y);
      lcd.print(buffer);

      free(buffer);
    };

    void renderDynamicMenuItemWithText(MenuItem const&item) const {
      char *dynamicTextBuffer = (*(item.formatValue))(*(item.value), item.dynamicTextbufferSize);
      char *staticTextBuffer = getStaticMenuItemFromPGM(item.staticMenuTextIndex, item.staticTextbufferSize);
      
      lcd.setCursor(item.x, item.y);
      lcd.print(staticTextBuffer);
      
      lcd.setCursor(item.x + strlen(staticTextBuffer), item.y);
      lcd.print(dynamicTextBuffer);
      
      free(dynamicTextBuffer);
      free(staticTextBuffer);
    }

    void updateMenuItem(MenuItem const&item) const override {
      clearScreenArea(item);
      renderMenuItem(item);
    };
    
    void blinkMenuItem(
      MenuItem const* item,
      bool itemState
    ) const override {
      if (itemState) {
        updateMenuItem(*item);
      } else {
        clearScreenArea(*item);
      }
    };

    void clearScreenArea(MenuItem const&item) const override {
      char *empty = (char *) malloc(sizeof(char) * item.dynamicTextbufferSize);
      for (byte i = 0; i < item.dynamicTextbufferSize - 1; i++) {
        empty[i] = 32;
      };

      empty[item.dynamicTextbufferSize - 1] = '\0';

      if(item.staticMenuTextIndex >= 0) {
        lcd.setCursor(item.x + item.staticTextbufferSize - 1, item.y);
        lcd.print(empty);
      } else {
        lcd.setCursor(item.x, item.y);
        lcd.print(empty);
      }

      free(empty);
    }; 

    void clearScreen() const override {
      lcd.clear();
    }
};

MainMenuRenderer mainMenuRenderer;
MenuSystem menuSystem(mainMenuRenderer);

Menu mainScreen;
Menu timersScreen;
Menu manualModeScreen(0);

// TODO: EXTRACT NUMBERS TO CONSTANTS
MenuItem hours(Settings.hours, 0, 0, 3, numToTimeFormat, updateHours);
MenuItem minutes(Settings.minutes, 2, 0, 3, numToTimeFormat, updateMinutes, 1, 2);
MenuItem dayOfWeek(Settings.dayOfWeek, 11, 0, 6, numToWeekFormate, updateDayOfWeek);
MenuItem zalState(Settings.livingRoomState, 0, 1, 4, numToOnOffState, updateState, 2, 5, 0);
MenuItem roomState(Settings.roomState, 8, 1, 4, numToOnOffState, updateState, 0, 6, 0);

MenuItem wekdOnHours(Settings.wekdOnHours, 0, 0, 3, numToTimeFormat, updateHours, 3, 6);
MenuItem wekdOnMinutes(Settings.wekdOnMinutes, 7, 0, 3, numToTimeFormat, updateMinutes, 1, 2);
MenuItem wekdOffHours(Settings.wekdOffHours, 11, 0, 3, numToTimeFormat, updateHours);
MenuItem wekdOffMinutes(Settings.wekdOffMinutes, 13, 0, 3, numToTimeFormat, updateMinutes, 1, 2);

MenuItem wkndOnHours(Settings.wkndOnHours, 0, 1, 3, numToTimeFormat, updateHours, 4, 6);
MenuItem wkndOnMinutes(Settings.wkndOnMinutes, 7, 1, 3, numToTimeFormat, updateMinutes, 1, 2);
MenuItem wkndOffHours(Settings.wkndOffHours, 11, 1, 3, numToTimeFormat, updateHours);
MenuItem wkndOffMinutes(Settings.wkndOffMinutes, 13, 1, 3, numToTimeFormat, updateMinutes, 1, 2);

MenuItem selectableZalState(Settings.livingRoomState, 0, 0, 4, numToOnOffState, updateState, 2, 5);
MenuItem selectableRoomState(Settings.roomState, 0, 1, 4, numToOnOffState, updateState, 0, 6);

// FUNCTIONS
// ----------------------------------
char* getStaticMenuItemFromPGM(int staticMenuTextIndex, uint8_t bufferLength) {
  uint16_t ptr = pgm_read_word(&(staticMenuText[staticMenuTextIndex]));
  uint8_t i = 0;

  char *buffer = (char *) malloc(sizeof(char) * bufferLength);
  do {
    buffer[i] = (char)(pgm_read_byte(ptr++));
  } while (buffer[i++] != NULL);
  buffer[bufferLength - 1] = '\0';

  return buffer;
}

char* numToTimeFormat(uint8_t num, uint8_t bufferLength) {
  char *buffer = (char *) malloc(sizeof(char) * bufferLength);
  {
    String str = "";

    if(num < 10) {
      str += "0";
    }

    str += num;
    str.toCharArray(buffer, bufferLength);
  }
  buffer[bufferLength - 1] = '\0';

  return buffer;
}
char* numToWeekFormate(uint8_t num, uint8_t bufferLength) {
  char *buffer = (char *) malloc(sizeof(char) * bufferLength);
  const char* dOW = daysOfTheWeekArr[num];
  uint8_t i = 0;

  do
  {
    buffer[i] = dOW[i];
  } while (buffer[i++] != NULL);
  buffer[bufferLength - 1] = '\0';

  return buffer;
}
char* numToOnOffState(uint8_t num, uint8_t bufferLength) {
  char *buffer = (char *) malloc(sizeof(char) * bufferLength);
  
  if(num == 0) {
    buffer[0] = 'o';
    buffer[1] = 'f';
    buffer[2] = 'f';
    buffer[3] = '\0';
  } else if(num == 1) {
    buffer[0] = 'o';
    buffer[1] = 'n';
    buffer[2] = '\0';
  } else {
    buffer[0] = 'e';
    buffer[1] = 'r';
    buffer[2] = 'r';
    buffer[3] = '\0';
  }
 
  return buffer;
}

uint8_t updateHours(uint8_t value, bool operationType) {
  int updateValue = value;

  if(operationType == UPDATE_INCREMENT) {
    updateValue++;
  } else if(operationType == UPDATE_DECREMENT) {
    updateValue--;
  }

  if(updateValue > 23) {
    updateValue = 0;
  } else if (updateValue < 0) {
    updateValue = 23;
  }
  
  return updateValue;
};
uint8_t updateMinutes(uint8_t value, bool operationType) {
  int updateValue = value;

  if(operationType == UPDATE_INCREMENT) {
    updateValue++;
  } else if(operationType == UPDATE_DECREMENT) {
    updateValue--;
  }

  if(updateValue > 59) {
    updateValue = 0;
  } else if (updateValue < 0) {
    updateValue = 59;
  }
  
  return updateValue;
};
uint8_t updateDayOfWeek(uint8_t value, bool operationType) {
  int updateValue = value;

  if(operationType == UPDATE_INCREMENT) {
    updateValue++;
  } else if(operationType == UPDATE_DECREMENT) {
    updateValue--;
  }

  if(updateValue > 6) {
    updateValue = 0;
  } else if (updateValue < 0) {
    updateValue = 6;
  }
  
  return updateValue;
};
uint8_t updateState(uint8_t value, bool operationType) {
  int updateValue = value;

  if(operationType == UPDATE_INCREMENT) {
    updateValue++;
  } else if(operationType == UPDATE_DECREMENT) {
    updateValue--;
  }

  if(updateValue > 1) {
    updateValue = 0;
  } else if (updateValue < 0) {
    updateValue = 1;
  }
  
  return updateValue;
};

void clearEEPROM() {
  for (int i = 0; i < 1023; i++) {
    EEPROM.write(i, 0);
  }
}
void initializeSettings() {
  if (EEPROM.read(INIT_ADDR) != INIT_KEY) {
    clearEEPROM();

    rtc.setDOW(MONDAY);
    rtc.setTime(0, 0, 0);

    EEPROM.put(0, Settings);

    EEPROM.put(INIT_ADDR, INIT_KEY);
  }
  
  EEPROM.get(0, Settings);

  time = rtc.getTime();
  Settings.dayOfWeek = time.dow - 1;
  Settings.hours = time.hour;
  Settings.minutes = time.min;
};
void saveSettings() {
  EEPROM.put(0, Settings);
  rtc.setTime(Settings.hours, Settings.minutes, 0);
  rtc.setDOW(Settings.dayOfWeek + 1);
};

void updateTime(Time const&time) {
  if(time.sec == 0 && Settings.minutes != time.min) {
    Settings.minutes = time.min;

    if(menuSystem.getActiveMenuNum() == MAIN_SCREEN_NUM) {
      menuSystem.updateMenuItem(&minutes);
    }
  }
       
  if(time.sec == 0 && time.min == 0 && Settings.hours != time.hour) {
    Settings.hours = time.hour;
    Settings.dayOfWeek = time.dow - 1;

    if(menuSystem.getActiveMenuNum() == MAIN_SCREEN_NUM) {
      menuSystem.updateMenuItem(&hours);
      menuSystem.updateMenuItem(&dayOfWeek);
    }
  }
};

void handleLivingRoomRelay(Time const&time) {
  int onTimeSum;
  int offTimeSum;
  int currentTimeSum = time.hour * 60 + time.min;

  if(time.dow == 6 || time.dow == 7) {
    onTimeSum = Settings.wkndOnHours * 60 + Settings.wkndOnMinutes;
    offTimeSum = Settings.wkndOffHours * 60 + Settings.wkndOffMinutes;
  } else {
    onTimeSum = Settings.wekdOnHours * 60 + Settings.wekdOnMinutes;
    offTimeSum = Settings.wekdOffHours * 60 + Settings.wekdOffMinutes;
  }
  
  if(onTimeSum < offTimeSum) {
    if(
      Settings.livingRoomRelayState == 0 &&
      isCurrentTimeInInterval(
        currentTimeSum, 
        onTimeSum, 
        offTimeSum
      )
    ) {
      toggleLivingRoomRelay = 1;
    } else if(
      Settings.livingRoomRelayState == 1 &&
      !isCurrentTimeInInterval(
        currentTimeSum, 
        onTimeSum, 
        offTimeSum
      )
    ) {
      toggleLivingRoomRelay = 1;
    }
  } else if(onTimeSum > offTimeSum) {
    if(
      Settings.livingRoomRelayState == 0 &&
      !isCurrentTimeInInterval(
        currentTimeSum, 
        offTimeSum, 
        onTimeSum
      )
    ) {
      toggleLivingRoomRelay = 1;
    } else if(
      Settings.livingRoomRelayState == 1 &&
      isCurrentTimeInInterval(
        currentTimeSum, 
        offTimeSum, 
        onTimeSum
      )
    ) {
      toggleLivingRoomRelay = 1;
    }
  }
};

bool isCurrentTimeInInterval(int currentTimeSum, int onTimeSum, int offTimeSum) {
  if(
    currentTimeSum >= onTimeSum &&
    currentTimeSum < offTimeSum
  ) {
    return true;
  } else if(
    currentTimeSum < onTimeSum ||
    currentTimeSum >= offTimeSum
  ) {
    return false;
  }
};

void toggleRelays() {
  if(toggleLivingRoomRelay) {
      digitalWrite(LIVING_ROOM_RELAY_PIN, HIGH);
    } 
  if(toggleRoomRelay) {
    digitalWrite(ROOM_RELAY_PIN, HIGH);
  }

  if(toggleRelayTimer == 0) {
    toggleRelayTimer = millis();
  }

  if(millis() - toggleRelayTimer > 11000) {
    toggleRelayTimer = 0;

    if(toggleLivingRoomRelay) {
      digitalWrite(LIVING_ROOM_RELAY_PIN, LOW);
      toggleLivingRoomRelay = 0;

      Settings.livingRoomRelayState = !Settings.livingRoomRelayState;
      Settings.livingRoomState = Settings.livingRoomRelayState;

      menuSystem.updateMenuItem(&zalState);
    } 
    if(toggleRoomRelay) {
      digitalWrite(ROOM_RELAY_PIN, LOW);
      toggleRoomRelay = 0;

      Settings.roomRelayState = !Settings.roomRelayState;
      Settings.roomState = Settings.roomRelayState;
    }

    saveSettings();

    if(menuSystem.getActiveMenuNum() == MAIN_SCREEN_NUM) {
      menuSystem.updateMenuItem(&roomState);
      menuSystem.updateMenuItem(&zalState);
    }
  } else if(millis() - toggleRelayTimer > 10500) {
    digitalWrite(POWER_RELAY_PIN, LOW);
  } else if(millis() - toggleRelayTimer > 500) {
    digitalWrite(POWER_RELAY_PIN, HIGH);
  }
};

bool isToggleRelays() {
  return toggleLivingRoomRelay || toggleRoomRelay;
}

void setup() {
  Serial.begin(9600);

  rtc.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  initializeSettings();

  mainScreen.addItem(&hours);
  mainScreen.addItem(&minutes);
  mainScreen.addItem(&dayOfWeek);
  mainScreen.addItem(&zalState);
  mainScreen.addItem(&roomState);

  timersScreen.addItem(&wekdOnHours);
  timersScreen.addItem(&wekdOnMinutes);
  timersScreen.addItem(&wekdOffHours);
  timersScreen.addItem(&wekdOffMinutes);

  timersScreen.addItem(&wkndOnHours);
  timersScreen.addItem(&wkndOnMinutes);
  timersScreen.addItem(&wkndOffHours);
  timersScreen.addItem(&wkndOffMinutes);

  manualModeScreen.addItem(&selectableZalState);
  manualModeScreen.addItem(&selectableRoomState);

  menuSystem.addMenu(&mainScreen);
  menuSystem.addMenu(&timersScreen);
  menuSystem.addMenu(&manualModeScreen);

  if(Settings.isManualMode) {
    menuSystem.setScreen(MANUAL_MODE_SCREEN_NUM);
  }

  pinMode(POWER_RELAY_PIN, OUTPUT);
  pinMode(LIVING_ROOM_RELAY_PIN, OUTPUT);
  pinMode(ROOM_RELAY_PIN, OUTPUT);
}

// extern int __bss_end;
// extern void *__brkval;
// int memoryFree() {
//   int freeValue;
//   if ((int)__brkval == 0)
//     freeValue = ((int)&freeValue) - ((int)&__bss_end);
//   else
//     freeValue = ((int)&freeValue) - ((int)__brkval);
//   return freeValue;
// }

void loop() {
  // Serial.println(memoryFree());

  if(isToggleRelays()) {
    toggleRelays();
  }

  if(centralButton.isHeld()) {
    menuSystem.toggleEditingMode();
    menuSystem.setLastPressInEditingModeToCurrentMillis();
    
    if(!menuSystem.isEditingMode()) {
      menuSystem.resetEditingMode();
      saveSettings();
    }
  }

  if(
    !menuSystem.isEditingMode() && 
    (leftButton.isHeld() || 
    rightButton.isHeld())
  ) {
    Settings.isManualMode = !Settings.isManualMode;
    saveSettings();
    menuSystem.setScreen(
      Settings.isManualMode ? 
      MANUAL_MODE_SCREEN_NUM : 
      MAIN_SCREEN_NUM
    );
  }

  if(menuSystem.isEditingMode()) {
    if(centralButton.isClicked()) {
      menuSystem.nextFocusItem();
    } else if(leftButton.isClicked() || leftButton.isHeld()) {
      menuSystem.changeActiveFocusItemValue(UPDATE_DECREMENT);
    } else if(rightButton.isClicked() || rightButton.isHeld()) {
      menuSystem.changeActiveFocusItemValue(UPDATE_INCREMENT);
    } else {
      menuSystem.blink();
    }


    if(millis() - menuSystem.getLastPressInEditingMode() > 5000) {
      menuSystem.resetEditingMode();
      initializeSettings();
    }
  } else if(!menuSystem.isEditingMode() && !Settings.isManualMode) {
    if(leftButton.isClicked()) {
      menuSystem.prevScreen();
    }
    if(rightButton.isClicked()) {
      menuSystem.nextScreen();
    }
    
    time = rtc.getTime();
    updateTime(time);
    handleLivingRoomRelay(time);

    menuSystem.display();
  } else if(!menuSystem.isEditingMode() && Settings.isManualMode) {
    if(Settings.livingRoomRelayState != Settings.livingRoomState) {
      toggleLivingRoomRelay = 1;
    }

    if(Settings.roomRelayState != Settings.roomState) {
      toggleRoomRelay = 1;
    }

    menuSystem.display();
  }
}