/*****************************************************************
 * WISEMEDS V2
 * Smart IoT Medication Dispensing System
 *
 * Main Firmware
 *
 * ESP32
 * Arduino IDE 2.3.7
 *
 * Author: WISEMEDS
 *****************************************************************/
#include "espnow_manager.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <FS.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "pins.h"
#include "config.h"
#include "wifi_manager.h"
#include "rtc_manager.h"
#include "medication.h"
#include "scheduler.h"
#include "servo_manager.h"
#include "display_manager.h"
#include "buzzer_manager.h"
#include "firebase_manager.h"
#include "storage_manager.h"
#include "webserver_manager.h"
#include "api_manager.h"
#include "ota_manager.h"
#include "utilities.h"
#include "ota_manager.h"
WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/**************************************************
 * Global Variables
 **************************************************/

Preferences preferences;

bool wifiConnected = false;
bool firebaseConnected = false;
bool fsMounted = false;
bool medicationLoaded = false;

unsigned long lastWiFiCheck = 0;
unsigned long lastFirebaseSync = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSchedulerCheck = 0;
unsigned long lastLogSave = 0;

const unsigned long WIFI_INTERVAL = 10000;
const unsigned long FIREBASE_INTERVAL = 5000;
const unsigned long DISPLAY_INTERVAL = 1000;
const unsigned long SCHEDULER_INTERVAL = 1000;
const unsigned long LOG_INTERVAL = 30000;

/**************************************************
 * Medication Data
 **************************************************/

MedicationSchedule schedules[MAX_MEDICATIONS];

uint8_t medicationCount = 0;

/**************************************************
 * Device Status
 **************************************************/

struct DeviceStatus
{
    bool wifi;
    bool firebase;
    bool rtc;
    bool servo;
    bool storage;
    bool dispensing;
    bool alarm;
};

DeviceStatus status;

/**************************************************
 * Runtime Variables
 **************************************************/

bool manualDispenseRequest = false;
bool scheduledDispenseRequest = false;

String requestedMedication = "";
String caregiverName = "";
String patientName = "";

String deviceID = DEVICE_ID;

unsigned long bootMillis;

/**************************************************
 * Function Prototypes
 **************************************************/

void initializeSystem();
void initializeFilesystem();
void initializeWiFi();
void initializeDisplay();
void initializeRTC();
void initializeServo();
void initializeFirebase();
void initializeWebServer();

void updateDeviceStatus();

void handleScheduler();

void handleDispensing();

void handleLogging();

void handleDisplay();

void handleFirebaseSync();

void handleWiFiReconnect();

void saveLogs();

void loadSchedules();

void loopTasks();
/**************************************************
 * SETUP
 **************************************************/

void setup()
{
    Serial.begin(115200);

    connectToWiFi();

    espnowBegin();

    discoveryBegin();
    
    firebaseBegin();

    startWebServer();

    otaBegin();
}
    bootMillis = millis();

    Serial.println();
    Serial.println("========================================");
    Serial.println("      WISEMEDS V2 BOOTING");
    Serial.println("========================================");

    initializeSystem();

    Serial.println();
    Serial.println("System Ready.");
    Serial.println("========================================");
}

/**************************************************
 * Initialize Whole System
 **************************************************/

void initializeSystem()
{
    initializeDisplay();

    displaySplashScreen();

    initializeFilesystem();

    initializeServo();

    initializeRTC();

    initializeWiFi();

    initializeFirebase();

    loadSchedules();

    initializeWebServer();

    initializeOTA();

    updateDeviceStatus();

    displayHomeScreen();
}

/**************************************************
 * Filesystem
 **************************************************/

void initializeFilesystem()
{
    Serial.print("Mounting LittleFS... ");

    if (LittleFS.begin(true))
    {
        fsMounted = true;
        status.storage = true;

        Serial.println("OK");
    }
    else
    {
        fsMounted = false;
        status.storage = false;

        Serial.println("FAILED");
    }
}

/**************************************************
 * WiFi
 **************************************************/

void initializeWiFi()
{
    Serial.println();
    Serial.println("Connecting WiFi...");

    wifiConnected = connectToWiFi();

    status.wifi = wifiConnected;

    if (wifiConnected)
    {
        Serial.print("IP Address : ");
        Serial.println(WiFi.localIP());

        if (MDNS.begin("wisemeds"))
        {
            Serial.println("mDNS started.");
        }
    }
    else
    {
        Serial.println("Offline Mode Enabled.");
    }
}

