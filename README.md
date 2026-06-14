# Periscope-OS (v2.0.0-SIGINT) 📡⚓

---

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
 LICENSE: MIT License
 For educational, defensive validation, and security auditing telemetry mapping only.
=========================================================================================
*/
```

**Periscope-OS** is an open-source, passive 2.4GHz RF Spectrum Analyzer and Tactical Signals Intelligence (SIGINT) Radar Suite custom-built for the **M5Stack M5StickS3**. 

Operating completely silently without transmitting a single radio wave, it intercepts, parses, maps, and logs local 802.11 Wi-Fi management layer attacks (such as Deauthentication and Disassociation frames) occurring in your immediate airspace.

---

## 🛠️ Hardware Requirements & Specifications

* **Core Platform:** M5Stack M5StickS3 (ESP32-S3 Dual-Core Xtensa LX7)
* **Motion Sensor:** Internal BMI270 6-Axis IMU (used for real-time body-shielding direction integration)
* **Power Management:** Onboard AXP2101 PMU (used for real-time power grid monitoring)
* **Haptics:** Internal Vibration Motor (Mapped natively to **GPIO 44**)
* **Audio:** Built-in I2S AW88298 Power Amplifier + Buzzer (used for the Sonar Acoustic Engine)

---

## 📡 Core Telemetry Features

### 1. Passive Radio Intercept Core (DPI Engine)
The firmware forces the ESP32-S3 radio driver into **Promiscuous Mode**, bypassing hardware MAC filtering to ingest raw 802.11 management frames (`WIFI_PKT_MGMT`).
* **Deep Packet Inspection (DPI):** Dissects Type `0xC0` (Deauth) and `0xA0` (Disassociation) frames on the fly to extract the Attacker MAC, Victim Client MAC, Target AP BSSID, and 802.11 Disconnection Reason Codes.
* **Acoustic Fingerprinting:** Parses the first 3 bytes of the MAC address (OUI) to classify weaponized hardware in the field:
  * `ESPRESSIF`: Flipper Zero, Wi-Fi Marauder builds, or pocket deauther nodes.
  * `RASPBERRY PI`: Portable Linux drop-boxes or mobile Kali Linux field units.
  * `ALFA CARD`: High-power operational packet injection adapters.
  * `HAK5 GEAR`: Tactical specialized routing gear (Wi-Fi Pineapple arrays).
  * `REALTEK/PINE`: Low-profile smart-wardriving transceivers.

### 2. Live Instrument Dashboard (Landscape Split-Screen)
Flipping the screen rotation matrix sideways ($240 \times 135$ pixels) creates a real-time, high-contrast dual instrumentation deck:
* **Left Pane (Sonar Display):** A glowing phosphor-style CRT circular radar sweep. The rotating beam simulates an analog radar using a 5-layer ghosting decay trail. Targets are plotted as glowing red blips that scale their depth distance from the crosshairs based on live signal strength (RSSI).
* **Right Pane (Volumetric Traffic Equalizer):** A live real-time RF frequency distribution graph across Channels 1–11. Floating above each active column is a **Peak RSSI Tick Line** that snaps to the maximum observed power level and decays slowly over time to catch hidden or intermittent transmitters.

### 3. Spatial Inertial Processor (Body Shielding Math)
Using an internal calculus integration loop, the firmware tracks the raw horizontal rotation velocity along the Z-axis gyroscope (`data.gyro.z`). By computing change-in-time ($dt$), it calculates your precise physical yaw relative to your starting position. As you turn 360 degrees, your body acts as an RF shield; the firmware binds the peak un-attenuated RSSI with your physical yaw angle to pinpoint the exact directional vector of the attacker.

---

## 🕹️ Tactical Control Mapping

Periscope-OS features a multi-layered hardware button matrix mapped directly to its landscape orientation:

| Control Trigger | Radar Deck Action | Logbook Deck Action |
| :--- | :--- | :--- |
| **Button A (Face) - Click** | Toggle **Channel Lock** (Parks radio on current channel) | Cycle forward through saved threat sheets |
| **Button A (Face) - Hold** | Toggle **Silent Running** (Stealth haptic tracking) | Purge/Wipe the data registry file system |
| **Button B (Side) - Click** | Toggle **Target Lock** (Isolates attacker MAC profile) | Dismiss log console / Return to Radar |
| **Button B (Side) - Hold** | Drop into the **Captain's Logbook Deck** | Dismiss log console / Return to Radar |

---

## 🎛️ Operational Modes

### 🤫 Silent Running Mode
Activated by holding **Button A** for 2 seconds. The display backlight cuts to black and the speaker is completely muted. Telemetry is transferred entirely to the internal haptic motor. The device generates clean haptic pulses that vibrate faster as your body rotates directly into the threat vector line, allowing for complete touch-based tracking in low-profile situations.

### 🚨 Perimeter Breach Alarm
If an active target crosses a critical perimeter threshold ($\ge -42\text{ dBm}$), a master priority alert overrides standard operations. The display flashes bright red, the haptic motor holds a continuous hum, and the speaker blasts a dual-frequency acoustic naval klaxon horn (`850Hz` / `700Hz`) to signal immediate physical proximity.

### 📜 The Captain's Logbook
Activated by holding **Button B** for 2 seconds. This screen reads compressed, pipe-delimited threat strings directly from the onboard flash file system (SPIFFS) to build interactive dossier pages without overloading RAM. Each sheet contains forensic data logs including:
* **Attacker MAC & Hardware Profile Assignment**
* **Target Network BSSID (Access Point Router)**
* **Target Destination Client (Victim Machine)**
* **802.11 Reason Code & Micro-Script Verification**
* **Peak Signal Power & Channel Metrics**

---

## 🚀 Installation & Compilation Guide

1. Clone this repository to your local machine:
   ```bash
   git clone [https://github.com/M5RogueOps/Periscope-OS.git](https://github.com/M5RogueOps/Periscope-OS.git)

```

2. Open the project directory inside the **Arduino IDE**.
3. Ensure you have the **ESP32 Arduino Core v3.x** installed via the Board Manager.
4. Install the following dependency via the Arduino Library Manager:
* **M5Unified** (Handles core screen rendering, I2S audio, power systems, and IMU access).


5. In your IDE settings, select **M5StickC-Plus2** or **ESP32S3 Dev Module** as the target board, set the partition scheme to `Default 4MB with SPIFFS`, and upload the code over USB-C.

---

## ⚖️ License

This project is licensed under the **MIT License** - see the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.

---

Developed by **@M5RogueOps** * **Developer Hub:** [www.EthicalHackersDen.org](http://www.EthicalHackersDen.org)

* **Source Repo:** [https://github.com/M5RogueOps/Periscope-OS](https://github.com/M5RogueOps/Periscope-OS)

*Disclaimer: Periscope-OS is a strictly passive wireless reconnaissance tool designed for educational, defensive validation, and security auditing telemetry mapping only.*

```

```
