/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (compressedbsafile.hpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

  Compressed BSA stuff added by cc9cii 2018

 */

#ifndef BSA_COMPRESSED_BSA_FILE_H
#define BSA_COMPRESSED_BSA_FILE_H

#include <map>

#include <components/bsa/bsa_file.hpp>
#include <filesystem>

namespace Bsa
{
    class CompressedBSAFile : private BSAFile
    {
    private:
        // special marker for invalid records,
        // equal to max uint32_t value
        static const uint32_t sInvalidOffset;

        // bit marking compression on file size
        static const uint32_t sCompressedFlag;

        struct FileRecord
        {
            std::uint32_t size;
            std::uint32_t offset;

            FileRecord();
            bool isCompressed(bool bsaCompressedByDefault) const;
            bool isValid() const;
            std::uint32_t getSizeWithoutCompressionFlag() const;
        };

        // if files in BSA  without 30th bit enabled are compressed
        bool mCompressedByDefault;

        // if each file record begins with BZ string with file name
        bool mEmbeddedFileNames;

        std::uint32_t mVersion{ 0u };

        struct FolderRecord
        {
            std::uint32_t count;
            std::uint64_t offset;
            std::map<std::uint64_t, FileRecord> files;
        };
        std::map<std::uint64_t, FolderRecord> mFolders;

        FileRecord getFileRecord(const std::string& str) const;

        void getBZString(std::string& str, std::istream& filestream);
        // mFiles used by OpenMW will contain uncompressed file sizes
        void convertCompressedSizesToUncompressed();
        /// \brief Normalizes given filename or folder and generates format-compatible hash. See
        /// https://en.uesp.net/wiki/Tes4Mod:Hash_Calculation.
        static std::uint64_t generateHash(const std::filesystem::path& stem, std::string extension);
        Files::IStreamPtr getFile(const FileRecord& fileRecord);

    public:
        using BSAFile::getFilename;
        using BSAFile::getList;
        using BSAFile::open;

        CompressedBSAFile();
        virtual ~CompressedBSAFile();

        /// Read header information from the input source
        void readHeader() override;

        Files::IStreamPtr getFile(const char* filePath);
        Files::IStreamPtr getFile(const FileStruct* fileStruct);
        void addFile(const std::string& filename, std::istream& file);
    };
}

#endif
