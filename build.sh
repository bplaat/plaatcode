if [ "$1" == "release" ]; then
    gcc -s -O2 $(find src -name "*.c") -lgdi32 -Wl,--subsystem,windows -o plaatcode
else
    gcc -g -DDEBUG -Wall -Wextra -Wshadow -Wpedantic --std=c99 $(find src -name "*.c") -lgdi32 -o plaatcode
fi
