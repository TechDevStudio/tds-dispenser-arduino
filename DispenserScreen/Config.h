#ifndef CONFIG_H
#define CONFIG_H

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480

//pin definition
#define TFT_BL_PIN 2
#define RX_RFREADER_PIN 38  // Your RX pin
#define TX_RFREADER_PIN -1  // Not needed for RDM6300, set to -1

// Buffer size
#define DRAW_BUF_SIZE ((SCREEN_WIDTH * SCREEN_HEIGHT / 10) * (sizeof(uint16_t)))

// WiFi credentials
const char* WIFI_SSID = "DD-Simba";
const char* WIFI_PASSWORD = "zombie@1502";

// MQTT settings
const char* MQTT_BROKER = "192.168.220.105";
const int MQTT_PORT = 1883;

#endif