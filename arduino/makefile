#install from github.com/sudar/Arduino-Makefile
#ARDUINO_DIR      = /home/cantora/install/Arduino
ARDUINO_MK_DIR   = /home/cantora/install/Arduino-Makefile
BOARD_TAG        = atmega8
ARDUINO_PORT     = /dev/ttyUSB0

TARGET           = pipeduino
CLIENT_SRC_DIR   = ../src
EXTRA_CFLAGS     := -iquote$(CLIENT_SRC_DIR)
EXTRA_CXXFLAGS   := $(EXTRA_CFLAGS)

include $(ARDUINO_MK_DIR)/arduino-mk/Arduino.mk
