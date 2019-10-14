# Various compile-time options.
ifndef STUFF
STUFF		:=
endif

# Add feature selections here.
ifndef EXTRAS
EXTRAS		:=
endif

ifndef DEV_BUILD
DEV_BUILD	:= n
endif
ifndef FLTO
FLTO		:= full
endif

ifeq ($(DEV_BUILD), y)
 ifndef DEBUG
  DEBUG		:= y
 endif
 ifndef DEV_BRANCH
  DEV_BRANCH	:= y
 endif
 ifndef AMD_K
  AMD_K		:= y
 endif
 ifndef CRASHDUMP
  CRASHDUMP	:= y
 endif
 ifndef I686
  I686		:= y
 endif
 ifndef LASERXT
  LASERXT	:= y
 endif
 ifndef MRTHOR
  MRTHOR	:= y
 endif
 ifndef PAS16
  PAS16		:= y
 endif
 ifndef PORTABLE3
  PORTABLE3	:= y
 endif
 ifndef PS2M70T4
  PS2M70T4	:= y
 endif
 ifndef VNC
  VNC		:= y
 endif
 ifndef XL24
  XL24		:= y
 endif
else
 ifndef DEBUG
  DEBUG		:= n
 endif
 ifndef DEV_BRANCH
  DEV_BRANCH	:= n
 endif
 ifndef AMD_K
  AMD_K		:= n
 endif
 ifndef CRASHDUMP
  CRASHDUMP	:= n
 endif
 ifndef I686
  I686		:= n
 endif
 ifndef LASERXT
  LASERXT	:= n
 endif
 ifndef MRTHOR
  MRTHOR	:= n
 endif
 ifndef PAS16
  PAS16		:= n
 endif
 ifndef PORTABLE3
  PORTABLE3	:= n
 endif
 ifndef PS2M70T4
  PS2M70T4	:= n
 endif
 ifndef VGAWONDER
  VGAWONDER	:= n
 endif
 ifndef VNC
  VNC		:= n
 endif
 ifndef XL24
  XL24		:= n
 endif
endif

# Defaults for several build options (possibly defined in a chained file.)
ifndef AUTODEP
AUTODEP		:= n
endif
ifndef OPTIM
OPTIM		:= n
endif
ifndef RELEASE
RELEASE		:= n
endif
UNAME_M := $(shell uname -m)
ifndef X64
	ifeq ($(UNAME_M),x86_64)
		X64		:= y
	else
		X64		:= n
	endif
endif
ifndef USB
USB		:= n
endif
ifndef OPENAL
OPENAL		:= y
endif
ifndef FLUIDSYNTH
FLUIDSYNTH	:= y
endif
ifndef MUNT
MUNT		:= y
endif
ifndef DYNAREC
 DYNAREC		:= y
endif


# Name of the executable.
ifndef PROG
  PROG		:= 86Box
endif

#########################################################################
#		Nothing should need changing from here on..		#
#########################################################################
VPATH		:= $(EXPATH) . cpu \
		   cdrom chipset disk floppy game machine \
		   printer \
		   sound \
		    sound/munt sound/munt/c_interface sound/munt/sha1 \
		    sound/munt/srchelper \
		    sound/resid-fp \
		   scsi video network network/slirp linux
ifeq ($(X64), y)
CPP		:= g++ -m64
CC		:= gcc -m64
else
CPP		:= g++ -m32
CC		:= gcc -m32
endif
STRIP := strip
DEPS		= -MMD -MF $*.d -c $<
DEPFILE		:= linux/.depends

# Set up the correct toolchain flags.
OPTS		:= $(EXTRAS) $(STUFF)
ifdef EXFLAGS
OPTS		+= $(EXFLAGS)
endif
ifdef EXINC
OPTS		+= -I$(EXINC)
endif

ifeq ($(X64), y)
 ifeq ($(OPTIM), y)
  DFLAGS	:= -march=native
 else
  DFLAGS	:=
 endif
else
 ifeq ($(OPTIM), y)
  DFLAGS	:= -march=native
 else
  DFLAGS	:= -march=i686
 endif
endif
ifeq ($(DEBUG), y)
 DFLAGS		+= -ggdb -DDEBUG
 AOPTIM		:=
 ifndef COPTIM
  COPTIM	:= -Og
 endif
else
 DFLAGS		+= -g0
 ifeq ($(OPTIM), y)
  AOPTIM	:= -mtune=native
  ifndef COPTIM
   COPTIM	:= -O3 -flto=$(FLTO)
  endif
 else
  ifndef COPTIM
   COPTIM	:= -O3
  endif
 endif
