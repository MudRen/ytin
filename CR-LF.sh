echo Converting CR-LF to LF ...
for file in `find . -type f -print`; do
  perl -i -pe 's:\r$::g' $file
done
cd readline
perl -e "s/\r$//g" -p -i *
cd ../protos
perl -e "s/\r$//g" -p -i *
cd ..
echo Done!

