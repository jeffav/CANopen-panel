/**
 * @file LampOutput.cpp
 * @brief Lamp output controller implementation
 */

#include "LampOutput.hpp"

/* LampOutput Implementation */

LampOutput::LampOutput(uint8_t lampIndex)
    : lampIndex_(lampIndex),
      mode_(LampMode::OFF),
      brightness_(255),
      outputState_(false),
      blinkOnTimeMs_(500),
      blinkOffTimeMs_(500),
      lastBlinkToggleMs_(0),
      blinkPhase_(false)
{
}

void LampOutput::init()
{
    mode_ = LampMode::OFF;
    brightness_ = 255;
    outputState_ = false;
    blinkPhase_ = false;
    lastBlinkToggleMs_ = 0;
}

void LampOutput::update(uint32_t currentTimeMs)
{
    /* Handle blinking mode */
    if (mode_ == LampMode::BLINK) {
        uint32_t elapsed = currentTimeMs - lastBlinkToggleMs_;
        uint32_t threshold = blinkPhase_ ? blinkOnTimeMs_ : blinkOffTimeMs_;

        if (elapsed >= threshold) {
            /* Toggle blink phase */
            blinkPhase_ = !blinkPhase_;
            outputState_ = blinkPhase_;
            lastBlinkToggleMs_ = currentTimeMs;
        }
    }
}

void LampOutput::turnOn(uint8_t brightness)
{
    brightness_ = brightness;

    if (brightness == 255) {
        mode_ = LampMode::ON;
        outputState_ = true;
    } else if (brightness > 0) {
        mode_ = LampMode::PWM;
        outputState_ = true;
    } else {
        turnOff();
    }
}

void LampOutput::turnOff()
{
    mode_ = LampMode::OFF;
    outputState_ = false;
}

void LampOutput::setState(bool state)
{
    if (state) {
        turnOn(brightness_);
    } else {
        turnOff();
    }
}

void LampOutput::setBrightness(uint8_t brightness)
{
    brightness_ = brightness;

    if (mode_ == LampMode::ON || mode_ == LampMode::PWM) {
        if (brightness == 255) {
            mode_ = LampMode::ON;
            outputState_ = true;
        } else if (brightness > 0) {
            mode_ = LampMode::PWM;
            outputState_ = true;
        } else {
            turnOff();
        }
    }
}

void LampOutput::startBlinking(uint32_t onTimeMs, uint32_t offTimeMs)
{
    mode_ = LampMode::BLINK;
    blinkOnTimeMs_ = onTimeMs;
    blinkOffTimeMs_ = offTimeMs;
    blinkPhase_ = true;
    outputState_ = true;
    lastBlinkToggleMs_ = 0; /* Will be set on first update */
}

void LampOutput::stopBlinking()
{
    if (mode_ == LampMode::BLINK) {
        /* Return to ON state with current brightness */
        if (brightness_ > 0) {
            turnOn(brightness_);
        } else {
            turnOff();
        }
    }
}

/* LampArray Implementation */

LampArray::LampArray(uint8_t numLamps)
    : lamps_(nullptr),
      numLamps_(numLamps)
{
    /* Allocate array of lamp pointers */
    lamps_ = new LampOutput*[numLamps_];

    /* Create lamp instances */
    for (uint8_t i = 0; i < numLamps_; i++) {
        lamps_[i] = new LampOutput(i);
    }
}

LampArray::~LampArray()
{
    /* Delete lamp instances */
    if (lamps_) {
        for (uint8_t i = 0; i < numLamps_; i++) {
            delete lamps_[i];
        }
        delete[] lamps_;
    }
}

void LampArray::init()
{
    for (uint8_t i = 0; i < numLamps_; i++) {
        lamps_[i]->init();
    }
}

void LampArray::updateAll(uint32_t currentTimeMs)
{
    for (uint8_t i = 0; i < numLamps_; i++) {
        lamps_[i]->update(currentTimeMs);
    }
}

void LampArray::setFromMask(uint8_t lampMask)
{
    for (uint8_t i = 0; i < numLamps_; i++) {
        bool state = (lampMask & (1U << i)) != 0;
        lamps_[i]->setState(state);
    }
}

uint8_t LampArray::getOutputMask() const
{
    uint8_t mask = 0;

    for (uint8_t i = 0; i < numLamps_; i++) {
        if (lamps_[i]->getOutputState()) {
            mask |= (1U << i);
        }
    }

    return mask;
}

LampOutput* LampArray::getLamp(uint8_t index)
{
    if (index < numLamps_) {
        return lamps_[index];
    }
    return nullptr;
}
