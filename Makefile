# Uncomment the next two lines to use parallel processing (OpenMP)
PARALLEL_FLAGS = -DPARALLEL_SUPPORT -fopenmp
PARALLEL_LIB = -lgomp

CC = g++
RAVERSION=2010_4rc2
LIBCDSPATH = libcds/
LIBRLCSAPATH = incbwt/
CPPFLAGS = -Wall -I$(LIBRLCSAPATH) -I$(LIBCDSPATH)includes/ -g -DMASSIVE_DATA_RLCSA $(PARALLEL_FLAGS) -std=c++0x -O3 -DNDEBUG
LIBCDS = $(LIBCDSPATH)lib/libcds.a
LIBRLCSA = $(LIBRLCSAPATH)rlcsa.a

FMINDEXOBJS = FMIndex.o Tools.o HuffWT.o BitRank.o ResultSet.o
OBJS = InputReader.o OutputWriter.o Pattern.o TextCollection.o TextCollectionBuilder.o \
       Query.o TextStorage.o

all: metaenumerate builder metaserver

fastqqualitytrim: fastqqualitytrim.o
	$(CC) $(CPPFLAGS) -o fastqqualitytrim fastqqualitytrim.o

maptoreads: $(LIBCDS) $(LIBRLCSA) maptoreads.o $(FMINDEXOBJS) $(OBJS)
	$(CC) $(CPPFLAGS) -o maptoreads maptoreads.o $(FMINDEXOBJS) $(OBJS) $(LIBCDS) $(LIBRLCSA)

aaaligner: $(LIBCDS) $(LIBRLCSA) aaaligner.o $(FMINDEXOBJS) $(OBJS)
	$(CC) $(CPPFLAGS) -o aaaligner aaaligner.o $(FMINDEXOBJS) $(OBJS) $(LIBCDS) $(LIBRLCSA)

metaserver: metaserver.o  ServerSocket.o
	$(CC) $(CPPFLAGS) -o metaserver metaserver.o ServerSocket.o -lm

metaenumerate: $(LIBCDS) $(LIBRLCSA) $(FMINDEXOBJS) $(OBJS) metaenumerate.o ClientSocket.o EnumerateQuery.o
	$(CC) $(CPPFLAGS) -o metaenumerate metaenumerate.o $(OBJS) $(FMINDEXOBJS) $(LIBCDS) $(LIBRLCSA) $(PARALLEL_LIB) ClientSocket.o EnumerateQuery.o

builder: $(LIBCDS) $(LIBRLCSA) $(FMINDEXOBJS) $(OBJS) builder.o
	$(CC) $(CPPFLAGS) -o builder builder.o $(OBJS) $(FMINDEXOBJS) $(LIBCDS) $(LIBRLCSA)

sabuilder: $(LIBCDS) $(LIBRLCSA) $(FMINDEXOBJS) $(OBJS) sabuilder.o
	$(CC) $(CPPFLAGS) -o sabuilder sabuilder.o $(OBJS) $(FMINDEXOBJS) $(LIBCDS) $(LIBRLCSA)

$(LIBRLCSA):
	@make -C $(LIBRLCSAPATH) library

$(LIBCDS):
	@make -C $(LIBCDSPATH)

depend:
	g++ -I$(LIBRLCSAPATH) -I$(LIBCDSPATH)includes/ -MM -std=c++0x *.cpp > dependencies.mk

clean:
	rm -f core *.o *~ metaenumerate builder metaserver fastqqualitytrim sabuilder maptoreads aaaligner
	@make -C $(LIBCDSPATH) clean
	@make -C $(LIBRLCSAPATH) clean

shallow_clean:
	rm -f core *.o *~ metaenumerate builder metaserver fastqqualitytrim sabuilder maptoreads aaaligner

include dependencies.mk
