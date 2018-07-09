xterm -hold -e "cd Coordinador/Debug && ./Coordinador configCoordinador.cfg" &
sleep 1
xterm -hold -e "cd Planificador/Debug && ./Planificador configPlanificador.cfg" &
sleep 1
xterm -hold -e "cd Instancia/Debug && ./Instancia configInstancia1.cfg" &
sleep 1
lxterminal  --working-directory=ESI/Debug