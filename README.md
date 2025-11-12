## 📖 Overview
This project focuses on developing a **smart indoor air-quality and occupancy-based ventilation system** using a **standalone ATmega328P IC** (no Arduino board).  
The system continuously monitors air pollutants and room occupancy to automatically adjust fan speed using PWM control, ensuring healthy indoor air and energy-efficient operation.

---

## ⚙️ Hardware Components
- **ATmega328P IC** – Main microcontroller  
- **ENS160 Air Quality Sensor** – Detects CO₂ and VOC levels  
- **OLED Display** – Real-time air-quality and occupancy visualization  
- **IR Sensors** – Detects people entering and leaving the room  
- **L298N Motor Driver** – Drives and controls 12 V DC fan speed  
- **12 V DC Fan** – Ventilation control  
- **SD Card Module** – Logs sensor data for future analysis  
- **RTC DS3231 Module** – Provides accurate date and time for data timestamping  
- **5 V / 12 V Regulated Power Supply** – Stable system operation  

---

## 🧠 System Functions
- Measure air quality using ENS160 (CO₂, VOC levels)  
- Count room occupancy using dual IR sensors  
- Control fan speed through PWM according to air quality and occupancy  
- Display live readings on OLED screen  
- Record timestamped data into SD card via RTC module  
- Maintain compact, low-power hardware design using standalone ATmega328P  

---

## 💡 Technical Highlights
- Integrated **analog and I²C communication** with multiple sensors  
- **PWM fan speed control** based on real-time sensor feedback  
- **Data logging** using SD card and **timestamping** via RTC  
- **OLED-based real-time visualization**  
- Compact embedded design with low power consumption  

---

## ✅ Outcomes
- Developed a **fully functional prototype** capable of monitoring air pollutants and occupancy while automatically regulating ventilation.  
- Enhanced understanding of **sensor calibration, embedded programming, PWM control, and circuit design**.  
- Foundation for future **IoT-enabled smart ventilation systems** with Wi-Fi or LoRa connectivity.

---

## 🧰 Skills & Tools
`Embedded C Programming` • `ATmega328P` • `Sensor Integration` • `PWM Control`  
`I²C Communication` • `RTC Interfacing` • `SD Card Data Logging` • `OLED Display`  
`Circuit Design` • `Power Electronics` • `L298N Motor Control` • `IoT Systems` • `Hardware Debugging`



## 🚀 Future Enhancements
- Transition to **custom PCB design** for compactness and reliability  
- Add **Wi-Fi or LoRa** for IoT-based remote monitoring  
- Use **Li-ion battery** with charger for continuous operation  
- Expand to **multi-room air-quality monitoring** system  

### 🏁 Conclusion
This project demonstrates a complete embedded system capable of monitoring and controlling indoor air quality intelligently using sensor feedback, PWM control, and real-time data logging. It highlights the integration of **hardware, firmware, and environmental intelligence** in one cohesive design.

---

