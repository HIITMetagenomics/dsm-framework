CC = g++
MCSTL_ROOT = /fs-3/d/jltsiren/suds/mcstl
SIZE_FLAGS = -DMASSIVE_DATA_RLCSA
# PARALLEL_FLAGS = -DMULTITHREAD_SUPPORT -I$(MCSTL_ROOT)/c++ -fopenmp
CPPFLAGS = -Wall -O3 $(SIZE_FLAGS) $(PARALLEL_FLAGS)
OBJS = rlcsa.o rlcsa_builder.o sasamples.o lcpsamples.o bits/array.o bits/bitvector.o bits/deltavector.o bits/rlevector.o bits/vectors.o misc/parameters.o misc/utils.o qsufsort/qsufsort.o

VPATH = bits:misc:utils


default: parallel_build sample_lcp build_plcp lcp_test read_bwt rlcsa_test


library: $(OBJS)
	ar rcs rlcsa.a $(OBJS)

depend:
	g++ -MM *.cpp bits/*.cpp misc/*.cpp qsufsort/*.c utils/*.cpp > dependencies.mk

rlcsa_test: rlcsa_test.o library
	$(CC) $(CPPFLAGS) -o rlcsa_test rlcsa_test.o rlcsa.a

lcp_test: lcp_test.o library
	$(CC) $(CPPFLAGS) -o lcp_test lcp_test.o rlcsa.a

parallel_build: parallel_build.o library
	$(CC) $(CPPFLAGS) -o parallel_build parallel_build.o rlcsa.a

build_rlcsa: build_rlcsa.o library
	$(CC) $(CPPFLAGS) -o build_rlcsa build_rlcsa.o rlcsa.a

locate_test: locate_test.o library
	$(CC) $(CPPFLAGS) -o locate_test locate_test.o rlcsa.a

display_test: display_test.o library
	$(CC) $(CPPFLAGS) -o display_test display_test.o rlcsa.a

read_bwt: read_bwt.o library
	$(CC) $(CPPFLAGS) -o read_bwt read_bwt.o rlcsa.a

text_generator: text_generator.o library
	$(CC) $(CPPFLAGS) -o text_generator text_generator.o rlcsa.a

extract_sequence: extract_sequence.o library
	$(CC) $(CPPFLAGS) -o extract_sequence extract_sequence.o rlcsa.a

rlcsa_grep: rlcsa_grep.o library
	$(CC) $(CPPFLAGS) -o rlcsa_grep rlcsa_grep.o rlcsa.a

build_plcp: build_plcp.o library
	$(CC) $(CPPFLAGS) -o build_plcp build_plcp.o rlcsa.a

sample_lcp: sample_lcp.o library
	$(CC) $(CPPFLAGS) -o sample_lcp sample_lcp.o rlcsa.a

extract_text: extract_text.o
	$(CC) $(CPPFLAGS) -o utils/extract_text extract_text.o

convert_patterns: convert_patterns.o
	$(CC) $(CPPFLAGS) -o utils/convert_patterns convert_patterns.o misc/utils.o

split_text: split_text.o
	$(CC) $(CPPFLAGS) -o utils/split_text split_text.o misc/utils.o

genpatterns: genpatterns.c
	gcc -O3 -Wall -o utils/genpatterns utils/genpatterns.c

clean:
	rm -f rlcsa.a rlcsa_test lcp_test parallel_build
	rm -f build_rlcsa locate_test display_test read_bwt text_generator extract_sequence rlcsa_grep build_plcp sample_lcp
	rm -f utils/extract_text utils/convert_patterns utils/split_text
	rm -f *.o bits/*.o misc/*.o qsufsort/*.o utils/*.o

package:
	mkdir rlcsa
	mkdir rlcsa/bits rlcsa/misc rlcsa/qsufsort rlcsa/utils
	cp LICENSE Makefile README dependencies.mk *.cpp *.h rlcsa
	cp bits/*.cpp bits/*.h rlcsa/bits
	cp misc/*.cpp misc/*.h rlcsa/misc
	cp qsufsort/*.c qsufsort/*.h rlcsa/qsufsort
	cp utils/*.cpp utils/*.py rlcsa/utils
	tar cfz rlcsa.tgz rlcsa
	rm -r rlcsa/*
	rmdir rlcsa

include dependencies.mk
