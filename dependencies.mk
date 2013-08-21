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
 libcds/includes/alphabet_mapper.h libcds/includes/alphabet_mapper_none.h \
 libcds/includes/alphabet_mapper_cont.h \
 libcds/includes/static_bitsequence_builder.h \
 libcds/includes/static_bitsequence_builder_rrr02.h \
 libcds/includes/static_bitsequence_builder_rrr02_light.h \
 libcds/includes/static_bitsequence_builder_brw32.h \
 libcds/includes/static_bitsequence_builder_sdarray.h \
 libcds/includes/static_sequence.h \
 libcds/includes/static_sequence_wvtree.h \
 libcds/includes/wt_node_internal.h libcds/includes/wt_node.h \
 libcds/includes/wt_coder.h libcds/includes/wt_coder_huff.h \
 libcds/includes/huffman_codes.h libcds/includes/huff.h \
 libcds/includes/wt_coder_binary.h libcds/includes/wt_node_leaf.h \
 libcds/includes/static_sequence_gmr_chunk.h \
 libcds/includes/static_permutation.h \
 libcds/includes/static_permutation_mrrr.h libcds/includes/perm.h \
 libcds/includes/static_permutation_builder.h \
 libcds/includes/static_permutation_builder_mrrr.h \
 libcds/includes/static_sequence_wvtree_noptrs.h \
 libcds/includes/static_sequence_gmr.h \
 libcds/includes/static_sequence_builder.h \
 libcds/includes/static_sequence_builder_wvtree.h \
 libcds/includes/static_sequence_builder_wvtree_noptrs.h \
 libcds/includes/static_sequence_builder_gmr.h \
 libcds/includes/static_sequence_builder_gmr_chunk.h \
 libcds/includes/static_sequence_bs.h
InputReader.o: InputReader.cpp InputReader.h Pattern.h Tools.h
OutputWriter.o: OutputWriter.cpp OutputWriter.h Pattern.h Tools.h \
 TextCollection.h
Pattern.o: Pattern.cpp Pattern.h Tools.h
Query.o: Query.cpp Query.h Pattern.h Tools.h InputReader.h OutputWriter.h \
 TextCollection.h
ResultSet.o: ResultSet.cpp ResultSet.h
ServerSocket.o: ServerSocket.cpp ServerSocket.h Tools.h
TextCollection.o: TextCollection.cpp TextCollection.h Tools.h FMIndex.h \
 BlockArray.h ArrayDoc.h TextStorage.h \
 libcds/includes/static_bitsequence.h libcds/includes/basics.h \
 libcds/includes/static_bitsequence_rrr02.h \
 libcds/includes/table_offset.h \
 libcds/includes/static_bitsequence_rrr02_light.h \
 libcds/includes/static_bitsequence_naive.h \
 libcds/includes/static_bitsequence_brw32.h \
 libcds/includes/static_bitsequence_sdarray.h libcds/includes/sdarray.h \
 libcds/includes/alphabet_mapper.h libcds/includes/alphabet_mapper_none.h \
 libcds/includes/alphabet_mapper_cont.h \
 libcds/includes/static_bitsequence_builder.h \
 libcds/includes/static_bitsequence_builder_rrr02.h \
 libcds/includes/static_bitsequence_builder_rrr02_light.h \
 libcds/includes/static_bitsequence_builder_brw32.h \
 libcds/includes/static_bitsequence_builder_sdarray.h \
 libcds/includes/static_sequence.h \
 libcds/includes/static_sequence_wvtree.h \
 libcds/includes/wt_node_internal.h libcds/includes/wt_node.h \
 libcds/includes/wt_coder.h libcds/includes/wt_coder_huff.h \
 libcds/includes/huffman_codes.h libcds/includes/huff.h \
 libcds/includes/wt_coder_binary.h libcds/includes/wt_node_leaf.h \
 libcds/includes/static_sequence_gmr_chunk.h \
 libcds/includes/static_permutation.h \
 libcds/includes/static_permutation_mrrr.h libcds/includes/perm.h \
 libcds/includes/static_permutation_builder.h \
 libcds/includes/static_permutation_builder_mrrr.h \
 libcds/includes/static_sequence_wvtree_noptrs.h \
 libcds/includes/static_sequence_gmr.h \
 libcds/includes/static_sequence_builder.h \
 libcds/includes/static_sequence_builder_wvtree.h \
 libcds/includes/static_sequence_builder_wvtree_noptrs.h \
 libcds/includes/static_sequence_builder_gmr.h \
 libcds/includes/static_sequence_builder_gmr_chunk.h \
 libcds/includes/static_sequence_bs.h rlcsa_wrapper.h incbwt/rlcsa.h \
 incbwt/bits/vectors.h incbwt/bits/deltavector.h incbwt/bits/bitvector.h \
 incbwt/bits/../misc/definitions.h incbwt/bits/bitbuffer.h \
 incbwt/bits/rlevector.h incbwt/sasamples.h incbwt/misc/definitions.h \
 incbwt/bits/bitbuffer.h incbwt/bits/deltavector.h incbwt/lcpsamples.h \
 incbwt/bits/array.h incbwt/misc/utils.h incbwt/misc/definitions.h \
 incbwt/misc/parameters.h
