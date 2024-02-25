# WinbondW25N on Raspberry

Inspired by https://github.com/squaresausage/WinbondW25N
Converted code to Raspberry.

Tested on Raspberry Zero W with W25N01GV

# Build
Require [wiringPi](https://github.com/WiringPi/WiringPi) which is obsolete on Raspberry

    g++ -o main main.cpp WinbondW25N.cpp -lwiringPi

# TODO
- verify
- info flash
- improve code
- migrate from wiringPi to modern library
