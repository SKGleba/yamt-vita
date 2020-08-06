(
cd plugin/kernel/;
cmake ./ && make;
mv yamt.skprx ../yamt.skprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm yamt.elf && rm yamt.velf;
cd ../kernel_lite/;
cmake ./ && make;
mv yamt.skprx ../yamt_lite.skprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm yamt.elf && rm yamt.velf;
cd ../helper/;
cmake ./ && make;
mv hyamt.skprx ../helper.skprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm hyamt.elf && rm hyamt.velf;
cd ../user/;
cmake ./ && make;
mv yamt.suprx ../yamt.suprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm peripherals_settings.xml.o && rm user && rm user.velf
cd ../user_lite/;
cmake ./ && make;
mv yamt.suprx ../yamt_lite.suprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm peripherals_settings.xml.o && rm user && rm user.velf
cd ../../;
cmake ./ && make;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm yamt && rm yamt.velf && rm yamt.self && rm yamt.vpk_param.sfo;
cd plugin/;
rm yamt.suprx && rm yamt.skprx && rm helper.skprx;
rm yamt_lite.suprx && rm yamt_lite.skprx;
cd kernel/;
rm -rf stubs && rm stubs.yml;
cd ../helper/;
rm -rf stubs && rm stubs.yml;
echo "";
echo "DONE!";
echo "";
)
