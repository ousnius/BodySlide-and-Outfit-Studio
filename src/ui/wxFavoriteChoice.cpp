/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxFavoriteChoice.h"

#include <wx/dc.h>
#include <wx/settings.h>
#include <wx/time.h>

wxIMPLEMENT_DYNAMIC_CLASS(wxFavoriteChoice, wxOwnerDrawnComboBox);

namespace {
const wxString FavoriteStarText = wxString::FromUTF8("\xE2\x98\x85 ");
const wxColour FavoriteStarColour(230, 170, 20);

// Typing pause after which a new search starts
constexpr unsigned long TypeAheadTimeoutMs = 1000;
}

int wxFavoriteChoicePopup::FindPrefix(const wxString& prefix) const {
	const size_t length = prefix.length();
	const int count = static_cast<int>(GetCount());
	for (int i = 0; i < count; i++) {
		const wxString item = GetString(i);
		if (item.length() >= length && prefix.CmpNoCase(item.Left(length)) == 0)
			return i;
	}

	return wxNOT_FOUND;
}

bool wxFavoriteChoicePopup::Create(wxWindow* parent) {
	if (!wxVListBoxComboPopup::Create(parent))
		return false;

	// Bound handlers run before the base class event table, which would otherwise
	// treat space as a click and commit the highlighted item on every character
	Bind(wxEVT_KEY_DOWN, &wxFavoriteChoicePopup::OnPopupKeyDown, this);
	Bind(wxEVT_CHAR, &wxFavoriteChoicePopup::OnPopupChar, this);
	return true;
}

void wxFavoriteChoicePopup::OnPopup() {
	typedPrefix.clear();
	wxVListBoxComboPopup::OnPopup();
}

unsigned long wxFavoriteChoicePopup::GetEventTime(const wxKeyEvent& event) {
	long time = event.GetTimestamp();
	if (time == 0)
		time = static_cast<long>(wxGetLocalTimeMillis().GetLo());

	return static_cast<unsigned long>(time);
}

bool wxFavoriteChoicePopup::IsTypingAhead(unsigned long now) const {
	// Unsigned difference handles the millisecond counter wrapping around
	return !typedPrefix.empty() && now - lastTypedTime <= TypeAheadTimeoutMs;
}

bool wxFavoriteChoicePopup::HandleTypeAhead(const wxKeyEvent& event) {
	const wxChar keyChar = event.GetUnicodeKey();
	if (!m_combo->HasFlag(wxCB_READONLY) || keyChar == WXK_NONE || !wxIsprint(keyChar) || event.ControlDown() || event.AltDown())
		return false;

	TypeAhead(keyChar, GetEventTime(event));
	return true;
}

void wxFavoriteChoicePopup::TypeAhead(wxChar keyChar, unsigned long now) {
	if (!IsTypingAhead(now))
		typedPrefix.clear();

	lastTypedTime = now;
	typedPrefix += keyChar;

	const int found = FindPrefix(typedPrefix);
	if (found == wxNOT_FOUND)
		return;

	if (m_combo->IsPopupShown()) {
		// Only move the highlight: Enter or a click commits it, while Escape
		// leaves the value (m_value and m_stringValue) as it was
		wxVListBox::SetSelection(found);
		return;
	}

	if (found == m_value)
		return;

	m_combo->ChangeValue(m_strings[found]);
	m_value = found;
	SendComboBoxEvent(found);
}

void wxFavoriteChoicePopup::OnComboCharEvent(wxKeyEvent& event) {
	if (!HandleTypeAhead(event))
		wxVListBoxComboPopup::OnComboCharEvent(event);
}

void wxFavoriteChoicePopup::OnPopupKeyDown(wxKeyEvent& event) {
	// Space continues a search in progress ("CBBE Body") instead of acting as a click
	if (event.GetKeyCode() == WXK_SPACE && !event.HasAnyModifiers() && m_combo->HasFlag(wxCB_READONLY)) {
		const unsigned long now = GetEventTime(event);
		if (IsTypingAhead(now)) {
			TypeAhead(' ', now);
			return;
		}
	}

	event.Skip();
}

void wxFavoriteChoicePopup::OnPopupChar(wxKeyEvent& event) {
	if (!HandleTypeAhead(event))
		event.Skip();
}

void wxFavoriteChoice::DoSetPopupControl(wxComboPopup* popup) {
	if (!popup)
		popup = new wxFavoriteChoicePopup();

	wxOwnerDrawnComboBox::DoSetPopupControl(popup);
}

int wxFavoriteChoice::AppendItem(const wxString& name, bool favorite, const wxString& suffix) {
	int index = Append(name);
	if (index < 0)
		return index;

	if (static_cast<size_t>(index) >= itemStyles.size())
		itemStyles.resize(index + 1);

	itemStyles[index].favorite = favorite;
	itemStyles[index].suffix = suffix;
	return index;
}

void wxFavoriteChoice::DoClear() {
	itemStyles.clear();
	wxOwnerDrawnComboBox::DoClear();
}

void wxFavoriteChoice::DoDeleteOneItem(unsigned int n) {
	if (n < itemStyles.size())
		itemStyles.erase(itemStyles.begin() + n);

	wxOwnerDrawnComboBox::DoDeleteOneItem(n);
}

const wxFavoriteChoice::ItemStyle* wxFavoriteChoice::GetItemStyle(int item) const {
	if (item < 0 || static_cast<size_t>(item) >= itemStyles.size())
		return nullptr;

	return &itemStyles[item];
}

void wxFavoriteChoice::OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const {
	if (item < 0 || item >= static_cast<int>(GetCount()))
		return;

	// The caller has already set the text colour for the item's state
	const wxColour textColour = dc.GetTextForeground();
	const ItemStyle* style = GetItemStyle(item);

	wxCoord x = rect.x + ((flags & wxODCB_PAINTING_CONTROL) ? GetMargins().x : FromDIP(2));
	const wxCoord y = rect.y + (rect.height - dc.GetCharHeight()) / 2;

	if (style && style->favorite) {
		dc.SetTextForeground(FavoriteStarColour);
		dc.DrawText(FavoriteStarText, x, y);
		x += dc.GetTextExtent(FavoriteStarText).x;
	}

	const wxString name = GetString(item);
	dc.SetTextForeground(textColour);
	dc.DrawText(name, x, y);

	if (style && !style->suffix.empty()) {
		x += dc.GetTextExtent(name).x;
		if (!(flags & wxODCB_PAINTING_SELECTED))
			dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

		dc.DrawText(style->suffix, x, y);
	}

	dc.SetTextForeground(textColour);
}

wxCoord wxFavoriteChoice::OnMeasureItemWidth(size_t item) const {
	if (item >= GetCount())
		return -1;

	wxString text = GetString(item);
	if (const ItemStyle* style = GetItemStyle(static_cast<int>(item))) {
		if (style->favorite)
			text = FavoriteStarText + text;
		text += style->suffix;
	}

	return GetTextExtent(text).x + FromDIP(4);
}
