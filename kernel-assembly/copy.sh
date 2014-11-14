while read line; do
     INCREMENT=$line
done < ./current.ver

echo filename would be testkernel_3.$INCREMENT.zip

read -p "Increment version numer or use current version: $INCREMENT (y for increment)? " 
if [[ $REPLY =~ ^[Yy]$ ]]
then
  INCREMENT=$((INCREMENT + 1)) 
  echo $INCREMENT > current.ver
fi

echo $INCREMENT > current.ver

echo "Using version $INCREMENT ..."

adb push ./testkernel.zip /storage/sdcard0/testkernel_3.$INCREMENT.zip

echo "Done."
