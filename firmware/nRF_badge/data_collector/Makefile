PROJECT_NAME := ble_app_template_s130_pca10028

ifndef NRF_SDK_PATH
$(warning NRF_SDK_PATH environment variable undefined.  Default: /usr/local/nRF51_SDK_8.1.0_b6ed55f)
endif

NRF_SDK_PATH ?= /usr/local/nRF51_SDK_8.1.0_b6ed55f
SDK_PATH := $(NRF_SDK_PATH)

export OUTPUT_FILENAME
#MAKEFILE_NAME := $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) ) 

TEMPLATE_PATH = $(SDK_PATH)/components/toolchain/gcc
ifeq ($(OS),Windows_NT)
include $(TEMPLATE_PATH)/Makefile.windows
else
include $(TEMPLATE_PATH)/Makefile.posix
endif

APP_MEMORY_OFFSET = 1c000  ##application memory offset for S130 softdevice
JLINK_OPTIONS = -device nrf51422_xxac -if swd -speed 0
JLINK = JLinkExe $(JLINK_OPTIONS)

MK := mkdir
RM := rm -rf

#echo suspend
ifeq ("$(VERBOSE)","1")
NO_ECHO := 
else
NO_ECHO := @
endif

# Toolchain commands
CC       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc"
AS       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-as"
AR       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ar" -r
LD       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ld"
NM       		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-nm"
OBJDUMP  		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objdump"
OBJCOPY  		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objcopy"
SIZE    		:= "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-size"

#function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))



#target for printing all targets
help:
	@echo
	@echo "The following targets are available:"
	@echo "  - Boards/options:  NRF51DK   badge_03   badge_03_noDebug"
	@echo "                     badge_03v2_rigado   badge_03v2_dynastream"
	@echo "                     badge_03v2_rigado_noDebug	badge_03v2_rigado_tester"
	@echo "                     badge_03v4	 badge_03v4_noDebug   badge_03v4_tester"
	@echo "  - Actions:         flashUnlock  flashErase  flashS130  flashAPP"
	@echo "  - Info:            help  helpDetailed"
	@echo

helpDetailed:
	@echo
	@echo "The following targets are available:"
	@echo "   - NRF51DK:            compile for NRF51 devkit board."
	@echo "   - badge_03:           compile for Badge 03 board (square blue badge)"
	@echo "   - badge_03_noDebug:   compile for Badge 03, UART disabled"
	@echo " -----"
	@echo "   - badge_03v2_[rigado|dynastream]:  compile for Badge 03v2"
	@echo "                           (with corresponding BLE module)"
	@echo "   - badge_03v2_rigado_noDebug:  compile for Badge 03v2, UART disabled"
	@echo "   - badge_03v2_rigado_tester:   compile 03v2 Rigado self tester"
	@echo " -----"
	@echo "   - badge_03v4:         compile for Badge 03v4 board"
	@echo "   - badge_03v4_noDebug: compile for Badge 03v4 board, UART disabled"
	@echo "   - badge_03v4_tester:  compile for Badge 03v4 board, UART disabled"
	@echo " -----"
	@echo "   - flashUnlock:        set flags required to enable erasing/programming of flash"
	@echo "                           (only required for brand new NRF51 chips)"
	@echo "   - flashErase:         erase all flash (including softdevice)"
	@echo "   - flashS130:          flash S130 (BLE simultaneous central/peripheral) softdevice"
	@echo "   - flashAPP:           flash the badge firmware (after the softdevice)"
	@echo





#source files
C_SOURCE_FILES += \
main.c \
incl/analog.c \
incl/rtc_timing.c \
incl/ble_setup.c \
incl/debug_log.c \
incl/internal_flash.c \
incl/external_flash.c \
incl/scanning.c \
incl/self_test.c \
$(SDK_PATH)/components/libraries/util/app_error.c \
$(SDK_PATH)/components/libraries/util/nrf_assert.c \
$(SDK_PATH)/components/libraries/uart/retarget.c \
$(SDK_PATH)/components/drivers_nrf/hal/nrf_adc.c \
$(SDK_PATH)/components/drivers_nrf/hal/nrf_delay.c \
$(SDK_PATH)/components/drivers_nrf/uart/app_uart_fifo.c \
$(SDK_PATH)/components/libraries/fifo/app_fifo.c \
$(SDK_PATH)/components/drivers_nrf/common/nrf_drv_common.c \
$(SDK_PATH)/components/drivers_nrf/gpiote/nrf_drv_gpiote.c \
$(SDK_PATH)/components/drivers_nrf/rtc/nrf_drv_rtc.c \
$(SDK_PATH)/components/drivers_nrf/ble_flash/ble_flash.c \
$(SDK_PATH)/components/drivers_nrf/spi_master/spi_master.c \
$(SDK_PATH)/components/drivers_nrf/pstorage/pstorage.c \
$(SDK_PATH)/components/ble/common/ble_advdata.c \
$(SDK_PATH)/components/ble/ble_advertising/ble_advertising.c \
$(SDK_PATH)/components/ble/common/ble_srv_common.c \
$(SDK_PATH)/components/toolchain/system_nrf51.c \
$(SDK_PATH)/components/softdevice/common/softdevice_handler/softdevice_handler.c \
$(SDK_PATH)/components/ble/ble_services/ble_bas/ble_bas.c \
$(SDK_PATH)/components/ble/ble_services/ble_nus/ble_nus.c \


