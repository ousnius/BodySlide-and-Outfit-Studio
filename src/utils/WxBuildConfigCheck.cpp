#include <wx/setup.h>
#include <wx/chkconf.h>

#if !defined(wxUSE_GUI) || !wxUSE_GUI
#error "BodySlide and Outfit Studio require wxWidgets GUI support."
#endif

#if !defined(wxUSE_XRC) || !wxUSE_XRC
#error "BodySlide and Outfit Studio require wxWidgets XRC support."
#endif

#if !defined(wxUSE_XML) || !wxUSE_XML
#error "BodySlide and Outfit Studio require wxWidgets XML support."
#endif

#if !defined(wxUSE_GLCANVAS) || !wxUSE_GLCANVAS
#error "BodySlide and Outfit Studio require wxGLCanvas support."
#endif

#if !defined(wxUSE_PROPGRID) || !wxUSE_PROPGRID
#error "BodySlide and Outfit Studio require wxPropertyGrid support."
#endif

#if !defined(wxUSE_HTML) || !wxUSE_HTML
#error "BodySlide and Outfit Studio require wxHTML support."
#endif

#if !defined(wxUSE_DEBUGREPORT) || !wxUSE_DEBUGREPORT
#error "BodySlide and Outfit Studio require wxDebugReport support."
#endif

#if !defined(wxUSE_STACKWALKER) || !wxUSE_STACKWALKER
#error "BodySlide and Outfit Studio require wxStackWalker support."
#endif

#if !defined(wxUSE_ON_FATAL_EXCEPTION) || !wxUSE_ON_FATAL_EXCEPTION
#error "BodySlide and Outfit Studio require wxHandleFatalExceptions support."
#endif

#if !defined(wxUSE_LOG) || !wxUSE_LOG
#error "BodySlide and Outfit Studio require wxLog support."
#endif

#if !defined(wxUSE_THREADS) || !wxUSE_THREADS
#error "BodySlide and Outfit Studio require wxWidgets thread support."
#endif

#if !defined(wxUSE_STREAMS) || !wxUSE_STREAMS
#error "BodySlide and Outfit Studio require wxWidgets stream support."
#endif

#if !defined(wxUSE_ZIPSTREAM) || !wxUSE_ZIPSTREAM
#error "BodySlide and Outfit Studio require wxZip stream support."
#endif

#if !defined(wxUSE_INTL) || !wxUSE_INTL
#error "BodySlide and Outfit Studio require wxLocale/internationalization support."
#endif

#if !defined(wxUSE_CMDLINE_PARSER) || !wxUSE_CMDLINE_PARSER
#error "BodySlide and Outfit Studio require wxCmdLineParser support."
#endif

#if !defined(wxUSE_CONFIG) || !wxUSE_CONFIG
#error "BodySlide and Outfit Studio require wxConfig support."
#endif

#if !defined(wxUSE_SNGLINST_CHECKER) || !wxUSE_SNGLINST_CHECKER
#error "BodySlide and Outfit Studio require wxSingleInstanceChecker support."
#endif

#if !defined(wxUSE_IPC) || !wxUSE_IPC
#error "BodySlide and Outfit Studio require wxIPC support."
#endif

#if !defined(wxUSE_IMAGE) || !wxUSE_IMAGE
#error "BodySlide and Outfit Studio require wxImage support."
#endif

#if !defined(wxUSE_LIBPNG) || !wxUSE_LIBPNG
#error "BodySlide and Outfit Studio require wxPNG image support."
#endif

#if !defined(wxUSE_LIBJPEG) || !wxUSE_LIBJPEG
#error "BodySlide and Outfit Studio require wxJPEG image support."
#endif

#if !defined(wxUSE_LISTCTRL) || !wxUSE_LISTCTRL
#error "BodySlide and Outfit Studio require wxListCtrl support."
#endif

#if !defined(wxUSE_TREECTRL) || !wxUSE_TREECTRL
#error "BodySlide and Outfit Studio require wxTreeCtrl support."
#endif

#if !defined(wxUSE_SLIDER) || !wxUSE_SLIDER
#error "BodySlide and Outfit Studio require wxSlider support."
#endif

#if !defined(wxUSE_GAUGE) || !wxUSE_GAUGE
#error "BodySlide and Outfit Studio require wxGauge support."
#endif

#if !defined(wxUSE_SPINCTRL) || !wxUSE_SPINCTRL
#error "BodySlide and Outfit Studio require wxSpinCtrl support."
#endif

#if !defined(wxUSE_CHECKBOX) || !wxUSE_CHECKBOX
#error "BodySlide and Outfit Studio require wxCheckBox support."
#endif

#if !defined(wxUSE_CHOICE) || !wxUSE_CHOICE
#error "BodySlide and Outfit Studio require wxChoice support."
#endif

#if !defined(wxUSE_COMBOBOX) || !wxUSE_COMBOBOX
#error "BodySlide and Outfit Studio require wxComboBox support."
#endif

#if !defined(wxUSE_TEXTCTRL) || !wxUSE_TEXTCTRL
#error "BodySlide and Outfit Studio require wxTextCtrl support."
#endif

#if !defined(wxUSE_FILEDLG) || !wxUSE_FILEDLG
#error "BodySlide and Outfit Studio require wxFileDialog support."
#endif

#if !defined(wxUSE_DIRDLG) || !wxUSE_DIRDLG
#error "BodySlide and Outfit Studio require wxDirDialog support."
#endif

#if !defined(wxUSE_FILEPICKERCTRL) || !wxUSE_FILEPICKERCTRL
#error "BodySlide and Outfit Studio require wxFilePickerCtrl support."
#endif

#if !defined(wxUSE_DIRPICKERCTRL) || !wxUSE_DIRPICKERCTRL
#error "BodySlide and Outfit Studio require wxDirPickerCtrl support."
#endif

#if !defined(wxUSE_COLOURPICKERCTRL) || !wxUSE_COLOURPICKERCTRL
#error "BodySlide and Outfit Studio require wxColourPickerCtrl support."
#endif

#if !defined(wxUSE_COLLPANE) || !wxUSE_COLLPANE
#error "BodySlide and Outfit Studio require wxCollapsiblePane support."
#endif

#if !defined(wxUSE_NOTEBOOK) || !wxUSE_NOTEBOOK
#error "BodySlide and Outfit Studio require wxNotebook support."
#endif

#if !defined(wxUSE_TOOLBAR) || !wxUSE_TOOLBAR
#error "BodySlide and Outfit Studio require wxToolBar support."
#endif

#if !defined(wxUSE_STATUSBAR) || !wxUSE_STATUSBAR
#error "BodySlide and Outfit Studio require wxStatusBar support."
#endif

#if !defined(wxUSE_MENUS) || !wxUSE_MENUS
#error "BodySlide and Outfit Studio require wxMenu support."
#endif

#include <wx/cmdline.h>
#include <wx/config.h>
#include <wx/debugrpt.h>
#include <wx/filepicker.h>
#include <wx/glcanvas.h>
#include <wx/html/htmlwin.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/ipc.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/propgrid/propgrid.h>
#include <wx/snglinst.h>
#include <wx/stackwalk.h>
#include <wx/xrc/xmlres.h>
#include <wx/zipstrm.h>

namespace {
[[maybe_unused]] constexpr bool wxBuildConfigValidated = true;
}