#! /usr/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

#lab2b_1.png
set title "Throughput v. Number of Threads (mutex and spin-lock synchronized)"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput (operations/second)"
set logscale y 10
set output 'lab2b_1.png'
set key right top
#dvor?? idk if right
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/(($6)/($5))) title 'Mutex' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/(($6)/($5))) title 'Spin' with linespoints lc rgb 'red'


#lab2b_2.png
set title "Average Time per Mutex Wait and Operation v. Number of Threads"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key right top
#cassar?? idk if right
plot \
    "< grep -E 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) title 'Average Mutex Wait Time' with linespoints lc rgb 'blue', \
    "< grep -E 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) title 'Average Operation Time' with linespoints lc rgb 'red'


#lab2b_3.png
set title "Successful Iterations v. Number of Threads"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
set key right top
#cassar?? idk if right
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) title 'Unprotected' with points lc rgb 'blue', \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) title 'Mutex' with points lc rgb 'red', \
	"< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) title 'Spin' with points lc rgb 'green'


#lab2b_4.png
set title "Throughput v. Number of Threads (mutex synchronized partitioned)"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'
#cassar?? idk if right
plot \
    "< grep 'list-none-m,[0-9]*,[0-9]*,1,' lab2b_list.csv" using ($2):(1000000000/($7)) title '1 list' with linespoints lc rgb 'blue', \
    "< grep 'list-none-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):(1000000000/($7)) title '4 lists' with linespoints lc rgb 'red', \
	"< grep 'list-none-m,[0-9]*,[0-9]*,8,' lab2b_list.csv" using ($2):(1000000000/($7)) title '8 lists' with linespoints lc rgb 'green', \
	"< grep 'list-none-m,[0-9]*,[0-9]*,16,' lab2b_list.csv" using ($2):(1000000000/($7)) title '16 lists' with linespoints lc rgb 'yellow'


#lab2b_5.png
set title "Throughput v. Number of Threads (spinlock synchronized partitioned)"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_5.png'
#cassar?? idk if right
plot \
    "< grep 'list-none-s,[0-9]*,[0-9]*,1,' lab2b_list.csv" using ($2):(1000000000/($7)) title '1 list' with linespoints lc rgb 'blue', \
    "< grep 'list-none-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):(1000000000/($7)) title '4 lists' with linespoints lc rgb 'red', \
	"< grep 'list-none-s,[0-9]*,[0-9]*,8,' lab2b_list.csv" using ($2):(1000000000/($7)) title '8 lists' with linespoints lc rgb 'green', \
	"< grep 'list-none-s,[0-9]*,[0-9]*,16,' lab2b_list.csv" using ($2):(1000000000/($7)) title '16 lists' with linespoints lc rgb 'yellow'