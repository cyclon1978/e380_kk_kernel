export JAVA_HOME=/home/esprit/tools/java/jdk1.6
unset JAVA_TOOL_OPTIONS
unset _JAVA_OPTIONS

export PATH=$JAVA_HOME/bin:$PATH:/home/esprit/tools/arm-linux-androideabi-4.9/bin

export ARCH=arm
export CROSS_COMPILE=arm-linux-androideabi-

#Workaround for + appended on kernelrelease, may not be required
export LOCALVERSION=

# this is essential to build a working kernel!
export TARGET_BUILD_VARIANT=user

export KBUILD_BUILD_USER=cyclon1978
export KBUILD_BUILD_HOST=web.de

./mk simcom89_we_kk n k

