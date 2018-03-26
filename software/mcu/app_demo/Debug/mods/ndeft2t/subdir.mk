################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mods/ndeft2t/ndeft2t.c 

OBJS += \
./mods/ndeft2t/ndeft2t.o 

C_DEPS += \
./mods/ndeft2t/ndeft2t.d 


# Each subdirectory must supply rules for building sources it contributes
mods/ndeft2t/%.o: ../mods/ndeft2t/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c99 -DDEBUG -DMIME=\"tlogger/demo.nhs.nxp\" -D__CODE_RED -DCORE_M0PLUS -DAPP_BUILD_TIMESTAMP -D__REDLIB__ -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\app_demo\inc" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_board_dp\inc" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_chip_nss\inc" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\app_demo\mods" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_board_dp\mods" -I"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_chip_nss\mods" -include"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\app_demo\mods\app_sel.h" -include"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_board_dp\mods\board_sel.h" -include"F:\NXP\Temp\LPC8N04_CES_DEMO\LPC8N04\lib_chip_nss\mods/chip_sel.h" -Og -fno-common -g3 -Wall -Wextra -Wconversion -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


