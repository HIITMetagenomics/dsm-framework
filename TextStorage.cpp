#include "TextStorage.h"

TextStorage * TextStorage::Load(std::FILE *file)
{
    char type = 0;
    if (std::fread(&type, sizeof(char), 1, file) != 1)
        throw std::runtime_error("TextStorage::Load(): file read error (type).");

    switch(type)
    {
    case (TYPE_PLAIN_TEXT):
        return new TextStoragePlainText(file);
//    case (TYPE_LZ_INDEX):
//        return new TextStorageLzIndex(file);
    default:
        std::cerr << "TextStorage::Load(): Unknown type in save file!" << std::endl;
        exit(1);
    }
}

TextStorage::TextStorage(std::FILE * file)
    : n_(0), offsets_(0), numberOfTexts_(0)
{
    if (std::fread(&(this->n_), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("TextStorage::Load(): file read error (n_).");

    if (std::fread(&(this->numberOfTexts_), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("TextStorage::Load(): file read error (numberOfTexts_).");

    offsets_ = static_bitsequence::load(file);
}

void TextStorage::Save(FILE *file, char type) const
{
    if (std::fwrite(&type, sizeof(char), 1, file) != 1)
        throw std::runtime_error("TextStorage::Save(): file write error (type).");
        
    if (std::fwrite(&(this->n_), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("TextStorage::Save(): file write error (n_).");
    
    if (std::fwrite(&(this->numberOfTexts_), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("TextStorage::Save(): file write error (n_).");

    offsets_->save(file);
}
