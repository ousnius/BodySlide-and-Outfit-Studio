/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/odcombo.h>

#include <vector>

// Popup with type-ahead search: typed characters within a short interval build
// a prefix that selects the first item starting with it (case-insensitive).
// Intervals come from the key event timestamps rather than a timer, so a slow
// selection handler (e.g. loading an outfit) doesn't break up what was typed.
// While the list is open, typing only moves the highlight; Enter or a click
// commits it like any other popup selection.
class wxFavoriteChoicePopup : public wxVListBoxComboPopup {
public:
	bool Create(wxWindow* parent) override;
	void OnComboCharEvent(wxKeyEvent& event) override;
	void OnPopup() override;

private:
	wxString typedPrefix;
	unsigned long lastTypedTime = 0;

	static unsigned long GetEventTime(const wxKeyEvent& event);
	bool IsTypingAhead(unsigned long now) const;
	bool HandleTypeAhead(const wxKeyEvent& event);
	void TypeAhead(wxChar keyChar, unsigned long now);
	int FindPrefix(const wxString& prefix) const;

	void OnPopupKeyDown(wxKeyEvent& event);
	void OnPopupChar(wxKeyEvent& event);
};

// Read-only dropdown that keeps items in the order they're appended and still
// supports type-ahead search (a native wxChoice only does that with wxCB_SORT).
// Item strings stay plain names so the search matches them; a favorite star and
// a dimmed suffix are drawn next to the name instead of being part of the string.
class wxFavoriteChoice : public wxOwnerDrawnComboBox {
public:
	wxFavoriteChoice() = default;

	int AppendItem(const wxString& name, bool favorite = false, const wxString& suffix = wxString());

	void OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const override;
	wxCoord OnMeasureItemWidth(size_t item) const override;

protected:
	void DoSetPopupControl(wxComboPopup* popup) override;
	void DoClear() override;
	void DoDeleteOneItem(unsigned int n) override;

private:
	struct ItemStyle {
		bool favorite = false;
		wxString suffix;
	};

	std::vector<ItemStyle> itemStyles;

	const ItemStyle* GetItemStyle(int item) const;

	DECLARE_DYNAMIC_CLASS(wxFavoriteChoice)
};
