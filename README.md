# pirafetch - only for arch

# how to run?
- install gcc (sudo pacman -S gcc)
- git clone https://github.com/Asplushii/pirafetch && cd pirafetch
- gcc -O3 -march=native -Wall -Wextra -o pirafetch main.c
- or if you wish to use it anywhere: 
- sudo gcc -O3 -march=native -Wall -Wextra -o /sbin/pirafetch main.c
