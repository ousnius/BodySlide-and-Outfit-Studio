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

#include <wx/mstream.h>
#include <wx/zstream.h>
#include <vector>
#include <algorithm>

#include <../LZ4F/lz4frame_static.h>

#include <dxgiformat.h>
#include "../DDS.h"


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
static bool BSAReadSizedString(wxFile &bsa, std::string &s) {
	wxUint8 len;
	if (bsa.Read((char *)&len, 1) != 1)
		return false;

	if (len <= 0) {
		s.clear();
		return true;
	}

	wxMemoryBuffer b(len);
	if (bsa.Read(b.GetData(), len) == len) {
		s = b;
		return true;
	}

	return false;
}

wxMemoryBuffer gUncompress(const wxMemoryBuffer &data, wxUint32 unpackedSize = 0, int skip = 0) {
	if (data.GetDataLen() <= 4) {
		// Input data is truncated
		return wxMemoryBuffer();
	}

	wxMemoryInputStream input((char*)data.GetData() + skip, data.GetDataLen() - skip);
	wxZlibInputStream zlibStream(input);

	wxMemoryBuffer result;
	if (!zlibStream.CanRead())
		return result;

	if (unpackedSize > 0) {
		// Allocate if unpacked size is known
		result.SetBufSize(unpackedSize);
		result.SetDataLen(unpackedSize);
		zlibStream.Read(result.GetData(), unpackedSize);
	}
	else {
		wxMemoryOutputStream output;
		zlibStream.Read(output);

		result.SetBufSize(output.GetLength());
		result.SetDataLen(output.GetLength());
		output.CopyTo(result.GetData(), result.GetDataLen());
	}

	return result;
}

wxMemoryBuffer lz4fUncompress(const wxMemoryBuffer &data) {
	if (data.GetDataLen() <= 4) {
		// Input data is truncated
		return wxMemoryBuffer();
	}

	size_t srcSize = data.GetDataLen() - 4;
	size_t dstSize = ((unsigned int*)data.GetData())[0];

	wxMemoryBuffer result(dstSize);

	LZ4F_decompressionContext_t dCtx = nullptr;
	LZ4F_createDecompressionContext(&dCtx, LZ4F_VERSION);

	LZ4F_decompressOptions_t options = { 0 };

	LZ4F_decompress(dCtx, result.GetData(), &dstSize, (char*)data.GetData() + 4, &srcSize, &options);
	LZ4F_freeDecompressionContext(dCtx);

	result.SetDataLen(dstSize);
	return result;
}


BSA::BSA(const std::string &filename) : FSArchiveFile(), bsa(filename), bsaInfo(filename), status("initialized") {
	bsaPath = bsaInfo.GetPathWithSep() + bsaInfo.GetFullName();
	bsaBase = bsaInfo.GetPath();
	bsaName = bsaInfo.GetFullName();
	headerVersion = 0;
}

BSA::~BSA() {
	close();
}

