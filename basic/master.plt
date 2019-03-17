set terminal png
set output "exp_plot.png"

set title "Congestion Window"
set xlabel "Time"
set ylabel "No. of bytes"
plot "exp2_1.cwnd" using 1:2 title 'Congestion 1' with linespoints, "exp2_2.cwnd" using 1:2 title 'Congestion 2' with linespoints
