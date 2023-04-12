#!/usr/bin/env bash
SRC=${BASH_SOURCE[0]}
echo SRC=$SRC >&2
so=${SRC%/*}/cache_readdir_stat.so
if [[ -f $so ]];then
    export LD_PRELOAD="$so ${LD_PRELOAD:-}"
else
    echo $'\e[41m'"Warning: no file $so"$'\e[0m' >&2
fi
exec "$@"
