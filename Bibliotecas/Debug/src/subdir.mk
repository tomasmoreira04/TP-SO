################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Bibliotecas.c \
../src/Configuracion.c \
../src/Socket.c 

OBJS += \
./src/Bibliotecas.o \
./src/Configuracion.o \
./src/Socket.o 

C_DEPS += \
./src/Bibliotecas.d \
./src/Configuracion.d \
./src/Socket.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -Dcommons -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


