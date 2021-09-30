/**
 * Homie Node for Max7219 8x8 Banners.
 *
 */
#include "LedBanner.hpp"

LedBanner::LedBanner(const char *id, const char *name, const char *cType, const MD_MAX72XX::moduleType_t ledHwType, const int dataPin, const int clkPin, const int csPin, const int maxDevices)
    : HomieNode(id, name, cType)
{
    _dataIn = dataPin;
    _clock = clkPin;
    _chipSelect = csPin;
    _devices = maxDevices;
    _ledHardwareType = ledHwType;
    snprintf(curMessage, sizeof(curMessage) - 3, "%s%s", "Welcome to Skoona.net, node: ", getName());

}

/**
 *
 */
void LedBanner::printCaption()
{
    Homie.getLogger() << getName() << ": " << getType() << endl;
}

/**
 * Handles the received MQTT messages from Homie.
 *
 */
bool LedBanner::handleInput(const HomieRange &range, const String &property, const String &value)
{
    Homie.getLogger() << "nodeInputHandler()  " << property << ": " << value << endl;

    if (property.equalsIgnoreCase(SKN_NODE_MESSAGE_PROPERTY_ID))
    {
        if (!newMessageAvailable)
        {
            snprintf(newMessage, sizeof(newMessage) - 3, "%s", value.c_str());
            newMessageAvailable = true;
        }
    }
    else if (property.equalsIgnoreCase(SKN_NODE_SPEED_PROPERTY_ID))
    {
        if (!newSpeedAvailable)
        {
            scrollSpeed = (uint16_t)value.toInt();
            snprintf(newMessage, sizeof(newMessage) - 3, "New Speed value is: %s", value.c_str());
            newSpeedAvailable = true;
        }
    }
    else if (property.equalsIgnoreCase(SKN_NODE_BRIGHTNESS_PROPERTY_ID))
    {
        if (!newBrightnessAvailable)
        {
            brightness = (uint8_t)value.toInt();
            snprintf(newMessage, sizeof(newMessage) - 3, "New Brightness value is: %s", value.c_str());
            newBrightnessAvailable = true;
        }
    }

    return true;
}

/**
 *
 */
void LedBanner::loop()
{
    if (Pmx.displayAnimate())
    {
        if (newMessageAvailable)
        {
            strcpy(curMessage, newMessage);
            setProperty(SKN_NODE_MESSAGE_PROPERTY_ID).send(newMessage);
            newMessageAvailable = false;
        }

        if (newSpeedAvailable)
        {
            Pmx.setSpeed(scrollSpeed);
            setProperty(SKN_NODE_SPEED_PROPERTY_ID).send(String(scrollSpeed));
            newSpeedAvailable = false;
        }

        if (newBrightnessAvailable)
        {
            Pmx.setIntensity(brightness);
            setProperty(SKN_NODE_BRIGHTNESS_PROPERTY_ID).send(String(brightness));
            newBrightnessAvailable = false;
        }

        Pmx.displayReset();
    }
}

/**
 *
 */
void LedBanner::setNewMessage(const char *pMsg) {
    snprintf(newMessage, sizeof(newMessage) - 3, "%s", pMsg);
    newMessageAvailable = true;
}

/**
 *
 */
void LedBanner::onReadyToOperate() { Serial.println("Led Node Ready"); }

/**
 * @brief 
 * 
 */
void LedBanner::displaySetupHandler()
{
    Pmx.begin();
    Pmx.setIntensity(brightness); //Intensity 0-15
    Pmx.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
    newMessageAvailable = false;
}

/**
 *
 */
void LedBanner::setup()
{
    printCaption();

    advertise(SKN_NODE_MESSAGE_PROPERTY_ID)
        .setName(SKN_NODE_MESSAGE_PROPERTY_NAME)
        .setDatatype("string")
        .setRetained(true)
        .settable();

    advertise(SKN_NODE_SPEED_PROPERTY_ID)
        .setName(SKN_NODE_SPEED_PROPERTY_NAME)
        .setDatatype("integer")
        .setRetained(true)
        .settable();

    advertise(SKN_NODE_BRIGHTNESS_PROPERTY_ID)
        .setName(SKN_NODE_BRIGHTNESS_PROPERTY_NAME)
        .setDatatype("integer")
        .setRetained(true)
        .settable();

    displaySetupHandler();
}
