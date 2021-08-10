./builder.py --clean
./builder.py

magick convert res/icon.png -define icon:auto-resize="16,32,48,256" res/icon.ico
windres res/resources.rc -o target/x64/debug/resources.res

cd target/x64/debug
ResourceHacker -open plaatcode.exe -save plaatcode.exe -action addskip -res resources.res -log NUL
./plaatcode
