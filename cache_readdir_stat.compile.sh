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
    gcc -Wall -Wno-unused-function  -g -rdynamic -fsanitize=bounds    $PWD/$name.c -lpthread -o $exe -ldl
    ls -l $exe
else
    if false;then
        gcc -O0 -Wall -Wno-unused-function  -g  -fpic -shared -ldl -o $so $PWD/$name.c  -lpthread -ldl
    else
        as=''
        if false;then
        as="-fsanitize=address -fno-omit-frame-pointer"
        asan=/usr/lib/llvm-10/lib/clang/10.0.0/lib/linux/libclang_rt.asan-x86_64.so
        [[ ! -s $asan ]] && as=''
        fi
        clang -O0 -Wall -Wno-unused-function  -g $as -rdynamic  -fpic -shared -ldl -o $so $PWD/$name.c  -lpthread -ldl
        fi
    #       gcc -c -Wall  -fpic $name.c;    gcc -shared -o $name.so $name.o -ldl
    cp -v -u $name.sh $dst/
    ls -l $so
fi
