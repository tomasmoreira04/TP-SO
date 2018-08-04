echo "WELCOME TO SOMALIA"


cd ../
echo "Instalando Commons"
git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
sudo make install
cd ../
echo "Instalando PARSI"
git clone https://github.com/sisoputnfrba/parsi.git
cd parsi
sudo make install
echo "Compilando"


cd /home/utnso/tp-2018-1c-PMD/ESI/Debug
rm ESI
make ESI

cd ../../Instancia/Debug
rm Instancia
make Instancia

cd ../../Planificador/Debug
rm Planificador
make Planificador

cd ../../Coordinador/Debug
rm Coordinador
make Coordinador

chmod 0777 *





