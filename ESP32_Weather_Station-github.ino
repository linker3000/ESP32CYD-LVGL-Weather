/*  Linker3000:
 *   
 *   Published under the same terms as the original code - see below. For fun and 
 *   experimentation only. Supplied 'as is' with no warranty or support.
 *     
 *   Inspired by https://randomnerdtutorials.com/esp32-cyd-lvgl-weather-station/
 *
 * - More accurate update of lv_tick_inc.
 * - RGB LED blink added (can be disabled).
 * - Optimised weather elements lookups.
 * - Changed background colour.
 * - Added time display using NTP server.
 *   Wifi info shown on screen.
 * - Added flashing cursor. Changes to + when NTP fetch in progress.
 * - Added date format in DD-MM-YYYY (configurable in code).
 * - Local timekeeping. Updated from NTP every hour.
 * - Wifi first connect max 10 retries.
 * - Will try to reconnect if wifi drops.
 *
 *  Rui Santos & Sara Santos - Random Nerd Tutorials -
 https://RandomNerdTutorials.com/esp32-cyd-lvgl-weather-station/   |
 https://RandomNerdTutorials.com/esp32-tft-lvgl-weather-station/ THIS EXAMPLE
 WAS TESTED WITH THE FOLLOWING HARDWARE: 1) ESP32-2432S028R 2.8 inch 240×320
 also known as the Cheap Yellow Display (CYD):
 https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/ SET UP
 INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/ 2) REGULAR ESP32 Dev
 Board + 2.8 inch 240x320 TFT Display:
 https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and
 https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/ SET UP
 INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/ Permission is
 hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files. The above copyright notice and this
 permission notice shall be included in all copies or substantial portions of
 the Software.
*/

