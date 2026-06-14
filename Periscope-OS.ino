/*
=========================================================================================
 
  _____  ______ _____  _____  _____  _____ ____  _____  ______     ____   _____ 
 |  __ \|  ____|  __ \|_   _|/ ____|/ ____/ __ \|  __ \|  ____|   / __ \ / ____|
 | |__) | |__  | |__) | | | | (___ | |   | |  | | |__) | |__     | |  | | (___  
 |  ___/|  __| |  _  /  | |  \___ \| |   | |  | |  ___/|  __|    | |  | |\___ \ 
 | |    | |____| | \ \ _| |_ ____) | |___| |__| | |    | |____   | |__| |____) |
 |_|    |______|_|  \_\_____|_____/ \_____\____/|_|    |______|   \____/|_____/ 
                                                                                
=========================================================================================
 [+] PROJECT     : Periscope-OS (v2.0.0-SIGINT)
 [+] CAPABILITY  : Passive 2.4GHz RF Spectrum Analyzer & Tactical Sonar Radar Deck
 [+] HARDWARE    : M5Stack M5StickS3 (ESP32-S3 + BMI270 IMU + AXP2101 PMU)
=========================================================================================
                                       
                                       |
                                   _  _|_  _
                                  [_ _ _ _ _]
                                    \_____/
                                       |
                     __________________|__________________
                    /  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  \
                   /  /                                 \  \
     _____________/  /      [ SILENT RUNNING MODE ]      \  \_____________
    |               /                                     \               |
    |==============|==============[ ELINT ]================|==============|
    |____    ______\                                      /______    _____|
         \  \       \____________________________________/       /  /
          \  \__________________________________________________/  /
           \______________________________________________________/
                       V  V  V                       V  V  V
                       
=========================================================================================
 [?] DESIGNED BY : @M5RogueOps
 [?] SOURCE REPO : https://github.com/M5RogueOps/Periscope-OS
 [?] DEV HUB     : www.EthicalHackersDen.org
=========================================================================================
 LICENSE: Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 For educational, defensive validation, and security auditing telemetry mapping only.
=========================================================================================
*/
#include <M5Unified.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <FS.h>
#include <SPIFFS.h>
#include <math.h>

const int MOTOR_PIN = 44; 

// --- CONFIGURATION ---
const uint8_t TARGET_MAC[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}; 

enum ViewMode { SCREEN_RADAR, SCREEN_LOGBOOK };
ViewMode currentView = SCREEN_RADAR;

// --- GLOBAL TELEMETRY STORAGE ---
uint8_t last_detected_mac[6] = {0, 0, 0, 0, 0, 0};
uint8_t locked_mac[6]        = {0, 0, 0, 0, 0, 0};
bool target_locked           = false;              
bool silent_running          = false; 
bool proximity_breach        = false;

int current_rssi = -100;
unsigned long last_packet_time = 0;
float current_yaw_rad = 0.0;
float target_heading = 0.0;
int best_rssi = -100;
bool attack_in_progress = false;

// Deep Packet Inspection (DPI) Signals Intelligence
uint8_t victim_client_mac[6]  = {0, 0, 0, 0, 0, 0};
uint8_t victim_ap_bssid[6]    = {0, 0, 0, 0, 0, 0};
uint16_t attack_reason_code   = 0;
bool tool_is_automated_script = false;

int rolling_prev_rssi = -100;
unsigned long last_vector_calc_time = 0;
String target_vector_status = "STATIONARY"; 
String attacker_hardware_class = "UNKNOWN"; 

// Logbook Scrolling Tracker
int current_log_index = 0;
int total_log_entries = 0;

// --- REAL-TIME WI-FI VISUALIZER TELEMETRY ---
float channel_visualizer_bars[12] = {0.0f}; 
int channel_peak_rssi[12]         = {-100};
unsigned long last_visualizer_decay_time = 0;

// Channel Hopping Framework
uint8_t current_wifi_channel = 1;
unsigned long last_channel_hop_time = 0;
const unsigned long HOP_INTERVAL = 120; 
bool channel_lock = false; 

