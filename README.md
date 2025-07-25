# ESP32CYD-LVGL-Weather
Weather Station and clock for the ESP32 Cheap Yellow Display (CYD) ESP32-2432S028R

![Image](esp32-weather.jpg)

I bought one of the above units to have a bit of fun and came across the excellent guides from Random Nerd Tutorials.

This code is based on their tutorial "ESP32 CYD with LVGL: Weather Station (Description, Temperature, Humidity)" at: https://randomnerdtutorials.com/esp32-cyd-lvgl-weather-station/ with the following changes:

* More accurate update of lv_tick_inc.
* RGB LED blink added (can be disabled).
* Optimised weather elements lookups.
* Changed background colour.
* Added time display using NTP server.
* Added flashing cursor. Changes to + when NTP fetch in progress.
* Added date format in DD-MM-YYYY (configurable in code).
* Local timekeeping. Updated from NTP every hour.
* Wifi info shown on screen.
* Wifi first connect max 10 retries.
* WIll try to reconnect if wifi drops.

The code has also been tidied and optimised, with plenty of comments to explain the changes.

To use this code, follow the instructions in its comments or on the tutorials page mentioned above. Remember to set the user definable variables for wifi etc.
