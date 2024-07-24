## boost install

./bootstrap.sh
./b2 install cxxflags="-std=c++14"


## Arrow

cmake -DARROW_COMPUTE=ON -DARROW_CSV=ON -DARROW_DATASET=ON -DARROW_FILESYSTEM=ON ..




```shell
# xsimd
git clone https://github.com/xtensor-stack/xsimd.git
cd xsmid && mkdir build && cd build
cmake ..
make 
make install
```