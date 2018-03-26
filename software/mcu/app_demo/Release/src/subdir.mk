################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/crp.c \
../src/main.c \
../src/ssd1306.c \
../src/timer.c 

OBJS += \
./src/crp.o \
./src/main.o \
./src/ssd1306.o \
./src/timer.o 

C_DEPS += \
./src/crp.d \
./src/main.d \
./src/ssd1306.d \
./src/timer.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c99 -DNDEBUG -D__CODE_RED -DCORE_M0PLUS -D__REDLIB__ -I"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\app_demo\inc" -I"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\lib_board_dp\inc" -I"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\lib_chip_nss\inc" -I"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\app_demo\mods" -I"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\lib_board_dp\mods" -I"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\lib_chip_nss\mods" -include"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\app_demo\mods\app_sel.h" -include"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\lib_board_dp\mods\board_sel.h" -include"C:\NXP Working File\NXP\lpc_sample\LPC8N04\5. Templates\LPC8N04_ALARMCLOCK\lib_chip_nss\mods/chip_sel.h" -Os -fno-common -g -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


