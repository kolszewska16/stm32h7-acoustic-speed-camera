# stm32h7-acoustic-speed-camera
Real-time acoustic event logger on STM32H723ZG. Hardware-accelerated PDM acquisition and CMSIS-DSP analysis. Features automatic event triggering with SD card logging and camera integration.

### Overview
This project is a system for monitoring and analyzing traffic noise in real-time. It doesn't just measure decibels — it "understands" the frequency spectrum and takes action. When a noise threshold is violated, the system triggers a camera and logs the event data (photo) to an SD card.

### Tech Stack & Key Features
* **MCU:** STM32H723ZG (Cortex-M7)
* **Audio:** Digital PDM MEMS microphones via **DFSDM**
* **OS:** **FreeRTOS** for multitasking
* **Storage:** **SDMMC + FatFs** for high-speed event logging