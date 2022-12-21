# LCHP-PM-Sensor

This document describes the preliminary design for the Low-cost High-precision Particulate Matter(PM) detector.


The PM detection system consists of a battery-powered particulate matter sensing unit and a smartphone application. The smartphone APP is based on Android, and is responsible for viewing data and setting some parameters for the sensing unit. The PM sensing unit is based on Arduino MKR WiFi 1010 board and uses three PM sensors, one temperature sensor, one Bluetooth low energy(BLE) module, one Real time clock (RTC) module, and one SD card module. The use of three PM sensors is to eliminate part-to-part variation, and each sensor will be calibrated using machine learning similar to the way that Barcelo-Ordinas et al. have done [1]. The use of temperature & humidity sensors is to offset the variations caused by temperature and humidity differences [1],[2]. Data processing algorithms and supporting firmware run on Arduino to realize the required sensing and data processing functionality.
