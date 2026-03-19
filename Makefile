CC = gcc
CFLAGS = -Iinclude -DINCLUDEALL -O2 -Wall

SRC = prowiz.c \
misc/misc.c \
misc/testbag.c \
r/ZenPacker.c \
r/AMOS-MusicBank.c \
r/AMOS-PPBk.c \
r/BP.c \
r/BSI-FutureComposer.c \
r/Bytekiller.c \
r/ChipTracker.c \
r/Crunchmania-Address.c \
r/Crunchmania-Simple.c \
r/datacrunchers.c \
r/DefjamCr.c \
r/DigiBooster17.c \
r/DigitalIllusion.c \
r/DoubleAction.c \
r/Dragpack100.c \
r/Dragpack252.c \
r/EurekaPacker.c \
r/FT2.c \
r/FC-M-Packer.c \
r/FuchsTracker.c \
r/THX.c \
r/FC14.c \
r/FuzzacPacker.c \
r/GMC.c \
r/GNUPacker12.c \
r/GnuPlayer.c \
r/GPMO.c \
r/HeatseekerMC10.c \
r/HornetPacker.c \
r/HQC2.c \
r/JamCracker.c \
r/TitanicsPlayer.c \
r/MastCrunch30-Add.c \
r/MaxPacker12.c \
r/MED-Octamed.c \
r/MegaCruncher.c \
r/MegaCruncher-Obj.c \
r/MOD-compatible.c \
r/ModuleProtector.c \
r/Mugician.c \
r/Newtron.c \
r/NoiseFromHeaven.c \
r/NP1.c \
r/NP2.c \
r/NP3.c \
r/NoiseRunner.c \
r/PhaPacker.c \
r/PolkaPacker.c \
r/PowerMusic.c \
r/PowerPacker23.c \
r/PowerPacker30.c \
r/PowerPacker40.c \
r/PowerPacker40-lib.c \
r/Promizer01.c \
r/Promizer10c.c \
r/Promizer18a.c \
r/Promizer20.c \
r/Promizer40.c \
r/ProPacker10.c \
r/ProPacker21.c \
r/ProPacker30.c \
r/Prorunner10.c \
r/Prorunner20.c \
r/QC.c \
r/RelokIt10.c \
r/SGT-Packer.c \
r/SkytPacker.c \
r/SoundFX.c \
r/SoundTracker.c \
r/SoundTracker26.c \
r/SpikeCruncher.c \
r/StartrekkerPack.c \
r/StimPacker.c \
r/StoneCracker270.c \
r/StoneCracker299.c \
r/StoneCracker299b.c \
r/StoneCracker299d.c \
r/StoneCracker300.c \
r/StoneCracker310.c \
r/SuperCruncher27.c \
r/SyncroPacker46.c \
r/TetraPack1.c \
r/TetraPack21.c \
r/TetraPack22.c \
r/TheDarkDemon.c \
r/TP22a.c \
r/TP30a.c \
r/TP40.c \
r/TP41a.c \
r/TP50a.c \
r/TP60a.c \
r/TP61a.c \
r/TimeCruncher17.c \
r/TMK.c \
r/TNMCruncher.c \
r/TrackerPacker1.c \
r/TrackerPacker2.c \
r/TrackerPacker3.c \
r/TryItCruncher101.c \
r/TurboSqueezer61.c \
r/UnicTracker.c \
r/UnicTracker2.c \
r/WantonPacker.c \
r/AC1D_packer.c \
r/KSM.c \
r/NewtronOld.c \
r/NovoTrade.c \
r/skizzo.c \
r/StoneArtsPlayer.c \
r/SlamPacker.c \
r/S3M.c \
r/mosh.c \
r/HCD.c \
r/struggle.c \
r/AMF.c \
r/okta.c \
r/FC13.c \
r/sidmon1.c \
r/sidmon2.c \
r/Perfsong.c \
r/IT.c \
r/HighPresCr.c \
r/bnr.c \
r/XannPlayer.c \
r/IFF.c \
r/SA.c \
r/DM1.c \
r/pmd3.c

OBJ = $(SRC:.c=.o)

BIN = prowiz

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)
