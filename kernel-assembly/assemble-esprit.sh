export CROSS_COMPILE=arm-eabi-
#mkdir system
#mkdir system/lib
#mkdir system/lib/modules
# cleanup old files
rm ./boot.img-kernel.img
#rm ./system/lib/modules/*.ko
rm ./testkernel.zip
#rm ./f3_testkernel_v41.zip
#rm ./f3_testkernel_v42.zip
cp ../out/Download/flash/kernel_simcom89_we_kk.bin ./boot.img-kernel.img
#cp ../kernel/out/mediatek/platform/mt6577/kernel/drivers/m4u/m4u.ko ./system/lib/modules/m4u.ko
#cp ../kernel/out/drivers/staging/zram/zram.ko ./system/lib/modules/zram.ko
# cp ../kernel/out/lib/lzo/lzo_compress.ko ./system/lib/modules/lzo_compress.ko
# cp ../kernel/out/lib/lzo/lzo_decompress.ko ./system/lib/modules/lzo_decompress.ko
# strip modules
#echo "**** Patching all built modules (.ko) in /build_result/modules/ ****"
#find ./system/lib/modules/ -type f -name '*.ko' | xargs -n 1 $TOOLCHAIN/arm-eabi-strip --strip-unneeded
# fix permissions
chmod 755 ./boot.img-ramdisk/ -R
/home/esprit/tools/mtk-tools/repack-MTK.pl -boot boot.img-kernel.img boot.img-ramdisk boot.img
cp ./testkernel_template.zip ./testkernel.zip
zip -u -r ./testkernel.zip ./boot.img

