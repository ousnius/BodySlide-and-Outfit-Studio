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

#include "FSBSA.h"
#include <vector>
#include <wx/zstream.h>
#include <wx/mstream.h>


/* Default header data */
#define MW_BSAHEADER_FILEID  0x00000100 //!< Magic for Morrowind BSA
#define OB_BSAHEADER_FILEID  0x00415342 //!< Magic for Oblivion BSA, the literal string "BSA\0".
#define OB_BSAHEADER_VERSION 0x67 //!< Version number of an Oblivion BSA
#define F3_BSAHEADER_VERSION 0x68 //!< Version number of a Fallout 3 BSA

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

//! \file bsa.cpp OBBSAHeader / \link OBBSAFileInfo FileInfo\endlink / \link OBBSAFolderInfo FolderInfo\endlink; MWBSAHeader, MWBSAFileSizeOffset

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

	//friend QDebug operator<<(QDebug dbg, const OBBSAHeader & head)
	//{
	//	return dbg << "BSAHeader:"
	//		<< "\n  folder offset" << head.FolderRecordOffset
	//		<< "\n  archive flags" << head.ArchiveFlags
	//		<< "\n  folder Count" << head.FolderCount
	//		<< "\n  file Count" << head.FileCount
	//		<< "\n  folder name length" << head.FolderNameLength
	//		<< "\n  file name length" << head.FileNameLength
	//		<< "\n  file flags" << head.FileFlags;
	//}
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

// see bsa.h
wxUint32 BSA::BSAFile::size() const {
	return sizeFlags & OB_BSAFILE_SIZEMASK;
}

// see bsa.h
bool BSA::BSAFile::compressed() const {
	return sizeFlags & OB_BSAFILE_FLAG_COMPRESS;
}

//! Reads a foldername sized string (length + null-terminated string) from the BSA
static bool BSAReadSizedString(wxFile &bsa, wxString &s) {
	//qDebug() << "BSA is at" << bsa.pos();
	wxUint8 len;
	if (bsa.Read((char *)&len, 1) != 1) {
		//qDebug() << "bailout on" << __FILE__ << "line" << __LINE__;
		return false;
	}
	//qDebug() << "folder string length is" << len;

	wxMemoryBuffer b(len);
	if (bsa.Read(b.GetData(), len) == len) {
		s = b;
		//qDebug() << "bailout on" << __FILE__ << "line" << __LINE__;
		return true;
	}
	else {
		//qDebug() << "bailout on" << __FILE__ << "line" << __LINE__;
		return false;
	}
}

// see bsa.h
BSA::BSA(const wxString &filename) : FSArchiveFile(), bsa(filename), bsaInfo(filename), status("initialized") {
	// see bsa.h
	bsaPath = bsaInfo.GetPathWithSep() + bsaInfo.GetFullName();
	bsaBase = bsaInfo.GetPath();
	bsaName = bsaInfo.GetFullName();
}

// see bsa.h
BSA::~BSA() {
	close();
}

// see bsa.h
bool BSA::canOpen(const wxString &fn) {
	wxFile f(fn);
	if (f.IsOpened()) {
		wxUint32 magic, version;

		if (f.Read((char *)& magic, sizeof(magic)) != 4)
			return false;

		//qDebug() << "Magic:" << QString::number( magic, 16 );
		if (magic == OB_BSAHEADER_FILEID) {
			if (f.Read((char *)& version, sizeof(version)) != 4)
				return false;

			return (version == OB_BSAHEADER_VERSION || version == F3_BSAHEADER_VERSION);
		}
		else
			return magic == MW_BSAHEADER_FILEID;
	}

	return false;
}

