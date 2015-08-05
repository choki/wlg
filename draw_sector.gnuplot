a=0 
set title 'I/O trace'
set datafile separator ","
set yrange [-10:1000]
set ytics 32
set xtics 1
set ylabel 'Qeueued sectors'
set xlabel 'Time'
f(x)=((x) eq 'C'?a=a-$6:a=a+$6)
#plot "trace" u ($2*1000):(f(strcol(4))) with steps title 'Issue / Complete'
plot "trace_1" u ($2*1000):(f(strcol(4))) with steps title 'Issue / Complete', "trace_2" u ($2*1000):(f(strcol(4))) with steps 
