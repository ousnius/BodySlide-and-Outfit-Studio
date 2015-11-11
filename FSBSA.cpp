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
#include "DDS.h"
#include "zlib\zlib.h"

#include <vector>

#pragma warning (disable : 4389)


wxUint32 BSA::BSAFile::size() const {
	if (sizeFlags > 0) {
		// Skyrim and earlier
		return sizeFlags & OB_BSAFILE_SIZEMASK;
	}

	return (packedLength == 0) ? unpackedLength : packedLength;
}

bool BSA::BSAFile::compressed() const {
	return (sizeFlags & OB_BSAFILE_FLAG_COMPRESS) != 0;
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

wxMemoryBuffer gUncompress(const wxMemoryBuffer &data) {
	if (data.GetBufSize() <= 4) {
		//qWarning("gUncompress: Input data is truncated");
		return wxMemoryBuffer();
	}

	wxMemoryBuffer result;

	int ret;
	z_stream strm;
	static const int CHUNK_SIZE = 1024;
	char out[CHUNK_SIZE];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = data.GetBufSize();

	char *dataPtr = static_cast<char*>(data.GetData());
	strm.next_in = (Bytef*)(dataPtr + 4);

	ret = inflateInit2(&strm, 15 + 32); // gzip decoding
	if (ret != Z_OK)
		return wxMemoryBuffer();

	// run inflate()
	do {
		strm.avail_out = CHUNK_SIZE;
		strm.next_out = (Bytef*)(out);

		ret = inflate(&strm, Z_NO_FLUSH);
		//Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

		switch (ret) {
		case Z_NEED_DICT:
			ret = Z_DATA_ERROR;     // and fall through
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			(void)inflateEnd(&strm);
			return wxMemoryBuffer();
		}

		result.AppendData(out, CHUNK_SIZE - strm.avail_out);
	} while (strm.avail_out == 0);

	// clean up and return
	inflateEnd(&strm);
	return result;
}


BSA::BSA(const wxString &filename) : FSArchiveFile(), bsa(filename), bsaInfo(filename), status("initialized") {
	bsaPath = bsaInfo.GetPathWithSep() + bsaInfo.GetFullName();
	bsaBase = bsaInfo.GetPath();
	bsaName = bsaInfo.GetFullName();
}

BSA::~BSA() {
	close();
}

bool BSA::canOpen(const wxString &fn) {
	wxFile f(fn);
	if (f.IsOpened()) {
		wxUint32 magic, version;

		if (f.Read((char *)& magic, sizeof(magic)) != 4)
			return false;

		//qDebug() << "Magic:" << QString::number( magic, 16 );
		if (magic == F4_BSAHEADER_FILEID) {
			if (f.Read((char *)&version, sizeof(version)) != 4)
				return false;

			return version == F4_BSAHEADER_VERSION;
		}
		else if (magic == OB_BSAHEADER_FILEID) {
			if (f.Read((char *)& version, sizeof(version)) != 4)
				return false;

			return (version == OB_BSAHEADER_VERSION || version == F3_BSAHEADER_VERSION);
		}
		else
			return magic == MW_BSAHEADER_FILEID;
	}

	return false;
}

bool BSA::open() {
	wxMutexLocker lock(bsaMutex);

	try {
		bsa.Open(bsaPath);
		if (!bsa.IsOpened())
			throw wxString("file open");

		wxUint32 magic, version;

		bsa.Read((char*)&magic, sizeof(magic));

		if (magic == F4_BSAHEADER_FILEID) {
			bsa.Read((char*)&version, sizeof(version));

			if (version != F4_BSAHEADER_VERSION)
				throw wxString("file version");

			F4BSAHeader header;
			if (bsa.Read((char *)&header, sizeof(header)) != sizeof(header))
				throw wxString("header size");

			numFiles = header.numFiles;

			std::vector<wxString> filepaths;
			if (bsa.Seek(header.nameTableOffset)) {
				for (wxUint32 i = 0; i < header.numFiles; i++) {
					wxUint16 length;
					bsa.Read((char*)&length, 2);

					wxMemoryBuffer strdata(length);
					bsa.Read(strdata.GetData(), length);

					wxString filepath(strdata, strdata.GetBufSize());
					filepaths.push_back(filepath);
				}
			}

			
			wxString h(header.type, 4);
			if (h == "GNRL") {
				// General BA2 Format
				if (bsa.Seek(sizeof(header) + 8)) {
					for (wxUint32 i = 0; i < header.numFiles; i++) {
						F4GeneralInfo finfo;
						bsa.Read((char*)&finfo, 36);

						wxString fullpath = filepaths[i];
						fullpath.Replace("\\", "/");

						wxString filename = fullpath.AfterLast('/');
						wxString folderName = fullpath.BeforeLast('/');

						BSAFolder * folder = insertFolder(folderName);

						insertFile(folder, filename, finfo.packedSize, finfo.unpackedSize, finfo.offset);
					}
				}
			}
			else if (h == "DX10") {
				// Texture BA2 Format
				if (bsa.Seek(sizeof(header) + 8)) {
					for (wxUint32 i = 0; i < header.numFiles; i++) {
						F4Tex tex;
						bsa.Read((char*)&tex.header, 24);

						std::vector<F4TexChunk> texChunks;
						for (wxUint32 j = 0; j < tex.header.numChunks; j++) {
							F4TexChunk texChunk;
							bsa.Read((char*)&texChunk, 24);
							texChunks.push_back(texChunk);
						}

						tex.chunks = texChunks;

						wxString fullpath = filepaths[i];
						fullpath.Replace("\\", "/");

						wxString filename = fullpath.AfterLast('/');
						wxString folderName = fullpath.BeforeLast('/');

						BSAFolder * folder = insertFolder(folderName);

						F4TexChunk chunk = tex.chunks[0];
						insertFile(folder, filename, chunk.packedSize, chunk.unpackedSize, chunk.offset, tex);
					}
				}
			}
		}
		// From NifSkope
		else if (magic == OB_BSAHEADER_FILEID) {
			bsa.Read((char*)&version, sizeof(version));

			if (version != OB_BSAHEADER_VERSION && version != F3_BSAHEADER_VERSION)
				throw wxString("file version");

			OBBSAHeader header;

			if (bsa.Read((char *)& header, sizeof(header)) != sizeof(header))
				throw wxString("header size");

			numFiles = header.FileCount;
			//qWarning() << bsaName << header;

			if ((header.ArchiveFlags & OB_BSAARCHIVE_PATHNAMES) == 0 || (header.ArchiveFlags & OB_BSAARCHIVE_FILENAMES) == 0)
				throw wxString("header flags");

			compressToggle = (header.ArchiveFlags & OB_BSAARCHIVE_COMPRESSFILES) != 0;

			if (version == F3_BSAHEADER_VERSION)
				namePrefix = (header.ArchiveFlags & F3_BSAARCHIVE_PREFIXFULLFILENAMES) != 0;
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

			numFiles = header.FileCount;
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

void BSA::close() {
	wxMutexLocker lock(bsaMutex);

	bsa.Close();
	for (auto &it : root.children)
		delete it.second;
	for (auto &it : root.files)
		delete it.second;

	root.children.clear();
	root.files.clear();
	folders.clear();
}

wxInt64 BSA::fileSize(const wxString & fn) const {
	// note: lazy size count (not accurate for compressed files)
	if (const BSAFile * file = getFile(fn)) {
		if (file->sizeFlags > 0)
			return file->size();

		wxUint64 size = file->unpackedLength;

		if (file->tex.chunks.size()) {
			for (int i = 1; i < file->tex.chunks.size(); i++) {
				size += file->tex.chunks[i].unpackedSize;
			}
		}

		return size;
	}
	return 0;
}

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

			if (file->tex.chunks.size()) {
				// Fill DDS Header for BA2
				DDS_HEADER ddsHeader = { 0 };

				ddsHeader.dwSize = sizeof(ddsHeader);
				ddsHeader.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE | DDS_HEADER_FLAGS_MIPMAP;
				ddsHeader.dwHeight = file->tex.header.height;
				ddsHeader.dwWidth = file->tex.header.width;
				ddsHeader.dwMipMapCount = file->tex.header.numMips;
				ddsHeader.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
				ddsHeader.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;

				bool ok = true;

				switch (file->tex.header.format) {
				case DXGI_FORMAT_BC1_UNORM:
					ddsHeader.ddspf.dwFlags = DDS_FOURCC;
					ddsHeader.ddspf.dwFourCC = MAKEFOURCC('D', 'X', 'T', '1');
					ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height / 2;	// 4bpp
					break;

				case DXGI_FORMAT_BC2_UNORM:
					ddsHeader.ddspf.dwFlags = DDS_FOURCC;
					ddsHeader.ddspf.dwFourCC = MAKEFOURCC('D', 'X', 'T', '3');
					ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
					break;

				case DXGI_FORMAT_BC3_UNORM:
					ddsHeader.ddspf.dwFlags = DDS_FOURCC;
					ddsHeader.ddspf.dwFourCC = MAKEFOURCC('D', 'X', 'T', '5');
					ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
					break;

				case DXGI_FORMAT_BC5_UNORM:
					// Incorrect
					ddsHeader.ddspf.dwFlags = DDS_FOURCC;
					ddsHeader.ddspf.dwFourCC = MAKEFOURCC('A', 'T', 'I', '2');
					//ddsHeader.ddspf.dwFourCC =		MAKEFOURCC('D', 'X', 'T', '5');
					ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
					break;

				case DXGI_FORMAT_BC7_UNORM:
					// Incorrect
					ddsHeader.ddspf.dwFlags = DDS_FOURCC;
					ddsHeader.ddspf.dwFourCC = MAKEFOURCC('B', 'C', '7', '\0');
					ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
					break;

				case DXGI_FORMAT_B8G8R8A8_UNORM:
					ddsHeader.ddspf.dwFlags = DDS_RGBA;
					ddsHeader.ddspf.dwRGBBitCount = 32;
					ddsHeader.ddspf.dwRBitMask = 0x00FF0000;
					ddsHeader.ddspf.dwGBitMask = 0x0000FF00;
					ddsHeader.ddspf.dwBBitMask = 0x000000FF;
					ddsHeader.ddspf.dwABitMask = 0xFF000000;
					ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height * 4;	// 32bpp
					break;

				case DXGI_FORMAT_R8_UNORM:
					ddsHeader.ddspf.dwFlags = DDS_RGB;
					ddsHeader.ddspf.dwRGBBitCount = 8;
					ddsHeader.ddspf.dwRBitMask = 0xFF;
					ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
					break;

				default:
					ok = false;
					break;
				}

				char buf[sizeof(ddsHeader)];
				memcpy(buf, &ddsHeader, sizeof(ddsHeader));

				// Append DDS Header
				wxString dds = "DDS ";
				content.SetBufSize(sizeof(ddsHeader) + 4);
				content.AppendData(dds.data(), 4);
				content.AppendData(buf, sizeof(ddsHeader));
			}

			wxMemoryBuffer firstChunk;
			firstChunk.SetBufSize(filesz);
			if (ok != wxInvalidOffset && bsa.Read(firstChunk.GetData(), filesz) == filesz) {
				if (file->sizeFlags > 0) {
					// BSA
					if (file->compressed() ^ compressToggle) {
						firstChunk = gUncompress(firstChunk);
						content.AppendData(firstChunk, firstChunk.GetBufSize());
					}
				}
				else if (file->packedLength > 0) {
					// BA2
					firstChunk = gUncompress(firstChunk);
					content.AppendData(firstChunk, firstChunk.GetBufSize());
				}

				if (file->tex.chunks.size()) {
					// Start at 2nd chunk for BA2
					for (int i = 1; i < file->tex.chunks.size(); i++) {
						F4TexChunk chunk = file->tex.chunks[i];
						if (bsa.Seek(chunk.offset)) {
							wxMemoryBuffer chunkData;

							if (chunk.packedSize > 0) {
								chunkData.SetBufSize(chunk.packedSize);
								if (bsa.Read(chunkData.GetData(), chunk.packedSize) == chunk.packedSize)
									chunkData = gUncompress(chunkData);

								if (chunkData.GetBufSize() != chunk.unpackedSize) {
									//qCritical() << "Size does not match at " << chunk.offset;
									return false;
								}

							}
							else {
								chunkData.SetBufSize(chunk.unpackedSize);
								if (!(bsa.Read(chunkData.GetData(), chunk.unpackedSize) == chunk.unpackedSize)) {
									//qCritical() << "Size does not match at " << chunk.offset;
									return false;
								}
							}
							
							content.AppendData(chunkData.GetData(), chunkData.GetBufSize());
						}
						else {
							//qCritical() << "Seek error";
							return false;
						}
					}
				}
				return true;
			}
		}
	}
	return false;
}

wxString BSA::absoluteFilePath(const wxString &fn) const {
	if (hasFile(fn)) {
		wxFileName fileInfo(fn);
		return fileInfo.GetPath(true) + fileInfo.GetName();
	}

	return wxString();
}

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

BSA::BSAFile *BSA::insertFile(BSAFolder *folder, wxString name, wxUint32 sizeFlags, wxUint32 offset) {
	name = name.Lower();

	BSAFile *file = new BSAFile;
	file->sizeFlags = sizeFlags;
	file->offset = offset;

	folder->files[name.ToStdString()] = file;
	return file;
}

BSA::BSAFile *BSA::insertFile(BSAFolder *folder, wxString name, wxUint32 packed, wxUint32 unpacked, wxUint64 offset, F4Tex dds) {
	name = name.Lower();

	BSAFile * file = new BSAFile;
	file->tex = dds;
	file->packedLength = packed;
	file->unpackedLength = unpacked;
	file->offset = offset;
	folder->files[name.ToStdString()] = file;

	return file;
}

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

bool BSA::hasFile(const wxString &fn) const {
	return getFile(fn) != nullptr;
}

bool BSA::hasFolder(const wxString &fn) const {
	return getFolder(fn) != nullptr;
}

wxDateTime BSA::fileTime(const wxString&) const {
	wxDateTime created;
	bsaInfo.GetTimes(nullptr, nullptr, &created);
	return created;
}
