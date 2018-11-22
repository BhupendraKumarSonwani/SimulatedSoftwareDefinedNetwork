#*******************************
# Makefile
# CMPUT 379 Assignment 2
# Adit Hasan (1459800)
#*******************************
target= submit
allFiles= Makefile a3sdn.c UtilityHeader.h ParseUtility.h PrintUtility.h IncludeUtility.h

all: start

start: start.c
	gcc -w a3sdn.c -o a3sdn.out

start.c:

clean:
	rm -rf *.out
	rm -rf *.tar

tar:
	touch $(target).tar
	tar -cvf $(target).tar $(allFiles)
