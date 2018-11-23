#*******************************
# Makefile
# CMPUT 379 Assignment 3
# Adit Hasan (1459800)
#*******************************
target= submit
allFiles= Makefile a3sdn.c UtilityHeader.c ParseUtility.c PrintUtility.c IncludeUtility.h

all: start

start: 
	gcc -w a3sdn.c -o a3sdn

clean:
	rm -rf a3sdn
	rm -rf *.tar

tar:
	touch $(target).tar
	tar -cvf $(target).tar $(allFiles)
