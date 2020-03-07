SYNFIREOUT = synfire
SYNFIRESRC	= SynfireMain1.cpp 
SYNFIREOBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SYNFIRESRC)) 


ANSWEROUT = answer
ANSWERSRC = AnsMain1.cpp ans_t.cpp \
               circuit_t.cpp Device_t.cpp \
               drawpix_t.cpp event_t.cpp
ANSWEROBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(ANSWERSRC)) 

TREEOUT = tree
TREESRC = TreeMain1.cpp
TREEOBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TREESRC)) 

EXPT1OUT = expt1
EXPT1SRC = Expt1Main.cpp ans_t.cpp \
               circuit_t.cpp Device_t.cpp \
               drawpix_t.cpp event_t.cpp
EXPT1OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(EXPT1SRC)) 


GENERICS_DIR = Generics
OBJ_DIR = obj
BIN_DIR = bin


GENERICSSRC = rand.cpp flat.cpp filename.cpp lex.cpp dumpchan.cpp dfprintf.cpp
GENERICSOBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(GENERICSSRC)) 


# Bash is used over sh for hashtables when building orchestrate (and probably
# other reasons).
SHELL = /bin/bash

# Smart directory creation.
MKDIR = mkdir --parents



VPATH := $(GENERICS_DIR)

OUT		= gentest
CC		= g++
FLAGS	= -g -c -Wall -I$(GENERICS_DIR)
LFLAGS	= -I$(GENERICS_DIR)


all: generics synfire answer tree


generics: $(GENERICSOBJS)


synfire: $(SYNFIREOBJS) generics
	@$(shell $(MKDIR) $(BIN_DIR))
	$(CC) -g $(SYNFIREOBJS) $(GENERICSOBJS) -o $(BIN_DIR)/$(SYNFIREOUT) $(LFLAGS)


answer: $(ANSWEROBJS) generics
	@$(shell $(MKDIR) $(BIN_DIR))
	$(CC) -g $(ANSWEROBJS) $(GENERICSOBJS) -o $(BIN_DIR)/$(ANSWEROUT) $(LFLAGS)


tree: $(TREEOBJS) generics
	@$(shell $(MKDIR) $(BIN_DIR))
	$(CC) -g $(TREEOBJS) $(GENERICSOBJS) -o $(BIN_DIR)/$(TREEOUT) $(LFLAGS)


expt1: $(EXPT1OBJS) generics
	@$(shell $(MKDIR) $(BIN_DIR))
	$(CC) -g $(EXPT1OBJS) $(GENERICSOBJS) -o $(BIN_DIR)/$(EXPT1OUT) $(LFLAGS)


# Make .o files for the common things we need from the generics dir
.SECONDEXPANSION:
$(GENERICSOBJS): $$(patsubst $$(OBJ_DIR)/%.o,$$(GENERICS_DIR)/%.cpp,$$@)
	@$(shell $(MKDIR) $(OBJ_DIR))
	$(CC) $(FLAGS) -o $@ $< -std=c++11

# Generic rule for .o files
$(OBJ_DIR)/%.o: %.cpp
	@$(shell $(MKDIR) $(OBJ_DIR))
	$(CC) $(FLAGS) -o $@ $< -std=c++11

%.o: %.cpp
	$(CC) $(FLAGS) -o $@ $< -std=c++11


clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
