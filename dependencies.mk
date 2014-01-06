BitRank.o: BitRank.cpp BitRank.h Tools.h
builder.o: builder.cpp TextCollectionBuilder.h TextCollection.h Tools.h
ClientSocket.o: ClientSocket.cpp ClientSocket.h Tools.h
EnumerateQuery.o: EnumerateQuery.cpp EnumerateQuery.h Query.h Pattern.h \
 Tools.h InputReader.h OutputWriter.h TextCollection.h ClientSocket.h
FMIndex.o: FMIndex.cpp FMIndex.h TextCollection.h Tools.h BlockArray.h \
 ArrayDoc.h TextStorage.h libcds/includes/static_bitsequence.h \
 libcds/includes/basics.h libcds/includes/static_bitsequence_rrr02.h \
 libcds/includes/table_offset.h \
 libcds/includes/static_bitsequence_rrr02_light.h \
 libcds/includes/static_bitsequence_naive.h \
 libcds/includes/static_bitsequence_brw32.h \
 libcds/includes/static_bitsequence_sdarray.h libcds/includes/sdarray.h \
 HuffWT.h BitRank.h ResultSet.h
HuffWT.o: HuffWT.cpp HuffWT.h BitRank.h Tools.h
InputReader.o: InputReader.cpp InputReader.h Pattern.h Tools.h
metaenumerate.o: metaenumerate.cpp Query.h Pattern.h Tools.h \
 InputReader.h OutputWriter.h TextCollection.h EnumerateQuery.h \
 ClientSocket.h
metaserver.o: metaserver.cpp TrieReader.h Tools.h ServerSocket.h
OutputWriter.o: OutputWriter.cpp OutputWriter.h Pattern.h Tools.h \
 TextCollection.h
Pattern.o: Pattern.cpp Pattern.h Tools.h
Query.o: Query.cpp Query.h Pattern.h Tools.h InputReader.h OutputWriter.h \
 TextCollection.h
ResultSet.o: ResultSet.cpp ResultSet.h
ServerSocket.o: ServerSocket.cpp ServerSocket.h Tools.h
TextCollectionBuilder.o: TextCollectionBuilder.cpp incbwt/rlcsa_builder.h \
 incbwt/rlcsa.h incbwt/bits/deltavector.h incbwt/bits/bitvector.h \
 incbwt/bits/../misc/definitions.h incbwt/bits/bitbuffer.h \
 incbwt/bits/rlevector.h incbwt/bits/nibblevector.h \
 incbwt/bits/succinctvector.h incbwt/sasamples.h incbwt/sampler.h \
 incbwt/misc/utils.h incbwt/misc/definitions.h incbwt/bits/bitbuffer.h \
 incbwt/alphabet.h incbwt/misc/definitions.h incbwt/lcpsamples.h \
 incbwt/bits/array.h incbwt/misc/parameters.h incbwt/suffixarray.h \
 TextCollectionBuilder.h TextCollection.h Tools.h FMIndex.h BlockArray.h \
 ArrayDoc.h TextStorage.h libcds/includes/static_bitsequence.h \
 libcds/includes/basics.h libcds/includes/static_bitsequence_rrr02.h \
 libcds/includes/table_offset.h \
 libcds/includes/static_bitsequence_rrr02_light.h \
 libcds/includes/static_bitsequence_naive.h \
 libcds/includes/static_bitsequence_brw32.h \
 libcds/includes/static_bitsequence_sdarray.h libcds/includes/sdarray.h \
 HuffWT.h BitRank.h ResultSet.h
TextCollection.o: TextCollection.cpp TextCollection.h Tools.h FMIndex.h \
 BlockArray.h ArrayDoc.h TextStorage.h \
 libcds/includes/static_bitsequence.h libcds/includes/basics.h \
 libcds/includes/static_bitsequence_rrr02.h \
 libcds/includes/table_offset.h \
 libcds/includes/static_bitsequence_rrr02_light.h \
 libcds/includes/static_bitsequence_naive.h \
 libcds/includes/static_bitsequence_brw32.h \
 libcds/includes/static_bitsequence_sdarray.h libcds/includes/sdarray.h \
 HuffWT.h BitRank.h ResultSet.h
TextStorage.o: TextStorage.cpp TextStorage.h TextCollection.h Tools.h \
 libcds/includes/static_bitsequence.h libcds/includes/basics.h \
 libcds/includes/static_bitsequence_rrr02.h \
 libcds/includes/table_offset.h \
 libcds/includes/static_bitsequence_rrr02_light.h \
 libcds/includes/static_bitsequence_naive.h \
 libcds/includes/static_bitsequence_brw32.h \
 libcds/includes/static_bitsequence_sdarray.h libcds/includes/sdarray.h
Tools.o: Tools.cpp Tools.h