// Spatial Clocks
unsigned long last_imu_time = 0;
float current_yaw_deg = 0.0;
unsigned long last_ping_time = 0;
unsigned long last_haptic_time = 0;

// Geometry Coordinates Layout
const int centerX = 65;   
const int centerY = 67;   
const int maxRadius = 55; 
float sweepAngle = 0;

// RGB565 Palettes
uint16_t COLOR_CORE_GREEN, COLOR_MID_GREEN, COLOR_GLOW_GREEN, COLOR_DARK_GREEN;
uint16_t COLOR_CORE_RED, COLOR_GLOW_RED, COLOR_LOCK_CYAN, COLOR_AMBER;

String getVendorString(const uint8_t* mac) {
    uint32_t oui = (mac[0] << 16) | (mac[1] << 8) | mac[2];
    switch(oui) {
        case 0x240AC4: case 0x30AEA4: case 0x840D8E: case 0xBCDDC2: 
        case 0xA4CF12: case 0xECFABC: case 0x545A16: case 0x10CDAE:
            return "ESPRESSIF"; 
        case 0xB827EB: case 0xDCA632: case 0xE45F01: case 0x2CCF67:
            return "RASPBERRY PI";    
        case 0x00C0CA: case 0x7CDD90: case 0x002586:
            return "ALFA CARD";   
        case 0x0013EF: case 0x40A3CC: case 0x001F1F:
            return "HAK5 GEAR";
        case 0x00143C: case 0x222441: case 0x0006DC: case 0x00E04C:
            return "REALTEK/PINE";
        default:
            return "UNKNOWN"; 
    }
}

void logThreatToFlash(uint8_t* attacker, uint8_t* bssid, uint8_t* victim, uint8_t channel, int rssi, uint16_t reason, bool isScript) {
    File logFile = SPIFFS.open("/captains_log.dat", FILE_APPEND);
    if (!logFile) return;

    char entryLine[128];
    snprintf(entryLine, sizeof(entryLine), 
             "%02X%02X%02X%02X%02X%02X|%02X%02X%02X%02X%02X%02X|%02X%02X%02X%02X%02X%02X|%d|%d|%d|%d\n",
             attacker[0], attacker[1], attacker[2], attacker[3], attacker[4], attacker[5],
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
             victim[0], victim[1], victim[2], victim[3], victim[4], victim[5],
             channel, rssi, reason, isScript ? 1 : 0);

    logFile.print(entryLine);
    logFile.close();
}

void updateLogCounts() {
    total_log_entries = 0;
    File logFile = SPIFFS.open("/captains_log.dat", FILE_READ);
    if (!logFile) return;
    while (logFile.available()) {
        String line = logFile.readStringUntil('\n');
        if (line.length() > 10) total_log_entries++;
    }
    logFile.close();
}

void clearLogFile() {
    SPIFFS.remove("/captains_log.dat");
    total_log_entries = 0;
    current_log_index = 0;
    M5.Speaker.tone(600, 400);
}

void wifi_sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t *payload = pkt->payload;
    
    // VISUALIZER FUEL: Process every ambient packet to drive the equalizer bars
    channel_visualizer_bars[current_wifi_channel] += 1.8f; 
    if (channel_visualizer_bars[current_wifi_channel] > 48.0f) {
        channel_visualizer_bars[current_wifi_channel] = 48.0f; 
    }
    
    if (pkt->rx_ctrl.rssi > channel_peak_rssi[current_wifi_channel]) {
        channel_peak_rssi[current_wifi_channel] = pkt->rx_ctrl.rssi;
    }

    if (type != WIFI_PKT_MGMT) return; 
    uint8_t frame_type = payload[0];
    bool is_attack_frame = (frame_type == 0xC0 || frame_type == 0xA0);
    if (!is_attack_frame) return;

    uint8_t src_mac[6];
    for (int i = 0; i < 6; i++) src_mac[i] = payload[10 + i];

    if (target_locked) {
        bool mac_match = true;
        for (int i = 0; i < 6; i++) {
            if (src_mac[i] != locked_mac[i]) { mac_match = false; break; }
        }
        if (!mac_match) return; 
    }

    for (int i = 0; i < 6; i++) victim_client_mac[i] = payload[4 + i];
    for (int i = 0; i < 6; i++) victim_ap_bssid[i] = payload[16 + i];

    uint16_t seq_control = payload[22] | (payload[23] << 8);
    tool_is_automated_script = ((seq_control >> 4) == 0);
    attack_reason_code = payload[24] | (payload[25] << 8);

    bool is_new_mac = false;
    for (int i = 0; i < 6; i++) {
        if (last_detected_mac[i] != src_mac[i]) is_new_mac = true;
        last_detected_mac[i] = src_mac[i];
    }
    
    if (is_new_mac && !target_locked) {
        attacker_hardware_class = getVendorString(src_mac);
        logThreatToFlash(src_mac, victim_ap_bssid, victim_client_mac, current_wifi_channel, pkt->rx_ctrl.rssi, attack_reason_code, tool_is_automated_script);
    }

    current_rssi = pkt->rx_ctrl.rssi;
    last_packet_time = millis();
    attack_in_progress = true;
    if (best_rssi == -100) best_rssi = current_rssi;
}

