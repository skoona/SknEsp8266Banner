// ESP8266 DevKit V1 source
#include <Arduino.h>

// Use the MD_MAX72XX library to scroll text on the display
// received through the ESP8266 WiFi interface.
//
// Demonstrates the use of the callback function to control what
// is scrolled on the display text. User can enter text through
// a mqtt message and this will display as a scrolling message on
// the display.
//

#include <Homie.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define SKN_MOD_NAME "ESP8266 Max7219 Banner"
#define SKN_MOD_VERSION "1.0.4"
#define SKN_MOD_BRAND "SknSensors"

#define SKN_NODE_TITLE "Message Banner"
#define SKN_NODE_TYPE "8x8x8 LED"
#define SKN_NODE_ID "LEDBanner"

#define SKN_NODE_MESSAGE_PROPERTY_NAME "Banner Text"
#define SKN_NODE_MESSAGE_PROPERTY_ID "message"

#define SKN_NODE_SPEED_PROPERTY_NAME "Scrolling Speed"
#define SKN_NODE_SPEED_PROPERTY_ID "speed"

#define SKN_NODE_BRIGHTNESS_PROPERTY_NAME "Display Brightness"
#define SKN_NODE_BRIGHTNESS_PROPERTY_ID "brightness"

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN  D5  // or SCK
#define DATA_PIN D7  // or MOSI
#define CS_PIN   D8  // or SS

// SPI hardware interface
MD_Parola Pmx = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

uint8_t scrollSpeed = 50; // default frame delay value  85 too slow, 25 too fast
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000; // in milliseconds
uint8_t brightness = 6;

const uint16_t MESG_SIZE = 512;
char curMessage[MESG_SIZE] = {""};
char newMessage[MESG_SIZE] = {""};
bool newMessageAvailable = false;
bool newSpeedAvailable = false;
bool newBrightnessAvailable = false;

bool broadcastHandler(const String &level, const String &value)
{
  Homie.getLogger() << "Received broadcast level " << level << ": " << value << endl;

  // if (!newMessageAvailable) {
  snprintf(newMessage, sizeof(newMessage) - 3, "%s: %s", level.c_str(), value.c_str());
  newMessageAvailable = true;
  // }

  return true;
}

bool nodeInputHandler(const HomieRange &range, const String &property, const String &value)
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

HomieSetting<long> cfgBrightness("brightness", "The intensity of the leds from 0 thru 15.");
HomieSetting<long> cfgSpeed("speed", "The scrolling speed in milliseconds.");

HomieNode banner(SKN_NODE_ID, SKN_NODE_TITLE, SKN_NODE_TYPE, false, 0, 0, nodeInputHandler);

void displaySetupHandler()
  {
    // Display initialization
    brightness = (uint8_t)cfgBrightness.get();
    scrollSpeed = (uint8_t)cfgSpeed.get();

    Pmx.begin();
    Pmx.setIntensity(brightness); //Intensity 0-15
    Pmx.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
    newMessageAvailable = false;
  }

void displayLoopHandler()
  {
    if (Pmx.displayAnimate())
    {
      if (newMessageAvailable)
      {
        strcpy(curMessage, newMessage);
        banner.setProperty(SKN_NODE_MESSAGE_PROPERTY_ID).send(newMessage);
        newMessageAvailable = false;
      }

      if (newSpeedAvailable)
      {
        Pmx.setSpeed(scrollSpeed);
        banner.setProperty(SKN_NODE_SPEED_PROPERTY_ID).send(String(scrollSpeed));
        newSpeedAvailable = false;
      }

      if (newBrightnessAvailable)
      {
        Pmx.setIntensity(brightness);
        banner.setProperty(SKN_NODE_BRIGHTNESS_PROPERTY_ID).send(String(brightness));
        newBrightnessAvailable = false;
      }

      Pmx.displayReset();
    }
  }

void setup() {
  Serial.begin(115200);

  // Set up first message as the IP address
  snprintf(curMessage, sizeof(curMessage) - 3, "%s", "Welcome to Skoona.net");
  Serial.printf("%s%s", "\nAssigned IP ", curMessage);

  Homie_setFirmware(SKN_MOD_NAME, SKN_MOD_VERSION);
  Homie_setBrand(SKN_MOD_BRAND);

  cfgBrightness
      .setDefaultValue(brightness)
      .setValidator([](long candidate)
                    { return candidate > 0 && candidate < 15; });

  cfgSpeed
      .setDefaultValue(scrollSpeed)
      .setValidator([](long candidate)
                    { return candidate > 0 && candidate < 150; });

  Homie.setBroadcastHandler(broadcastHandler)
      .setLedPin(LED_BUILTIN, LOW)
      .disableResetTrigger()
      .setSetupFunction(displaySetupHandler)
      .setLoopFunction(displayLoopHandler);

  banner.advertise(SKN_NODE_MESSAGE_PROPERTY_ID)
      .setName(SKN_NODE_MESSAGE_PROPERTY_NAME)
      .setDatatype("string")
      .setRetained(true)
      .settable();

  banner.advertise(SKN_NODE_SPEED_PROPERTY_ID)
      .setName(SKN_NODE_SPEED_PROPERTY_NAME)
      .setDatatype("integer")
      .setRetained(true)
      .settable();

  banner.advertise(SKN_NODE_BRIGHTNESS_PROPERTY_ID)
      .setName(SKN_NODE_BRIGHTNESS_PROPERTY_NAME)
      .setDatatype("integer")
      .setRetained(true)
      .settable();

  Homie.setup();
}

void loop() {
  Homie.loop();    
}
