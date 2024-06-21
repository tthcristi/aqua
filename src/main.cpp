#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>
#include <SHTSensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_PIN 15
#define PIEZO_PIN 26

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SHTSensor sht(SHTSensor::SHT4X);
Preferences preferences;

bool holdDone = false;
bool buttonPressed = false;
bool editMode = false;

unsigned long buttonPressedTime = 0;
unsigned long refreshTime = 0;
unsigned long editModeTime = 0;
unsigned long sleepTime = 0;
unsigned long piezoTime = 0;

const unsigned int HOLD_DURATION = 1000;
const unsigned int REFRESH_RATE = 500;
const unsigned int EDIT_MODE_TIMEOUT = 10 * 1000;
const unsigned int SLEEP_TIMEOUT = 61 * 1000;

unsigned int desiredRH;
unsigned int intervalCheck;
unsigned int piezoActiveTime;

unsigned int editOption = 0;

// Displays formated text on screen
void displayPrintf(const char *format, ...)
{
    char buffer[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(buffer);
    display.display();
}

// Sends the device into deep sleep
void deep_sleep()
{
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(BUTTON_PIN), 0);
    esp_sleep_enable_timer_wakeup(intervalCheck * 60 * 1000000);
    Serial.println("Going to deep sleep now...");
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    digitalWrite(PIEZO_PIN, LOW);
    esp_deep_sleep_start();
}

// Called upon wake up reasons
void wake_up()
{
    display.ssd1306_command(SSD1306_DISPLAYON);
}

void setup()
{
    // Start serial
    Serial.begin(115200);
    delay(1000);

    // Check wakeup reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
        Serial.println("Woke up from deep sleep due to button press");
        wake_up();
    }
    else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        Serial.println("Woke up from deep sleep due to timer");
        wake_up();
    }
    else
    {
        Serial.println("Not a deep sleep wakeup");
    }

    // Read memory
    preferences.begin("aqua", false);
    desiredRH = preferences.getInt("desiredRH", 80);
    intervalCheck = preferences.getInt("intervalCheck", 20);
    piezoActiveTime = preferences.getInt("piezoActiveTime", 10);

    // Init timeouts
    sleepTime = millis();
    piezoTime = millis();
    refreshTime = millis();

    // Init screen
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    // Init sensor
    sht.init();

    // Init pins
    pinMode(PIEZO_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(PIEZO_PIN, LOW);
}

void loop()
{
    // Things should not be checked at every moment
    if (millis() - refreshTime >= REFRESH_RATE)
    {
        refreshTime = millis();

        if (millis() - sleepTime >= SLEEP_TIMEOUT)
            deep_sleep();

        // Check for edit mode
        if (!editMode)
        {
            // Read sensor and act accordingly
            if (sht.readSample())
            {
                char loading[11];
                strcpy(loading, "----------\0");
                if (millis() - piezoTime <= piezoActiveTime * 1000)
                {
                    if (sht.getHumidity() < desiredRH)
                    {
                        digitalWrite(PIEZO_PIN, HIGH);
                        loading[(refreshTime / 500) % 10] = ' ';
                    }
                }
                else
                    digitalWrite(PIEZO_PIN, LOW);
                displayPrintf("\nRH: %.1f%%\n T: %.1f%cC\n%s", sht.getHumidity(), sht.getTemperature(), (char)247, loading);
            }
            else
            {
                displayPrintf("\n-SNSR-ERR-");
            }
        }
        else
        {
            // Display edit menu
            switch (editOption)
            {
            case 0:
                displayPrintf("  >RH:%d%%\nFREQ: %.1dm\nSPRY: %ds\n---edit---", desiredRH, intervalCheck, piezoActiveTime);
                break;
            case 1:
                displayPrintf("  RH: %d%%\n>FREQ:%.1dm\nSPRY: %ds\n---edit---", desiredRH, intervalCheck, piezoActiveTime);
                break;
            case 2:
                displayPrintf("  RH: %d%%\nFREQ: %.1dm\n>SPRY:%ds\n---edit---", desiredRH, intervalCheck, piezoActiveTime);
                break;
            default:
                break;
            }
        }
    }

    // Only buttons should be permanently checked
    if (digitalRead(BUTTON_PIN) == LOW)
    {
        sleepTime = millis();
        editModeTime = millis();
        
        // Check if the button is hold
        if (!buttonPressed)
        {
            buttonPressed = true;
            buttonPressedTime = millis();
        }
        else if (!holdDone && millis() - buttonPressedTime >= HOLD_DURATION)
        {
            Serial.print("Hold\n");
            holdDone = true;
            if (editMode)
            {
                editOption = (editOption + 1) % 3;
            }
            else
            {
                editMode = true;
            }
        }
        delay(100);
    }
    else
    {
        // Edit mode timeout
        if (editMode && millis() - editModeTime >= EDIT_MODE_TIMEOUT)
        {
            // Reset timeouts and save settings
            editMode = false;
            piezoTime = millis();
            preferences.putInt("desiredRH", desiredRH);
            preferences.putInt("intervalCheck", intervalCheck);
            preferences.putInt("piezoActiveTime", piezoActiveTime);
        }

        if (buttonPressed)
        {
            // Holding might still be possible
            if (!holdDone)
            {
                Serial.print("Press\n");
                // Check for edit mode when pressing the button
                if (editMode)
                {
                    switch (editOption)
                    {
                    case 0:
                        desiredRH = desiredRH >= 95 ? 60 : desiredRH + 1;
                        break;
                    case 1:
                        intervalCheck = intervalCheck >= 180 ? 5 : intervalCheck + 5;
                        break;
                    case 2:
                        piezoActiveTime = piezoActiveTime >= 60 ? 5 : piezoActiveTime + 5;
                        break;
                    default:
                        break;
                    }
                }
            }
            buttonPressed = false;
            holdDone = false;
        }
    }
}