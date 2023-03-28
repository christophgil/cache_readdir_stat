#!/usr/bin/env bash
name=cache_readdir_stat
exe=~/compiled/$name
so=~/compiled/$name.so
mkdir -p ~/compiled
rm "$exe" "$so" 2>/dev/null

if grep '^ *int  *main(' $name.c;then
    echo "Found int main(...  Therefore compiling test program $exe ..."
    echo "You need to remove main to make the shared lib."

    gcc   -g -rdynamic -fsanitize=bounds -Wall   $name.c -o $exe -ldl
    ls -l $exe
else
    echo "Not found int main(...  Therefore compiling $so ..."
    gcc -Wall -fpic -shared -ldl -o $so $name.c
#       gcc -c -Wall  -fpic $name.c;    gcc -shared -o $name.so $name.o -ldl

    ls -l $so
fi
