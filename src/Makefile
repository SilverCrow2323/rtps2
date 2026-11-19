EE_BIN = NEXUS.ELF
IRX_FILES += iomanX.irx fileXio.irx
EE_OBJS = nexus_main.o nexus_input.o nexus_ui.o nexus_renderer.o nexus_atlas.o nexus_cover_cache.o nexus_cover_cache_policy.o nexus_library.o nexus_library_scan.o nexus_filexio.o nexus_actions.o nexus_sha256.o nexus_asset_blob.o nexus_devices.o nexus_navigation.o $(IRX_FILES:.irx=_irx.o)
EE_INCS += -Iinclude
EE_LIBS = -lfileXio -lpatches -lfont -lpacket -ldma -lgraph -ldraw -lpad -ldebug -lc

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS) NEXUS.map

.PHONY: all clean

# Embed IOMANX and fileXio IRXs in the ELF; activation remains explicit.
%_irx.c:
	$(PS2SDK)/bin/bin2c $(PS2SDK)/iop/irx/$*.irx $@ $*_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
