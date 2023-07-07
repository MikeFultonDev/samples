.PHONY: all
all: dchmod dataset_test accessid_test

dchmod: dchmod.o dchmodcli.o dchmodracf.o dchmodts.o dchmodacf2.o zossys.o dataset.o accessid.o

dataset_test: dataset_test.o dataset.o

accessid_test: accessid_test.o accessid.o

dchmod.o: dchmod.c dchmod.h

dchmodcli.o: dchmodcli.c dchmod.h dchmodcli.h dataset.h accessid.h

dchmodracf.o: dchmodracf.c dchmod.h dchmodracf.h

dchmodts.o: dchmodts.c dchmod.h dchmodts.h

dchmodacf2.o: dchmodacf2.c dchmod.h dchmodacf2.h

dataset.o: dataset.c dataset.h 

dataset_test.o: dataset_test.c dataset.h

accessid.o: accessid.c accessid.h 

accessid_test.o: accessid_test.c accessid.h

.PHONY: clean
clean: 
	rm -f dchmod dataset_test accessid_test *.o *.lst
