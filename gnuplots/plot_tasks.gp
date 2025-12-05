set terminal pngcairo size 1200,800
set output 'hist_rtms.png'
set datafile separator ","
bin(x,w)=w*floor(x/w)
width=0.001

plot 'log_tasks.csv' using (bin($4,width)):(1.0) smooth freq with boxes title "RT_ms"