// see bsa.h
bool BSA::open() {
	wxMutexLocker lock(bsaMutex);

	try {
		bsa.Open(bsaPath);
		if (!bsa.IsOpened())
			throw wxString("file open");

		wxUint32 magic, version;

		bsa.Read((char*)&magic, sizeof(magic));

		if (magic == OB_BSAHEADER_FILEID)
		{
			bsa.Read((char*)&version, sizeof(version));

			if (version != OB_BSAHEADER_VERSION && version != F3_BSAHEADER_VERSION)
				throw wxString("file version");

			OBBSAHeader header;

			if (bsa.Read((char *)& header, sizeof(header)) != sizeof(header))
				throw wxString("header size");

			//qWarning() << bsaName << header;

			if ((header.ArchiveFlags & OB_BSAARCHIVE_PATHNAMES) == 0 || (header.ArchiveFlags & OB_BSAARCHIVE_FILENAMES) == 0)
				throw wxString("header flags");

			compressToggle = header.ArchiveFlags & OB_BSAARCHIVE_COMPRESSFILES;

			if (version == F3_BSAHEADER_VERSION)
				namePrefix = header.ArchiveFlags & F3_BSAARCHIVE_PREFIXFULLFILENAMES;
			else
				namePrefix = false;

			if (bsa.Seek(header.FolderRecordOffset + header.FolderNameLength + header.FolderCount * (1 + sizeof(OBBSAFolderInfo)) + header.FileCount * sizeof(OBBSAFileInfo)) == wxInvalidOffset)
				throw wxString("file name seek");

			wxMemoryBuffer fileNames(header.FileNameLength);
			if (bsa.Read(fileNames.GetData(), header.FileNameLength) != header.FileNameLength)
				throw wxString("file name read");

			wxUint32 fileNameIndex = 0;

			//qDebug() << bsa.pos() - header.FileNameLength << fileNames;

			if (bsa.Seek(header.FolderRecordOffset) == wxInvalidOffset)
				throw wxString("folder info seek");

			std::vector<OBBSAFolderInfo> folderInfos(header.FolderCount);
			if (bsa.Read((char *)folderInfos.data(), header.FolderCount * sizeof(OBBSAFolderInfo)) != header.FolderCount * sizeof(OBBSAFolderInfo))
				throw wxString("folder info read");

			wxUint32 totalFileCount = 0;

			for (const OBBSAFolderInfo folderInfo : folderInfos) {
				// useless?
				/*
				qDebug() << __LINE__ << "position" << bsa.pos() << "offset" << folderInfo.offset;
				if ( folderInfo.offset < header.FileNameLength || ! bsa.seek( folderInfo.offset - header.FileNameLength ) )
				throw QString( "folder content seek" );
				*/


				wxString folderName;
				if (!BSAReadSizedString(bsa, folderName) || folderName.IsEmpty()) {
					//qDebug() << "folderName" << folderName;
					throw wxString("folder name read");
				}

				// qDebug() << folderName;

				BSAFolder * folder = insertFolder(folderName);

				wxUint32 fcnt = folderInfo.fileCount;
				totalFileCount += fcnt;
				std::vector<OBBSAFileInfo> fileInfos(fcnt);
				if (bsa.Read((char *)fileInfos.data(), fcnt * sizeof(OBBSAFileInfo)) != fcnt * sizeof(OBBSAFileInfo))
					throw wxString("file info read");

				for (const OBBSAFileInfo fileInfo : fileInfos)
				{
					if (fileNameIndex >= header.FileNameLength)
						throw wxString("file name size");

					wxString fileName = static_cast<char*>(fileNames.GetData()) + fileNameIndex;
					fileNameIndex += fileName.length() + 1;

					insertFile(folder, fileName, fileInfo.sizeFlags, fileInfo.offset);
				}
			}

			if (totalFileCount != header.FileCount)
				throw wxString("file count");
		}
		else if (magic == MW_BSAHEADER_FILEID) {
			MWBSAHeader header;

			if (bsa.Read((char *)& header, sizeof(header)) != sizeof(header))
				throw wxString("header");


			compressToggle = false;
			namePrefix = false;

			// header is 12 bytes, hash table is 8 bytes per entry
			wxUint32 dataOffset = 12 + header.HashOffset + header.FileCount * 8;

			// file size/offset table
			std::vector<MWBSAFileSizeOffset> sizeOffset(header.FileCount);
			if (bsa.Read((char *)sizeOffset.data(), header.FileCount * sizeof(MWBSAFileSizeOffset)) != header.FileCount * sizeof(MWBSAFileSizeOffset))
				throw wxString("file size/offset");

			// filename offset table
			std::vector<wxUint32> nameOffset(header.FileCount);
			if (bsa.Read((char *)nameOffset.data(), header.FileCount * sizeof(wxUint32)) != header.FileCount * sizeof(wxUint32))
				throw wxString("file name offset");

			// filenames. size is given by ( HashOffset - ( 8 * number of file/size offsets) - ( 4 * number of filenames) )
			// i.e. ( HashOffset - ( 12 * number of files ) )
			wxMemoryBuffer fileNames;
			fileNames.SetBufSize(header.HashOffset - 12 * header.FileCount);
			if (bsa.Read((char *)fileNames.GetData(), header.HashOffset - 12 * header.FileCount) != header.HashOffset - 12 * header.FileCount)
				throw wxString("file names");

			// table of 8 bytes of hash values follow, but we don't need to know what they are
			// file data follows that, which is fetched by fileContents

			for (wxUint32 c = 0; c < header.FileCount; c++) {
				wxString fname = static_cast<char*>(fileNames.GetData()) + nameOffset[c];
				wxString dname;
				int x = fname.Last('\\');
				if (x > 0) {
					dname = fname.Left(x);
					fname = fname.Remove(0, x + 1);
				}

				// qDebug() << "inserting" << dname << fname;

				insertFile(insertFolder(dname), fname, sizeOffset[c].size, dataOffset + sizeOffset[c].offset);
			}
		}
		else
			throw wxString("file magic");
	}
	catch (wxString e) {
		status = e;
		return false;
	}

	status = "loaded successful";

	return true;
}

// see bsa.h
void BSA::close() {
	wxMutexLocker lock(bsaMutex);

	bsa.Close();
	for (auto it : root.children)
		delete it.second;
	for (auto it : root.files)
		delete it.second;
	root.children.clear();
	root.files.clear();
	folders.clear();
}

