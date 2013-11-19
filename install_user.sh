cd src/common
make clean
make
mv libXeDAQ.so /usr/lib/
cd ../user
make clean
make
cd ../..