void startWifiSniffer() {
    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_cb);
    esp_wifi_set_channel(current_wifi_channel, WIFI_SECOND_CHAN_NONE);
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    M5.Display.setRotation(1); 
    
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);

    M5.Speaker.begin();
    M5.Speaker.setVolume(120); 

    if (!SPIFFS.begin(true)) {
        M5.Display.fillScreen(RED);
        while(1); 
    }

    COLOR_CORE_GREEN = M5.Display.color565(200, 255, 200); 
    COLOR_MID_GREEN  = M5.Display.color565(0, 255, 0);     
    COLOR_GLOW_GREEN = M5.Display.color565(0, 75, 0);      
    COLOR_DARK_GREEN = M5.Display.color565(0, 30, 0);      
    COLOR_CORE_RED   = M5.Display.color565(255, 180, 180);
    COLOR_GLOW_RED   = M5.Display.color565(110, 0, 0);
    COLOR_LOCK_CYAN  = M5.Display.color565(0, 230, 255);
    COLOR_AMBER      = M5.Display.color565(240, 150, 0);

    for(int i=1; i<=11; i++) channel_peak_rssi[i] = -100;

    updateLogCounts();
    last_imu_time = millis();
    startWifiSniffer(); 
}

void processSpatialAudioAndHaptics() {
    M5.Imu.update();
    auto data = M5.Imu.getImuData(); 

    unsigned long now = millis();
    float dt = (now - last_imu_time) / 1000.0f;
    last_imu_time = now;
    if (dt > 0.1f) dt = 0.03f;

    current_yaw_deg += data.gyro.z * dt; 
    if (current_yaw_deg >= 360.0f) current_yaw_deg -= 360.0f;
    if (current_yaw_deg < 0.0f) current_yaw_deg += 360.0f;
    current_yaw_rad = current_yaw_deg * M_PI / 180.0f;

    if (!channel_lock && (now - last_channel_hop_time >= HOP_INTERVAL)) {
        last_channel_hop_time = now;
        current_wifi_channel++;
        if (current_wifi_channel > 11) current_wifi_channel = 1; 
        esp_wifi_set_channel(current_wifi_channel, WIFI_SECOND_CHAN_NONE);
    }

    if (now - last_visualizer_decay_time >= 45) {
        last_visualizer_decay_time = now;
        for (int c = 1; c <= 11; c++) {
            if (channel_visualizer_bars[c] > 0.0f) channel_visualizer_bars[c] -= 0.8f;
            if (channel_visualizer_bars[c] < 0.0f) channel_visualizer_bars[c] = 0.0f;
            if (channel_peak_rssi[c] > -100) channel_peak_rssi[c] -= 1;
        }
    }

    unsigned long time_since_packet = now - last_packet_time;

    if (time_since_packet < 250 && current_rssi > best_rssi) {
        best_rssi = current_rssi;
        target_heading = current_yaw_rad;
    }

    if (attack_in_progress && (now - last_vector_calc_time >= 1500)) {
        last_vector_calc_time = now;
        if (rolling_prev_rssi != -100) {
            int delta = current_rssi - rolling_prev_rssi;
            if (delta > 3) target_vector_status = "CLOSING";    
            else if (delta < -3) target_vector_status = "EVADING"; 
            else target_vector_status = "STATIONARY";
        }
        rolling_prev_rssi = current_rssi;
    }

    if (time_since_packet > 6000) {
        best_rssi = -100;
        rolling_prev_rssi = -100;
        attack_in_progress = false;
        proximity_breach = false;
        target_vector_status = "STATIONARY";
        digitalWrite(MOTOR_PIN, LOW);
        return;
    }

    proximity_breach = (attack_in_progress && current_rssi >= -42);

    float angle_diff = fabs(current_yaw_rad - target_heading);
    if (angle_diff > M_PI) angle_diff = (2 * M_PI) - angle_diff;
    
    int dynamic_factor = (target_vector_status == "CLOSING") ? 70 : 100;
    int pulse_interval = map(angle_diff * 100, 0, M_PI * 100, 60, 1300);
    pulse_interval = (pulse_interval * dynamic_factor) / 100;

    if (attack_in_progress) {
        if (silent_running) {
            if (now - last_haptic_time >= pulse_interval) {
                digitalWrite(MOTOR_PIN, HIGH); delay(30); digitalWrite(MOTOR_PIN, LOW);
                last_haptic_time = now;
            }
        } else if (proximity_breach) {
            if (now - last_ping_time >= 250) {
                M5.Speaker.tone(now % 500 > 250 ? 850 : 700, 120);
                digitalWrite(MOTOR_PIN, (now / 250) % 2);
                last_ping_time = now;
            }
        } else {
            digitalWrite(MOTOR_PIN, LOW);
            if (now - last_ping_time >= pulse_interval) {
                M5.Speaker.tone(target_locked ? 3100 : 2400, 35); 
                last_ping_time = now;
            }
        }
    } else {
        digitalWrite(MOTOR_PIN, LOW);
    }
}

