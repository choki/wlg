set title 'I/O trace'
set datafile separator ","
set yrange [-10:100]
set ytics 1
set xtics 1
set ylabel 'Latency'
set xlabel 'Time'
f(x,y)= (x eq "512"?y:0)
plot "trace_p" u ($2*1000):(f(strcol(6),6)) with points title 'Issue / Complete'