endif
AFLAGS		:= -msse2 -mfpmath=sse
ifeq ($(RELEASE), y)
OPTS		+= -DRELEASE_BUILD
endif
ifeq ($(VRAMDUMP), y)
OPTS		+= -DENABLE_VRAM_DUMP
endif
ifeq ($(X64), y)
PLATCG		:= codegen_x86-64.o
CGOPS		:= codegen_ops_x86-64.h
VCG		:= vid_voodoo_codegen_x86-64.h
else
PLATCG		:= codegen_x86.o
CGOPS		:= codegen_ops_x86.h
VCG		:= vid_voodoo_codegen_x86.h
endif

# Mandatory POSIX environment setup
OPTS		+= $(shell pkg-config --cflags freetype2)

# Optional modules.
ifeq ($(DYNAREC), y)
OPTS		+= -DUSE_DYNAREC
DYNARECOBJ	:= 386_dynarec_ops.o \
		    codegen.o \
		    codegen_ops.o \
		    codegen_timing_common.o codegen_timing_486.o \
		    codegen_timing_686.o codegen_timing_pentium.o \
		    codegen_timing_winchip.o $(PLATCG)
endif

ifeq ($(OPENAL), y)
OPTS		+= -DUSE_OPENAL
endif
ifeq ($(FLUIDSYNTH), y)
OPTS		+= -DUSE_FLUIDSYNTH
FSYNTHOBJ	:= midi_fluidsynth.o
endif

ifeq ($(MUNT), y)
OPTS		+= -DUSE_MUNT
MUNTOBJ		:= midi_mt32.o \
		    Analog.o BReverbModel.o File.o FileStream.o LA32Ramp.o \
		    LA32FloatWaveGenerator.o LA32WaveGenerator.o \
		    MidiStreamParser.o Part.o Partial.o PartialManager.o \
		    Poly.o ROMInfo.o SampleRateConverter_dummy.o Synth.o \
		    Tables.o TVA.o TVF.o TVP.o sha1.o c_interface.o
endif

ifeq ($(VNC), y)
OPTS		+= -DUSE_VNC
 ifneq ($(VNC_PATH), )
  OPTS		+= -I$(VNC_PATH)\INCLUDE
  VNCLIB	:= -L$(VNC_PATH)\LIB
 endif
VNCLIB		+= -lvncserver
VNCOBJ		:= vnc.o vnc_keymap.o
endif

# Options for the DEV branch.
ifeq ($(DEV_BRANCH), y)
OPTS		+= -DDEV_BRANCH
DEVBROBJ	:=

ifeq ($(AMD_K), y)
OPTS		+= -DUSE_AMD_K
endif

ifeq ($(CRASHDUMPOBJ), y)
OPTS		+= -DUSE_CRASHDUMP
endif

ifeq ($(I686), y)
OPTS		+= -DUSE_I686
endif

ifeq ($(LASERXT), y)
OPTS		+= -DUSE_LASERXT
DEVBROBJ	+= m_xt_laserxt.o
endif

ifeq ($(MRTHOR), y)
OPTS		+= -DUSE_MRTHOR
endif

ifeq ($(PAS16), y)
OPTS		+= -DUSE_PAS16
DEVBROBJ	+= snd_pas16.o
endif

ifeq ($(PORTABLE3), y)
OPTS		+= -DUSE_PORTABLE3
endif

ifeq ($(PS2M70T4), y)
OPTS		+= -DUSE_PS2M70T4
endif

ifeq ($(VGAWONDER), y)
OPTS		+= -DUSE_VGAWONDER
endif

ifeq ($(XL24), y)
OPTS		+= -DUSE_XL24
endif

endif


# Options for works-in-progress.
ifndef SERIAL
SERIAL		:= serial.o
endif


# Final versions of the toolchain flags.
CFLAGS		:= $(OPTS) $(DFLAGS) $(COPTIM) $(AOPTIM) \
		   $(AFLAGS) -fomit-frame-pointer -mstackrealign -Wall \
		   -fno-strict-aliasing
CXXFLAGS	:= $(CFLAGS)


#########################################################################
#		Create the (final) list of objects to build.		#
#########################################################################
MAINOBJ		:= pc.o config.o random.o timer.o io.o apm.o dma.o nmi.o \
		   pic.o pit.o port_92.o ppi.o pci.o mca.o mcr.o mem.o \
		   rom.o device.o nvr.o nvr_at.o nvr_ps2.o $(VNCOBJ) $(RDPOBJ)

INTELOBJ	:= intel_flash.o \
		    intel_sio.o intel_piix.o

CPUOBJ		:= cpu.o cpu_table.o \
		    808x.o 386.o \
		    386_dynarec.o \
		    x86seg.o x87.o \
		    $(DYNARECOBJ)

