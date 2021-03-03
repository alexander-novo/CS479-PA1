if (!exists("outfile")) outfile='plot.png'

# Read quadratic parameters
paramNames = system(' head -1 '.paramFile)
paramCount = words(paramNames)
paramLine = system('head -2 '.paramFile.' | tail -1')

do for [i = 1:paramCount] {
	eval(word(paramNames, i).' = '.word(paramLine, i))
}

# General Quadratic, a function of two variables - we're only interested in the contour curve at f(x,y) = 0, though.
f(x,y) = a*x*x + b*x*y + c*y*y + d*x + e*y + f

set contour
set view map
unset surface
set cntrparam levels discrete 0
set isosamples 4000,4000

set xrange [xmin:xmax]
set yrange [ymin:ymax]
set table $ContourTable
splot f(x, y)

unset table
unset contour
unset xrange
unset yrange


# set terminal pdf enhanced size 8in, 4.8in
set terminal png enhanced size 8000, 4800 transparent truecolor font "arial,60"
set encoding utf8
set output outfile

set autoscale fix

set border lw 3
set style fill transparent solid 0.075 noborder
set style circle radius 0.03

set title plotTitle

plot sample1 u 1:2 w circles notitle,\
     sample2 u 1:2 w circles notitle,\
	 $ContourTable w lines lw 6 lc rgb 'black' notitle,\
	 keyentry w circles fill solid 1.0 noborder lc 1 title "ω_1",\
	 keyentry w circles fill solid 1.0 noborder lc 2 title "ω_2"