/*  Install the "lvgl" library version 9.X by kisvegabor to interface with the
   TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work
   with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO
   USE THE EXAMPLES FROM RANDOM NERD TUTORIALS *** FULL INSTRUCTIONS AVAILABLE
   ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or
   https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display -
   https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work
   with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER
   TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS *** FULL INSTRUCTIONS
   AVAILABLE ON HOW CONFIGURE THE LIBRARY:
   https://RandomNerdTutorials.com/cyd-lvgl/ or
   https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>

#include "weather_images.h"

// ******************************************
// Most commonly changeable settings are here
// ******************************************

// Network credentials and other settings
const char*  ssid      = "** YOUR SSID **";
const char*  password  = "** YOUR WIFI PASSWORD **";
const String latitude  = "0.00000";
const String longitude = "-0.0000";
const String location  = "** YOUR LOCATION FOR THE DISPLAY **";
const String timezone  = "Europe/London";

// Set to 0 for temperature in fahrenheit
#define TEMP_CELSIUS 1

// L3K for getting NTP time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;            // e.g. 0 for GMT, 3600 for GMT+1
const int daylightOffset_sec = 3600;     // e.g. 3600 for daylight saving time

const long interval =
    3600000;  // How often to fetch time from NTP. 3600000 miliseconds = 1 hour

// Set to false if you don't want the LED to flash
static bool flash_led = false;

const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 320;

// ******************************************

unsigned long currentMillis = millis();  // For counting elapsed time
unsigned long previousMillis =
    -999999;  // Initial value triggers an immediate time update
char timeBuffer[6];  // Format: HH:MM

// RGB LED pin definitions
const int LED_RED_PIN = 4;
const int LED_GREEN_PIN = 16;
const int LED_BLUE_PIN = 17;

// LED state variables
bool ledState = false;
bool wifiConnected = false;
bool weatherApiSuccess = false;

// L3K Table-driven weather descriptions...
typedef struct {
  int code;
  const char *description;
  const void *image;
} WeatherInfo;

/*
  WMO Weather interpretation codes (WW)- Code  Description
  0 Clear sky
  1, 2, 3 Mainly clear, partly cloudy, and overcast
  45, 48  Fog and depositing rime fog
  51, 53, 55  Drizzle: Light, moderate, and dense intensity
  56, 57  Freezing Drizzle: Light and dense intensity
  61, 63, 65  Rain: Slight, moderate and heavy intensity
  66, 67  Freezing Rain: Light and heavy intensity
  71, 73, 75  Snow fall: Slight, moderate, and heavy intensity
  77  Snow grains
  80, 81, 82  Rain showers: Slight, moderate, and violent
  85, 86  Snow showers slight and heavy
  95 *  Thunderstorm: Slight or moderate
  96, 99 *  Thunderstorm with slight and heavy hail
*/
const WeatherInfo weather_table[] = {
  {2,  "PARTLY CLOUDY",                &image_weather_cloud},
  {3,  "OVERCAST",                     &image_weather_cloud},
  {45, "FOG",                          &image_weather_cloud},
  {48, "DEPOSITING RIME FOG",          &image_weather_cloud},
  {51, "DRIZZLE LIGHT INTENSITY",      &image_weather_rain},
  {53, "DRIZZLE MODERATE INTENSITY",   &image_weather_rain},
  {55, "DRIZZLE DENSE INTENSITY",      &image_weather_rain},
  {56, "FREEZING DRIZZLE LIGHT",       &image_weather_rain},
  {57, "FREEZING DRIZZLE DENSE",       &image_weather_rain},
  {61, "RAIN SLIGHT INTENSITY",        &image_weather_rain},
  {63, "RAIN MODERATE INTENSITY",      &image_weather_rain},
  {65, "RAIN HEAVY INTENSITY",         &image_weather_rain},
  {66, "FREEZING RAIN LIGHT INTENSITY",&image_weather_rain},
  {67, "FREEZING RAIN HEAVY INTENSITY",&image_weather_rain},
  {71, "SNOW FALL SLIGHT INTENSITY",   &image_weather_snow},
  {73, "SNOW FALL MODERATE INTENSITY", &image_weather_snow},
  {75, "SNOW FALL HEAVY INTENSITY",    &image_weather_snow},
  {77, "SNOW GRAINS",                  &image_weather_snow},
  {80, "RAIN SHOWERS SLIGHT",          &image_weather_rain},
  {81, "RAIN SHOWERS MODERATE",        &image_weather_rain},
  {82, "RAIN SHOWERS VIOLENT",         &image_weather_rain},
  {85, "SNOW SHOWERS SLIGHT",          &image_weather_snow},
  {86, "SNOW SHOWERS HEAVY",           &image_weather_snow},
  {95, "THUNDERSTORM",                 &image_weather_thunder},
  {96, "THUNDERSTORM SLIGHT HAIL",     &image_weather_thunder},
  {99, "THUNDERSTORM HEAVY HAIL",      &image_weather_thunder}
};

// L3K For checking wifi connection status
// Now a local var String ssidLabelText;
String wifiIP;

int currentHour = 0;
int currentMinute = 0;
int previousMinute = 99;
struct tm timeinfo;

static bool last_flash_state = false;

// Store weather data and date & time weather fetched
String current_date;
String last_weather_update;
String temperature;
String humidity;
int is_day;
int weather_code = 0;
String weather_description;

#if TEMP_CELSIUS
String temperature_unit = "";
const char degree_symbol[] = "\u00B0C";
#else
String temperature_unit = "&temperature_unit=fahrenheit";
const char degree_symbol[] = "\u00B0F";
#endif

constexpr int DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8));
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// RGB LED Functions
void initRGBLED() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  
  // Turn off all LEDs initially (inverted logic)
  digitalWrite(LED_RED_PIN, HIGH);
  digitalWrite(LED_GREEN_PIN, HIGH);
  digitalWrite(LED_BLUE_PIN, HIGH);
}

void setRGBLED(bool red, bool green, bool blue) {
  if (!flash_led) return;
  digitalWrite(LED_RED_PIN, red ? LOW : HIGH);
  digitalWrite(LED_GREEN_PIN, green ? LOW : HIGH);
  digitalWrite(LED_BLUE_PIN, blue ? LOW : HIGH);
}

