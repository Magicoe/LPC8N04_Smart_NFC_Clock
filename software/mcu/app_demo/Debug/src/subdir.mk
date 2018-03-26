################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/buzzer.c \
../src/crp.c \
../src/main.c \
../src/memory.c \
../src/msghandler.c \
../src/rtc.c \
../src/ssd1306.c \
../src/text.c \
../src/timer.c \
../src/validate.c 

OBJS += \
./src/buzzer.o \
./src/crp.o \
./src/main.o \
./src/memory.o \
./src/msghandler.o \
./src/rtc.o \
./src/ssd1306.o \
./src/text.o \
./src/timer.o \
./src/validate.o 

C_DEPS += \
./src/buzzer.d \
./src/crp.d \
./src/main.d \
./src/memory.d \
./src/msghandler.d \
./src/rtc.d \
./src/ssd1306.d \
./src/text.d \
./src/timer.d \
./src/validate.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c99 -DDEBUG -DMIME=\"tlogger/demo.nhs.nxp\" -D__CODE_RED -DCORE_M0PLUS -DAPP_BUILD_TIMESTAMP -D__REDLIB__ -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\app_demo\inc" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_board_dp\inc" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_chip_nss\inc" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\app_demo\mods" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_board_dp\mods" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_chip_nss\mods" -include"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\app_demo\mods\app_sel.h" -include"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_board_dp\mods\board_sel.h" -include"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_chip_nss\mods/chip_sel.h" -Og -fno-common -g3 -Wall -Wextra -Wconversion -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