/**************************************************
 * RTC / Internet Time
 **************************************************/

void initializeRTC()
{
    Serial.println("Initializing RTC...");

    timeClient.begin();

    timeClient.setTimeOffset(TIMEZONE_OFFSET);

    if (timeClient.forceUpdate())
    {
        status.rtc = true;

        Serial.print("Current Time : ");
        Serial.println(timeClient.getFormattedTime());
    }
    else
    {
        status.rtc = false;

        Serial.println("RTC Sync Failed.");
    }
}

/**************************************************
 * Servo
 **************************************************/

void initializeServo()
{
    Serial.println("Initializing Servo...");

    setupServo();

    status.servo = true;
}

/**************************************************
 * Display
 **************************************************/

void initializeDisplay()
{
    setupDisplay();

    displayBootMessage("Initializing...");
}

/**************************************************
 * Firebase
 **************************************************/

void initializeFirebase()
{
    Serial.println("Connecting Firebase...");

    firebaseConnected = firebaseBegin();

    status.firebase = firebaseConnected;

    if(firebaseConnected)
    {
        Serial.println("Firebase Connected.");
    }
    else
    {
        Serial.println("Firebase Offline.");
    }
}

/**************************************************
 * Web Server
 **************************************************/

void initializeWebServer()
{
    setupRoutes(server);

    server.begin();

    Serial.println("Web Server Started.");

    Serial.print("Open Browser: http://");

    Serial.println(WiFi.localIP());
}
/**************************************************
 * LOOP
 **************************************************/

void loop()
{
    webServerLoop();

    schedulerLoop();

    firebaseLoop();

    firebaseMaintenance();

    espnowLoop();

    otaLoop();
}
    server.handleClient();

    handleOTA();

    loopTasks();
}

/**************************************************
 * Main Task Scheduler
 **************************************************/

void loopTasks()
{
    unsigned long currentMillis = millis();

    /*-----------------------------
      WiFi Monitor
    ------------------------------*/
    if (currentMillis - lastWiFiCheck >= WIFI_INTERVAL)
    {
        lastWiFiCheck = currentMillis;
        handleWiFiReconnect();
    }

    /*-----------------------------
      Firebase Sync
    ------------------------------*/
    if (currentMillis - lastFirebaseSync >= FIREBASE_INTERVAL)
    {
        lastFirebaseSync = currentMillis;
        handleFirebaseSync();
    }

    /*-----------------------------
      Display Refresh
    ------------------------------*/
    if (currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL)
    {
        lastDisplayUpdate = currentMillis;
        handleDisplay();
    }

    /*-----------------------------
      Medication Scheduler
    ------------------------------*/
    if (currentMillis - lastSchedulerCheck >= SCHEDULER_INTERVAL)
    {
        lastSchedulerCheck = currentMillis;
        handleScheduler();
    }

    /*-----------------------------
      Save Logs
    ------------------------------*/
    if (currentMillis - lastLogSave >= LOG_INTERVAL)
    {
        lastLogSave = currentMillis;
        handleLogging();
    }

    /*-----------------------------
      Dispensing Engine
    ------------------------------*/
    handleDispensing();
}

/**************************************************
 * Update Device Status
 **************************************************/

void updateDeviceStatus()
{
    status.wifi = (WiFi.status() == WL_CONNECTED);
    status.firebase = firebaseConnected;
    status.storage = fsMounted;
}

/**************************************************
 * WiFi Reconnect
 **************************************************/

void handleWiFiReconnect()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        wifiConnected = true;
        status.wifi = true;
        return;
    }

    Serial.println("[WiFi] Reconnecting...");

    wifiConnected = connectToWiFi();

    status.wifi = wifiConnected;

    if (wifiConnected)
    {
        Serial.println("[WiFi] Connected.");
    }
    else
    {
        Serial.println("[WiFi] Failed.");
    }
}

/**************************************************
 * Firebase Sync
 **************************************************/

void handleFirebaseSync()
{
    if (!wifiConnected)
        return;

    firebaseSyncSchedules();

    firebaseUploadLogs();

    firebaseDownloadCommands();
}

/**************************************************
 * Display Update
 **************************************************/

void handleDisplay()
{
    updateHomeDisplay(
        patientName,
        WiFi.status() == WL_CONNECTED,
        firebaseConnected,
        status.dispensing
    );
}

/**************************************************
 * Logging
 **************************************************/

void handleLogging()
{
    saveLogs();
}
/**************************************************
 * Scheduler
 **************************************************/

