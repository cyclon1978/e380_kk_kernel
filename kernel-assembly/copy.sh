MAJOR_VERSION=9

while read line; do
     INCREMENT=$line
done < ./current.ver

VERSTR=$MAJOR_VERSION.$(printf %03d $INCREMENT)
FILENAME=testkernel_$VERSTR.zip

echo filename would be $FILENAME

read -p "Increment minor version numer or use current minor version: $INCREMENT (y for increment)? " 
if [[ $REPLY =~ ^[Yy]$ ]]
then
  INCREMENT=$((INCREMENT + 1)) 
  echo $INCREMENT > current.ver
fi

VERSTR=$MAJOR_VERSION.$(printf %03d $INCREMENT)
FILENAME=testkernel_$VERSTR.zip

echo "Using version $INCREMENT and that will result in filename $FILENAME"

adb push ./testkernel.zip /storage/sdcard0/$FILENAME
rm ./$FILENAME
mv ./testkernel.zip $FILENAME

echo "Done."
