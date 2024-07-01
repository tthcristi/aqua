# Aqua - Humidifier for cheese caves
As cheese caves are a rare sight on the market and an expensive one, this project can turn your fridge into one.
The ideea is simple, one sensor that triggers the piezo humidifier which creates water vapors, increasing the humidity. This is done periodically as the device will deep sleep.
Through the screen and one button you can adjust the sleep duration (5-180m), humidifier active time (5-60s) and the targeted humidity (60-95%).

## Components
- ESP32 development board with 30 pins (Can be replaced by any device wich supports Arduino framework)
- SSD1306 OLED screen (128*64 screen wich is great for a minimal interface)
- Ultrasonic Piezoelectric Humidifier (Powered by 5V, can turn water into vapors!)
- SHT45 humidity sensor (Can be replaced by any SHT sensor)
- 2N2222A transistor (To control the piezo module)
- A simple button

## Diagram
![aqua drawio](https://github.com/tthcristi/aqua/assets/41587818/9e760300-1915-4d9d-958b-c9a72f60aad1)
