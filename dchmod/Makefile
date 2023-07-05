.PHONY: all
all: dchmod dataset_test

dchmod: dchmod.o dchmodcli.o dchmodracf.o dchmodts.o dchmodacf2.o zossys.o dataset.o

dataset_test: dataset_test.o dataset.o

dchmod.o: dchmod.c dchmod.h

dchmodcli.o: dchmodcli.c dchmod.h dchmodcli.h

dchmodracf.o: dchmodracf.c dchmod.h dchmodracf.h

dchmodts.o: dchmodts.c dchmod.h dchmodts.h

dchmodacf2.o: dchmodacf2.c dchmod.h dchmodacf2.h

dataset.o: dataset.c dataset.h 

dataset_test.o: dataset_test.c dataset.h

.PHONY: clean
clean: 
	rm -f dchmod dataset_test *.o *.lst
