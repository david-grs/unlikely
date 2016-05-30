The code here is measuring the effect of a good, bad or non usage of the likely/unlikely hints. 


Results
========
[The results are here](https://docs.google.com/spreadsheets/d/1EeC85u_8TcMqYpP68IKHj1Ej9prlyyI7pzeqY_eVRD8/pubhtml)

Environment:
 - CPU Ivy Bridge i7-3537U
 - GCC 5.3.0 - Release mode
 - Linux 3.13.0


How it works
============
Two main tests that are run three times &mdash; with the correct hint, the wrong hint and no hint at all:
 - non-taken conditional jumps, where the size of the dead code (called *junk* in the source code) can vary
 - taken conditional jumps, with one or multiple instructions in the taken branch

Both tests have a number of 255 conditional jumps. 

Before the actual measurements, we can *train* the branch predictor (but also other caches... so the results have
to be taken with a grain of salt here) a number of times ; this is given as argument of the binary.

*Note:* the *non-taken conditional jumps* test is definitely better for measurements as you do not execute anything 
but the jumps...


Example of usage
================
This run the test on non-taken condition jumps, without any previous *training* of the branch predictor. 
The three numbers are the time in nanoseconds, for wrong hint, no hint and correct hint.

```
./hint not-taken 0
4061;3822;4131
```

In order to see the effect of the *training* (same comment as above though):

```
$ for i in `seq 0 1 10`; do ./hint not-taken $i; done
4061;3822;4131
1892;3979;2895
1983;3961;1534
2160;3981;1493
2084;4109;1476
1877;4148;1509
2085;4192;1507
1907;4170;1466
1854;4015;1483
1872;3941;1518
1876;4152;1502
```