void drawInterface() {
    M5.Display.startWrite();
    M5.Display.fillScreen(BLACK);

    if (currentView == SCREEN_LOGBOOK) {
        M5.Display.setTextColor(COLOR_AMBER);
        M5.Display.setCursor(4, 4);
        M5.Display.printf("LOG DECK: SHEET %02d/%02d", current_log_index + 1, total_log_entries);
        M5.Display.drawFastHLine(0, 15, 240, COLOR_AMBER);
        
        File logFile = SPIFFS.open("/captains_log.dat", FILE_READ);
        if (!logFile || total_log_entries == 0) {
            M5.Display.setCursor(4, 30);
            M5.Display.setTextColor(COLOR_DARK_GREEN);
            M5.Display.printf("[AIRSPACE CLEAR: NO TELEMETRY ON STORAGE]");
        } else {
            String targetLine = "";
            int scanIdx = 0;
            while (logFile.available()) {
                String line = logFile.readStringUntil('\n');
                if (scanIdx == current_log_index) { targetLine = line; break; }
                scanIdx++;
            }
            logFile.close();

            if (targetLine.length() > 20) {
                int p1 = targetLine.indexOf('|');
                int p2 = targetLine.indexOf('|', p1 + 1);
                int p3 = targetLine.indexOf('|', p2 + 1);
                int p4 = targetLine.indexOf('|', p3 + 1);
                int p5 = targetLine.indexOf('|', p4 + 1);
                int p6 = targetLine.indexOf('|', p5 + 1);

                String r_atk = targetLine.substring(0, p1);
                String r_bsd = targetLine.substring(p1 + 1, p2);
                String r_vic = targetLine.substring(p2 + 1, p3);
                int r_ch     = targetLine.substring(p3 + 1, p4).toInt();
                int r_rssi   = targetLine.substring(p4 + 1, p5).toInt();
                int r_reas   = targetLine.substring(p5 + 1, p6).toInt();
                bool r_scr   = targetLine.substring(p6 + 1).toInt() == 1;

                uint8_t dummy_mac[3] = {(uint8_t)strtol(r_atk.substring(0,2).c_str(),NULL,16),
                                        (uint8_t)strtol(r_atk.substring(2,4).c_str(),NULL,16),
                                        (uint8_t)strtol(r_atk.substring(4,6).c_str(),NULL,16)};
                String r_vendor = getVendorString(dummy_mac);

                M5.Display.setTextColor(COLOR_MID_GREEN);
                M5.Display.setCursor(4, 22);  M5.Display.printf("ATTACKER : %s:%s:%s:%s:%s:%s", r_atk.substring(0,2).c_str(), r_atk.substring(2,4).c_str(), r_atk.substring(4,6).c_str(), r_atk.substring(6,8).c_str(), r_atk.substring(8,10).c_str(), r_atk.substring(10,12).c_str());
                M5.Display.setCursor(4, 34);  M5.Display.printf("HW CLASS : %s", r_vendor.c_str());
                M5.Display.setCursor(4, 46);  M5.Display.printf("NET BSSID: %s:%s:%s:%s:%s:%s", r_bsd.substring(0,2).c_str(), r_bsd.substring(2,4).c_str(), r_bsd.substring(4,6).c_str(), r_bsd.substring(6,8).c_str(), r_bsd.substring(8,10).c_str(), r_bsd.substring(10,12).c_str());
                M5.Display.setCursor(4, 58);  M5.Display.printf("TARGET   : %s:%s:%s:%s:%s:%s", r_vic.substring(0,2).c_str(), r_vic.substring(2,4).c_str(), r_vic.substring(4,6).c_str(), r_vic.substring(6,8).c_str(), r_vic.substring(8,10).c_str(), r_vic.substring(10,12).c_str());
                M5.Display.setCursor(4, 70);  M5.Display.printf("PAYLOAD  : 802.11 REASON CODE %d", r_reas);
                M5.Display.setCursor(4, 82);  M5.Display.printf("WEAPON   : %s", r_scr ? "AUTOMATED MICRO-SCRIPT" : "MANUAL DRIVER INJECT");
                M5.Display.setCursor(4, 94);  M5.Display.printf("METRICS  : %d dBm CAPTURED ON CHANNEL %d", r_rssi, r_ch);
            }
        }
        M5.Display.drawFastHLine(0, 110, 240, COLOR_DARK_GREEN);
        M5.Display.setTextColor(COLOR_DARK_GREEN);
        M5.Display.setCursor(4, 116);
        M5.Display.printf("A: CYCL SHEET | HOLD B: EXIT RADAR | HOLD A: PURGE ALL");
        M5.Display.endWrite();
        return;
    }
    
    if (proximity_breach && !silent_running) {
        if ((millis() / 200) % 2 == 0) M5.Display.fillScreen(COLOR_GLOW_RED);
    }

    // LEFT HALF: SONAR DECK
    uint16_t radarColor = silent_running ? COLOR_DARK_GREEN : (target_locked ? COLOR_LOCK_CYAN : COLOR_MID_GREEN);
    M5.Display.drawCircle(centerX, centerY, maxRadius, radarColor);
    M5.Display.drawCircle(centerX, centerY, maxRadius / 2, COLOR_DARK_GREEN);
    M5.Display.drawFastHLine(centerX - maxRadius, centerY, maxRadius * 2, COLOR_DARK_GREEN);
    M5.Display.drawFastVLine(centerX, centerY - maxRadius, maxRadius * 2, COLOR_DARK_GREEN);

    sweepAngle += 0.07;
    if (sweepAngle >= 2 * M_PI) sweepAngle = 0;
    for (int i = 5; i > 0; i--) {
        float trailAngle = sweepAngle - (i * 0.03);
        M5.Display.drawLine(centerX, centerY, centerX + maxRadius * cos(trailAngle), centerY + maxRadius * sin(trailAngle), silent_running ? COLOR_DARK_GREEN : COLOR_GLOW_GREEN);
    }
    M5.Display.drawLine(centerX, centerY, centerX + maxRadius * cos(sweepAngle), centerY + maxRadius * sin(sweepAngle), target_locked ? COLOR_LOCK_CYAN : COLOR_CORE_GREEN);

    unsigned long now = millis();
    if (attack_in_progress && (now - last_packet_time < 5000) && !silent_running) {
        int radius = map(current_rssi, -95, -30, maxRadius - 4, 6);
        radius = constrain(radius, 5, maxRadius - 2);

        int bx = centerX + radius * cos(target_heading - current_yaw_rad + M_PI_2);
        int by = centerY + radius * sin(target_heading - current_yaw_rad + M_PI_2);

        M5.Display.fillCircle(bx, by, 6, COLOR_GLOW_RED);  
        M5.Display.fillCircle(bx, by, 3, target_locked ? COLOR_LOCK_CYAN : RED);             
        if (target_locked) M5.Display.drawRect(bx - 5, by - 5, 11, 11, COLOR_LOCK_CYAN);
    }

    // RIGHT HALF: PERIPHERALS PANEL
    int paneX = 130;
    M5.Display.drawFastVLine(paneX, 0, 135, COLOR_DARK_GREEN); 
    
    M5.Display.setCursor(paneX + 6, 4);
    if (silent_running) {
        M5.Display.setTextColor(COLOR_DARK_GREEN);
        M5.Display.printf("[SILENT RUNNING]");
    } else if (proximity_breach) {
        M5.Display.setTextColor(COLOR_CORE_RED);
        M5.Display.printf("BREACH PERIMETER");
    } else {
        M5.Display.setTextColor(target_locked ? COLOR_LOCK_CYAN : COLOR_MID_GREEN);
        M5.Display.printf("CH: %d %s", current_wifi_channel, channel_lock ? "[HOLD]" : "[SCAN]");
    }

    M5.Display.setCursor(paneX + 6, 14);
    if (attack_in_progress) {
        uint16_t vecColor = (target_vector_status == "CLOSING") ? RED : ((target_vector_status == "EVADING") ? COLOR_AMBER : COLOR_GLOW_GREEN);
        M5.Display.setTextColor(vecColor);
        M5.Display.printf("VCTR: %s", target_vector_status.c_str());
    } else {
        M5.Display.setTextColor(COLOR_DARK_GREEN);
        M5.Display.printf("VCTR: PASSIVE");
    }

    // --- RE-ALIGNMENT: MINIMALIST GRAPH FLOOR OVERLAY ---
    int graphX = paneX + 6;
    int graphY = 25;
    int cellW = 7; 
    int maxGraphHeight = 48;
    int baseLineY = graphY + maxGraphHeight + 8; 

    // Pure, clean phosphor target indicator dot map loop (No overflow text headers)
    for (int c = 0; c < 11; c++) {
        int dotX = graphX + (c * cellW) + (cellW / 2) - 1;
        int dotY = graphY + 1;
        uint16_t dotColor = (current_wifi_channel == (c + 1)) ? COLOR_MID_GREEN : COLOR_DARK_GREEN;
        if (silent_running) dotColor = COLOR_DARK_GREEN;
        M5.Display.fillRect(dotX, dotY, 2, 2, dotColor);
    }
    
    // Analyzer framework container box
    M5.Display.drawRect(graphX - 2, graphY + 6, (11 * cellW) + 3, maxGraphHeight + 4, COLOR_DARK_GREEN);

    for (int c = 0; c < 11; c++) {
        int channelNum = c + 1;
        int barHeight = (int)channel_visualizer_bars[channelNum];
        int drawX = graphX + (c * cellW);
        
        if (barHeight > 0) {
            uint16_t barColor = COLOR_GLOW_GREEN;
            if (channel_peak_rssi[channelNum] >= -50)      barColor = RED;
            else if (channel_peak_rssi[channelNum] >= -72) barColor = COLOR_AMBER;
            else                                           barColor = COLOR_MID_GREEN;

            if (silent_running) barColor = COLOR_DARK_GREEN;
            M5.Display.fillRect(drawX, baseLineY - barHeight, cellW - 2, barHeight, barColor);
        }

        int peakRSSIVal = channel_peak_rssi[channelNum];
        if (peakRSSIVal > -100) {
            int peakY = map(peakRSSIVal, -95, -30, baseLineY, baseLineY - maxGraphHeight);
            peakY = constrain(peakY, baseLineY - maxGraphHeight, baseLineY);
            
            uint16_t tickColor = silent_running ? COLOR_DARK_GREEN : COLOR_CORE_GREEN;
            M5.Display.drawFastHLine(drawX, peakY, cellW - 2, tickColor);
        }
    }

    M5.Display.setCursor(paneX + 6, 92);
    if (attack_in_progress) {
        M5.Display.setTextColor(COLOR_MID_GREEN);
        M5.Display.printf("HW: %s", attacker_hardware_class.c_str());
    } else {
        M5.Display.setTextColor(COLOR_DARK_GREEN);
        M5.Display.printf("HW: PASSIVE");
    }

    M5.Display.setCursor(paneX + 6, 104);
    if (target_locked) {
        M5.Display.setTextColor(COLOR_LOCK_CYAN);
        M5.Display.printf("LCK:%02X:%02X:%02X..", locked_mac[0], locked_mac[1], locked_mac[2]);
        M5.Display.setCursor(paneX + 6, 114);
        M5.Display.printf("    ..%02X:%02X:%02X", locked_mac[3], locked_mac[4], locked_mac[5]);
    } else if (attack_in_progress) {
        M5.Display.setTextColor(COLOR_AMBER);
        M5.Display.printf("TRGT ACQUIRED");
        M5.Display.setCursor(paneX + 6, 114);
        M5.Display.printf("RSSI: %d dBm", current_rssi);
    } else {
        M5.Display.setTextColor(COLOR_DARK_GREEN);
        M5.Display.printf("AIRSPACE VACANT");
    }

    // BATTERY RESERVES RAIL
    int batX = 232;
    int batY = 6;
    int batW = 4;
    int batH = 123;
    int batPct = M5.Power.getBatteryLevel();
    batPct = constrain(batPct, 0, 100);
    
    int fillH = map(batPct, 0, 100, 0, batH - 2);
    uint16_t batColor = (batPct > 50) ? COLOR_GLOW_GREEN : ((batPct > 20) ? COLOR_AMBER : RED);
    
    M5.Display.drawRect(batX, batY, batW, batH, COLOR_DARK_GREEN); 
    M5.Display.fillRect(batX + 1, batY + (batH - 1 - fillH), batW - 2, fillH, batColor); 

    M5.Display.endWrite();
}

