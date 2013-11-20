#install from github.com/sudar/Arduino-Makefile
ARDUINO_MK_DIR   = /home/cantora/install/Arduino-Makefile
BOARD_TAG        = atmega8
ARDUINO_PORT     = /dev/ttyUSB1

CLIENT_SRC_DIR   = ../src
EXTRA_CFLAGS     := -iquote$(CLIENT_SRC_DIR)
EXTRA_CXXFLAGS   := $(EXTRA_CFLAGS)

OTHER_OBJS       = $(OBJDIR)/proto_encode.o

include $(ARDUINO_MK_DIR)/arduino-mk/Arduino.mk

$(OBJDIR)/proto_encode.o: $(CLIENT_SRC_DIR)/proto_encode.c $(COMMON_DEPS) | $(OBJDIR)
	@$(MKDIR) $(dir $@)
	$(CC) -MMD -c $(CPPFLAGS) $(CFLAGS) $< -o $@
