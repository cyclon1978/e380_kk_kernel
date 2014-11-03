export PATH=$PATH:$(pwd)/../../tools/android-ndk-r10c/toolchains/arm-linux-androideabi-4.8/prebuilt/linux-x86_64/bin

export ARCH=arm
export CROSS_COMPILE=arm-linux-androideabi-

./makeMtk -t simcom89_we_kk mrproper k
./makeMtk -t simcom89_we_kk c k

