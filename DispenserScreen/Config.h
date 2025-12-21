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

#define DEBUG false
#define TIMEOUT_NO_SELECTION 10000 //segundos de inactividad para terminar proceso
#define TIMEOUT_NO_WRISTBAND_RESPONSE 2000 //safe scan of wristband

// WiFi credentials
const char* WIFI_SSID = "DD-DSPNSR-NTWRK";
const char* WIFI_PASSWORD = "2glNhN600r7wLtnquS";

// MQTT settings
const char* MQTT_BROKER = "mqtt-disp.techdev-studio.com"; //"192.168.220.105";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "dispenser_user";
const char* MQTT_PASSWORD = "tds@dispenser";

#endif