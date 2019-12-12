sudo rmmod soilmois_dev
sudo rmmod temper_dev
cd SoilMois
sudo make clean
sudo make
sudo insmod soilmois_dev.ko
cd ..
cd Temper
sudo make clean
sudo make
sudo insmod temper_dev.ko
cd ..
rm FlowerPot_app
gcc -o FlowerPot_app FlowerPot_app.c -lpthread
