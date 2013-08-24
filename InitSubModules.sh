git submodule init
git submodule update
cd Firmware/ChibiOS/ext/
unzip fatfs-0.9-patched.zip
unzip lwip-1.4.1_patched.zip
cd ..
patch -p1 < ../../mac_lld.patch 
