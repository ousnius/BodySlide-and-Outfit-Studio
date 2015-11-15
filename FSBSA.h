/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#pragma once

#include "FSEngine.h"

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/thread.h>
#include <unordered_map>


/* Default header data */
#define MW_BSAHEADER_FILEID  0x00000100 //!< Magic for Morrowind BSA
#define OB_BSAHEADER_FILEID  0x00415342 //!< Magic for Oblivion BSA, the literal string "BSA\0".
#define F4_BSAHEADER_FILEID  0x58445442	//!< Magic for Fallout 4 BA2, the literal string "BTDX".
#define OB_BSAHEADER_VERSION 0x67 //!< Version number of an Oblivion BSA
#define F3_BSAHEADER_VERSION 0x68 //!< Version number of a Fallout 3 BSA
#define F4_BSAHEADER_VERSION 0x01 //!< Version number of a Fallout 4 BA2

/* Archive flags */
#define OB_BSAARCHIVE_PATHNAMES           0x0001 //!< Whether the BSA has names for paths
#define OB_BSAARCHIVE_FILENAMES           0x0002 //!< Whether the BSA has names for files
#define OB_BSAARCHIVE_COMPRESSFILES       0x0004 //!< Whether the files are compressed
#define F3_BSAARCHIVE_PREFIXFULLFILENAMES 0x0100 //!< Whether the name is prefixed to the data?

/* File flags */
#define OB_BSAFILE_NIF  0x0001 //!< Set when the BSA contains NIF files
#define OB_BSAFILE_DDS  0x0002 //!< Set when the BSA contains DDS files
#define OB_BSAFILE_XML  0x0004 //!< Set when the BSA contains XML files
#define OB_BSAFILE_WAV  0x0008 //!< Set when the BSA contains WAV files
#define OB_BSAFILE_MP3  0x0010 //!< Set when the BSA contains MP3 files
#define OB_BSAFILE_TXT  0x0020 //!< Set when the BSA contains TXT files
#define OB_BSAFILE_HTML 0x0020 //!< Set when the BSA contains HTML files
#define OB_BSAFILE_BAT  0x0020 //!< Set when the BSA contains BAT files
#define OB_BSAFILE_SCC  0x0020 //!< Set when the BSA contains SCC files
#define OB_BSAFILE_SPT  0x0040 //!< Set when the BSA contains SPT files
#define OB_BSAFILE_TEX  0x0080 //!< Set when the BSA contains TEX files
#define OB_BSAFILE_FNT  0x0080 //!< Set when the BSA contains FNT files
#define OB_BSAFILE_CTL  0x0100 //!< Set when the BSA contains CTL files

/* Bitmasks for the size field in the header */
#define OB_BSAFILE_SIZEMASK 0x3fffffff //!< Bit mask with OBBSAFileInfo::sizeFlags to get the size of the file

/* Record flags */
#define OB_BSAFILE_FLAG_COMPRESS 0xC0000000 //!< Bit mask with OBBSAFileInfo::sizeFlags to get the compression status

//! The header of an Oblivion BSA.
/*!
* Follows OB_BSAHEADER_FILEID and OB_BSAHEADER_VERSION.
*/
struct OBBSAHeader {
	wxUint32 FolderRecordOffset; //!< Offset of beginning of folder records
	wxUint32 ArchiveFlags; //!< Archive flags
	wxUint32 FolderCount; //!< Total number of folder records (OBBSAFolderInfo)
	wxUint32 FileCount; //!< Total number of file records (OBBSAFileInfo)
	wxUint32 FolderNameLength; //!< Total length of folder names
	wxUint32 FileNameLength; //!< Total length of file names
	wxUint32 FileFlags; //!< File flags
};

//! Info for a file inside an Oblivion BSA
struct OBBSAFileInfo {
	wxUint64 hash; //!< Hash of the filename
	wxUint32 sizeFlags; //!< Size of the data, possibly with OB_BSAFILE_FLAG_COMPRESS set
	wxUint32 offset; //!< Offset to raw file data
};

//! Info for a folder inside an Oblivion BSA
struct OBBSAFolderInfo {
	wxUint64 hash; //!< Hash of the folder name
	wxUint32 fileCount; //!< Number of files in folder
	wxUint32 offset; //!< Offset to name of this folder
};

//! The header of a Morrowind BSA
struct MWBSAHeader {
	wxUint32 HashOffset; //!< Offset of hash table minus header size (12)
	wxUint32 FileCount; //!< Number of files in the archive
};

//! The file size and offset of an entry in a Morrowind BSA
struct MWBSAFileSizeOffset {
	wxUint32 size; //!< The size of the file
	wxUint32 offset; //!< The offset of the file
};

#pragma pack(push, 4)
struct F4BSAHeader {
	char type[4]; //!< 08 GNRL=General, DX10=Textures
	wxUint32 numFiles; //!< 0C
	wxUint64 nameTableOffset; //!< 10 - relative to start of file
};

// 24
struct F4GeneralInfo {
	wxUint32 unk00; //!< 00 - hash?
	char ext[4]; //!< 04 - extension
	wxUint32 unk08; //!< 08 - hash?
	wxUint32 unk0C; //!< 0C - flags? 00100100
	wxUint64 offset; //!< 10 - relative to start of file
	wxUint32 packedSize; //!< 18 - packed length (zlib)
	wxUint32 unpackedSize; //!< 1C - unpacked length
	wxUint32 unk20; //!< 20 - BAADF00D
};
#pragma pack(pop)