void updateRGBLED() {
  // Check overall system status
  bool systemOK = wifiConnected && weatherApiSuccess;
  
  if (systemOK) {
    // System OK - blink green in sync with seconds dot
    if (ledState) {
      setRGBLED(false, true, false);  // Green on
    } else {
      setRGBLED(false, false, false); // All off
    }
  } else {
    // System has issues - blink red
    if (ledState) {
      setRGBLED(true, false, false);  // Red on
    } else {
      setRGBLED(false, false, false); // All off
    }
  }
}

// If logging is enabled, it will inform the user about what is happening in the
// library
void log_print(lv_log_level_t level, const char* buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}
// L3K
static lv_obj_t* text_label_curtime;
static lv_obj_t* text_label_secdot;
static lv_obj_t* text_label_SSID;
static lv_obj_t* text_label_IP;

static lv_obj_t* weather_image;
static lv_obj_t* text_label_date;
static lv_obj_t* text_label_temperature;
static lv_obj_t* text_label_humidity;
static lv_obj_t* text_label_weather_description;
static lv_obj_t* text_label_time_location;

static void timer_cb(lv_timer_t* timer) {
  LV_UNUSED(timer);
  get_weather_data();
  get_weather_description(weather_code);
  lv_label_set_text(text_label_date, current_date.c_str());
  lv_label_set_text(text_label_temperature,
                    String("      " + temperature + degree_symbol).c_str());
  lv_label_set_text(text_label_humidity,
                    String("   " + humidity + "% RH").c_str());
  lv_label_set_text(text_label_weather_description,
                    weather_description.c_str());
  lv_label_set_text(
      text_label_time_location,
      String("Last Update: " + last_weather_update + "  |  " + location)
          .c_str());
}

