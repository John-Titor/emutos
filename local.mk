#
# Non-mainline ports.
#

help: help-local
.PHONY: help-local
NODEP += help-local
help-local:
	@echo "target  meaning"
	@echo "------  -------"
	@echo "pt68k5  $(EMUTOS_PT68K5), EmuTOS for PT68K5"
	@echo "ip940   $(EMUTOS_IP940), EmuTOS for IP940 ROM"
	@echo "qemu    $(EMUTOS_QEMU), suitable for qemu-system-m68k -machine type=atarist"

#
# The rule for emutos.img is instantiated before we are included,
# so we need some hackery here to correctly get our local objects
# built.
#
LOCAL_OBJS = pt68k5.c \
             ip940.c \
             qemu.c qemu_pci.c qemu_video.c qemu_virtio.c qemu2.S

bios_src += $(LOCAL_OBJS)

emutos.img: $(patsubst %.c,obj/%.o,$(patsubst %.S,obj/%.o,$(LOCAL_OBJS)))

################################################################################
#
# PT68K5
#

EMUTOS_PT68K5 = emutosk5.rom
PT68K5_DEFS =
TOCLEAN += $(EMUTOS_PT68K5)

.PHONY: pt68k5
NODEP += pt68k5
pt68k5: UNIQUE = $(COUNTRY)
pt68k5: OPTFLAGS = $(SMALL_OPTFLAGS)
pt68k5: CPUFLAGS = -m68020
pt68k5: override DEF += -DTARGET_PT68K5 $(PT68K5_DEFS)
pt68k5: WITH_AES=1
pt68k5:
	$(MAKE) CPUFLAGS='$(CPUFLAGS)' \
		DEF='$(DEF)' \
		OPTFLAGS='$(OPTFLAGS)' \
		WITH_AES=$(WITH_AES) \
		UNIQUE=$(UNIQUE) \
		EMUTOS_PT68K5=$(EMUTOS_PT68K5) \
		$(EMUTOS_PT68K5)
	@printf "$(LOCALCONFINFO)"

$(EMUTOS_PT68K5): emutos.img pt68k5boot.img pt68k5_installboot
	$(OBJCOPY) -I binary -O binary emutos.img $@

pt68k5boot.img: obj/pt68k5boot.o obj/pt68k5boot2.o obj/doprintf.o
	$(LD) $+ -Wl,--oformat=binary,-Ttext=0x00200000,--entry=0x00200000 -o $@

obj/pt68k5boot.o: obj/ramtos.h

TOCLEAN += pt68k5_installboot
NODEP += pt68k5_installboot
pt68k5_installboot: tools/pt68k5_installboot.c
	$(NATIVECC) $< -o $@

.PHONY: release-pt68k5
NODEP += release-pt68k5
RELEASE_PT68K5 = emutos-pt68k5-$(VERSION)
release-pt68k5:
	$(MAKE) clean
	$(MAKE) pt68k5
	mkdir -p $(RELEASE_DIR)/$(RELEASE_PT68K5)
	cp $(EMUTOS_PT68K5) $(RELEASE_DIR)/$(RELEASE_PT68K5)
	cp desk/icon.def $(RELEASE_DIR)/$(RELEASE_PT68K5)/emuicon.def
	cp desk/icon.rsc $(RELEASE_DIR)/$(RELEASE_PT68K5)/emuicon.rsc
	cat doc/readme-pt68k5.txt readme.txt >$(RELEASE_DIR)/$(RELEASE_PT68K5)/readme.txt
	mkdir -p $(RELEASE_DIR)/$(RELEASE_PT68K5)/doc
	cp $(DOCFILES) $(RELEASE_DIR)/$(RELEASE_PT68K5)/doc
	mkdir -p $(RELEASE_DIR)/$(RELEASE_PT68K5)/extras
	cp $(EXTRAFILES) $(RELEASE_DIR)/$(RELEASE_PT68K5)/extras
	cp aes/mform.def $(RELEASE_DIR)/$(RELEASE_PT68K5)/extras/emucurs.def
	cp aes/mform.rsc $(RELEASE_DIR)/$(RELEASE_PT68K5)/extras/emucurs.rsc
	find $(RELEASE_DIR)/$(RELEASE_PT68K5) -name '*.txt' -exec unix2dos '{}' ';'
	cd $(RELEASE_DIR) && zip -9 -r $(RELEASE_PT68K5).zip $(RELEASE_PT68K5)
	rm -r $(RELEASE_DIR)/$(RELEASE_PT68K5)


