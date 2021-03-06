#
# embedXcode
# ----------------------------------
# Embedded Computing on Xcode 4
#
# Created by Rei VILO 
# Copyright © 2012-2013 http://embedxcode.weebly.com
#
# Last update: Jun 21, 2013 release 54

# References and contribution
# ----------------------------------
# See About folder
# 


# Arduino 1.0 specifics
# ----------------------------------
#
PLATFORM         := Arduino
PLATFORM_TAG     := ARDUINO=104 EMBEDXCODE=$(RELEASE)
APPLICATION_PATH := /Applications/Arduino.app/Contents/Resources/Java

APP_TOOLS_PATH   := $(APPLICATION_PATH)/hardware/tools/avr/bin
CORE_LIB_PATH    := $(APPLICATION_PATH)/hardware/arduino/cores/Arduino
APP_LIB_PATH     := $(APPLICATION_PATH)/libraries
BOARDS_TXT       := $(APPLICATION_PATH)/hardware/arduino/boards.txt

# Sketchbook/Libraries path
# wildcard required for ~ management
#
ifeq ($(USER_PATH)/Library/Arduino/preferences.txt,)
    $(error Error: run Arduino once and define sketchbook path)
endif

ifeq ($(wildcard $(SKETCHBOOK_DIR)),)
    SKETCHBOOK_DIR = $(shell grep sketchbook.path $(USER_PATH)/Library/Arduino/preferences.txt | cut -d = -f 2)
endif
ifeq ($(wildcard $(SKETCHBOOK_DIR)),)
   $(error Error: sketchbook path not found)
endif
USER_LIB_PATH  = $(wildcard $(SKETCHBOOK_DIR)/?ibraries)

# Rules for making a c++ file from the main sketch (.pde)
#
PDEHEADER      = \\\#include \"Arduino.h\"  

# Tool-chain names
#
CC      = $(APP_TOOLS_PATH)/avr-gcc
CXX     = $(APP_TOOLS_PATH)/avr-g++
AR      = $(APP_TOOLS_PATH)/avr-ar
OBJDUMP = $(APP_TOOLS_PATH)/avr-objdump
OBJCOPY = $(APP_TOOLS_PATH)/avr-objcopy
SIZE    = $(APP_TOOLS_PATH)/avr-size
NM      = $(APP_TOOLS_PATH)/avr-nm

# Specific AVRDUDE location and options
#
AVRDUDE_COM_OPTS  = -D -p$(MCU) -C$(AVRDUDE_CONF)

BOARD        = $(call PARSE_BOARD,$(BOARD_TAG),board)
#LDSCRIPT     = $(call PARSE_BOARD,$(BOARD_TAG),ldscript)
VARIANT      = $(call PARSE_BOARD,$(BOARD_TAG),build.variant)
VARIANT_PATH = $(APPLICATION_PATH)/hardware/arduino/variants/$(VARIANT)

MCU_FLAG_NAME  = mmcu
EXTRA_LDFLAGS  = 
EXTRA_CPPFLAGS = -MMD -I$(VARIANT_PATH) $(addprefix -D, $(PLATFORM_TAG))

# Leonardo USB PID VID
#
USB_TOUCH := $(call PARSE_BOARD,$(BOARD_TAG),upload.protocol)
USB_VID   := $(call PARSE_BOARD,$(BOARD_TAG),build.vid)
USB_PID   := $(call PARSE_BOARD,$(BOARD_TAG),build.pid)

ifneq ($(USB_PID),)
    USB_FLAGS += -DUSB_PID=$(USB_PID)
else
    USB_FLAGS += -DUSB_PID=null
endif

ifneq ($(USB_VID),)
    USB_FLAGS += -DUSB_VID=$(USB_VID)
else
    USB_FLAGS += -DUSB_VID=null
endif

# Serial 1200 reset
#
ifeq ($(USB_TOUCH),avr109)
    USB_RESET  = $(UTILITIES_PATH)/serial1200.py
endif
