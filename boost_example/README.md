## boost install

```shell
./bootstrap.sh
./b2 install cxxflags="-std=c++14"
```

## Arrow

```shell
cd cpp
mkdir build && cd build
cmake -DARROW_CSV=ON ..
make -j32 && make install
```



```shell
# abseil-cpp
git clone https://github.com/abseil/abseil-cpp.git
cd abseil-cpp && mkdir build && cd build
cmake .. && make -j32 && make install

# re2
git clone https://github.com/google/re2.git
cd re2 && mkdir build && cd build
cmake .. && make -j32 && make install

# xsimd
git clone https://github.com/xtensor-stack/xsimd.git
cd xsimd && mkdir build && cd build
cmake .. && make -j32 && make install

git clone https://github.com/JuliaStrings/utf8proc.git
cd utf8proc && mkdir build && cd build
cmake .. && make -j32 && make install
```