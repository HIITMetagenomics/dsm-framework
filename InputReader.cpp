#include "InputReader.h"

InputReader* InputReader::build(input_format_t input, unsigned n, 
                          std::string file, std::string qual)
{
    switch (input) 
    {
    case input_lines:
        return new SimpleLineInputReader(n, file, qual);
        break;
    case input_fasta:
        return new FastaInputReader(n, file, qual);
        break;
    case input_fastq:
        if (qual != "")
            std::cerr << "Warning: reading FASTQ format, quality file " 
                      << qual << " will be ignored!" << std::endl; 
        return new FastqInputReader(n, file);
        break;
    default:
        std::abort();
    }
}
