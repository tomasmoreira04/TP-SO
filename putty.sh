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

chmod 0777 *





