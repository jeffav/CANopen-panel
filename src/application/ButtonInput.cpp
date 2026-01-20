/**
 * @file ButtonInput.cpp
 * @brief Button input handler implementation
 */

#include "ButtonInput.hpp"
#include <cstring>

/* ButtonInput Implementation */

ButtonInput::ButtonInput(uint8_t buttonIndex, uint32_t debounceTimeMs)
    : buttonIndex_(buttonIndex),
      state_(ButtonState::RELEASED),
      debounceTimeMs_(debounceTimeMs),
      stateChangeTimeMs_(0),
      pressEdge_(false),
      releaseEdge_(false),
      lastRawState_(false)
{
}

void ButtonInput::init()
{
    state_ = ButtonState::RELEASED;
    stateChangeTimeMs_ = 0;
    pressEdge_ = false;
    releaseEdge_ = false;
    lastRawState_ = false;
}

bool ButtonInput::update(bool rawState, uint32_t currentTimeMs)
{
    bool stateChanged = false;

    /* Calculate time since last state change */
    uint32_t timeInState = currentTimeMs - stateChangeTimeMs_;

    /* State machine for debouncing */
    switch (state_) {
        case ButtonState::RELEASED:
            if (rawState && !lastRawState_) {
                /* Button pressed - start debouncing */
                state_ = ButtonState::DEBOUNCING;
                stateChangeTimeMs_ = currentTimeMs;
            }
            break;

        case ButtonState::DEBOUNCING:
            if (!rawState) {
                /* Button released before debounce - false trigger */
                state_ = ButtonState::RELEASED;
                stateChangeTimeMs_ = currentTimeMs;
            } else if (timeInState >= debounceTimeMs_) {
                /* Debounce time elapsed - button is pressed */
                state_ = ButtonState::PRESSED;
                stateChangeTimeMs_ = currentTimeMs;
                pressEdge_ = true;
                stateChanged = true;
            }
            break;

        case ButtonState::PRESSED:
            if (!rawState) {
                /* Button released - start debouncing release */
                state_ = ButtonState::RELEASING;
                stateChangeTimeMs_ = currentTimeMs;
            }
            break;

        case ButtonState::RELEASING:
            if (rawState) {
                /* Button pressed again before debounce - still pressed */
                state_ = ButtonState::PRESSED;
                stateChangeTimeMs_ = currentTimeMs;
            } else if (timeInState >= debounceTimeMs_) {
                /* Debounce time elapsed - button is released */
                state_ = ButtonState::RELEASED;
                stateChangeTimeMs_ = currentTimeMs;
                releaseEdge_ = true;
                stateChanged = true;
            }
            break;
    }

    lastRawState_ = rawState;
    return stateChanged;
}

bool ButtonInput::isPressedEdge()
{
    bool edge = pressEdge_;
    pressEdge_ = false; /* Clear edge flag */
    return edge;
}

bool ButtonInput::isReleasedEdge()
{
    bool edge = releaseEdge_;
    releaseEdge_ = false; /* Clear edge flag */
    return edge;
}

uint8_t ButtonInput::getBitMask() const
{
    if (state_ == ButtonState::PRESSED) {
        return (1U << buttonIndex_);
    }
    return 0;
}

void ButtonInput::reset()
{
    state_ = ButtonState::RELEASED;
    stateChangeTimeMs_ = 0;
    pressEdge_ = false;
    releaseEdge_ = false;
    lastRawState_ = false;
}

/* ButtonArray Implementation */

ButtonArray::ButtonArray(uint8_t numButtons, uint32_t debounceTimeMs)
    : buttons_(nullptr),
      numButtons_(numButtons),
      debounceTimeMs_(debounceTimeMs)
{
    /* Allocate array of button pointers */
    buttons_ = new ButtonInput*[numButtons_];

    /* Create button instances */
    for (uint8_t i = 0; i < numButtons_; i++) {
        buttons_[i] = new ButtonInput(i, debounceTimeMs_);
    }
}

ButtonArray::~ButtonArray()
{
    /* Delete button instances */
    if (buttons_) {
        for (uint8_t i = 0; i < numButtons_; i++) {
            delete buttons_[i];
        }
        delete[] buttons_;
    }
}

void ButtonArray::init()
{
    for (uint8_t i = 0; i < numButtons_; i++) {
        buttons_[i]->init();
    }
}

bool ButtonArray::updateAll(const bool* rawStates, uint32_t currentTimeMs)
{
    bool anyChanged = false;

    for (uint8_t i = 0; i < numButtons_; i++) {
        if (buttons_[i]->update(rawStates[i], currentTimeMs)) {
            anyChanged = true;
        }
    }

    return anyChanged;
}

uint8_t ButtonArray::getButtonMask() const
{
    uint8_t mask = 0;

    for (uint8_t i = 0; i < numButtons_; i++) {
        mask |= buttons_[i]->getBitMask();
    }

    return mask;
}

ButtonInput* ButtonArray::getButton(uint8_t index)
{
    if (index < numButtons_) {
        return buttons_[index];
    }
    return nullptr;
}
