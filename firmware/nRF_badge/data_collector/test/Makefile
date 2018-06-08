#PROJECT_NAME := ble_app_template_s130_pca10028

ifndef NRF_SDK_PATH
$(warning NRF_SDK_PATH environment variable undefined.  Default: /usr/local/nRF51_SDK_8.1.0_b6ed55f)
endif

NRF_SDK_PATH ?= /usr/local/nRF51_SDK_8.1.0_b6ed55f
# Michael: ADDED /nRF5_SDK_12.3.0_d7731ad
SDK_PATH := $(NRF_SDK_PATH)/nRF5_SDK_12.3.0_d7731ad

export OUTPUT_FILENAME
#MAKEFILE_NAME := $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) )




# Where to find test mocks
MOCK_DIR = mock/incl

# Where to find the firmware
FIRMWARE_DIR = ..

OUTPUT_FILENAME := nrf51422_xxac_s130




MK := mkdir
RM := rm -rf


NO_ECHO := @


# Toolchain commands
CC       		:= "gcc"
AS       		:= "as"
AR       		:= "ar" -r
LD       		:= "ld"
NM       		:= "nm"
OBJDUMP  		:= "objdump"
OBJCOPY  		:= "objcopy"
SIZE    		:= "size"

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
	@echo "                     badge_03v6	 badge_03v6_noDebug   badge_03v6_tester"
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
	@echo "   - badge_03v46:        compile for Badge 03v6 board"
	@echo "   - badge_03v6_noDebug: compile for Badge 03v6 board, UART disabled"
	@echo "   - badge_03v6_tester:  compile for Badge 03v6 board, UART disabled"
	@echo " -----"
	@echo "   - flashUnlock:        set flags required to enable erasing/programming of flash"
	@echo "                           (only required for brand new NRF51 chips)"
	@echo "   - flashErase:         erase all flash (including softdevice)"
	@echo "   - flashS130:          flash S130 (BLE simultaneous central/peripheral) softdevice"
	@echo "   - flashAPP:           flash the badge firmware (after the softdevice)"
	@echo


###################################################### SOURCE #####################################
	

#source files
C_SOURCE_FILES += \
main.c \


#mock source files
C_SOURCE_FILES += \
$(MOCK_DIR)/eeprom_lib_mock.c \
$(MOCK_DIR)/debug_lib_mock.c \
$(MOCK_DIR)/flash_lib_mock.c \


#includes
INC_PATHS  = -I$(MOCK_DIR)
INC_PATHS  = -I$(FIRMWARE_DIR)/config
INC_PATHS += -I$(FIRMWARE_DIR)/incl
INC_PATHS += -I$(SDK_PATH)/components/device
INC_PATHS += -I$(SDK_PATH)/components/toolchain
INC_PATHS += -I$(SDK_PATH)/components/toolchain/gcc
INC_PATHS += -I$(SDK_PATH)/components/toolchain/cmsis/include
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/uart
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/hal
INC_PATHS += -I$(SDK_PATH)/components/libraries/util
INC_PATHS += -I$(SDK_PATH)/components/softdevice/s130/headers
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/common
INC_PATHS += -I$(SDK_PATH)/components/libraries/log
INC_PATHS += -I$(SDK_PATH)/components/libraries/log/src
INC_PATHS += -I$(SDK_PATH)/components/boards
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/delay
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/spi_master
INC_PATHS += -I$(SDK_PATH)/components/libraries/fstorage
INC_PATHS += -I$(SDK_PATH)/components/libraries/experimental_section_vars
INC_PATHS += -I$(SDK_PATH)/components/softdevice/common/softdevice_handler
INC_PATHS += -I$(SDK_PATH)/components/drivers_nrf/clock




###################################################### INCLUDE #####################################


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

badge_03v6: CFLAGS += -DBOARD_CUSTOM
badge_03v6: CFLAGS += -DBOARD_BADGE_03V6 			#Cambiar a DBOARD_BADGE_03V6
badge_03v6_noDebug: CFLAGS += -DBOARD_CUSTOM
badge_03v6_noDebug: CFLAGS += -DBOARD_BADGE_03V6
badge_03v6_tester: CFLAGS += -DBOARD_CUSTOM
badge_03v6_tester: CFLAGS += -DBOARD_BADGE_03V6
badge_03v6_tester: CFLAGS += -DTESTER_ENABLE #enables tester mode


CFLAGS += -DDEBUG_LOG_ENABLE    #enable UART debug logger
badge_03_noDebug: CFLAGS += -UDEBUG_LOG_ENABLE    #disable if noDebug target specified
badge_03v2_rigado_noDebug: CFLAGS += -UDEBUG_LOG_ENABLE
badge_03v4_noDebug: CFLAGS += -UDEBUG_LOG_ENABLE
badge_03v6_noDebug: CFLAGS += -UDEBUG_LOG_ENABLE

CFLAGS += --std=gnu99
CFLAGS += -Wall -Werror -O3
# keep every function in separate section. This will allow linker to dump unused functions
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums

GIT_TAG += $(shell git describe --tags)
GIT_BRANCH += $(shell git rev-parse --abbrev-ref HEAD)
GIT_COMMIT += $(shell git rev-parse --short HEAD)
CFLAGS += -DGIT_COMMIT='"$(GIT_COMMIT)"'
CFLAGS += -DGIT_BRANCH='"$(GIT_BRANCH)"'
CFLAGS += -DGIT_TAG='"$(GIT_TAG)"'



#Added Michael
LDFLAGS += -u _printf_float

# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections



C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )


vpath %.c $(C_PATHS)


OBJECTS = $(C_OBJECTS)


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
	mkdir -p $(OBJECT_DIRECTORY)

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

badge_03v6: clean nrf51422_xxac_s130
badge_03v6_noDebug: clean nrf51422_xxac_s130
badge_03v6_tester: clean nrf51422_xxac_s130

#=================================  Action rules  =====================================


run: clean nrf51422_xxac_s130 
	clear
#	@echo Output: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(NO_ECHO)$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	
# dummy to be able to call make with "make test badge_03v6 run"
test: clean

