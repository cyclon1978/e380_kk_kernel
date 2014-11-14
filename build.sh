export PATH=$PATH:$(pwd)/../../tools/android-ndk-r10c/toolchains/arm-linux-androideabi-4.8/prebuilt/linux-x86_64/bin

export ARCH=arm
export CROSS_COMPILE=arm-linux-androideabi-

#Workaround for + appended on kernelrelease, may not be required
export LOCALVERSION=

# this is essential to build a working kernel!
export TARGET_BUILD_VARIANT=user

export KBUILD_BUILD_USER=cyclon1978
export KBUILD_BUILD_HOST=web.de


./mk simcom89_we_kk n k

