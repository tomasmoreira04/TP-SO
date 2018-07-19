################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Coordinador.c 

OBJS += \
./src/Coordinador.o 

C_DEPS += \
./src/Coordinador.d 


# Each subdirectory must supply rules for building sources it contributes
src/Coordinador.o: ../src/Coordinador.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -Dcommons -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"src/Coordinador.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


