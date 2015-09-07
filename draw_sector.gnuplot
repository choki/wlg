a=0 
set title 'I/O trace'
set datafile separator ","
set yrange [-10:1000]
set xrange [80000:900000]
set ytics 300
set xtics 100
set ylabel 'Qeueued sectors'
set xlabel 'Time'
f(x)=((x) eq 'C'?a=a-$6:a=a+$6)
plot "trace.blkparse" u ($2*1000):(f(strcol(4))) with steps title 'Issue / Complete'
#plot "trace.blkparse_gen" u ($2*1000):(f(strcol(4))) with steps title 'Issue / Complete', "trace.blkparse_rep" u ($2*1000):(f(strcol(4))) with steps 
