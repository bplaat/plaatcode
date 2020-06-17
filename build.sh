rm -f plaatcode.exe
if [ "$1" == "release" ]; then
    if windres src/resource.rc resource.o; then
        gcc -s -O2 $(find src -name "*.c") resource.o -lgdi32 -lcomctl32 -Wl,--subsystem,windows -o plaatcode
        rm resource.o
    fi
else
    if windres -DDEBUG src/resource.rc resource.o; then
        gcc -g -DDEBUG -Wall -Wextra -Wshadow -Wpedantic --std=c99 $(find src -name "*.c") resource.o -lgdi32 -lcomctl32 -o plaatcode
        rm resource.o
    fi
fi
