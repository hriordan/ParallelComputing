timecount: counter.c 
		gcc -std=gnu99 -D_XOPEN_SOURCE=600 -O3 -pthread -DMUTEX -Wall counter.c locks.c Utils/stopwatch.c -o timecounter

workcount: workcounter.c locks.c
		gcc -std=gnu99 -D_XOPEN_SOURCE=600 -O3 -pthread -DMUTEX -Wall workcounter.c locks.c Utils/stopwatch.c -o workcount
