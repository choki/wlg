a=0
set title 'I/O trace'
set datafile separator ","
set yrange [0:0.0005]
set ytics 0.0001
set xtics 100
set ylabel 'Latency'
set xlabel 'Time'
f1(x)=((x) eq '32'?a=$7:NaN)
f2(x)=((x) eq '256'?a=$7:NaN)
f3(x)=((x) eq '8'?a=$7:NaN)
plot "trace_p" u ($2*1000):(f1(strcol(6))) with points title '512', "trace_p" u ($2*1000):(f2(strcol(6))) with points title '256', "trace_p" u ($2*1000):(f3(strcol(6))) with points title '8'