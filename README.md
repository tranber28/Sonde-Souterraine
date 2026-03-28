[README.md](https://github.com/user-attachments/files/26325831/README.md)
# 🌱 Station Coggia — Sonde Souterraine

Firmware pour une station de surveillance souterraine déployée sur **LilyGO TTGO LoRa32 V1** (ESP32). Elle mesure l'humidité du sol à 3 profondeurs et la température via des sondes DS18B20, puis transmet les données toutes les heures via **LoRaWAN (OTAA)** vers un serveur ChirpStack.

---

## 📦 Matériel

| Composant | Rôle |
|---|---|
| LilyGO TTGO LoRa32 V1 (SX1276) | MCU + radio LoRa |
| 3× DS18B20 (bus OneWire) | Températures sol |
| 3× Capteur capacitif d'humidité | Humidité à 10 / 30 / 60 cm |
| Écran OLED SSD1306 128×64 | Affichage local |

---

## 🔌 Câblage

| Signal | GPIO |
|---|---|
| DS18B20 (data) | 13 |
| VCC alimentation sondes temp | 14 |
| VCC alimentation capteurs humidité | 25 |
| Humidité 10 cm (ADC) | 34 |
| Humidité 30 cm (ADC) | 35 |
| Humidité 60 cm (ADC) | 39 |
| LoRa NSS | 18 |
| LoRa RST | 23 |
| LoRa DIO0 / DIO1 / DIO2 | 26 / 33 / 32 |
| SPI SCK / MISO / MOSI | 5 / 19 / 27 |
| OLED SDA / SCL | 21 / 22 |
| OLED RST | 4 |

> Les VCC des capteurs sont commutés par GPIO pour économiser l'énergie en deep sleep.

---

## ⚡ Comportement

1. Au réveil, l'ESP32 rejoint le réseau LoRaWAN (OTAA).
2. Après le JOIN, les capteurs sont alimentés, les mesures sont effectuées, et un paquet de **6 octets** est envoyé sur le port 1.
3. L'écran affiche les données pendant **15 secondes**.
4. La carte entre en **deep sleep pendant 1 heure** (3600 s), puis recommence.

---

## 📡 Format du payload LoRaWAN

| Octet | Contenu | Type | Résolution |
|---|---|---|---|
| 0 | Température sonde 1 | `int8_t` | ×10 (ex. `215` → 21.5 °C) |
| 1 | Température sonde 2 | `int8_t` | ×10 |
| 2 | Température sonde 3 | `int8_t` | ×10 |
| 3 | Humidité 10 cm | `uint8_t` | % (0–100) |
| 4 | Humidité 30 cm | `uint8_t` | % (0–100) |
| 5 | Humidité 60 cm | `uint8_t` | % (0–100) |

**Codec JavaScript (ChirpStack) :**
```javascript
function decodeUplink(input) {
  return {
    data: {
      temp1: input.bytes[0] / 10.0,
      temp2: input.bytes[1] / 10.0,
      temp3: input.bytes[2] / 10.0,
      moisture_10cm: input.bytes[3],
      moisture_30cm: input.bytes[4],
      moisture_60cm: input.bytes[5],
    }
  };
}
```

---

## 🛠 Calibration des capteurs d'humidité

Les valeurs ADC brutes sont mappées sur 0–100 % avec les points de référence mesurés :

| Capteur | Valeur à sec | Valeur saturée |
|---|---|---|
| 10 cm | 2975 | 1400 |
| 30 cm | 2470 | 1400 |
| 60 cm | 2960 | 1400 |

Pour recalibrer, mesurer les valeurs ADC brutes à sec et immergé dans l'eau, et mettre à jour les constantes dans `readSensors()`.

---

## 🔧 Configuration PlatformIO

```ini
[env:ttgo-lora32-v1]
platform  = espressif32
board     = ttgo-lora32-v1
framework = arduino
```

**Dépendances :**
- `mcci-catena/MCCI LoRaWAN LMIC library` — stack LoRaWAN
- `milesburton/DallasTemperature` + `paulstoffregen/OneWire` — sondes DS18B20
- `adafruit/Adafruit SSD1306` + `adafruit/Adafruit GFX Library` — écran OLED

**Flags importants :**
- `CFG_eu868=1` — bande EU868
- `CFG_sx1276_radio=1` — radio SX1276 (LilyGO V1)
- `LMIC_USE_INTERRUPTS` — gestion des DIO par interruptions

---

## 🔑 Identifiants LoRaWAN

Les clés OTAA (`DEVEUI`, `APPEUI`, `APPKEY`) sont définies dans `main.cpp` en format **LSB** pour DEVEUI/APPEUI et **MSB** pour APPKEY, conformément à la convention LMIC.

> ⚠️ Ne pas committer les clés en clair dans un dépôt public. Passer par des `#define` dans un fichier `secrets.h` ignoré par `.gitignore`.

---

## 📁 Structure du projet

```
souterrain sonde/
├── src/
│   └── main.cpp
├── platformio.ini
└── .vscode/
    ├── launch.json
    └── extensions.json
```