bool BSA::canOpen(const std::string &fn) {
	wxFile f(fn);
	if (f.IsOpened()) {
		wxUint32 magic, version;

		if (f.Read((char *)& magic, sizeof(magic)) != 4)
			return false;

		if (magic == F4_BSAHEADER_FILEID) {
			if (f.Read((char *)&version, sizeof(version)) != 4)
				return false;

			return version == F4_BSAHEADER_VERSION;
		}
		else if (magic == OB_BSAHEADER_FILEID) {
			if (f.Read((char *)& version, sizeof(version)) != 4)
				return false;

			return (version == OB_BSAHEADER_VERSION || version == F3_BSAHEADER_VERSION || version == SSE_BSAHEADER_VERSION);
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
			throw std::string("file open");

		wxUint32 magic, version;

		bsa.Read((char*)&magic, sizeof(magic));

		if (magic == F4_BSAHEADER_FILEID) {
			bsa.Read((char*)&version, sizeof(version));

			if (version != F4_BSAHEADER_VERSION)
				throw std::string("file version");

			headerVersion = version;

			F4BSAHeader header;
			if (bsa.Read((char *)&header, sizeof(header)) != sizeof(header))
				throw std::string("header size");

			numFiles = header.numFiles;
			namePrefix = false;

			char* superbuffer = new char[numFiles * (MAX_PATH + 2) + 1];
			std::vector<wxUint32> path_sizes(numFiles * 2);

			if (bsa.Seek(header.nameTableOffset)) {
				bsa.Read(superbuffer, numFiles * (MAX_PATH + 2));
				wxUint32 cursor = 0;
				wxUint32 n = 0;
				for (wxUint32 i = 0; i < header.numFiles; i++) {
					wxUint16 len;
					len = *(wxUint16*)&superbuffer[cursor];
					cursor += 2;
					path_sizes[n++] = cursor;
					cursor += len;
					path_sizes[n++] = cursor;
				}
			}

			std::replace(superbuffer, superbuffer + numFiles * (MAX_PATH + 2), '\\', '/');

			std::string h(header.type, 4);
			if (h == "GNRL") {
				// General BA2 Format
				if (bsa.Seek(sizeof(header) + 8)) {
					root.files.reserve(header.numFiles);

					F4GeneralInfo* finfo = new F4GeneralInfo[header.numFiles];
					bsa.Read(finfo, 36 * numFiles);

					wxUint32 n = 0;
					for (wxUint32 i = 0; i < header.numFiles; i++) {
						insertFile(superbuffer + path_sizes[n], path_sizes[n + 1] - path_sizes[n], finfo[i].packedSize, finfo[i].unpackedSize, finfo[i].offset);
						n += 2;
					}
					delete[] finfo;
				}
			}
			else if (h == "DX10") {
				// Texture BA2 Format
				if (bsa.Seek(sizeof(header) + 8)) {
					root.files.reserve(header.numFiles);

					wxUint32 n = 0;
					for (wxUint32 i = 0; i < header.numFiles; i++) {
						F4Tex tex;
						bsa.Read((char*)&tex.header, 24);

						std::vector<F4TexChunk> texChunks(tex.header.numChunks);
						bsa.Read(texChunks.data(), tex.header.numChunks * 24);
						tex.chunks = std::move(texChunks);

						const F4TexChunk& chunk = tex.chunks[0];
						insertFile(superbuffer + path_sizes[n], path_sizes[n + 1] - path_sizes[n], chunk.packedSize, chunk.unpackedSize, chunk.offset, &tex);
						n += 2;
					}
				}
			}
			delete[] superbuffer;
		}
		// From NifSkope
		else if (magic == OB_BSAHEADER_FILEID) {
			bsa.Read((char*)&version, sizeof(version));

			if (version != OB_BSAHEADER_VERSION && version != F3_BSAHEADER_VERSION && version != SSE_BSAHEADER_VERSION)
				throw std::string("file version");

			headerVersion = version;

			OBBSAHeader header;

			if (bsa.Read((char *)& header, sizeof(header)) != sizeof(header))
				throw std::string("header size");

			numFiles = header.FileCount;

			if ((header.ArchiveFlags & OB_BSAARCHIVE_PATHNAMES) == 0 || (header.ArchiveFlags & OB_BSAARCHIVE_FILENAMES) == 0)
				throw std::string("header flags");

			compressToggle = (header.ArchiveFlags & OB_BSAARCHIVE_COMPRESSFILES) != 0;

			if (version == F3_BSAHEADER_VERSION || version == SSE_BSAHEADER_VERSION)
				namePrefix = (header.ArchiveFlags & F3_BSAARCHIVE_PREFIXFULLFILENAMES) != 0;
			else
				namePrefix = false;

			wxUint32 folderSize = 0;
			if (version != SSE_BSAHEADER_VERSION)
				folderSize = sizeof(OBBSAFolderInfo);
			else
				folderSize = sizeof(SSEBSAFolderInfo);

			if (bsa.Seek(header.FolderRecordOffset + header.FolderNameLength + header.FolderCount * (1 + folderSize) + header.FileCount * sizeof(OBBSAFileInfo)) == wxInvalidOffset)
				throw std::string("file name seek");

			wxMemoryBuffer fileNames(header.FileNameLength);
			if (bsa.Read(fileNames.GetData(), header.FileNameLength) != (ssize_t)header.FileNameLength)
				throw std::string("file name read");

			wxUint32 fileNameIndex = 0;

			if (bsa.Seek(header.FolderRecordOffset) == wxInvalidOffset)
				throw std::string("folder info seek");

			BSAFolderInfo initInfo{ 0 };
			std::vector<BSAFolderInfo> folderInfos(header.FolderCount, initInfo);
			if (version != SSE_BSAHEADER_VERSION) {
				bool ok = true;
				for (wxUint32 i = 0; i < header.FolderCount; i++) {
					ok &= bsa.Read((char *)&folderInfos[i], 8) == 8;		// Hash
					ok &= bsa.Read((char *)&folderInfos[i] + 8, 4) == 4;	// File size
					ok &= bsa.Read((char *)&folderInfos[i] + 16, 4) == 4;	// Offset: this is reading a uint32 into a uint64 whose memory must be zeroed

					if (!ok)
						throw std::string("folder info read");
				}
			}
			else {
				if (bsa.Read((char *)folderInfos.data(), header.FolderCount * folderSize) != (ssize_t)(header.FolderCount * folderSize))
					throw std::string("folder info read");
			}

			wxUint32 totalFileCount = 0;

			for (const BSAFolderInfo folderInfo : folderInfos) {
				std::string folderName;
				if (!BSAReadSizedString(bsa, folderName))
					throw std::string("folder name read");

				BSAFolder *folder = insertFolder(folderName);

				wxUint32 fcnt = folderInfo.fileCount;
				totalFileCount += fcnt;
				std::vector<OBBSAFileInfo> fileInfos(fcnt);
				if (bsa.Read((char *)fileInfos.data(), fcnt * sizeof(OBBSAFileInfo)) != (ssize_t)(fcnt * sizeof(OBBSAFileInfo)))
					throw std::string("file info read");

				for (const OBBSAFileInfo fileInfo : fileInfos) {
					if (fileNameIndex >= header.FileNameLength)
						throw std::string("file name size");

					std::string fileName = static_cast<char*>(fileNames.GetData()) + fileNameIndex;
					fileNameIndex += fileName.length() + 1;

					insertFile(folder, fileName, fileInfo.sizeFlags, fileInfo.offset);
				}
			}

			if (totalFileCount != header.FileCount)
				throw std::string("file count");
		}
		else if (magic == MW_BSAHEADER_FILEID) {
			MWBSAHeader header;

			if (bsa.Read((char *)& header, sizeof(header)) != sizeof(header))
				throw std::string("header");

			numFiles = header.FileCount;
			compressToggle = false;
			namePrefix = false;

			// header is 12 bytes, hash table is 8 bytes per entry
			wxUint32 dataOffset = 12 + header.HashOffset + header.FileCount * 8;

			// file size/offset table
			std::vector<MWBSAFileSizeOffset> sizeOffset(header.FileCount);
			if (bsa.Read((char *)sizeOffset.data(), header.FileCount * sizeof(MWBSAFileSizeOffset)) != (ssize_t)(header.FileCount * sizeof(MWBSAFileSizeOffset)))
				throw std::string("file size/offset");

			// filename offset table
			std::vector<wxUint32> nameOffset(header.FileCount);
			if (bsa.Read((char *)nameOffset.data(), header.FileCount * sizeof(wxUint32)) != (ssize_t)(header.FileCount * sizeof(wxUint32)))
				throw std::string("file name offset");

			// filenames. size is given by ( HashOffset - ( 8 * number of file/size offsets) - ( 4 * number of filenames) )
			// i.e. ( HashOffset - ( 12 * number of files ) )
			wxMemoryBuffer fileNames;
			fileNames.SetBufSize(header.HashOffset - 12 * header.FileCount);
			if (bsa.Read((char *)fileNames.GetData(), header.HashOffset - 12 * header.FileCount) != (ssize_t)(header.HashOffset - 12 * header.FileCount))
				throw std::string("file names");

			// table of 8 bytes of hash values follow, but we don't need to know what they are
			// file data follows that, which is fetched by fileContents

			for (wxUint32 c = 0; c < header.FileCount; c++) {
				std::string fname = static_cast<char*>(fileNames.GetData()) + nameOffset[c];
				std::string dname;
				int x = fname.find_last_of('\\');
				if (x > 0) {
					dname = fname.substr(0, x);
					fname.erase(0, x + 1);
				}

				insertFile(insertFolder(dname), fname, sizeOffset[c].size, dataOffset + sizeOffset[c].offset);
			}
		}
		else
			throw std::string("file magic");
	}
	catch (std::string e) {
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

wxInt64 BSA::fileSize(const std::string & fn) const {
	// note: lazy size count (not accurate for compressed files)
	if (const BSAFile * file = getFile(fn)) {
		if (file->sizeFlags > 0)
			return file->size();

		wxUint64 size = file->unpackedLength;

		if (file->tex.chunks.size()) {
			for (size_t i = 1; i < file->tex.chunks.size(); i++) {
				size += file->tex.chunks[i].unpackedSize;
			}
		}

		return size;
	}
	return 0;
}

void BSA::addFilesOfFolders(const std::string &folderName, std::vector<std::string> &tree) const {
	if (const BSAFolder *folder = getFolder(folderName)) {
		tree.push_back(folderName);
		for (auto &child : folder->children) {
			addFilesOfFolders(folderName + "/" + child.first, tree);
		}
		for (auto &file : folder->files) {
			tree.push_back(folderName + "/" + file.first);
		}
	}
}

void BSA::fileTree(std::vector<std::string> &tree) const {
	tree.push_back(name());
	for (auto &folder : root.children)
		addFilesOfFolders(folder.first, tree);
}

bool BSA::fileContents(const std::string& fn, wxMemoryBuffer& content) {
	if (const BSAFile* file = getFile(fn)) {
		wxMutexLocker lock(bsaMutex);
		if (bsa.Seek(file->offset)) {
			wxInt64 filesz = file->size();
			ssize_t fileok = 1;
			if (namePrefix) {
				char len;
				fileok = bsa.Read(&len, 1);
				filesz -= len;
				if (fileok != wxInvalidOffset)
					fileok = bsa.Seek(file->offset + 1 + len);
			}

			if (file->tex.chunks.size() > 0) {
				// Fill DDS Header for BA2
				DirectX::DDS_HEADER ddsHeader = {};
				ddsHeader.size = sizeof(ddsHeader);
				ddsHeader.flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE | DDS_HEADER_FLAGS_MIPMAP;
				ddsHeader.height = file->tex.header.height;
				ddsHeader.width = file->tex.header.width;
				ddsHeader.mipMapCount = file->tex.header.numMips;
				ddsHeader.caps = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
				ddsHeader.pitchOrLinearSize = file->tex.header.width * file->tex.header.height; // 8bpp

				DirectX::DDS_HEADER_DXT10 ddsHeader10 = {};
				ddsHeader10.resourceDimension = DirectX::DDS_DIMENSION_TEXTURE2D;
				ddsHeader10.arraySize = 1;

				if (file->tex.header.unk16 == 2049) {
					ddsHeader.caps2 = DDS_CUBEMAP_ALLFACES;
					ddsHeader10.miscFlag = DirectX::DDS_RESOURCE_MISC_TEXTURECUBE;
					ddsHeader10.arraySize *= 6;
				}

				bool ok = true;

				switch (file->tex.header.format) {
					case DXGI_FORMAT_BC1_TYPELESS:
					case DXGI_FORMAT_BC1_UNORM:
					case DXGI_FORMAT_BC1_UNORM_SRGB:
						ddsHeader.ddspf = DirectX::DDSPF_DXT1;
						ddsHeader.pitchOrLinearSize /= 2; // 4bpp
						break;

					case DXGI_FORMAT_BC2_TYPELESS:
					case DXGI_FORMAT_BC2_UNORM:
					case DXGI_FORMAT_BC2_UNORM_SRGB: ddsHeader.ddspf = DirectX::DDSPF_DXT3; break;

					case DXGI_FORMAT_BC3_TYPELESS:
					case DXGI_FORMAT_BC3_UNORM:
					case DXGI_FORMAT_BC3_UNORM_SRGB: ddsHeader.ddspf = DirectX::DDSPF_DXT5; break;

					case DXGI_FORMAT_BC4_TYPELESS:
					case DXGI_FORMAT_BC4_UNORM:
					case DXGI_FORMAT_BC4_SNORM:
						ddsHeader.ddspf = DirectX::DDSPF_DX10;
						ddsHeader.pitchOrLinearSize /= 2; // 4bpp
						ddsHeader10.dxgiFormat = (DXGI_FORMAT)file->tex.header.format;
						break;

					case DXGI_FORMAT_BC5_TYPELESS:
					case DXGI_FORMAT_BC5_UNORM:
					case DXGI_FORMAT_BC5_SNORM:
						ddsHeader.ddspf = DirectX::DDSPF_DX10;
						ddsHeader10.dxgiFormat = (DXGI_FORMAT)file->tex.header.format;
						break;

					case DXGI_FORMAT_BC6H_TYPELESS:
					case DXGI_FORMAT_BC6H_UF16:
					case DXGI_FORMAT_BC6H_SF16:
						ddsHeader.ddspf = DirectX::DDSPF_DX10;
						ddsHeader10.dxgiFormat = (DXGI_FORMAT)file->tex.header.format;
						break;

					case DXGI_FORMAT_BC7_TYPELESS:
					case DXGI_FORMAT_BC7_UNORM:
					case DXGI_FORMAT_BC7_UNORM_SRGB:
						ddsHeader.ddspf = DirectX::DDSPF_DX10;
						ddsHeader10.dxgiFormat = (DXGI_FORMAT)file->tex.header.format;
						break;

					case DXGI_FORMAT_B8G8R8A8_TYPELESS:
					case DXGI_FORMAT_B8G8R8A8_UNORM:
					case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
						ddsHeader.ddspf = DirectX::DDSPF_A8R8G8B8;
						ddsHeader.pitchOrLinearSize *= 4; // 32bpp
						break;

					case DXGI_FORMAT_R8G8B8A8_TYPELESS:
					case DXGI_FORMAT_R8G8B8A8_UNORM:
					case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
						ddsHeader.ddspf = DirectX::DDSPF_A8R8G8B8;
						ddsHeader.pitchOrLinearSize *= 4; // 32bpp
						break;

					case DXGI_FORMAT_R8_TYPELESS:
					case DXGI_FORMAT_R8_UNORM:
					case DXGI_FORMAT_R8_SNORM: ddsHeader.ddspf = DirectX::DDSPF_L8_NVTT1; break;

					default: ok = false; break;
				}

				if (!ok)
					return false;

				// Append DDS Header
				content.AppendData(&DirectX::DDS_MAGIC, 4);
				content.AppendData(&ddsHeader, sizeof(ddsHeader));
				if (ddsHeader10.dxgiFormat != DXGI_FORMAT_UNKNOWN)
					content.AppendData(&ddsHeader10, sizeof(ddsHeader10));
			}

			wxMemoryBuffer chunk(filesz);
			chunk.SetDataLen(filesz);
			if (fileok != wxInvalidOffset && bsa.Read(chunk.GetData(), filesz) == filesz) {
				if (file->sizeFlags > 0) {
					// BSA
					if (file->compressed() ^ compressToggle) {
						if (headerVersion == SSE_BSAHEADER_VERSION) {
							chunk = lz4fUncompress(chunk);
							content.AppendData(chunk, chunk.GetDataLen());
						}
						else {
							chunk = gUncompress(chunk, 0, 4);
							content.AppendData(chunk, chunk.GetDataLen());
						}
					}
					else
						content.AppendData(chunk.GetData(), chunk.GetDataLen());
				}
				else if (file->packedLength > 0 && file->tex.chunks.empty()) {
					// GNRL BA2 compressed
					chunk = gUncompress(chunk, file->unpackedLength);
					content.AppendData(chunk, chunk.GetDataLen());
				}
				else if (file->tex.chunks.empty()) {
					// GNRL BA2 uncompressed
					content.AppendData(chunk.GetData(), chunk.GetDataLen());
				}

				if (!file->tex.chunks.empty()) {
					// Read chunks for DX10 BA2
					for (size_t i = 0; i < file->tex.chunks.size(); i++) {
						const F4TexChunk& chunkInfo = file->tex.chunks[i];
						if (bsa.Seek(chunkInfo.offset)) {
							wxMemoryBuffer currentChunk;

							if (chunkInfo.packedSize > 0) {
								currentChunk.SetBufSize(chunkInfo.packedSize);
								currentChunk.SetDataLen(chunkInfo.packedSize);
								if (bsa.Read(currentChunk.GetData(), chunkInfo.packedSize) == (ssize_t)chunkInfo.packedSize)
									currentChunk = gUncompress(currentChunk, chunkInfo.unpackedSize);

								if (currentChunk.GetDataLen() != chunkInfo.unpackedSize) {
									// Size does not match at chunkInfo.offset
									return false;
								}
							}
							else {
								currentChunk.SetBufSize(chunkInfo.unpackedSize);
								currentChunk.SetDataLen(chunkInfo.unpackedSize);
								if (!(bsa.Read(currentChunk.GetData(), chunkInfo.unpackedSize) == (ssize_t)chunkInfo.unpackedSize)) {
									// Size does not match at chunkInfo.offset
									return false;
								}
							}

							content.AppendData(currentChunk.GetData(), currentChunk.GetDataLen());
						}
						else {
							// Seek error
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

bool BSA::exportFile(const std::string &fn, const std::string &target) {
	wxMemoryBuffer content;
	if (!fileContents(fn, content))
		return false;

	if (content.IsEmpty())
		return false;

	wxFile file(target, wxFile::write);
	if (file.Error())
		return false;

	if (file.Write(content.GetData(), content.GetDataLen()) != content.GetDataLen())
		return false;

	file.Close();
	return true;
}

std::string BSA::absoluteFilePath(const std::string &fn) const {
	if (hasFile(fn)) {
		wxFileName fileInfo(fn);
		return (fileInfo.GetPath(true) + fileInfo.GetName()).ToStdString();
	}

	return std::string();
}

BSA::BSAFolder *BSA::insertFolder(std::string name) {
	if (name.empty())
		return &root;

	std::replace(name.begin(), name.end(), '\\', '/');
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	BSAFolder *folder = folders[name];
	if (!folder) {
		folder = new BSAFolder;
		folders[name] = folder;

		int p = name.find_last_of('/');
		if (p >= 0) {
			folder->parent = insertFolder(name.substr(0, p));
			folder->parent->children[name.substr(p + 1)] = folder;
		}
		else {
			folder->parent = &root;
			root.children[name] = folder;
		}
	}

	return folder;
}

BSA::BSAFolder* BSA::insertFolder( char* folder, int szFn) {
	auto loc = folders.find(std::string(folder, folder + szFn));
	if (loc != folders.end()) {
		return loc->second;
	}

	BSAFolder* fldr = new BSAFolder();
	folders[std::string(folder, folder + szFn)] = fldr;
	
	for (int p = szFn - 1; p >= 0; p--) {
		if (folder[p] == '/') {
			fldr->parent = insertFolder(folder, p);
			fldr->parent->children[std::string(folder + p + 1,szFn-p-1)] = fldr;
			return fldr;
		}
	}
	fldr->parent = &root;
	root.children[std::string(folder, folder + szFn)] = fldr;
	return fldr;	
}

BSA::BSAFile *BSA::insertFile(BSAFolder *folder, std::string name, wxUint32 sizeFlags, wxUint32 offset) {
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	BSAFile *file = new BSAFile;
	file->sizeFlags = sizeFlags;
	file->offset = offset;

	folder->files[name] = file;
	return file;
}

BSA::BSAFile* BSA::insertFile(char* filename, int szFn, wxUint32 packed, wxUint32 unpacked, wxUint64 offset, F4Tex* dds) {
	std::transform(filename, filename + szFn, filename, ::tolower);
	//int p;
	//for (p = szFn - 1; p >= 0; p--) {
	//	if (filename[p] == '/')
	//		break;
	//}
	//BSAFolder* folder;
	//if (p > -1)
	//	folder = insertFolder(filename, p);
	//else
	//	folder = &root;

	BSAFile *file = new BSAFile;
	if (dds)
		file->tex = *dds;

	file->packedLength = packed;
	file->unpackedLength = unpacked;
	file->offset = offset;
	//folder->files[name] = file;
	root.files.emplace(std::string(filename, szFn), file);
	return nullptr;
}

const BSA::BSAFolder *BSA::getFolder(std::string fn) const {
	std::transform(fn.begin(), fn.end(), fn.begin(), ::tolower);

	if (fn.empty()) {
		return &root;
	}
	else {
		auto it = folders.find(fn);
		if (it != folders.end()) {
			BSA::BSAFolder *folder = it->second;
			if (folder)
				return folder;
		}
	}

	return nullptr;
}

const BSA::BSAFile *BSA::getFile(std::string fn) const {
	std::transform(fn.begin(), fn.end(), fn.begin(), ::tolower);

	auto earlyfile = root.files.find(fn);
	if (earlyfile != root.files.end()) {
		return earlyfile->second;
	}

	std::string folderName;
	int p = fn.find_last_of('/');
	if (p >= 0) {
		folderName = fn.substr(0, p);
		fn = fn.substr(p + 1);
	}

	// TODO: Multiple matches occur and user has no say which version gets loaded
	// When it comes to the AUTO feature, should give preference to certain BSAs
	// or take the newest and or highest quality version.
	if (const BSAFolder *folder = getFolder(folderName)) {
		auto it = folder->files.find(fn);
		if (it != folder->files.end()) {
			BSA::BSAFile *file = it->second;
			if (file)
				return file;
		}
	}

	return nullptr;
}

bool BSA::hasFile(const std::string &fn) const {
	return getFile(fn) != nullptr;
}

bool BSA::hasFolder(const std::string &fn) const {
	return getFolder(fn) != nullptr;
}

wxDateTime BSA::fileTime(const std::string&) const {
	wxDateTime created;
	bsaInfo.GetTimes(nullptr, nullptr, &created);
	return created;
}