################################################################################
#
# IP940
#

EMUTOS_IP940 = emutos940.s19
EMUTOS_IP940_ELF = emutos940.elf
IP940_DEFS =
TOCLEAN += $(EMUTOS_IP940) $(EMUTOS_IP940_ELF) romdisk.ip940

.PHONY: ip940
NODEP += ip940
ip940: UNIQUE = $(COUNTRY)
ip940: CPUFLAGS = -m68040
ip940: override DEF += -DTARGET_IP940 $(IP940_DEFS)
ip940: WITH_AES = 0
ip940:
	$(MAKE) CPUFLAGS='$(CPUFLAGS)' \
		DEF='$(DEF)' \
		OPTFLAGS='$(OPTFLAGS)' \
		WITH_AES=$(WITH_AES) \
		UNIQUE=$(UNIQUE) \
		EMUTOS_IP940=$(EMUTOS_IP940) \
		$(EMUTOS_IP940)
	@printf "$(LOCALCONFINFO)"

obj/ip940_loader.o: emutos.img

$(EMUTOS_IP940_ELF): obj/ip940_loader.o
	$(LD) $+ -Wl,-Ttext=0x4000 -e start -o $@

$(EMUTOS_IP940): $(EMUTOS_IP940_ELF)
	$(OBJCOPY) -O srec --srec-forceS3 --srec-len 100 $< $@

.PHONY: release-ip940
NODEP += release-ip940
RELEASE_IP940 = emutos-ip940-$(VERSION)
release-ip940:
	$(MAKE) clean
	$(MAKE) ip940
	mkdir -p $(RELEASE_DIR)/$(RELEASE_IP940)
	cp $(EMUTOS_IP940) $(RELEASE_DIR)/$(RELEASE_IP940)
	cat doc/readme-ip940.txt readme.txt >$(RELEASE_DIR)/$(RELEASE_IP940)/readme.txt
	mkdir -p $(RELEASE_DIR)/$(RELEASE_IP940)/doc
	cp $(DOCFILES) $(RELEASE_DIR)/$(RELEASE_IP940)/doc
	mkdir -p $(RELEASE_DIR)/$(RELEASE_IP940)/extras
	find $(RELEASE_DIR)/$(RELEASE_IP940) -name '*.txt' -exec unix2dos '{}' ';'
	cd $(RELEASE_DIR) && zip -9 -r $(RELEASE_IP940).zip $(RELEASE_IP940)
	rm -r $(RELEASE_DIR)/$(RELEASE_IP940)


#
# qemu Image
#

EMUTOS_QEMU = emutos-qemu.img

.PHONY: qemu
NODEP += qemu
qemu: override DEF += -DMACHINE_QEMU
qemu: CPUFLAGS = -m68040
qemu: ROMSIZE = 1024
qemu: ROM_PADDED = $(EMUTOS_QEMU)
qemu: WITH_AES = 1
qemu:
	@echo "# Building QEMU EmuTOS into $(ROM_PADDED)"
	$(MAKE) CPUFLAGS='$(CPUFLAGS)' OPTFLAGS='$(OPTFLAGS)' DEF='$(DEF)' WITH_AES=$(WITH_AES) ROMSIZE=$(ROMSIZE) ROM_PADDED=$(ROM_PADDED) $(ROM_PADDED)

# override these to suit local paths
QEMU_DIR ?= ../../../_Emulators/qemu/qemu
QEMU ?= ../../../_Emulators/qemu/qemu/build/qemu-system-m68k
QEMU_DISK ?= ../disk.bin