#assembly files
ASM_SOURCE_FILES  = $(SDK_PATH)/components/toolchain/gcc/gcc_startup_nrf51.s

#includes
INC_PATHS  = -Iconfig
INC_PATHS += -Iincl
INC_PATHS += -I$(SDK_PATH)/components/device
INC_PATHS += -I$(SDK_PATH)/components/toolchain
INC_PATHS += -I$(SDK_PATH)/components/toolchain/gcc
INC_PATHS += -I$(SDK_PATH)/components/softdevice/s130/headers
INC_PATHS += -I$(SDK_PATH)/components/softdevice/common/softdevice_handler
INC_PATHS += -I$(SDK_PATH)/components/libraries/util
INC_PATHS += -I$(SDK_PATH)/components/libraries/fifo
INC_PATHS += -I$(SDK_PATH)/components/libraries/trace
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/common
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/rtc
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/hal
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/uart
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/config
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/gpiote
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/ble_flash
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/spi_master
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/pstorage
INC_PATHS += -I$(SDK_PATH)/components/ble/common
INC_PATHS += -I$(SDK_PATH)/components/ble/ble_advertising
INC_PATHS += -I$(SDK_PATH)/components/ble/ble_services/ble_bas
INC_PATHS += -I$(SDK_PATH)/components/ble/ble_services/ble_nus
INC_PATHS += -I$(SDK_PATH)/examples/bsp

OBJECT_DIRECTORY = _build
LISTING_DIRECTORY = $(OBJECT_DIRECTORY)
OUTPUT_BINARY_DIRECTORY = $(OBJECT_DIRECTORY)

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )



#======= Target-dependent compiler flags
NRF51DK: CFLAGS += -DBOARD_PCA10028

badge_03: CFLAGS += -DBOARD_CUSTOM
badge_03: CFLAGS += -DBOARD_BADGE_03
badge_03_noDebug: CFLAGS += -DBOARD_CUSTOM
badge_03_noDebug: CFLAGS += -DBOARD_BADGE_03

badge_03v2_rigado: CFLAGS += -DBOARD_CUSTOM
badge_03v2_rigado: CFLAGS += -DBOARD_BADGE_03V2_RIGADO
badge_03v2_rigado_noDebug: CFLAGS += -DBOARD_CUSTOM
badge_03v2_rigado_noDebug: CFLAGS += -DBOARD_BADGE_03V2_RIGADO
badge_03v2_rigado_tester: CFLAGS += -DBOARD_CUSTOM
badge_03v2_rigado_tester: CFLAGS += -DBOARD_BADGE_03V2_RIGADO
badge_03v2_rigado_tester: CFLAGS += -DTESTER_ENABLE #enables tester mode 
badge_03v2_dynastream: CFLAGS += -DBOARD_CUSTOM
badge_03v2_dynastream: CFLAGS += -DBOARD_BADGE_03V2_DYNASTREAM

badge_03v4: CFLAGS += -DBOARD_CUSTOM
badge_03v4: CFLAGS += -DBOARD_BADGE_03V4
badge_03v4_noDebug: CFLAGS += -DBOARD_CUSTOM
badge_03v4_noDebug: CFLAGS += -DBOARD_BADGE_03V4
badge_03v4_tester: CFLAGS += -DBOARD_CUSTOM
badge_03v4_tester: CFLAGS += -DBOARD_BADGE_03V4
badge_03v4_tester: CFLAGS += -DTESTER_ENABLE #enables tester mode



CFLAGS += -DDEBUG_LOG_ENABLE    #enable UART debug logger
badge_03_noDebug: CFLAGS += -UDEBUG_LOG_ENABLE    #disable if noDebug target specified
badge_03v2_rigado_noDebug: CFLAGS += -UDEBUG_LOG_ENABLE
badge_03v4_noDebug: CFLAGS += -UDEBUG_LOG_ENABLE



#======= Other compiler flags
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DNRF51
CFLAGS += -DS130
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DSWI_DISABLE0
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs --std=gnu99
CFLAGS += -Wall -Werror -O3
CFLAGS += -mfloat-abi=soft
# keep every function in separate section. This will allow linker to dump unused functions
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums

