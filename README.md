# OS-assignment

## How to run

### 1.1. Make sched 
To use `make sched`, we redirect to file `os-cfg.h` and comment out 2 lines:
- `#define MM_PAGING 1`
- `#define MM_FIXED_MEMSZ 1`
##### 1.1.1. With 3rd parameter (live priority)

Uncomment `#define MLQ_SCHED 1` (we need it)
__Sample input__:
`sched`:
4 2 3
0 p1s
3 p2s
1 p3s
 
##### 1.1.2. Without 3rd paramter (default priority):
Comment `#define MLQ_SCHED 1` (we don't need it)
__Sample input__:
`sched_0`:
2 1 2
0 s0
4 s1

### 1.2. Make all

##### 1.2.1. Fixed memory size:

With fixed mem size, uncomment the `#define MM_FIXED_MEMSZ 1`
__Sample input__:
`os_1_singleCPU_mlq`:
2 1 8
1 s4
2 s3
4 m1s
6 s2
7 m0s
9 p1s
11 s0
16 s1

##### 1.2.2. Non-fixed memory size:

Comment the `#define MM_FIXED_MEMSZ 1`

__Sample input__:
`os_1_mlq_paging`:
2 4 8
1048576 16777216 0 0 0
1 p0s 130
2 s3 39
4 m1s 15
6 s2 120
7 m0s 120
9 p1s 15
11 s0 38
16 s1 0
