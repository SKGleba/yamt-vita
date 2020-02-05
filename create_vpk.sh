(
cd plugin/kernel/;
cmake -DNEWFW=1 ./ && make;
mv yamt.skprx ../yamt_365.skprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm yamt.elf && rm yamt.velf;
cmake -DNEWFW=0 ./ && make;
mv yamt.skprx ../yamt_360.skprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm yamt.elf && rm yamt.velf;
cd ../user/;
cmake ./ && make;
mv yamt.suprx ../yamt.suprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm peripherals_settings.xml.o && rm user && rm user.velf
cd ../../;
cmake ./ && make;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm yamt && rm yamt.velf && rm yamt.self && rm yamt.vpk_param.sfo;
cd plugin/;
rm yamt.suprx && rm yamt_365.skprx && rm yamt_360.skprx;
echo "";
echo "DONE!";
echo "";
)