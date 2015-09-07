a=0
set title 'I/O trace'
set datafile separator ","
set yrange [0:0.0005]
set ytics 0.0002
set xtics 100
set ylabel 'Latency'
set xlabel 'Time'
f1(x)=((x) eq '2'?a=$7:NaN)
f2(x)=((x) eq '4'?a=$7:NaN)
f3(x)=((x) eq '8'?a=$7:NaN)
f4(x)=((x) eq '16'?a=$7:NaN)
f5(x)=((x) eq '32'?a=$7:NaN)
f6(x)=((x) eq '64'?a=$7:NaN)
f7(x)=((x) eq '128'?a=$7:NaN)
f8(x)=((x) eq '256'?a=$7:NaN)
f9(x)=((x) eq '512'?a=$7:NaN)
f10(x)=((x) eq '1024'?a=$7:NaN)
plot "trace_p" u ($2*1000):(f1(strcol(6))) with points title '2',      "trace_p" u ($2*1000):(f2(strcol(6))) with points title '4',      "trace_p" u ($2*1000):(f3(strcol(6))) with points title '8',      "trace_p" u ($2*1000):(f4(strcol(6))) with points title '16',      "trace_p" u ($2*1000):(f5(strcol(6))) with points title '32',	"trace_p" u ($2*1000):(f6(strcol(6))) with points title '64',	"trace_p" u ($2*1000):(f7(strcol(6))) with points title '128',	"trace_p" u ($2*1000):(f8(strcol(6))) with points title '256',	"trace_p" u ($2*1000):(f9(strcol(6))) with points title '512',	"trace_p" u ($2*1000):(f10(strcol(6))) with points title '1024'
