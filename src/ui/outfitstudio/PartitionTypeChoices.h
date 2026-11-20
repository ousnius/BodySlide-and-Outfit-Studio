#pragma once

#include "components/PartitionTypes.h"

#include <wx/choice.h>
#include <wx/string.h>

namespace PartitionTypeChoices {

inline void Populate(wxChoice* choice) {
	if (!choice)
		return;

	choice->Clear();
	for (const char* label : PartitionTypes::Labels)
		choice->Append(wxString::FromUTF8(label));

	if (choice->GetCount() > 0)
		choice->SetSelection(0);
}

}