// 18
struct F4TexInfo {
	wxUint32 nameHash; //!< 00
	char ext[4]; //!< 04
	wxUint32 dirHash; //!< 08
	wxUint8 unk0C; //!< 0C
	wxUint8 numChunks; //!< 0D
	wxUint16 chunkHeaderSize; //!< 0E - size of one chunk header
	wxUint16 height; //!< 10
	wxUint16 width; //!< 12
	wxUint8 numMips; //!<  14
	wxUint8 format; //!< 15 - DXGI_FORMAT
	wxUint16 unk16; //!< 16 - 0800
};

// 18
struct F4TexChunk {
	wxUint64 offset; //!< 00
	wxUint32 packedSize; //!< 08
	wxUint32 unpackedSize; //!< 0C
	wxUint16 startMip; //!< 10
	wxUint16 endMip; //!< 12
	wxUint32 unk14; //!< 14 - BAADFOOD
};

struct F4Tex {
	F4TexInfo header;
	std::vector<F4TexChunk> chunks;
};


class BSA final : public FSArchiveFile {
public:
	//! Constructor; creates a %BSA from the given file path.
	BSA(const std::string &filePath);
	//! Destructor; closes the file.
	~BSA();

	//! Opens the %BSA file
	bool open() override final;
	//! Closes the %BSA file
	void close() override final;

	//! Returns BSA::bsaPath.
	std::string path() const override final { return bsaPath; }
	//! Returns BSA::bsaBase.
	std::string base() const override final { return bsaBase; }
	//! Returns BSA::bsaName.
	std::string name() const override final { return bsaName; }

	//! Whether the specified folder exists or not
	bool hasFolder(const std::string&) const override final;

	//! Whether the specified file exists or not
	bool hasFile(const std::string&) const override final;
	//! Returns the size of the file per BSAFile::size().
	wxInt64 fileSize(const std::string&) const override final;
	//! Add all files of the folder to the map
	void addFilesOfFolders(const std::string&, std::vector<std::string>&) const override final;
	//! Returns the entire file tree of the BSA
	void fileTree(std::vector<std::string>&) const override final;

	//! Returns the contents of the specified file
	/*!
	* \param fn The filename to get the contents for
	* \param content Reference to the byte array that holds the file contents
	* \return True if successful
	*/
	bool fileContents(const std::string&, wxMemoryBuffer&) override final;

	//! Writes the contents to the specified file
	bool exportFile(const std::string&, const std::string&) override final;

	//! See QFileInfo::created().
	wxDateTime fileTime(const std::string&) const override final;
	//! See QFileInfo::absoluteFilePath().
	std::string absoluteFilePath(const std::string&) const override final;

	//! Whether the given file can be opened as a %BSA or not
	static bool canOpen(const std::string&);

	//! Returns BSA::status.
	std::string statusText() const { return status; }

	//! Returns BSA::numFiles.
	wxUint64 fileCount() const { return numFiles; }

protected:
	//! A file inside a BSA
	struct BSAFile
	{
		// Skyrim and earlier
		wxUint32 sizeFlags = 0; //!< The size of the file in the BSA

		// Fallout 4
		wxUint32 packedLength = 0;
		wxUint32 unpackedLength = 0;

		wxUint64 offset; //!< The offset of the file in the BSA

		//! The size of the file inside the BSA
		wxUint32 size() const;

		//! Whether the file is compressed inside the BSA
		bool compressed() const;

		F4Tex tex;
	};

	//! A folder inside a BSA
	struct BSAFolder
	{
		//! Constructor
		BSAFolder() : parent(0) {}
		//! Destructor
		~BSAFolder() {
			for (auto &it : children)
				delete it.second;
			for (auto &it : files)
				delete it.second;

			children.clear();
			files.clear();
		}

		BSAFolder *parent; //!< The parent item
		std::unordered_map<std::string, BSAFolder*> children; //!< A map of child folders
		std::unordered_map<std::string, BSAFile*> files; //!< A map of files inside the folder
	};

	//! Recursive function to generate the tree structure of folders inside a %BSA
	BSAFolder *insertFolder(std::string name);
	//! Inserts a file into the structure of a %BSA
	BSAFile *insertFile(BSAFolder *folder, std::string name, wxUint32 sizeFlags, wxUint32 offset);
	BSAFile *insertFile(BSAFolder *folder, std::string name, wxUint32 packed, wxUint32 unpacked, wxUint64 offset, F4Tex dds = F4Tex());

	//! Gets the specified folder, or the root folder if not found
	const BSAFolder *getFolder(std::string fn) const;
	//! Gets the specified file, or null if not found
	const BSAFile *getFile(std::string fn) const;

	//! The %BSA file
	wxFile bsa;
	//! File info for the %BSA
	wxFileName bsaInfo;

	//! Mutual exclusion handler
	wxMutex bsaMutex;

	//! The absolute name of the file, e.g. "d:/temp/test.bsa"
	std::string bsaPath;
	//! The base path of the file, e.g. "d:/temp"
	std::string bsaBase;
	//! The name of the file, e.g. "test.bsa"
	std::string bsaName;

	//! Map of folders inside a %BSA
	std::unordered_map<std::string, BSAFolder*> folders;
	//! The root folder
	BSAFolder root;

	//! Error string for exception handling
	std::string status;

	//! Number of files
	wxUint64 numFiles;

	//! Whether the %BSA is compressed
	bool compressToggle;
	//! Whether Fallout 3 names are prefixed with an extra string
	bool namePrefix;
};
