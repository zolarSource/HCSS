#include "MenuSystem.h"

Menu::Menu(bool isSelectable = 1) {
  this->isSelectable = isSelectable;
};

bool Menu::addItem(MenuItem* menuItem) {
  MenuItem **tmp = (MenuItem**) realloc(_itemsArray, (_itemsCount + 1) * sizeof(MenuItem*));

  if(tmp == NULL) {
    free(tmp);
    return false;
  } else {
    _itemsArray = tmp;
    _itemsArray[_itemsCount++] = menuItem;
    return true;
  }
};

uint8_t Menu::getItemsCount() const {
  return _itemsCount;
};

MenuItem const* Menu::getItem(uint8_t i) const {
  return _itemsArray[i];
};

void Menu::render(MenuRenderer const&renderer) const {
  renderer.renderMenu(*this);
};

MenuItem::MenuItem(
  uint8_t &value, 
  uint8_t x, 
  uint8_t y, 
  uint8_t dynamicTextbufferSize, 
  char* (*formatValue)(uint8_t, uint8_t), 
  uint8_t (*updateValue)(uint8_t, bool operationType),
  int staticMenuTextIndex = -1,
  uint8_t staticTextbufferSize = 0,
  bool isFocusable = 1
) {
  this->value = &value;
  this->x = x;
  this->y = y;
  this->dynamicTextbufferSize = dynamicTextbufferSize;
  this->formatValue = formatValue;
  this->updateValue = updateValue;
  this->staticMenuTextIndex = staticMenuTextIndex;
  this->staticTextbufferSize = staticTextbufferSize;
  this->isFocusable = isFocusable;
};

MenuSystem::MenuSystem(MenuRenderer const&renderer) {
  _renderer = &renderer;
};

bool MenuSystem::addMenu(Menu* menu) {
  Menu **tmp = (Menu**) realloc(_menuArray, (_menusNum + 1) * sizeof(Menu*));

  if(tmp == NULL) {
    free(tmp);
    return false;
  } else {
    _menuArray = tmp;
    _menuArray[_menusNum++] = menu;
    _activeMenu = _menuArray[_activeMenuNum];
    return true;
  }
};

void MenuSystem::nextScreen() {
  _activeMenuNum++;

  if(_activeMenuNum > (_menusNum - 1)) {
    _activeMenuNum = 0;
  }
  
  if((!_menuArray[_activeMenuNum]->isSelectable)) {
    nextScreen();
  }
  
  _activeMenu = _menuArray[_activeMenuNum];
  _renderer->clearScreen();
};

void MenuSystem::prevScreen() {
  _activeMenuNum--;

  if(_activeMenuNum < 0) {
    _activeMenuNum = (_menusNum - 1);
  }

  if(_menuArray[_activeMenuNum]->isSelectable) {
    _activeMenu = _menuArray[_activeMenuNum];
    _renderer->clearScreen();
  } else {
    prevScreen();
  }
};

void MenuSystem::setScreen(int screenNum) {
  _activeMenuNum = screenNum;
  
  if(_activeMenuNum > (_menusNum - 1)) {
    _activeMenuNum = 0;
  } else if(_activeMenuNum < 0) {
    _activeMenuNum = (_menusNum - 1);
  }

  _activeMenu = _menuArray[_activeMenuNum];
  _renderer->clearScreen();
};

void MenuSystem::nextFocusItem() {
  resetActiveFocusItem();
  setLastPressInEditingModeToCurrentMillis();
  
  _renderer->renderMenuItem(*(_activeMenu->getItem(_activeFocusItemNum)));

  _activeFocusItemNum++;

  if(_activeFocusItemNum + 1 > _activeMenu->getItemsCount()) {
    _activeFocusItemNum = 0;
  }

  MenuItem const* item = _activeMenu->getItem(_activeFocusItemNum);
  if(item->isFocusable == 0) {
    nextFocusItem();
  }
};

void MenuSystem::changeActiveFocusItemValue(bool operationType) {
  resetActiveFocusItem();
  setLastPressInEditingModeToCurrentMillis();

  MenuItem const* item = _activeMenu->getItem(_activeFocusItemNum);
  *(item->value) = item->updateValue(*(item->value), operationType);
};

void MenuSystem::display() {
  _renderer->renderMenu(*_activeMenu);
};

void MenuSystem::updateMenuItem(MenuItem *item) {
  _renderer->updateMenuItem(*item);
};

bool MenuSystem::isEditingMode() {
  return _isEditingMode;
};

void MenuSystem::toggleEditingMode() {
  _isEditingMode = !_isEditingMode;
}

void MenuSystem::setLastPressInEditingModeToCurrentMillis() {
  _lastPressInEditingMode = millis();
}

unsigned long MenuSystem::getLastPressInEditingMode() {
  return _lastPressInEditingMode;
};

int MenuSystem::getActiveMenuNum() {
  return _activeMenuNum;
}; 

void MenuSystem::blink() {
  if (millis() - _activeFocusItemLastBlink > 500) {
    _activeFocusItemState = !_activeFocusItemState;
    _activeFocusItemLastBlink = millis();
    
    _renderer->blinkMenuItem(_activeMenu->getItem(_activeFocusItemNum), _activeFocusItemState);
  }
};

void MenuSystem::resetEditingMode() {
  _isEditingMode = 0;
  _activeFocusItemNum = 0;
  _lastPressInEditingMode = 0;

  _renderer->clearScreen();
};

void MenuSystem::resetActiveFocusItem() {
  _activeFocusItemState = 0;
  _activeFocusItemLastBlink = 0;
};