#------------------------------------------------------------------------------#
#------------------------------- Directories-----------------------------------#

OBJDIR = bin/o
SODIR = bin/so
EXEDIR = exe
SRCDIR = src
TESTDIR = test
INCDIR = include

INCDIR_OUT = ../../ds/include
SRC_OUT = ../../ds/src

#------------------------------------------------------------------------------#


#----------------------------compiler and flags--------------------------------#
CC = gcc
CPPFLAGS = -I$(INCDIR_OUT) -I$(INCDIR)
DEBUG_FLAGS = -ansi -pedantic-errors -Wall -Wextra -g -fpic -lm
RELEASE_FLAGS = -ansi -pedantic-errors -Wall -Wextra -DNDBUG -O3 -fpic -lm 
SO_FLAGS = -shared -fpic
LDFLAGS = -L$(SODIR) -Wl,-rpath=$(SODIR)  

#------------------------------------------------------------------------------#


#----------------------------Targets tests-------------------------------------#
tests: $(EXEDIR)/wd_exe $(EXEDIR)/app_exe

$(EXEDIR)/app_exe: $(OBJDIR)/client_main.o $(SODIR)/libd_wd.so
	$(CC) $^ -o $@ $(DEBUG_FLAGS) $(LDFLAGS) -ld_wd -I$(INCDIR)

$(EXEDIR)/wd_exe: $(OBJDIR)/wd_main.o $(SODIR)/libd_wd.so
	$(CC) $^ -o $@ $(DEBUG_FLAGS) $(LDFLAGS) -ld_wd -I$(INCDIR)
#------------------------------------------------------------------------------#


#----------------------------Targets debug libs--------------------------------#
debug: $(SODIR)/libd_wd.so

$(SODIR)/libd_wd.so: $(OBJDIR)/d_wd.o $(OBJDIR)/scheduler.o $(OBJDIR)/task.o\
					 $(OBJDIR)/uid.o $(OBJDIR)/heap_pq.o $(OBJDIR)/vector.o\
					 $(OBJDIR)/heap.o
	$(CC) -o $@ $^ $(DEBUG_FLAGS) $(SO_FLAGS)
#------------------------------------------------------------------------------#


#----------------------------Targets release libs------------------------------#
release: 
#------------------------------------------------------------------------------#

#----------------------------Targets object debug------------------------------#
$(OBJDIR)/d_wd.o: $(SRCDIR)/wd.c $(INCDIR)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)

$(OBJDIR)/client_main.o: $(TESTDIR)/client_main.c $(INCDIR)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)

$(OBJDIR)/wd_main.o: $(SRCDIR)/wd_main.c $(INCDIR)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)

$(OBJDIR)/scheduler.o: $(SRC_OUT)/heap_scheduler.c $(INCDIR_OUT)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)

$(OBJDIR)/task.o: $(SRC_OUT)/task.c $(INCDIR_OUT)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)

$(OBJDIR)/uid.o: $(SRC_OUT)/uid.c $(INCDIR_OUT)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)

$(OBJDIR)/heap_pq.o: $(SRC_OUT)/heap_pq.c $(INCDIR_OUT)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)

$(OBJDIR)/vector.o: $(SRC_OUT)/vector.c $(INCDIR_OUT)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)	

$(OBJDIR)/heap.o: $(SRC_OUT)/heap.c $(INCDIR_OUT)/*.h
	$(CC) -c $< -o $@ $(DEBUG_FLAGS) $(CPPFLAGS)
#------------------------------------------------------------------------------#

#----------------------------Targets object release------------------------------#

#------------------------------------------------------------------------------#


#----------------------------------clean---------------------------------------#
clean:
	rm -f $(OBJDIR)/*.o $(SODIR)/*.so
	rm -f $(TESTS)
#------------------------------------------------------------------------------#
#------------------------------------------------------------------------------#
