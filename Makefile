CC = gcc
CFLAGS = -Iinclude -DINCLUDEALL -O2 -Wall
AR = ar

# --- common sources (scan driver, library API, utilities) ---
SRC_COMMON = \
prowiz_scan.c \
prowiz_lib.c \
misc/misc.c \
misc/testbag.c

# --- rip mode library (module format handlers) ---
SRC_RIP = \
rip/prowiz_scan_rip.c \
rip/AC1D_packer.c \
rip/AMF.c \
rip/AMOS-MusicBank.c \
rip/AMOS-PPBk.c \
rip/BP.c \
rip/BSI-FutureComposer.c \
rip/ChipTracker.c \
rip/Crunchmania-Address.c \
rip/Crunchmania-Simple.c \
rip/DigiBooster17.c \
rip/DigitalIllusion.c \
rip/DoubleAction.c \
rip/DM1.c \
rip/EurekaPacker.c \
rip/FC-M-Packer.c \
rip/FC13.c \
rip/FC14.c \
rip/FT2.c \
rip/FuchsTracker.c \
rip/FuzzacPacker.c \
rip/GMC.c \
rip/GnuPlayer.c \
rip/GPMO.c \
rip/HCD.c \
rip/HeatseekerMC10.c \
rip/HornetPacker.c \
rip/IT.c \
rip/JamCracker.c \
rip/KSM.c \
rip/MED-Octamed.c \
rip/MOD-compatible.c \
rip/ModuleProtector.c \
rip/mosh.c \
rip/Mugician.c \
rip/Newtron.c \
rip/NewtronOld.c \
rip/NoiseFromHeaven.c \
rip/NoiseRunner.c \
rip/NovoTrade.c \
rip/NP1.c \
rip/NP2.c \
rip/NP3.c \
rip/okta.c \
rip/Perfsong.c \
rip/PhaPacker.c \
rip/pmd3.c \
rip/PolkaPacker.c \
rip/PowerMusic.c \
rip/Promizer01.c \
rip/Promizer10c.c \
rip/Promizer18a.c \
rip/Promizer20.c \
rip/Promizer40.c \
rip/ProPacker10.c \
rip/ProPacker21.c \
rip/ProPacker30.c \
rip/Prorunner10.c \
rip/Prorunner20.c \
rip/QC.c \
rip/S3M.c \
rip/SA.c \
rip/SGT-Packer.c \
rip/sidmon1.c \
rip/sidmon2.c \
rip/skizzo.c \
rip/SlamPacker.c \
rip/SoundFX.c \
rip/SoundTracker.c \
rip/SoundTracker26.c \
rip/StartrekkerPack.c \
rip/StimPacker.c \
rip/StoneArtsPlayer.c \
rip/struggle.c \
rip/TheDarkDemon.c \
rip/THX.c \
rip/TitanicsPlayer.c \
rip/TMK.c \
rip/TP22a.c \
rip/TP30a.c \
rip/TP40.c \
rip/TP41a.c \
rip/TP50a.c \
rip/TP60a.c \
rip/TP61a.c \
rip/TrackerPacker1.c \
rip/TrackerPacker2.c \
rip/TrackerPacker3.c \
rip/UnicTracker.c \
rip/UnicTracker2.c \
rip/WantonPacker.c \
rip/XannPlayer.c \
rip/ZenPacker.c \
rip/bnr.c

# --- data mode library (data cruncher handlers) ---
SRC_DATA = \
data/prowiz_scan_data.c \
data/Bytekiller.c \
data/datacrunchers.c \
data/DefjamCr.c \
data/Dragpack100.c \
data/Dragpack252.c \
data/GNUPacker12.c \
data/HighPresCr.c \
data/HQC2.c \
data/IFF.c \
data/MastCrunch30-Add.c \
data/MaxPacker12.c \
data/MegaCruncher.c \
data/MegaCruncher-Obj.c \
data/PowerPacker23.c \
data/PowerPacker30.c \
data/PowerPacker40.c \
data/PowerPacker40-lib.c \
data/RelokIt10.c \
data/SkytPacker.c \
data/SpikeCruncher.c \
data/StoneCracker270.c \
data/StoneCracker299.c \
data/StoneCracker299b.c \
data/StoneCracker299d.c \
data/StoneCracker300.c \
data/StoneCracker310.c \
data/SuperCruncher27.c \
data/SyncroPacker46.c \
data/TetraPack1.c \
data/TetraPack21.c \
data/TetraPack22.c \
data/TimeCruncher17.c \
data/TNMCruncher.c \
data/TryItCruncher101.c \
data/TurboSqueezer61.c

OBJ_COMMON = $(SRC_COMMON:.c=.o)
OBJ_RIP = $(SRC_RIP:.c=.o)
OBJ_DATA = $(SRC_DATA:.c=.o)

BIN = prowiz

all: $(BIN)

# --- static libraries ---
libprowiz_rip.a: $(OBJ_COMMON) $(OBJ_RIP)
	$(AR) rcs $@ $^

libprowiz_data.a: $(OBJ_COMMON) $(OBJ_DATA)
	$(AR) rcs $@ $^

libs: libprowiz_rip.a libprowiz_data.a

# --- CLI binary (links both libraries) ---
$(BIN): prowiz.o libprowiz_rip.a libprowiz_data.a
	$(CC) prowiz.o -L. -lprowiz_rip -lprowiz_data -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_COMMON) $(OBJ_RIP) $(OBJ_DATA) prowiz.o $(BIN) libprowiz_rip.a libprowiz_data.a

# WASM build — requires Emscripten (emcc)
WASM_OUT = packages/prowiz-wasm/wasm
WASM_SOURCES = prowiz.c $(SRC_COMMON) $(SRC_RIP) $(SRC_DATA)

wasm:
	mkdir -p $(WASM_OUT)
	emcc $(WASM_SOURCES) \
	  -Iinclude -DINCLUDEALL -O2 \
	  -s EXPORTED_FUNCTIONS='["_prowiz_scan","_prowiz_extract","_malloc","_free"]' \
	  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU8","UTF8ToString"]' \
	  -s ALLOW_MEMORY_GROWTH=1 \
	  -s MODULARIZE=1 \
	  -s EXPORT_NAME="ProwizWasm" \
	  --no-entry \
	  -o $(WASM_OUT)/prowiz.js
