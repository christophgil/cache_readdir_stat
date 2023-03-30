#!/usr/bin/env bash
name=cache_readdir_stat
dst=~/compiled
exe=$dst/$name
so=$dst/$name.so
mkdir -p $dst
rm "$exe" "$so" 2>/dev/null

if grep '^ *int  *main(' $name.c;then
    echo "Found int main(...  Therefore compiling test program $exe ..."
    echo $'\e[31m You need to rename main(argc,argv)  to make the shared lib.  \nMaybe rename it to XXXmain( ) \e[0m'
    gcc -Wall -Wno-unused-function  -g -rdynamic -fsanitize=bounds    $name.c -lpthread -o $exe -ldl
    ls -l $exe
else
    gcc -Wall -Wno-unused-function  -g  -fpic -shared -ldl -o $so $name.c  -lpthread -ldl
    #       gcc -c -Wall  -fpic $name.c;    gcc -shared -o $name.so $name.o -ldl
    cp -v -u $name.sh $dst/
    ls -l $so
fi