void loop() {
    M5.update();

    if (currentView == SCREEN_RADAR) {
        if (M5.BtnA.wasClicked()) {
            channel_lock = !channel_lock;
            if (!silent_running) M5.Speaker.tone(1900, 60);
        }
        if (M5.BtnA.wasHold()) {
            silent_running = !silent_running;
            if (!silent_running) {
                M5.Speaker.tone(2000, 100); delay(80); M5.Speaker.tone(2500, 100);
            } else {
                digitalWrite(MOTOR_PIN, HIGH); delay(400); digitalWrite(MOTOR_PIN, LOW); 
            }
        }
        if (M5.BtnB.wasClicked()) {
            if (!target_locked && attack_in_progress) {
                for (int i = 0; i < 6; i++) locked_mac[i] = last_detected_mac[i];
                target_locked = true;
                attacker_hardware_class = getVendorString(locked_mac); 
                if (!silent_running) {
                    M5.Speaker.tone(1400, 80); delay(80); M5.Speaker.tone(3000, 150);
                }
            } else if (target_locked) {
                target_locked = false;
                best_rssi = -100;
                attacker_hardware_class = "UNKNOWN";
                if (!silent_running) M5.Speaker.tone(900, 250);
            }
        }
        if (M5.BtnB.wasHold()) {
            updateLogCounts(); 
            current_log_index = 0; 
            currentView = SCREEN_LOGBOOK;
            M5.Speaker.tone(1200, 100);
        }
    } 
    else if (currentView == SCREEN_LOGBOOK) {
        if (M5.BtnA.wasClicked()) {
            if (total_log_entries > 0) {
                current_log_index = (current_log_index + 1) % total_log_entries;
                M5.Speaker.tone(1600, 40); 
            }
        }
        if (M5.BtnA.wasHold()) {
            clearLogFile();
        }
        if (M5.BtnB.wasHold()) {
            currentView = SCREEN_RADAR;
            M5.Speaker.tone(1400, 80);
        }
    }

    processSpatialAudioAndHaptics();
    drawInterface();
    delay(20);
}