#enable SPI
CFLAGS += -DSPI_MASTER_0_ENABLE



# keep every function in separate section. This will allow linker to dump unused functions
LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_FILENAME).map
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs -lc -lnosys

# Assembler flags
ASMFLAGS += -x assembler-with-cpp
# ! vvv this has been here since badge 03, hasn't been updated, but hasn't seemed to cause any issues?
ASMFLAGS  += -DBOARD_PCA10028  #default to NRF51DK board
badge_03: ASMFLAGS += -UBOARD_PCA10028  #undefine NRF51DK, to define custom board flag
badge_03: ASMFLAGS += -DBOARD_CUSTOM
badge_03: ASMFLAGS += -DBOARD_BADGE_03
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DNRF51
ASMFLAGS += -DS130
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
ASMFLAGS += -DSWI_DISABLE0


C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

vpath %.c $(C_PATHS)
vpath %.s $(ASM_PATHS)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

nrf51422_xxac_s130: OUTPUT_FILENAME := nrf51422_xxac_s130
#NRF51 DK uses 51422 xxac variant - 256KB flash, 32KB RAM, ANT/BLE SoC
#nrf51422_xxac_s130: LINKER_SCRIPT=nrf51_xxac_S130_linker.ld
nrf51422_xxac_s130: LINKER_SCRIPT=$(SDK_PATH)/components/softdevice/s130/toolchain/armgcc/armgcc_s130_nrf51422_xxac.ld
nrf51422_xxac_s130: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e finalize

## Create build directories
$(BUILD_DIRECTORIES):
	echo $(MAKEFILE_NAME)
	$(MK) $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) -c -o $@ $<

# Assemble files
$(OBJECT_DIRECTORY)/%.o: %.s
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(ASMFLAGS) $(INC_PATHS) -c -o $@ $<


# Link
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out


## Create binary .bin file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

finalize: genbin genhex echosize

genbin:
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
genhex: 
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

echosize:
	-@echo ""
	$(NO_ECHO)$(SIZE) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	-@echo ""


clean:
	$(RM) $(BUILD_DIRECTORIES)

cleanobj:
	$(RM) $(BUILD_DIRECTORIES)/*.o


#==============================  Board option rules  ===================================

NRF51DK: clean nrf51422_xxac_s130

badge_03: clean nrf51422_xxac_s130
badge_03_noDebug: clean nrf51422_xxac_s130

badge_03v2_rigado: clean nrf51422_xxac_s130
badge_03v2_rigado_noDebug: clean nrf51422_xxac_s130
badge_03v2_rigado_tester: clean nrf51422_xxac_s130
badge_03v2_dynastream: clean nrf51422_xxac_s130

badge_03v4: clean nrf51422_xxac_s130
badge_03v4_noDebug: clean nrf51422_xxac_s130
badge_03v4_tester: clean nrf51422_xxac_s130


#=================================  Action rules  =====================================

#write some flash config registers to "unlock" flash
flashUnlock: clean flashUnlock.jlink 
	$(JLINK) $(OBJECT_DIRECTORY)/flashUnlock.jlink

#create flash unlocking script for JLinkExe.
flashUnlock.jlink:
	printf "w4 4001e504 2\nw4 4001e50c 1\nexit\n" > $(OBJECT_DIRECTORY)/flashUnlock.jlink	
	
	
#build and flash firmware
flashAPP: clean nrf51422_xxac_s130 flashAPP.jlink 
	$(JLINK) $(OBJECT_DIRECTORY)/flashAPP.jlink

#create application flashing script for JLinkExe.  1c000 offset is for S130 softdevice.
flashAPP.jlink:
	printf "loadbin $(OUTPUT_BINARY_DIRECTORY)/nrf51422_xxac_s130.hex $(APP_MEMORY_OFFSET)\nr\ng\nexit\n" > $(OBJECT_DIRECTORY)/flashAPP.jlink	


#flash SoftDevice
flashS130: flashS130.jlink 
	$(JLINK) $(OBJECT_DIRECTORY)/flashS130.jlink

#create SoftDevice flashing script for JLinkExe
flashS130.jlink:
	printf "loadbin $(SDK_PATH)/components/softdevice/s130/hex/s130_softdevice.hex 0\nr\ng\nexit\n" > $(OBJECT_DIRECTORY)/flashS130.jlink	
	
	
#erase all flash
flashErase: flashErase.jlink 
	$(JLINK) $(OBJECT_DIRECTORY)/flashErase.jlink

#create flash erasing for JLinkExe
flashErase.jlink:
	printf "erase\nr\ng\nexit\n" > $(OBJECT_DIRECTORY)/flashErase.jlink	
