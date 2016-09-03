include MakeVars

C_SRCS= $(shell ls *.c 2> /dev/null)
C_OBJS= $(patsubst %.c,%.o,$(C_SRCS))

all: $(EXE) $(DYNLIB) dirs

dirs: 
	@(for i in $(DIRS); \
	  do make -C $$i; \
	done)

%.o: %.c
	$(CC) $(CFLAGS) -c $< 

$(EXE) : $(C_OBJS)
	$(CC) $(CFLAGS) -I. -o $(EXE) $(C_OBJS) $(LIBS)

$(DYNLIB) : $(C_OBJS)
	$(LD) $(LDFLAGS) -I. -o $(DYNLIB) $(C_OBJS)

clean :
	@(for i in $(DIRS); \
	  do make -C $$i clean; \
	done)
	rm -f *.o $(EXE) $(DYNLIB)
	rm -f *~ 
	rm -f ./$(PRAC).$(CURSO).entrega.tar.gz 

entrega : clean
	@(tar cf ../$(PRAC).$(CURSO).entrega.tar . ; \
         rm -f ../$(PRAC).$(CURSO).entrega.tar.gz ; \
         gzip -9 ../$(PRAC).$(CURSO).entrega.tar  ; \
         mv ../$(PRAC).$(CURSO).entrega.tar.gz .  ; \
         echo "Fichero de entrega generado $(PRAC).$(CURSO).entrega.tar.gz"; \
         echo "Para entregar ejecute 'entrega.so4 $(PRAC).$(CURSO)'" )