// see bsa.h
wxInt64 BSA::fileSize(const wxString & fn) const {
	// note: lazy size count (not accurate for compressed files)
	if (const BSAFile *file = getFile(fn))
		return file->size();

	return 0;
}

// see bsa.h
bool BSA::fileContents(const wxString &fn, wxMemoryBuffer &content) {
	//qDebug() << "entering fileContents for" << fn;
	if (const BSAFile *file = getFile(fn)) {
		wxMutexLocker lock(bsaMutex);
		if (bsa.Seek(file->offset)) {
			wxInt64 filesz = file->size();
			ssize_t ok = 1;
			if (namePrefix) {
				char len;
				ok = bsa.Read(&len, 1);
				filesz -= len;
				if (ok != wxInvalidOffset)
					ok = bsa.Seek(file->offset + 1 + len);
			}

			content.SetBufSize(filesz);

			if (ok != wxInvalidOffset && bsa.Read(content.GetData(), filesz) == filesz) {
				if (file->compressed() ^ compressToggle) {
					char *dataPtr = static_cast<char*>(content.GetData());
					//wxUint8 a = dataPtr[0];
					//wxUint8 b = dataPtr[1];
					//wxUint8 c = dataPtr[2];
					//wxUint8 d = dataPtr[3];
					//dataPtr[0] = d;
					//dataPtr[1] = c;
					//dataPtr[2] = b;
					//dataPtr[3] = a;

					wxMemoryInputStream memInStream(dataPtr + 4, filesz);
					wxMemoryOutputStream memOutStream;
					wxZlibInputStream* zOutput = new wxZlibInputStream(memInStream);
					zOutput->Read(memOutStream);
					delete zOutput;

					size_t streamSize = memOutStream.GetSize();
					content.SetBufSize(streamSize);
					content.SetDataLen(streamSize);
					size_t numCopied = memOutStream.CopyTo(content.GetData(), streamSize);

					if (numCopied != streamSize)
						return false;
				}
				return true;
			}
		}
	}
	return false;
}

// see bsa.h
wxString BSA::absoluteFilePath(const wxString &fn) const {
	if (hasFile(fn)) {
		wxFileName fileInfo(fn);
		return fileInfo.GetPath(true) + fileInfo.GetName();
	}

	return wxString();
}

// see bsa.h
BSA::BSAFolder *BSA::insertFolder(wxString name) {
	if (name.IsEmpty())
		return &root;

	name.Replace("\\", "/");
	name = name.Lower();

	BSAFolder *folder = folders[name.ToStdString()];
	if (!folder) {
		// qDebug() << "inserting" << name;

		folder = new BSAFolder;
		folders[name.ToStdString()] = folder;

		int p = name.Last('/');
		if (p >= 0) {
			folder->parent = insertFolder(name.Left(p));
			folder->parent->children[name.Right(name.length() - p - 1).ToStdString()] = folder;
		}
		else {
			folder->parent = &root;
			root.children[name.ToStdString()] = folder;
		}
	}

	return folder;
}

// see bsa.h
BSA::BSAFile *BSA::insertFile(BSAFolder *folder, wxString name, wxUint32 sizeFlags, wxUint32 offset) {
	name = name.Lower();

	BSAFile *file = new BSAFile;
	file->sizeFlags = sizeFlags;
	file->offset = offset;

	folder->files[name.ToStdString()] = file;
	return file;
}

// see bsa.h
const BSA::BSAFolder *BSA::getFolder(wxString fn) const {
	if (fn.IsEmpty()) {
		return &root;
	}
	else {
		auto it = folders.find(fn.Lower().ToStdString());
		if (it != folders.end()) {
			BSA::BSAFolder* folder = it->second;
			if (folder)
				return folder;
		}
	}

	return nullptr;
}

// see bsa.h
const BSA::BSAFile *BSA::getFile(wxString fn) const {
	wxString folderName;
	wxString fileName = fn.Lower();
	int p = fileName.Last('/');
	if (p >= 0) {
		folderName = fileName.Left(p);
		fileName = fileName.Right(fileName.length() - p - 1);
	}

	// TODO: Multiple matches occur and user has no say which version gets loaded
	// When it comes to the AUTO feature, should give preference to certain BSAs
	// or take the newest and or highest quality version.
	if (const BSAFolder *folder = getFolder(folderName)) {
		auto it = folder->files.find(fileName.ToStdString());
		if (it != folder->files.end()) {
			BSA::BSAFile* file = it->second;
			if (file)
				return file;
		}
	}

	return nullptr;
}

// see bsa.h
bool BSA::hasFile(const wxString &fn) const {
	return getFile(fn);
}

// see bsa.h
bool BSA::hasFolder(const wxString &fn) const {
	return getFolder(fn);
}

// see bsa.h
wxUint32 BSA::ownerId(const wxString&) const {
	// not Windows
	return -2;
}

// see bsa.h
wxString BSA::owner(const wxString&) const {
	// not Windows
	return "";
}

// see bsa.h
wxDateTime BSA::fileTime(const wxString&) const {
	wxDateTime created;
	bsaInfo.GetTimes(nullptr, nullptr, &created);
	return created;
}
