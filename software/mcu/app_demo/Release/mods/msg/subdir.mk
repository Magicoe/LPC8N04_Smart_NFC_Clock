################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mods/msg/msg.c 

OBJS += \
./mods/msg/msg.o 

C_DEPS += \
./mods/msg/msg.d 


# Each subdirectory must supply rules for building sources it contributes
mods/msg/%.o: ../mods/msg/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c99 -DNDEBUG -D__CODE_RED -DCORE_M0PLUS -D__REDLIB__ -D"SW_MAJOR_VERSION=$(shell date +%y%V)" -D"APP_BUILD_TIMESTAMP=$(shell date --utc +%s)" -DMIME=\"tlogger/demo.nhs.nxp.log\" -I"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\app_demo\inc" -I"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\lib_board_dp\inc" -I"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\lib_chip_nss\inc" -I"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\mods" -I"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\app_demo\mods" -I"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\lib_board_dp\mods" -I"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\lib_chip_nss\mods" -include"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\app_demo\mods\app_sel.h" -include"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\lib_board_dp\mods\board_sel.h" -include"C:\Users\nxp73930\Documents\MCUXpressoIDE_10.1.0_541_alpha\workspace\lib_chip_nss\mods/chip_sel.h" -Os -fno-common -g -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


