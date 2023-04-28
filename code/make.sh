#/bin/sh

make
mkdir build
# find . -maxdepth 1 -type f ! -name 'xtic_enet_main.c'\
#  ! -name '*.h' ! -name 'Makefile' ! -name '.gitignore'\
#   ! -name 'README.md' ! -name 'make.sh' ! -name 'main.c' | grep -v '/$' | xargs -I{} mv {} build

