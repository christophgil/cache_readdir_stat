# cache_readdir_stat

## Motivation

The shared library timsdata.dll is used to read  proprietary mass spectrometry data from mass spectrometry files.

We encountered a performance problem.

The timsdata.dll   calls functions of the file API millions of times.  For normal local file systems these
library calls are cheap and are not a great problem. However, for our remote compressed file storage, they massively degrade performance..

## Implementation

Our software  provides a cache for these frequent API calls that are guaranteed to return each time the same result.
It is implemented as a hash table.
Only the first invocation is passed to the API.
Subsequently, the cached result is returned.

## Result

With cache_readdir_stat.so, the Bruker data can be read significantly faster.
The speed gain is great in our case where the data reside on remote compressed  file systems.
