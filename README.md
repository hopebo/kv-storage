# Simple-KV
Simple-KV is a fast key-value storage that provides an ordered mapping from string keys to string values.

## Features

- Keys and values are arbitrary byte arrays.
- Data is stored sorted by key.
- The basic opearation are `Put(key, value)`, `Get(key)`, `Delete(key)`.
- Client-server support.

## Overview

- db: main classes related to KV storage, including _data_base.cpp_, _storage_buffer.cpp_, _storage_engine.cpp_.
- structure: data structures involved, including _cache.cpp_, _concurrent_queue.h_, _read_write_lock.h_, _skip_list.h_, _thread_pool.h_.
- type: self-defined data types.
- util: utilities involved.
- benchmark
- unit-tests
- Makefile

## Start

```
make
./server_main
```

By default, KV storage server is listening to port 7777.

## Architecture

<img src="https://github.com/hopebo/Simple-KV/blob/master/images/architecture.png" width="70%" alt="Architecture"/>

## Benchmark

### Environment

```
Machine type: x86_64 Linux
CPU: Intel(R) Xeon(R) CPU E5-2680 v4 @ 2.40GHz(56 cores)
Memory available: 44,239 MB
```

### KV Storage Performance

Single thread, offset table cache num: 1000, key size: 25 bytes

1. _Operation Throughput(ops/s)_

| Value Size | Sequence Write | Random Write | Sequence Read | Random Read |
| :--------: | :------------: | :----------: | :-----------: | :---------: |
| 100 Bytes  |    106,589     |    93,692    |    64,399     |   66,253    |
| 1000 Bytes |     63,291     |    59,825    |    29,003     |   30,696    |

2. _CPU Usage(%)_

| Value Size | Sequence Write | Random Write | Sequence Read | Random Read |
| :--------: | :------------: | :----------: | :-----------: | :---------: |
| 100 Bytes  |      2.01      |     3.85     |     1.82      |    1.83     |
| 1000 Bytes |      3.77      |     3.88     |     2.23      |    1.80     |

### Network Server Performance

Multiple thread, offset table cache num: 1000, key size: 25 bytes

1. _Client Request Throughput(ops/s)_

| Value Size | Sequence Write | Random Write | Sequence Read | Random Read |
| :--------: | :------------: | :----------: | :-----------: | :---------: |
| 100 Bytes  |     83,404     |    79,317    |    90,854     |   87,638    |
| 1000 Bytes |     77,905     |    78,688    |    86,178     |   69,563    |

2. _Client Request Latency(usec)_

| Value Size | Sequence Write | Random Write | Sequence Read | Random Read |
| :--------: | :------------: | :----------: | :-----------: | :---------: |
| 100 Bytes  |     167.80     |    182.05    |    152.06     |   162.05    |
| 1000 Bytes |     181.76     |    181.11    |    162.61     |   206.38    |