void lv_create_main_gui(void) {
  LV_IMAGE_DECLARE(image_weather_sun);
  LV_IMAGE_DECLARE(image_weather_cloud);
  LV_IMAGE_DECLARE(image_weather_rain);
  LV_IMAGE_DECLARE(image_weather_thunder);
  LV_IMAGE_DECLARE(image_weather_snow);
  LV_IMAGE_DECLARE(image_weather_night);
  LV_IMAGE_DECLARE(image_weather_temperature);
  LV_IMAGE_DECLARE(image_weather_humidity);

  // Get the weather data from open-meteo.com API
  get_weather_data();

  weather_image = lv_image_create(lv_screen_active());
  lv_obj_align(weather_image, LV_ALIGN_CENTER, -80, -20);

  get_weather_description(weather_code);

  // L3K Change the active screen's background color
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x003a67), LV_PART_MAIN);

  // L3K Connected IP
  text_label_IP = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_IP, "Not connected");
  lv_obj_align(text_label_IP, LV_ALIGN_TOP_LEFT, 5, 3);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_IP, &lv_font_montserrat_12,
                             0);
  lv_obj_set_style_text_color((lv_obj_t*)text_label_IP,
                              lv_color_black(), 0);

  // L3K Connected SSID
  text_label_SSID = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_SSID, "Not connected");
  lv_obj_align(text_label_SSID, LV_ALIGN_TOP_LEFT, 5, 17);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_SSID, &lv_font_montserrat_12,
                             0);
  lv_obj_set_style_text_color((lv_obj_t*)text_label_SSID,
                              lv_color_black(), 0);

  // L3K Flashing seconds dot
  text_label_secdot = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_secdot, ".");
  lv_obj_align(text_label_secdot, LV_ALIGN_CENTER, 115, -100);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_secdot,
                             &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color((lv_obj_t*)text_label_secdot,
                              lv_palette_main(LV_PALETTE_TEAL), 0);

  // L3K Time
  text_label_curtime = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_curtime, "00:00");
  lv_obj_align(text_label_curtime, LV_ALIGN_CENTER, 70, -100);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_curtime,
                             &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color((lv_obj_t*)text_label_curtime,
                              lv_palette_main(LV_PALETTE_TEAL), 0);

  text_label_date = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_date, current_date.c_str());
  lv_obj_align(text_label_date, LV_ALIGN_CENTER, 70, -70);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_date, &lv_font_montserrat_12,
                             0);
  lv_obj_set_style_text_color((lv_obj_t*)text_label_date,
                              lv_palette_main(LV_PALETTE_TEAL), 0);

  lv_obj_t* weather_image_temperature = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_temperature, &image_weather_temperature);
  lv_obj_align(weather_image_temperature, LV_ALIGN_CENTER, 30, -25);
  text_label_temperature = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_temperature,
                    String("      " + temperature + degree_symbol).c_str());
  lv_obj_align(text_label_temperature, LV_ALIGN_CENTER, 70, -25);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_temperature,
                             &lv_font_montserrat_22, 0);

  lv_obj_t* weather_image_humidity = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_humidity, &image_weather_humidity);
  lv_obj_align(weather_image_humidity, LV_ALIGN_CENTER, 30, 20);
  text_label_humidity = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_humidity,
                    String("   " + humidity + "% RH").c_str());
  lv_obj_align(text_label_humidity, LV_ALIGN_CENTER, 90, 20);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_humidity,
                             &lv_font_montserrat_22, 0);

  text_label_weather_description = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_weather_description,
                    weather_description.c_str());
  lv_obj_align(text_label_weather_description, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_weather_description,
                             &lv_font_montserrat_18, 0);

  // Create a text label for the time and timezone aligned center in the bottom
  // of the screen
  text_label_time_location = lv_label_create(lv_screen_active());
  lv_label_set_text(
      text_label_time_location,
      String("Last Update: " + last_weather_update + "  |  " + location)
          .c_str());
  lv_obj_align(text_label_time_location, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_text_font((lv_obj_t*)text_label_time_location,
                             &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color((lv_obj_t*)text_label_time_location,
                              lv_palette_main(LV_PALETTE_GREY), 0);

  lv_timer_t* timer = lv_timer_create(timer_cb, 600000, NULL);
  lv_timer_ready(timer);
}

void updateNetInfo() {
  String ssidLabelText;  
  if (WiFi.isConnected()) {
    String ssid = WiFi.SSID();
    if (ssid.length() > 15) {
        ssid = ssid.substring(0, 12) + "...";
    }
    ssidLabelText = "SSID: " + ssid;
    wifiIP = "IP Addr: " + WiFi.localIP().toString();
    wifiConnected = true;
  } else {
    ssidLabelText = "WiFi not connected";
    wifiIP = "IP Addr: x.x.x.x";
    wifiConnected = false;
  }

  lv_label_set_text(text_label_SSID, ssidLabelText.c_str());
  lv_label_set_text(text_label_IP, wifiIP.c_str());
}

void updateTimeFromNTP() {
  lv_label_set_text(text_label_secdot, "+");  // Visual indicator
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    strcpy(timeBuffer, "--:--");
    return;
  }
  Serial.println(&timeinfo, "Current time: %A, %d %B %Y %H:%M:%S");

  currentHour = timeinfo.tm_hour;
  currentMinute = timeinfo.tm_min;

  // Format time for display
  sprintf(timeBuffer, "%02d:%02d", currentHour, currentMinute);
}

void get_weather_description(int code) {
  const void *image = NULL;

  // Handle special codes 0 and 1 separately (day/night logic)
  if (code == 0 || code == 1) {
    image = is_day ? (const void *)&image_weather_sun : (const void *)&image_weather_night;
    weather_description = (code == 0) ? "CLEAR SKY" : "MAINLY CLEAR";
  } else {
    int found = 0;
    for (int i = 0; i < sizeof(weather_table) / sizeof(weather_table[0]); i++) {
      if (weather_table[i].code == code) {
        weather_description = weather_table[i].description;
        image = weather_table[i].image;
        found = 1;
        break;
      }
    }

    if (!found) {
      weather_description = "UNKNOWN WEATHER CODE";
      image = &image_weather_cloud;  // fallback or NULL
    }
  }

  if (image != NULL) {
    lv_image_set_src(weather_image, image);
  }
}


void connectToWiFi(const char* ssid, const char* password) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected to WiFi.");
    wifiConnected = true;
    return;
  }

  WiFi.begin(ssid, password);
  Serial.print("🔌 Connecting to WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n Connected to WiFi!");
    Serial.print("📡 IP address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\n Failed to connect to WiFi after 10 attempts.");
    wifiConnected = false;
    // Optional: trigger fallback or retry logic
  }
}

