##================================================================
##	DIRECTORY TREE
##================================================================

PDIPATH		?= ./PDIInstall
BUILDPATH	?= .

LIB		= $(BUILDPATH)/lib
OBJ		= $(BUILDPATH)/obj
SRC		= src
INC		= include
DEP		= deps/iniparser


##================================================================
##	COMPILERS
##================================================================

CC		?= cc
MPICC		?= mpicc

##================================================================
##	FLAGS
##================================================================

PDIFLAGS	= -fPIC -g -Iinclude/

##================================================================
##	TARGETS
##================================================================

OBJ_PDI		= $(OBJ)


OBJS		= $(OBJ)/api.o $(OBJ)/topo.o 
		 


SHARED		= libpdi.so
STATIC		= libpdi.a

all: $(LIB)/$(SHARED) $(LIB)/$(STATIC)

$(OBJ)/%.o : $(SRC)/%.c
		@mkdir -p $(OBJ)
		$(MPICC) $(PDIFLAGS) -c $< -o $@


$(LIB)/$(SHARED) : $(OBJS)
		@mkdir -p $(LIB)
		$(CC) -shared -o $@ $(OBJS) -lc

$(LIB)/$(STATIC) : $(OBJS)
		@mkdir -p $(LIB)
		$(RM) $@
		$(AR) -cvq $@ $(OBJS)

install : $(LIB)/$(SHARED) $(LIB)/$(STATIC)
		install -d $(PDIPATH)/lib $(PAIPATH)/include
		install $(INC)/* $(PDIPATH)/include/
		install $(LIB)/* $(PDIPATH)/lib/

uninstall:
		$(RM) $(PDIPATH)/$(LIB)/* $(PDIPATH)/$(INC)/*
		if [ -d "$(PDIPATH)/$(LIB)" ]; then rmdir $(PDIPATH)/$(LIB); fi
		if [ -d "$(PDIPATH)/$(INC)" ]; then rmdir $(PDIPATH)/$(INC); fi
		if [ -d "$(PDIPATH)" ]; then rmdir $(PDIPATH); fi
		
clean:	
		$(RM) $(OBJ)/* $(LIB)/* 

.PHONY:		all install uninstall clean

