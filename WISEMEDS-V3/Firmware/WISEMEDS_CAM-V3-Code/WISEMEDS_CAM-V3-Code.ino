/*
==================================================
WISEMEDS V2
ESP32-CAM Firmware
Live Camera Streaming
==================================================
*/
#include "espnow_manager.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "logger.h"
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

/**************************************************
 * WiFi
 **************************************************/

const char* WIFI_SSID = WIFI_NAME;
const char* WIFI_PASSWORD = WIFI_PASS;

/**************************************************
 * Web Server
 **************************************************/

WebServer server(80);

/**************************************************
 * Camera Status
 **************************************************/

bool cameraReady = false;
bool streamEnabled = true;

/**************************************************
 * Forward Declarations
 **************************************************/

bool initializeCamera();

void startCameraServer();

void handleStream();

void handleCapture();

void handleStatus();

void handleRestart();
bool initializeCamera()
{
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;

    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk = XCLK_GPIO_NUM;

    config.pin_pclk = PCLK_GPIO_NUM;

    config.pin_vsync = VSYNC_GPIO_NUM;

    config.pin_href = HREF_GPIO_NUM;

    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;

    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_VGA;

    config.jpeg_quality = 12;

    config.fb_count = 2;

    if(esp_camera_init(&config)!=ESP_OK)
    {
        Serial.println("Camera Failed");

        return false;
    }

    cameraReady = true;

    return true;
}
void setup()
{
    Serial.begin(115200);

    connectWiFi();

    networkMonitorBegin();

    healthBegin();

    espnowBegin();

    cameraBegin();

    discoveryBegin();

    startCameraServer();

    printSystemInfo();

    ledBegin();

    statisticsBegin()

    apiBegin();
}
    if (!initializeCamera())
    {
        Serial.println("Camera Initialization Failed");

        while (true)
            delay(1000);
    }

    startCameraServer();

    Serial.println();
    Serial.println("======================");
    Serial.println("WISEMEDS Camera Ready");
    Serial.println("======================");

    Serial.print("Home: http://");
    Serial.println(getIP());

    Serial.print("Stream: http://");
    Serial.print(getIP());
    Serial.println("/stream");

    Serial.print("Capture: http://");
    Serial.print(getIP());
    Serial.println("/capture");
}

void loop()
{
    cameraLoop();

    storageBegin();

    timeBegin();

    loadCameraConfig();

    networkMonitorLoop();

    healthLoop();

    espnowLoop();

    server.handleClient();

    discoveryLoop();

    apiLoop();

    memoryLoop();
}
    firebaseCameraLoop();

    watchdogLoop();

    server.handleClient();

    delay(1);
}