#
# embedXcode
# ----------------------------------
# Embedded Computing on Xcode 4
#
# Created by Rei VILO 
# Copyright © 2012-2013 http://embedxcode.weebly.com
#
# Last update: Jul 10, 2013 release 59


# Parameters
# ----------------------------------
# Xcode takes BOARD_TAG and BOARD_PORT from the .xcconfig file
# For other IDEs than Xcode,

# BOARD_TAG is required
# If not defined, error BOARD_TAG not defined
#
#BOARD_TAG  = mega2560

# BOARD_PORT is optional
# If not defined, BOARD_PORT = /dev/tty.usb* (default)
#
#BOARD_PORT = /dev/tty.usbmodem*

# Xcode takes SKETCHBOOK_DIR from preferences.txt 
# If not defined, each IDE has its own SKETCHBOOK_DIR (default)
# To share the same SKETCHBOOK_DIR along all IDEs, define
# 
#SKETCHBOOK_DIR =

# SERIAL_BAUDRATE for the serial console, 9600 by default
# Uncomment and specify another speed
#
#SERIAL_BAUDRATE = 19200


# Libraries
# ----------------------------------
# Declare application Arduino/chipKIT/Digispark/Energia/Maple/Teensy/Wiring 
# and users libraries used 
# Short-listing libraries speeds-up building
# Typing = 0 takes none
# Otherwise, leaving empty considers all (default)

# List Arduino/chipKIT/Energia/Maple/Teensy/Wiring libraries here 
#
#APP_LIBS_LIST = Wire Wire/utility EEPROM Ethernet Ethernet/utility \
	SPI Firmata LiquidCrystal Matrix Sprite SD SD/utility Servo SoftwareSerial Stepper 

APP_LIBS_LIST = 0

# List users libraries here 
#
#USER_LIBS_LIST = Serial_LCD I2C_Serial NewSoftSerial
#	I2C_IMU I2C_Accelerometer I2C_Gyroscope I2C_Magnetometer
#	I2C_Climate I2C_Ambient I2C_Barometer I2C_Humidity I2C_Clock2
#	I2C_Clock I2C_Compass I2C_EEPROM
#	I2C_Height_IOs I2C_Potentiometer I2C_Stepper
#	I2C_Pressure I2C_Thermometer 
#	I2C_RGB_LED I2C_RGBC_Reader I2C_Proximity
#	MatrixMath pic32_RTC Time Timers AvrFlash
#	I2C_20x4 Serial_20x4 Serial_GPS Serial_WiFi

USER_LIBS_LIST = 0

# List core, application and user libraries to be excluded
# For example, WiFi may crash on Arduino 1.0.2
# For example, Esplora may crash on Arduino 1.0.3
# For example, Firmata may crash on Teensy 3.0
# For example, OneWire may crash on MPIDE 0023
# For example, HardwareSerial may conflict with MarlinSerial
#
EXCLUDE_LIBS = Firmata WiFi Esplora OneWire Robot_Control Robot_Control/utility Robot_Motor

# Manage path with space in the name
#
UTILITIES_PATH_AS := $(CURDIR)/Utilities
CURDIR:=$(shell echo '$(CURDIR)' | sed 's/ /\\\ /g')

MAKEFILE_PATH  = $(CURDIR)/Makefiles
UTILITIES_PATH = $(CURDIR)/Utilities

include $(MAKEFILE_PATH)/Step1.mk
