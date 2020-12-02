#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <Arduino.h>

class MenuItem;
class Menu;
class MenuRenderer;
class MenuSystem;

class MenuItem {
  public:
    uint8_t *value; 
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t dynamicTextbufferSize = 0;
    int staticMenuTextIndex = 0;
    char* (*formatValue)(uint8_t, uint8_t);
    uint8_t (*updateValue)(uint8_t, bool operationType);
    uint8_t staticTextbufferSize = 0;
    bool isFocusable = 1;
    
  public:
    MenuItem(
      uint8_t &value, 
      uint8_t x, 
      uint8_t y, 
      uint8_t dynamicTextbufferSize,
      char* (*formatValue)(uint8_t, uint8_t), 
      uint8_t (*updateValue)(uint8_t, bool operationType),
      int staticMenuTextIndex = -1,
      uint8_t staticTextbufferSize = 0,
      bool isFocusable = 1
    );
};

class Menu {
  public:
    bool isSelectable;

  private:
    MenuItem **_itemsArray;
    uint8_t _itemsCount = 0;

  public:
    Menu(bool isSelectable = 1);

    void render(MenuRenderer const&renderer) const;
    void next();
    bool addItem(MenuItem* menuItem);

    uint8_t getItemsCount() const;
    MenuItem const* getItem(uint8_t i) const;
};

class MenuSystem {
  private:
    MenuRenderer const *_renderer;
    Menu **_menuArray;
    Menu *_activeMenu;
    uint8_t _menusNum = 0;
    bool _isEditingMode = 0;
    int _activeMenuNum = 0;
    int _activeFocusItemNum = 0;
    bool _activeFocusItemState = 0;
    unsigned long _activeFocusItemLastBlink = 0;
    unsigned long _lastPressInEditingMode = 0;

  public:
    MenuSystem(MenuRenderer const&renderer);

    void display();
    void blink();

    void nextScreen();
    void prevScreen();
    void setScreen(int screenNum);

    void resetFocus();
    void nextFocusItem();
    void changeActiveFocusItemValue(bool operationType);
    void resetEditingMode();
    void resetActiveFocusItem();

    bool isEditingMode();
    void toggleEditingMode();
    void setLastPressInEditingModeToCurrentMillis();
    unsigned long getLastPressInEditingMode();

    int getActiveMenuNum();

    bool addMenu(Menu* menu);

    void updateMenuItem(MenuItem *item);
};

class MenuRenderer {
  public:
    virtual void renderMenu(Menu const&menu) const = 0;
    virtual void renderMenuItem(MenuItem const&item) const = 0;
    virtual void blinkMenuItem(
      MenuItem const* item,
      bool itemState
    ) const = 0;
    virtual void updateMenuItem(MenuItem const&item) const = 0;
    virtual void clearScreenArea(MenuItem const&item) const = 0;
    virtual void clearScreen() const = 0;
};

#endif