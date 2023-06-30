.PHONY: all
all: dchmod dataset_test

dchmod: dchmod.o dchmodcli.o dataset.o

dataset_test: dataset_test.o dataset.o

dchmod.o: dchmod.c dchmod.h

dchmodcli.o: dchmodcli.c dchmod.h dchmodcli.h

dataset.o: dataset.c dataset.h 

dataset_test.o: dataset_test.c dataset.h

.PHONY: clean
clean: 
	rm -f dchmod dataset_test *.o *.lst
