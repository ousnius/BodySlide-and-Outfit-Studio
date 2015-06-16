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

#ifndef BSA_H
#define BSA_H

#include "FSEngine.h"

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/thread.h>
#include <unordered_map>


//! \file bsa.h BSA file, BSAIterator

//! A Bethesda Software Archive file
/*!
 * See <a href="http://www.uesp.net/wiki/Main_Page">The UESP</a> for descriptions of the
 * <a href="http://www.uesp.net/wiki/Tes3Mod:BSA_File_Format">Morrowind format</a> and the
 * <a href="http://www.uesp.net/wiki/Tes4Mod:BSA_File_Format">Oblivion format</a>.
 *
 * \sa MWBSAHeader, MWBSAFileSizeOffset, OBBSAHeader, OBBSAFileInfo, OBBSAFolderInfo
 */
class BSA final : public FSArchiveFile {
public:
	//! Constructor; creates a %BSA from the given file path.
	BSA(const wxString &filePath);
	//! Destructor; closes the file.
	~BSA();

	//! Opens the %BSA file
	bool open() override final;
	//! Closes the %BSA file
	void close() override final;

	//! Returns BSA::bsaPath.
	wxString path() const override final { return bsaPath; }
	//! Returns BSA::bsaBase.
	wxString base() const override final { return bsaBase; }
	//! Returns BSA::bsaName.
	wxString name() const override final { return bsaName; }

	//! Whether the specified folder exists or not
	bool hasFolder(const wxString&) const override final;

	//! Whether the specified file exists or not
	bool hasFile(const wxString&) const override final;
	//! Returns the size of the file per BSAFile::size().
	wxInt64 fileSize(const wxString&) const override final;
	//! Returns the contents of the specified file
	/*!
	* \param fn The filename to get the contents for
	* \param content Reference to the byte array that holds the file contents
	* \return True if successful
	*/
	bool fileContents(const wxString&, wxMemoryBuffer&) override final;

	//! See QFileInfo::ownerId().
	wxUint32 ownerId(const wxString&) const override final;
	//! See QFileInfo::owner().
	wxString owner(const wxString&) const override final;
	//! See QFileInfo::created().
	wxDateTime fileTime(const wxString&) const override final;
	//! See QFileInfo::absoluteFilePath().
	wxString absoluteFilePath(const wxString&) const override final;

	//! Whether the given file can be opened as a %BSA or not
	static bool canOpen(const wxString&);

	//! Returns BSA::status.
	wxString statusText() const { return status; }

protected:
	//! A file inside a BSA
	struct BSAFile
	{
		wxUint32 sizeFlags; //!< The size of the file in the BSA
		wxUint32 offset; //!< The offset of the file in the BSA

		//! The size of the file inside the BSA
		wxUint32 size() const;
		//! Whether the file is compressed inside the BSA
		bool compressed() const;
	};

	//! A folder inside a BSA
	struct BSAFolder
	{
		//! Constructor
		BSAFolder() : parent(0) {}
		//! Destructor
		~BSAFolder() {
			for (auto it : children)
				delete it.second;
			for (auto it : files)
				delete it.second;

			children.clear();
			files.clear();
		}

		BSAFolder *parent; //!< The parent item
		std::unordered_map<std::string, BSAFolder*> children; //!< A map of child folders
		std::unordered_map<std::string, BSAFile*> files; //!< A map of files inside the folder
	};

	//! Recursive function to generate the tree structure of folders inside a %BSA
	BSAFolder *insertFolder(wxString name);
	//! Inserts a file into the structure of a %BSA
	BSAFile *insertFile(BSAFolder *folder, wxString name, wxUint32 sizeFlags, wxUint32 offset);

	//! Gets the specified folder, or the root folder if not found
	const BSAFolder *getFolder(wxString fn) const;
	//! Gets the specified file, or null if not found
	const BSAFile *getFile(wxString fn) const;

	//! The %BSA file
	wxFile bsa;
	//! File info for the %BSA
	wxFileName bsaInfo;

	//! Mutual exclusion handler
	wxMutex bsaMutex;

	//! The absolute name of the file, e.g. "d:/temp/test.bsa"
	wxString bsaPath;
	//! The base path of the file, e.g. "d:/temp"
	wxString bsaBase;
	//! The name of the file, e.g. "test.bsa"
	wxString bsaName;

	//! Map of folders inside a %BSA
	std::unordered_map<std::string, BSAFolder*> folders;
	//! The root folder
	BSAFolder root;

	//! Error string for exception handling
	wxString status;

	//! Whether the %BSA is compressed
	bool compressToggle;
	//! Whether Fallout 3 names are prefixed with an extra string
	bool namePrefix;
};

#endif
