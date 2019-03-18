#!/bin/bash
cd sc_aircon;
sed -i -e "s/SANYO_AC 0/SANYO_AC 1/g" sc_aircon.ino;
arduino --verify --preferences-file ../arduino_preferences_esp01.txt sc_aircon.ino
scp build/sc_aircon.ino.bin root@smart-classroom.foundationu.com:/srv/www/vhosts/updates/sc_aircon_sanyo.ino.bin
sed -i -e "s/SANYO_AC 1/SANYO_AC 0/g" sc_aircon.ino;
arduino --verify --preferences-file ../arduino_preferences_esp01.txt sc_aircon.ino
scp build/sc_aircon.ino.bin root@smart-classroom.foundationu.com:/srv/www/vhosts/updates/sc_aircon_daikin.ino.bin
cd ../sc_lights;
arduino --verify --preferences-file ../arduino_preferences_esp01.txt sc_lights.ino
scp build/sc_lights.ino.bin root@smart-classroom.foundationu.com:/srv/www/vhosts/updates/
cd ../sc_door;
arduino --verify --preferences-file ../arduino_preferences_nodemcu.txt sc_door.ino
scp build/sc_door.ino.bin root@smart-classroom.foundationu.com:/srv/www/vhosts/updates/