CHIPSETOBJ	:= acc2168.o acer_m3a.o ali1429.o headland.o \
		    intel_4x0.o neat.o opti495.o scat.o \
		    sis_85c471.o sis_85c496.o \
		    wd76c10.o

MCHOBJ		:= machine.o machine_table.o \
		    m_xt.o m_xt_compaq.o \
		    m_xt_t1000.o m_xt_t1000_vid.o \
		    m_xt_xi8088.o m_xt_zenith.o \
		    m_pcjr.o \
		    m_amstrad.o m_europc.o \
		    m_olivetti_m24.o m_tandy.o \
		    m_at.o m_at_commodore.o \
		    m_at_t3100e.o m_at_t3100e_vid.o \
		    m_ps1.o m_ps1_hdc.o \
		    m_ps2_isa.o m_ps2_mca.o \
		    m_at_compaq.o \
		    m_at_286_386sx.o m_at_386dx_486.o \
		    m_at_socket4_5.o m_at_socket7_s7.o

DEVOBJ		:= bugger.o ibm_5161.o isamem.o isartc.o lpt.o $(SERIAL) \
		    sio_acc3221.o \
		    sio_fdc37c66x.o sio_fdc37c669.o \
		    sio_fdc37c93x.o \
		    sio_pc87306.o sio_w83877f.o \
		    sio_um8669f.o \
		   keyboard.o \
		    keyboard_xt.o keyboard_at.o \
		   gameport.o \
		    joystick_standard.o joystick_ch_flightstick_pro.o \
		    joystick_sw_pad.o joystick_tm_fcs.o \
		   mouse.o \
		    mouse_bus.o \
		    mouse_serial.o mouse_ps2.o

FDDOBJ		:= fdd.o fdc.o fdi2raw.o \
		   fdd_common.o fdd_86f.o \
		   fdd_fdi.o fdd_imd.o fdd_img.o fdd_json.o \
		   fdd_mfm.o fdd_td0.o

HDDOBJ		:= hdd.o \
		    hdd_image.o hdd_table.o \
		   hdc.o \
		    hdc_st506_xt.o hdc_st506_at.o \
		    hdc_xta.o \
		    hdc_esdi_at.o hdc_esdi_mca.o \
		    hdc_xtide.o hdc_ide.o \
		    hdc_ide_sff8038i.o

CDROMOBJ	:= cdrom.o \
		    cdrom_dosbox.o cdrom_image.o

ZIPOBJ		:= zip.o

ifeq ($(USB), y)
USBOBJ		:= usb.o
endif

SCSIOBJ		:= scsi.o scsi_device.o \
		    scsi_cdrom.o scsi_disk.o \
		    scsi_x54x.o \
		    scsi_aha154x.o scsi_buslogic.o \
		    scsi_ncr5380.o scsi_ncr53c8xx.o

NETOBJ		:= network.o \
		    net_pcap.o \
		    net_slirp.o \
		     bootp.o ip_icmp.o misc.o socket.o tcp_timer.o cksum.o \
		     ip_input.o queue.o tcp_input.o debug.o ip_output.o \
		     sbuf.o tcp_output.o udp.o if.o mbuf.o slirp.o tcp_subr.o \
		    net_dp8390.o \
		    net_3c503.o net_ne2000.o \
			net_wd8003.o

PRINTOBJ	:= png.o prt_cpmap.o \
		    prt_escp.o prt_text.o			
			
SNDOBJ		:= sound.o \
		    openal.o \
		    snd_opl.o snd_dbopl.o \
		    dbopl.o nukedopl.o \
		    snd_resid.o \
		     convolve.o convolve-sse.o envelope.o extfilt.o \
		     filter.o pot.o sid.o voice.o wave6581__ST.o \
		     wave6581_P_T.o wave6581_PS_.o wave6581_PST.o \
		     wave8580__ST.o wave8580_P_T.o wave8580_PS_.o \
		     wave8580_PST.o wave.o \
		    midi.o midi_system.o \
		    snd_speaker.o \
		    snd_pssj.o \
		    snd_lpt_dac.o snd_lpt_dss.o \
		    snd_adlib.o snd_adlibgold.o snd_ad1848.o snd_audiopci.o \
		    snd_cms.o \
		    snd_gus.o \
		    snd_sb.o snd_sb_dsp.o \
		    snd_emu8k.o snd_mpu401.o \
		    snd_sn76489.o snd_ssi2001.o \
		    snd_wss.o \
		    snd_ym7128.o

