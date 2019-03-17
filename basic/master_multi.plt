set terminal png
set output "exp4_plot.png"

set multiplot layout 2,1 rowsfirst

set title "Without Channel"
set xlabel "Time"
set ylabel "No. of bytes"
set xrange [2:25]
plot "ULRLCexp4withoutchannel.txt" using 1:8 title 'Uplink Tx Bytes' with linespoints, "DLRLCexp4withoutchannel.txt" using 1:8 title 'Downlink Rx Bytes' with linespoints

set title "With Channel"
set xlabel "Time"
set ylabel "No. of bytes"
set xrange [2:25]
plot "ULRLCexp4withchannel.txt" using 1:8 title 'Uplink Tx Bytes' with linespoints, "DLRLCexp4withchannel.txt" using 1:8 title 'Downlink Rx Bytes' with linespoints