void get_weather_data() {
  weatherApiSuccess = false;  // Reset flag
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Construct the API endpoint
    String url =
        String("http://api.open-meteo.com/v1/forecast?latitude=" + latitude +
               "&longitude=" + longitude +
               "&current=temperature_2m,relative_humidity_2m,is_day,"
               "precipitation,rain,weather_code" +
               temperature_unit + "&timezone=" + timezone + "&forecast_days=1");
    http.begin(url);
    int httpCode = http.GET();  // Make the GET request

    if (httpCode > 0) {
      // Check for the response
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Serial.println("Request information:");
        // Serial.println(payload);
        //  Parse the JSON to extract the time
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          const char* datetime = doc["current"]["time"];
          temperature = String(doc["current"]["temperature_2m"]);
          humidity = String(doc["current"]["relative_humidity_2m"]);
          is_day = String(doc["current"]["is_day"]).toInt();
          weather_code = String(doc["current"]["weather_code"]).toInt();
          /*Serial.println(temperature);
          Serial.println(humidity);
          Serial.println(is_day);
          Serial.println(weather_code);
          Serial.println(String(timezone));*/

          // Split the datetime into date and time
          String datetime_str = String(datetime);
          int splitIndex = datetime_str.indexOf('T');

          // L3K UK date format:
          current_date = datetime_str.substring(8, 10) + "-" +
                         datetime_str.substring(5, 7) + "-" +
                         datetime_str.substring(0, 4);

          // Or use this for US format current_date = datetime_str.substring(0,
          // splitIndex);
          last_weather_update = datetime_str.substring(
              splitIndex + 1, splitIndex + 9);  // Extract time portion
          
          weatherApiSuccess = true;  // API call succeeded
          Serial.println("Weather data updated successfully");
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          weatherApiSuccess = false;
        }
      } else {
        Serial.println("Weather API HTTP request failed");
        weatherApiSuccess = false;
      }
    } else {
      Serial.printf("GET request failed, error: %s\n",
                    http.errorToString(httpCode).c_str());
      weatherApiSuccess = false;
    }
    http.end();  // Close connection
  } else {
    Serial.println("Not connected to Wi-Fi - cannot get weather data");
    weatherApiSuccess = false;
  }
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() +
                        "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Initialize RGB LED
  initRGBLED();
  Serial.println("🔴RGB LED initialized");

  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Create a display object
  lv_display_t* disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf,
                            sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  // Function to draw the GUI
  lv_create_main_gui();

   // Connect to Wi-Fi
  connectToWiFi(ssid, password);

  updateNetInfo();  // Connected SSID & IP

  // L3K Init NTP time for the first time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  updateTimeFromNTP();
}

void loop() {
  static unsigned long last_tick = millis();
  unsigned long now = millis();

  // Calculate elapsed time since last tick
  uint32_t elapsed = now - last_tick;
  last_tick = now;

  // Inform LVGL of actual elapsed time
  lv_tick_inc(elapsed);
  lv_task_handler();

  // L3K Seconds dot and LED sync
  bool flash = (now / 1000) % 2 == 0;

  if (flash != last_flash_state) {
    lv_label_set_text(text_label_secdot, flash ? "." : " ");
    ledState = flash;
    updateRGBLED();
    last_flash_state = flash;
  }

  // L3K Time sync logic
  currentMillis = now;
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    updateTimeFromNTP();
  } else {
    if (getLocalTime(&timeinfo)) {
      currentHour = timeinfo.tm_hour;
      currentMinute = timeinfo.tm_min;
      sprintf(timeBuffer, "%02d:%02d", currentHour, currentMinute);
    } else {
      strcpy(timeBuffer, "--:--");
    }

    if (previousMinute != currentMinute) {
      previousMinute = currentMinute;
      lv_label_set_text(text_label_curtime, timeBuffer);
      if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi(ssid, password);
      }
      updateNetInfo();
    }
  }

  delay(5);  // Give other tasks breathing room
}