VIDOBJ		:= video.o \
		    vid_table.o \
		    vid_cga.o vid_cga_comp.o \
		    vid_compaq_cga.o \
		    vid_mda.o \
		    vid_hercules.o vid_herculesplus.o vid_incolor.o \
		    vid_colorplus.o \
		    vid_genius.o \
		    vid_pgc.o vid_im1024.o \
		    vid_sigma.o \
		    vid_wy700.o \
		    vid_ega.o vid_ega_render.o \
		    vid_svga.o vid_svga_render.o \
		    vid_vga.o \
		    vid_ati_eeprom.o \
		    vid_ati18800.o vid_ati28800.o \
		    vid_ati_mach64.o vid_ati68860_ramdac.o \
		    vid_bt48x_ramdac.o \
		    vid_av9194.o \
		    vid_icd2061.o vid_ics2595.o \
		    vid_cl54xx.o \
		    vid_et4000.o vid_sc1502x_ramdac.o \
		    vid_et4000w32.o vid_stg_ramdac.o \
		    vid_ht216.o \
		    vid_oak_oti.o \
		    vid_paradise.o \
		    vid_ti_cf62011.o \
		    vid_tvga.o \
		    vid_tgui9440.o vid_tkd8001_ramdac.o \
		    vid_att20c49x_ramdac.o \
		    vid_s3.o vid_s3_virge.o \
		    vid_sdac_ramdac.o \
		    vid_voodoo.o

PLATOBJ		:= linux.o linux_thread.o linux_dynld.o linux_sdl.o linux_stub.o

OBJ		:= $(MAINOBJ) $(INTELOBJ) $(CPUOBJ) $(CHIPSETOBJ) $(MCHOBJ) \
		   $(DEVOBJ) $(FDDOBJ) $(CDROMOBJ) $(ZIPOBJ) $(HDDOBJ) \
		   $(USBOBJ) $(NETOBJ) $(PRINTOBJ) $(SCSIOBJ) $(SNDOBJ) $(VIDOBJ) \
		   $(PLATOBJ) $(UIOBJ) $(D2DOBJ) $(FSYNTHOBJ) $(MUNTOBJ) \
		   $(DEVBROBJ)
ifdef EXOBJ
OBJ		+= $(EXOBJ)
endif

ifeq ($(VNC), y)
LIBS		+= $(VNCLIB)
endif
LIBS		+= -lSDL2 -ldl
LIBS		+= -lopenal -lpng -lz -lm -lpthread -lstdc++

# Build module rules.
ifeq ($(AUTODEP), y)
%.o:		%.c
		@echo $<
		@$(CC) $(CFLAGS) $(DEPS) -c $<

%.o:		%.cc
		@echo $<
		@$(CPP) $(CXXFLAGS) $(DEPS) -c $<

%.o:		%.cpp
		@echo $<
		@$(CPP) $(CXXFLAGS) $(DEPS) -c $<
else
%.o:		%.c
		@echo $<
		@$(CC) $(CFLAGS) -c $<

%.o:		%.cc
		@echo $<
		@$(CPP) $(CXXFLAGS) -c $<

%.o:		%.cpp
		@echo $<
		@$(CPP) $(CXXFLAGS) -c $<

%.d:		%.c $(wildcard $*.d)
		@echo $<
		@$(CC) $(CFLAGS) $(DEPS) -E $< >NUL

%.d:		%.cc $(wildcard $*.d)
		@echo $<
		@$(CPP) $(CXXFLAGS) $(DEPS) -E $< >NUL

%.d:		%.cpp $(wildcard $*.d)
		@echo $<
		@$(CPP) $(CXXFLAGS) $(DEPS) -E $< >NUL
endif

all:		$(OBJ)
link:		$(PROG)



$(PROG):	$(OBJ)
		@echo Linking $(PROG) ..
		@$(CC) -o $(PROG) $(OBJ) $(LIBS)
ifneq ($(DEBUG), y)
		@$(STRIP) $(PROG)
endif



clean:
		@echo Cleaning objects..
		@-rm -f *.o 2>NUL
		@-rm -f *.res 2>NUL

clobber:	clean
		@echo Cleaning executables..
		@-rm -f *.d 2>NUL
		@-rm -f $(PROG) 2>NUL
#		@-rm -f $(DEPFILE) 2>NUL

ifneq ($(AUTODEP), y)
depclean:
		@-rm -f $(DEPFILE) 2>NUL
		@echo Creating dependencies..
		@echo # Run "make depends" to re-create this file. >$(DEPFILE)

depends:	DEPOBJ=$(OBJ:%.o=%.d)
depends:	depclean $(OBJ:%.o=%.d)
		@-cat $(DEPOBJ) >>$(DEPFILE)
		@-rm -f $(DEPOBJ)

$(DEPFILE):
endif


# Module dependencies.
ifeq ($(AUTODEP), y)
#-include $(OBJ:%.o=%.d)  (better, but sloooowwwww)
-include *.d
else
include $(wildcard $(DEPFILE))
endif


# End of Makefile.
