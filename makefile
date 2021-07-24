TARGET = DsEmuPsp
OBJS =\
	./desmume_core/arm_instructions.o \
	./desmume_core/armcpu.o \
	./desmume_core/bios.o \
	./desmume_core/cflash.o \
	./desmume_core/cp15.o \
	./desmume_core/ctrlssdl.o \
	./desmume_core/Disassembler.o \
	./desmume_core/decrypt.o \
	./desmume_core/FIFO.o \
	./desmume_core/fs-psp.o \
	./desmume_core/gfx3d.o \
	./desmume_core/GPU.o \
	./desmume_core/matrix_psp.o \
	./desmume_core/mc.o \
	./desmume_core/MMU.o \
	./desmume_core/NDSSystem.o \
	./desmume_core/render3D.o \
	./desmume_core/rtc.o \
	./desmume_core/ROMReader.o \
	./desmume_core/saves.o \
	./desmume_core/sndsdl.o \
	./desmume_core/SPU.o \
	./desmume_core/thumb_instructions.o \
	./PSP/Gudraw.o \
	./PSP/vram.o \
	./PSP/SystemButtons.o \
	./PSP/memcpy_vfpu.o \
	./PSP/FrontEnd.o \
	./PSP/main.o


#./desmume_core/matrix.o \
#	./desmume_core/wifi.o \
#	./desmume_core/debug.o \


#ASM      = psp-as
#CC       = psp-gcc

PSP_FW_VERSION = 371

INCDIR =  ./PSP ./desmume_core C:/pspdev/psp/include/SDL/
CFLAGS = -O3 -G0 -Wall -DPSP -D__psp__

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti 
#psp-as ./desmume_core/matrix_psp_asm.s -o
ASFLAGS = $(CFLAGS)
#$(ASM) ./desmume_core/matrix.s -o ./desmume_core/matrix.o \

#LIBDIR = ./SDL
LDFLAGS =
LIBS = -lpspwlan -lSDL -lGL -lGLU -glut -lpspvfpu -lpspgum_vfpu -lpspgu -lpspge -lpspirkeyb -lpsphprm -lpsppower -lpspaudio -lpsprtc -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = DsEmuPsp v0.1 by RNB_PSP
PSP_EBOOT_ICON  = ./icon/icon0.png
PSP_EBOOT_PIC1 = ./icon/pic1.png

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
