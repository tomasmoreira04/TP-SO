################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Consola.c \
../src/Planificador.c 

OBJS += \
./src/Consola.o \
./src/Planificador.o 

C_DEPS += \
./src/Consola.d \
./src/Planificador.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -Dcommons -Dpthread -I"/home/utnso/Escritorio/Nuevo/tp-2018-1C-PMD/Bibliotecas/src" -include"/home/utnso/Escritorio/Nuevo/tp-2018-1C-PMD/Bibliotecas/src/Configuracion.h" -include"/home/utnso/Escritorio/Nuevo/tp-2018-1C-PMD/Bibliotecas/src/Estructuras.h" -include"/home/utnso/Escritorio/Nuevo/tp-2018-1C-PMD/Bibliotecas/src/Macros.h" -include"/home/utnso/Escritorio/Nuevo/tp-2018-1C-PMD/Bibliotecas/src/Socket.h" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


