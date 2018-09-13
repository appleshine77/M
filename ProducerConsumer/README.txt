Documentation for Warmup Assignment 2
=====================================

+------------------------+
| BUILD & RUN (Required) |
+------------------------+

Replace "(Comments?)" below with the command the grader should use to compile
your program (it should simply be "make" or "make warmup2"; minor variation is
also fine).

To compile your code, the grader should type: make

+--------------+
| SELF-GRADING |
+--------------+

Replace each "?" below with a numeric value:

Basic running of the code : 0 out of 100 pts

Missing required section(s) in README file : -0 pts
Cannot compile : -0 pts
Compiler warnings : -0 its
"make clean" : -0 pts
Segmentation faults : -0 pts
Separate compilation : -0 pts
Using busy-wait : -0 pts
Handling of commandline arguments:
    1) -n : -0 pts
    2) -lambda : -0 pts
    3) -mu : -0 pts
    4) -r : -0 pts
    5) -B : -0 pts
    6) -P : -0 pts
Trace output :
    1) regular packets: -0 pts
    2) dropped packets: -0 pts
    3) removed packets: -0 pts
    4) token arrival (dropped or not dropped): -0 pts
    5) monotonically non-decreasing timestamps: -0 pts
Statistics output :
    1) inter-arrival time : -0 pts
    2) service time : -0 pts
    3) number of customers in Q1 : -0 pts
    4) number of customers in Q2 : -0 pts
    5) number of customers at a server : -0 pts
    6) time in system : -0 pts
    7) standard deviation for time in system : -0 pts
    8) drop probability : -0 pts
Output bad format : -0 pts
Output wrong precision for statistics (should be 6-8 significant digits) : -0 pts
Large service time test : -0 pts
Large inter-arrival time test : -0 pts
Tiny inter-arrival time test : -0 pts
Tiny service time test : -0 pts
Large total number of customers test : -0 pts
Large total number of customers with high arrival rate test : -0 pts
Dropped tokens test : -0 pts
Cannot handle <Cntrl+C> at all (ignored or no statistics) : -0 pts
Can handle <Cntrl+C> but statistics way off : -0 pts
Cannot stop token depositing thread when required : -0 pts
Not using condition variables and do some kind of busy-wait : -0 pts
Synchronization check : -0 pts
Deadlocks : -0 pts
Bad commandline or command : -0 pts

+----------------------+
| BUGS / TESTS TO SKIP |
+----------------------+

Are there are any tests mentioned in the grading guidelines test suite that you
know that it's not working and you don't want the grader to run it at all so you
won't get extra deductions, please replace "(Comments?)" below with your list.
(Of course, if the grader won't run such tests in the plus points section, you
will not get plus points for them; if the garder won't run such tests in the
minus points section, you will lose all the points there.)  If there's nothing
the grader should skip, please replace "(Comments?)" with "none".

Please skip the following tests: (Comments?)
No
+-----------------------------------------------+
| OTHER (Optional) - Not considered for grading |
+-----------------------------------------------+

Comments on design decisions: (Comments?)
No
