.PHONY: clean all
all: rfsend
clean:
	rm -f rfsendpru_bin.h rfsend
rfsendpru_bin.h: Makefile rfsendpru.p
	pasm -c -CprussPru0Code rfsendpru.p rfsendpru
rfsend: Makefile rfsend.c rfsendpru_bin.h
	gcc -o rfsend -g -l prussdrv rfsend.c

