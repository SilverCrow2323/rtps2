EE_BIN = NEXUS.ELF
EE_OBJS = nexus_main.o nexus_input.o nexus_ui.o nexus_devices.o nexus_navigation.o
EE_INCS += -Iinclude
EE_LIBS = -lpad -ldebug -lc

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS) NEXUS.map

.PHONY: all clean

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
