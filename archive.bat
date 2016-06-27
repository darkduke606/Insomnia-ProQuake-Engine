echo Erase zip files
erase *.zip
echo Create src.zip
7za a -tzip proquake-src.zip @srcfiles.txt
namedate /XYZ:"Ymd_HM" "proquake-src.zip"
echo Create bin.zip
7za a -tzip proquake.zip @binfiles.txt
namedate /XYZ:"Ymd_HM" "proquake.zip"
echo Done
pause