void handleScheduler()
{
    if (!status.rtc)
        return;

    timeClient.update();

    String currentTime = timeClient.getFormattedTime().substring(0,5);

    for (uint8_t i = 0; i < medicationCount; i++)
    {
        if (!schedules[i].enabled)
            continue;

        if (schedules[i].time != currentTime)
            continue;

        // Prevent multiple dispensing within the same minute
        if (schedules[i].lastDispensed == currentTime)
            continue;

        Serial.println("--------------------------------");
        Serial.print("Medication Due: ");
        Serial.println(schedules[i].medicine);

        requestedMedication = schedules[i].medicine;

        scheduledDispenseRequest = true;

        schedules[i].lastDispensed = currentTime;

        saveSchedules();

        break;
    }
}

/**************************************************
 * Dispensing Engine
 **************************************************/

void handleDispensing()
{
    if (!(manualDispenseRequest || scheduledDispenseRequest))
        return;

    status.dispensing = true;

    displayDispensingScreen(requestedMedication);

    playDispenseTone();

    Serial.println();
    Serial.println("================================");
    Serial.println("Dispensing Medication");
    Serial.println("================================");

    Serial.print("Medicine : ");
    Serial.println(requestedMedication);

    bool success = dispenseMedication(requestedMedication);

    if (success)
    {
        Serial.println("Dispense Success");

        addMedicationLog(
            requestedMedication,
            getCurrentTimestamp(),
            "SUCCESS"
        );

        firebaseSendDispenseLog(
            requestedMedication,
            "SUCCESS"
        );

        playSuccessTone();

        displayDispenseSuccess(requestedMedication);
    }
    else
    {
        Serial.println("Dispense Failed");

        addMedicationLog(
            requestedMedication,
            getCurrentTimestamp(),
            "FAILED"
        );

        firebaseSendDispenseLog(
            requestedMedication,
            "FAILED"
        );

        playErrorTone();

        displayDispenseFailed(requestedMedication);
    }

    delay(2500);

    displayHomeScreen();

    requestedMedication = "";

    manualDispenseRequest = false;
    scheduledDispenseRequest = false;

    status.dispensing = false;
}
/**************************************************
 * Save Medication Logs
 **************************************************/

void saveLogs()
{
    if (!fsMounted)
        return;

    saveMedicationLogs();
}

/**************************************************
 * Load Medication Schedules
 **************************************************/

void loadSchedules()
{
    if (!fsMounted)
    {
        Serial.println("LittleFS not mounted.");
        return;
    }

    medicationCount = loadMedicationSchedules(schedules);

    if (medicationCount > MAX_MEDICATIONS)
        medicationCount = MAX_MEDICATIONS;

    medicationLoaded = true;

    Serial.print("Loaded ");
    Serial.print(medicationCount);
    Serial.println(" medication schedules.");
}

/**************************************************
 * System Information
 **************************************************/

void printSystemInfo()
{
    Serial.println();
    Serial.println("========== SYSTEM INFO ==========");

    Serial.print("Device ID : ");
    Serial.println(deviceID);

    Serial.print("Firmware : ");
    Serial.println(FIRMWARE_VERSION);

    Serial.print("WiFi : ");
    Serial.println(status.wifi ? "Connected" : "Disconnected");

    Serial.print("Firebase : ");
    Serial.println(status.firebase ? "Connected" : "Offline");

    Serial.print("Storage : ");
    Serial.println(status.storage ? "Mounted" : "Not Mounted");

    Serial.print("RTC : ");
    Serial.println(status.rtc ? "Synced" : "Unavailable");

    Serial.print("Free Heap : ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    Serial.print("CPU Frequency : ");
    Serial.print(getCpuFrequencyMhz());
    Serial.println(" MHz");

    Serial.println("===============================");
}

/**************************************************
 * Uptime
 **************************************************/

String getUptime()
{
    unsigned long seconds = millis() / 1000;

    unsigned long days = seconds / 86400;
    seconds %= 86400;

    unsigned long hours = seconds / 3600;
    seconds %= 3600;

    unsigned long minutes = seconds / 60;
    seconds %= 60;

    char buffer[40];

    sprintf(buffer,
            "%luD %02luH %02luM %02luS",
            days,
            hours,
            minutes,
            seconds);

    return String(buffer);
}

/**************************************************
 * Heartbeat
 **************************************************/

void heartbeat()
{
    static unsigned long lastHeartbeat = 0;

    if (millis() - lastHeartbeat >= 1000)
    {
        lastHeartbeat = millis();

        Serial.print(".");
    }
}