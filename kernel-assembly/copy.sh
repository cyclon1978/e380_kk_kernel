MAJOR_VERSION=6

while read line; do
     INCREMENT=$line
done < ./current.ver

echo filename would be testkernel_$MAJOR_VERSION.$INCREMENT.zip

read -p "Increment version numer or use current version: $INCREMENT (y for increment)? " 
if [[ $REPLY =~ ^[Yy]$ ]]
then
  INCREMENT=$((INCREMENT + 1)) 
  echo $INCREMENT > current.ver
fi

echo $INCREMENT > current.ver

echo "Using version $INCREMENT ..."

adb push ./testkernel.zip /storage/sdcard0/testkernel_$MAJOR_VERSION.$INCREMENT.zip
rm ./testkernel_$MAJOR_VERSION.$INCREMENT.zip
mv ./testkernel.zip testkernel_$MAJOR_VERSION.$INCREMENT.zip

echo "Done."