TextCollectionBuilder.o: TextCollectionBuilder.cpp incbwt/rlcsa_builder.h \
 incbwt/rlcsa.h incbwt/bits/vectors.h incbwt/bits/deltavector.h \
 incbwt/bits/bitvector.h incbwt/bits/../misc/definitions.h \
 incbwt/bits/bitbuffer.h incbwt/bits/rlevector.h incbwt/sasamples.h \
 incbwt/misc/definitions.h incbwt/bits/bitbuffer.h \
 incbwt/bits/deltavector.h incbwt/lcpsamples.h incbwt/bits/array.h \
 incbwt/misc/utils.h incbwt/misc/definitions.h incbwt/misc/parameters.h \
 TextCollectionBuilder.h TextCollection.h Tools.h FMIndex.h BlockArray.h \
 ArrayDoc.h TextStorage.h libcds/includes/static_bitsequence.h \
 libcds/includes/basics.h libcds/includes/static_bitsequence_rrr02.h \
 libcds/includes/table_offset.h \
 libcds/includes/static_bitsequence_rrr02_light.h \
 libcds/includes/static_bitsequence_naive.h \
 libcds/includes/static_bitsequence_brw32.h \
 libcds/includes/static_bitsequence_sdarray.h libcds/includes/sdarray.h \
 libcds/includes/alphabet_mapper.h libcds/includes/alphabet_mapper_none.h \
 libcds/includes/alphabet_mapper_cont.h \
 libcds/includes/static_bitsequence_builder.h \
 libcds/includes/static_bitsequence_builder_rrr02.h \
 libcds/includes/static_bitsequence_builder_rrr02_light.h \
 libcds/includes/static_bitsequence_builder_brw32.h \
 libcds/includes/static_bitsequence_builder_sdarray.h \
 libcds/includes/static_sequence.h \
 libcds/includes/static_sequence_wvtree.h \
 libcds/includes/wt_node_internal.h libcds/includes/wt_node.h \
 libcds/includes/wt_coder.h libcds/includes/wt_coder_huff.h \
 libcds/includes/huffman_codes.h libcds/includes/huff.h \
 libcds/includes/wt_coder_binary.h libcds/includes/wt_node_leaf.h \
 libcds/includes/static_sequence_gmr_chunk.h \
 libcds/includes/static_permutation.h \
 libcds/includes/static_permutation_mrrr.h libcds/includes/perm.h \
 libcds/includes/static_permutation_builder.h \
 libcds/includes/static_permutation_builder_mrrr.h \
 libcds/includes/static_sequence_wvtree_noptrs.h \
 libcds/includes/static_sequence_gmr.h \
 libcds/includes/static_sequence_builder.h \
 libcds/includes/static_sequence_builder_wvtree.h \
 libcds/includes/static_sequence_builder_wvtree_noptrs.h \
 libcds/includes/static_sequence_builder_gmr.h \
 libcds/includes/static_sequence_builder_gmr_chunk.h \
 libcds/includes/static_sequence_bs.h rlcsa_wrapper.h incbwt/rlcsa.h
TextStorage.o: TextStorage.cpp TextStorage.h TextCollection.h Tools.h \
 libcds/includes/static_bitsequence.h libcds/includes/basics.h \
 libcds/includes/static_bitsequence_rrr02.h \
 libcds/includes/table_offset.h \
 libcds/includes/static_bitsequence_rrr02_light.h \
 libcds/includes/static_bitsequence_naive.h \
 libcds/includes/static_bitsequence_brw32.h \
 libcds/includes/static_bitsequence_sdarray.h libcds/includes/sdarray.h
Tools.o: Tools.cpp Tools.h
builder.o: builder.cpp TextCollectionBuilder.h TextCollection.h Tools.h
metaenumerate.o: metaenumerate.cpp Query.h Pattern.h Tools.h \
 InputReader.h OutputWriter.h TextCollection.h EnumerateQuery.h \
 ClientSocket.h
metaserver.o: metaserver.cpp TrieReader.h Tools.h ServerSocket.h
rlcsa_wrapper.o: rlcsa_wrapper.cpp rlcsa_wrapper.h incbwt/rlcsa.h \
 incbwt/bits/vectors.h incbwt/bits/deltavector.h incbwt/bits/bitvector.h \
 incbwt/bits/../misc/definitions.h incbwt/bits/bitbuffer.h \
 incbwt/bits/rlevector.h incbwt/sasamples.h incbwt/misc/definitions.h \
 incbwt/bits/bitbuffer.h incbwt/bits/deltavector.h incbwt/lcpsamples.h \
 incbwt/bits/array.h incbwt/misc/utils.h incbwt/misc/definitions.h \
 incbwt/misc/parameters.h TextCollection.h Tools.h
