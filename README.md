# iPref

the source code for the bounded size top-k query (ORD, ORU) problem.


before running, please download qhull and osqp, compile it and install so it can be used as static lib
an example of osqp is here: https://osqp.org/docs/get_started/sources.html

for qhull, use this link https://github.com/qhull/qhull to download and follow the same instruction introduced in osqp
to install qhull


The cmake file is ready for this project,
change the directory of qhull in CMakeList.txt if you install in any where else

to run:

-w 1 -k 5 -d 4 -X 50 -f ./pdt/pdt4d1600k.txt -m UTK_BB -W ./user/user4d200k.txt -n 400000 -i ./idx/idx7.txt 