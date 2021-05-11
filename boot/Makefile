# Main makefile
#
# Copyright (C) 2007 Beihang University
# Written by Zhu Like ( zlike@cse.buaa.edu.cn )
#

# ========= Configuration =========

INCLUDES	  := -I../include/

# ======== End of configuration ===

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<


%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c $< 

.PHONY: clean

all: start.o exc_hand.o

clean:
	rm -rf *~ *.o

include ../include.mk
