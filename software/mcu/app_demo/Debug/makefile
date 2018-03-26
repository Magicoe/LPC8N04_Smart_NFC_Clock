################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include mods/tmeas/subdir.mk
-include mods/storage/subdir.mk
-include mods/ndeft2t/subdir.mk
-include mods/msg/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: app_demo.axf

# Tool invocations
app_demo.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_board_dp\Debug" -L"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_chip_nss\Debug" -Xlinker -Map="app_demo.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m0 -mthumb -T "app_demo_Debug.ld" -o "app_demo.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) app_demo.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "app_demo.axf" ; arm-none-eabi-objcopy -v -O binary "app_demo.axf" "app_demo.bin" ; arm-none-eabi-objcopy -v -O ihex "app_demo.axf" "app_demo.hex" ; # checksum -p LPC8N04 -d "app_demo.bin" ;
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
