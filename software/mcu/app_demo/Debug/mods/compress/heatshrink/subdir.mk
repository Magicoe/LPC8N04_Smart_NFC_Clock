################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mods/compress/heatshrink/heatshrink_decoder.c \
../mods/compress/heatshrink/heatshrink_encoder.c 

OBJS += \
./mods/compress/heatshrink/heatshrink_decoder.o \
./mods/compress/heatshrink/heatshrink_encoder.o 

C_DEPS += \
./mods/compress/heatshrink/heatshrink_decoder.d \
./mods/compress/heatshrink/heatshrink_encoder.d 


# Each subdirectory must supply rules for building sources it contributes
mods/compress/heatshrink/%.o: ../mods/compress/heatshrink/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c99 -DDEBUG -D__CODE_RED -DCORE_M0PLUS -D__REDLIB__ -D"SW_MAJOR_VERSION=$(shell date +%y%V)" -D"APP_BUILD_TIMESTAMP=$(shell date --utc +%s)" -DMIME=\"tlogger/demo.nhs.nxp.log\" -I"C:\lpc_applications\LPC8N04\firmware\app_demo_dp_tlogger\inc" -I"C:\lpc_applications\LPC8N04\firmware\lib_board_dp\inc" -I"C:\lpc_applications\LPC8N04\firmware\lib_chip_nss\inc" -I"C:\lpc_applications\LPC8N04\firmware\mods" -I"C:\lpc_applications\LPC8N04\firmware\app_demo_dp_tlogger\mods" -I"C:\lpc_applications\LPC8N04\firmware\lib_board_dp\mods" -I"C:\lpc_applications\LPC8N04\firmware\lib_chip_nss\mods" -include"C:\lpc_applications\LPC8N04\firmware\app_demo_dp_tlogger\mods\app_sel.h" -include"C:\lpc_applications\LPC8N04\firmware\lib_board_dp\mods\board_sel.h" -include"C:\lpc_applications\LPC8N04\firmware\lib_chip_nss\mods/chip_sel.h" -O0 -fno-common -g3 -Wall -Wextra -Wconversion -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


