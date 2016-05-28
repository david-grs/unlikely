The code here is measuring the effect of a good / bad usage of the likely/unlikely macros. 


How it works
============
An array of 255 values is filled with zeroes, for each of the value we compare if it is or not greater than N (0 <= N <= 255). This conditional code is then never called and therefore is dead code, but only the user knows that.

We measure the speed of the two functions if we indicate to the compiler that the branch is likely or unlikely to be taken. 

In the following graph, the lower is the better:
 - the likely version (wrong hint) is in blue
 - the unlikely version (good hint) is in red
 - the x-axis represents the number of calls of the function before we actually call and measure it

![likely/unlikely usage](./results.png)


What we see
===========
- after three / four calls, the speed is stable and does decrease anymore ; almost all branches are well predicted (you can see it with the hardware counters printed by the benchmark).
- with *one* first call as training sequence, we see a big gap in the performance, this is not only due to better branches prediction but also to the TLB
- it might be different on other platforms / CPU, this test has run on the Ivy bridge CPU Intel i7-3537U.

