#!/usr/bin/env bash
SRC=${BASH_SOURCE[0]}
echo SRC=$SRC >&2
so=${SRC%/*}/cache_readdir_stat.so
if [[ -f $so ]];then
    asan=''
    if false;then
    asan=/usr/lib/llvm-10/lib/clang/10.0.0/lib/linux/libclang_rt.asan-x86_64.so
    [[ ! -s $asan ]] && asan=''
    fi
    export LD_PRELOAD="$asan:$so ${LD_PRELOAD:-}"
else
    echo $'\e[41m'"Warning: no file $so"$'\e[0m' >&2
fi
exec "$@"