# basic configuration
QEMU_MEM_SIZE = 64M
QEMU_OPTS = -kernel $(EMUTOS_QEMU)
QEMU_OPTS += -m $(QEMU_MEM_SIZE)
QEMU_OPTS += -drive file=$(QEMU_DISK),if=ide,format=raw
QEMU_OPTS += -rtc base=localtime

# prefer the SDL display for development / quick exit
QEMU_OPTS += -display sdl
#QEMU_OPTS += -display cocoa,full-screen=off

# select the memory-backend options to have emulator memory mapped / save to file
QEMU_OPTS += -machine type=atarist
#QEMU_MEM = /tmp/atari.mem
#QEMU_OPTS += -machine type=atarist,memory-backend=virt.ram
#QEMU_OPTS += -object memory-backend-file,size=$(QEMU_MEM_SIZE),id=virt.ram,mem-path=$(QEMU_MEM),share=on,prealloc=on

# serial options
QEMU_OPTS += -serial mon:stdio
#QEMU_OPTS += -monitor stdio

# networking
#QEMU_OPTS += -netdev user,id=n1,hostfwd=tcp:127.0.0.1:8000-:80
#QEMU_OPTS += -device virtio-net,netdev=n1
#QEMU_OPTS += -device virtio-9p-pci-non-transitional,fsdev=local
QEMU_OPTS += -virtfs local,path=/tmp,mount_tag=local,security_model=none,id=local

# optional devices
#QEMU_OPTS += -device cirrus-vga,romfile=vgabios-cirrus.bin
#QEMU_OPTS += -L $(QEMU_DIR)/pc-bios
#QEMU_OPTS += -device usb-ehci
#QEMU_OPTS += -device pci-serial-4x
#QEMU_OPTS += -device rtl8139
#QEMU_OPTS += -device sdhci-pci

# trace single instructions - huge logfiles
#QEMU_OPTS += -accel tcg,one-insn-per-tb=on,thread=single -d exec,cpu,in_asm -D /tmp/qemu.log
#QEMU_OPTS += -nographic
#QEMU_OPTS += -d trace:pci_cfg_'*' -D /tmp/qemu.log

# load image and set explicit entrypoint
#QEMU_OPTS += -device loader,file=$(EMUTOS_QEMU),addr=0x00e00000,force-raw=on
#QEMU_OPTS += -device loader,addr=0x00e00000,cpu-num=0

qemu-run: qemu
qemu-run:
	rm -f $(QEMU_MEM)
	$(QEMU) $(QEMU_OPTS)

.PHONY: release-qemu
NODEP += release-qemu
RELEASE_QEMU = emutos-qemu-$(VERSION)
release-qemu:
	$(MAKE) clean
	$(MAKE) qemu
	mkdir -p $(RELEASE_DIR)/$(RELEASE_QEMU)
	cp $(EMUTOS_QEMU) $(RELEASE_DIR)/$(RELEASE_QEMU)
	cp desk/icon.def $(RELEASE_DIR)/$(RELEASE_QEMU)/emuicon.def
	cp desk/icon.rsc $(RELEASE_DIR)/$(RELEASE_QEMU)/emuicon.rsc
	cat doc/readme-qemu.txt readme.txt >$(RELEASE_DIR)/$(RELEASE_QEMU)/readme.txt
	mkdir -p $(RELEASE_DIR)/$(RELEASE_QEMU)/doc
	cp $(DOCFILES) $(RELEASE_DIR)/$(RELEASE_QEMU)/doc
	mkdir -p $(RELEASE_DIR)/$(RELEASE_QEMU)/extras
	cp $(EXTRAFILES) $(RELEASE_DIR)/$(RELEASE_QEMU)/extras
	cp aes/mform.def $(RELEASE_DIR)/$(RELEASE_QEMU)/extras/emucurs.def
	cp aes/mform.rsc $(RELEASE_DIR)/$(RELEASE_QEMU)/extras/emucurs.rsc
	find $(RELEASE_DIR)/$(RELEASE_QEMU) -name '*.txt' -exec unix2dos '{}' ';'
	cd $(RELEASE_DIR) && zip -9 -r $(RELEASE_QEMU).zip $(RELEASE_QEMU)
	rm -r $(RELEASE_DIR)/$(RELEASE_QEMU)
