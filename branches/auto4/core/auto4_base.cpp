// Copyright (c) 2006, Niels Martin Hansen
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:jiifurusu@gmail.com
//

#include "auto4_base.h"
#include "ass_style.h"
#include "options.h"
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/thread.h>
#include <wx/sizer.h>
#include <wx/filefn.h>
#include <wx/tokenzr.h>

#ifdef WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

namespace Automation4 {

	bool CalculateTextExtents(AssStyle *style, wxString &text, int &width, int &height, int &descent, int &extlead)
	{
		width = height = descent = extlead = 0;

		double fontsize = style->fontsize;

#ifdef WIN32
		// This is almost copypasta from TextSub
		HDC thedc = CreateCompatibleDC(0);
		if (!thedc) return false;
		SetMapMode(thedc, MM_TEXT);

		HDC dczero = GetDC(0);
		fontsize = -MulDiv((int)(fontsize+0.5), GetDeviceCaps(dczero, LOGPIXELSY), 72);
		ReleaseDC(0, dczero);

		LOGFONT lf;
		ZeroMemory(&lf, sizeof(lf));
		lf.lfHeight = fontsize;
		lf.lfWeight = style->bold ? FW_BOLD : FW_NORMAL;
		lf.lfItalic = style->italic;
		lf.lfUnderline = style->underline;
		lf.lfStrikeOut = style->strikeout;
		lf.lfCharSet = style->encoding;
		lf.lfOutPrecision = OUT_TT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
		wcsncpy(lf.lfFaceName, style->font.wc_str(), 32);

		HFONT thefont = CreateFontIndirect(&lf);
		if (!thefont) return false;
		SelectObject(thedc, thefont);
		
		SIZE sz;
		size_t thetextlen = text.length();
		const wchar_t *thetext = text.wc_str();
		if (style->spacing) {
			width = 0;
			for (unsigned int i = 0; i < thetextlen; i++) {
				GetTextExtentPoint32(thedc, &thetext[i], 1, &sz);
				width += sz.cx + (int)style->spacing;
				height = sz.cy;
			}
		} else {
			GetTextExtentPoint32(thedc, thetext, (int)thetextlen, &sz);
			width = sz.cx;
			height = sz.cy;
		}

		// HACKISH FIX! This seems to work, but why? It shouldn't be needed?!?
		fontsize = style->fontsize;
		width = (int)(width * fontsize/height + 0.5);
		height = (int)(fontsize + 0.5);

		TEXTMETRIC tm;
		GetTextMetrics(thedc, &tm);
		descent = tm.tmDescent;
		extlead= tm.tmExternalLeading;

		DeleteObject(thedc);
		DeleteObject(thefont);

#else // not WIN32
		wxMemoryDC thedc;

		// fix fontsize to be 72 DPI
		fontsize = -FT_MulDiv((int)(fontsize+0.5), 72, thedc.GetPPI().y);

		// now try to get a font!
		// use the font list to get some caching... (chance is the script will need the same font very often)
		// USING wxTheFontList SEEMS TO CAUSE BAD LEAKS!
		//wxFont *thefont = wxTheFontList->FindOrCreateFont(
		wxFont thefont(
			fontsize,
			wxFONTFAMILY_DEFAULT,
			style->italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
			style->bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
			style->underline,
			style->font,
			wxFONTENCODING_SYSTEM); // FIXME! make sure to get the right encoding here, make some translation table between windows and wx encodings
		thedc.SetFont(thefont);

		if (style->spacing) {
			// If there's inter-character spacing, kerning info must not be used, so calculate width per character
			// NOTE: Is kerning actually done either way?!
			for (unsigned int i = 0; i < intext.length(); i++) {
				int a, b, c, d;
				thedc.GetTextExtent(intext[i], &a, &b, &c, &d);
				width += a + spacing;
				height = b > height ? b : height;
				descent = c > descent ? c : descent;
				extlead= d > extlead ? d : extlead;
			}
		} else {
			// If the inter-character spacing should be zero, kerning info can (and must) be used, so calculate everything in one go
			thedc.GetTextExtent(intext, &width, &height, &descent, &extlead);
		}
#endif

		// Compensate for scaling
		width = (int)(style->scalex / 100 * width + 0.5);
		height = (int)(style->scaley / 100 * height + 0.5);
		descent = (int)(style->scaley / 100 * descent + 0.5);
		extlead = (int)(style->scaley / 100 * extlead + 0.5);

		return true;
	}


	// Feature

	Feature::Feature(ScriptFeatureClass _featureclass, const wxString &_name)
		: featureclass(_featureclass)
		, name(_name)
	{
		// nothing to do
	}

	ScriptFeatureClass Feature::GetClass() const
	{
		return featureclass;
	}

	FeatureMacro* Feature::AsMacro()
	{
		if (featureclass == SCRIPTFEATURE_MACRO)
			// For VS, remember to enable building with RTTI, otherwise dynamic_cast<> won't work
			return dynamic_cast<FeatureMacro*>(this);
		return 0;
	}

	FeatureFilter* Feature::AsFilter()
	{
		if (featureclass == SCRIPTFEATURE_FILTER)
			return dynamic_cast<FeatureFilter*>(this);
		return 0;
	}

	FeatureSubtitleFormat* Feature::AsSubFormat()
	{
		if (featureclass == SCRIPTFEATURE_SUBFORMAT)
			return dynamic_cast<FeatureSubtitleFormat*>(this);
		return 0;
	}

	const wxString& Feature::GetName() const
	{
		return name;
	}


	// FeatureMacro

	FeatureMacro::FeatureMacro(const wxString &_name, const wxString &_description, MacroMenu _menu)
		: Feature(SCRIPTFEATURE_MACRO, _name)
		, description(_description)
		, menu(_menu)
	{
		// nothing to do
	}

	const wxString& FeatureMacro::GetDescription() const
	{
		return description;
	}

	MacroMenu FeatureMacro::GetMenu() const
	{
		return menu;
	}


	// FeatureFilter

	FeatureFilter::FeatureFilter(const wxString &_name, const wxString &_description, int _priority)
		: Feature(SCRIPTFEATURE_FILTER, _name)
		, AssExportFilter()
	{
		description = _description; // from AssExportFilter
		Register(_name, _priority);
	}


	// FeatureReader

	FeatureSubtitleFormat::FeatureSubtitleFormat(const wxString &_name, const wxString &_extension)
		: Feature(SCRIPTFEATURE_SUBFORMAT, _name)
		, extension(_extension)
	{
		// nothing to do
	}

	const wxString& FeatureSubtitleFormat::GetExtension() const
	{
		return extension;
	}

	bool FeatureSubtitleFormat::CanWriteFile(wxString filename)
	{
		return !filename.Right(extension.Length()).CmpNoCase(extension);
	}

	bool FeatureSubtitleFormat::CanReadFile(wxString filename)
	{
		return !filename.Right(extension.Length()).CmpNoCase(extension);
	}


	// ProgressSink

	ProgressSink::ProgressSink(wxWindow *parent)
		: wxDialog(parent, -1, _T("Automation"), wxDefaultPosition, wxDefaultSize, wxDOUBLE_BORDER)
		, cancelled(false)
		, has_inited(false)
	{
		// make the controls
		progress_display = new wxGauge(this, -1, 1000, wxDefaultPosition, wxSize(300, 20));
		title_display = new wxStaticText(this, -1, _T(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
		task_display = new wxStaticText(this, -1, _T(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
		cancel_button = new wxButton(this, wxID_CANCEL);

		// put it in a sizer
		wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(title_display, 0, wxEXPAND | wxALL, 5);
		sizer->Add(progress_display, 0, wxALL&~wxTOP, 5);
		sizer->Add(task_display, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
		sizer->Add(cancel_button, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 5);

		// make the title a slightly larger font
		wxFont title_font = title_display->GetFont();
		int fontsize = title_font.GetPointSize();
		title_font.SetPointSize(fontsize + fontsize/4 + fontsize/8);
		title_font.SetWeight(wxFONTWEIGHT_BOLD);
		title_display->SetFont(title_font);

		sizer->SetSizeHints(this);
		SetSizer(sizer);
		Center();
	}

	ProgressSink::~ProgressSink()
	{
	}

	void ProgressSink::SetProgress(float _progress)
	{
		if (wxThread::IsMain()) {
			progress_display->SetValue((int)(_progress*10));
		} else {
			wxMutexGuiLocker gui;
			progress_display->SetValue((int)(_progress*10));
		}
	}

	void ProgressSink::SetTask(const wxString &_task)
	{
		if (wxThread::IsMain()) {
			task_display->SetLabel(_task);
		} else {
			wxMutexGuiLocker gui;
			task_display->SetLabel(_task);
		}
	}

	void ProgressSink::SetTitle(const wxString &_title)
	{
		if (wxThread::IsMain()) {
			title_display->SetLabel(_title);
		} else {
			wxMutexGuiLocker gui;
			title_display->SetLabel(_title);
		}
	}

	BEGIN_EVENT_TABLE(ProgressSink, wxWindow)
		EVT_INIT_DIALOG(ProgressSink::OnInit)
		EVT_BUTTON(wxID_CANCEL, ProgressSink::OnCancel)
	END_EVENT_TABLE()

	void ProgressSink::OnInit(wxInitDialogEvent &evt)
	{
		has_inited = true;
	}

	void ProgressSink::OnCancel(wxCommandEvent &evt)
	{
		cancelled = true;
		cancel_button->Enable(false);
	}


	// Script

	Script::Script(const wxString &_filename)
		: filename(_filename)
		, name(_T(""))
		, description(_T(""))
		, author(_T(""))
		, version(_T(""))
		, loaded(false)
	{
		// copied from auto3
		include_path.clear();
		include_path.EnsureFileAccessible(filename);
		wxStringTokenizer toker(Options.AsText(_T("Automation Include Path")), _T("|"), false);
		while (toker.HasMoreTokens()) {
			// todo? make some error reporting here
			wxFileName path(toker.GetNextToken());
			if (!path.IsOk()) continue;
			if (path.IsRelative()) continue;
			if (!path.DirExists()) continue;
			if (include_path.Member(path.GetLongPath())) continue;
			include_path.Add(path.GetLongPath());
		}
	}

	const wxString& Script::GetFilename() const
	{
		return filename;
	}

	const wxString& Script::GetName() const
	{
		return name;
	}

	const wxString& Script::GetDescription() const
	{
		return description;
	}

	const wxString& Script::GetAuthor() const
	{
		return author;
	}

	const wxString& Script::GetVersion() const
	{
		return version;
	}

	bool Script::GetLoadedState() const
	{
		return loaded;
	}

	std::vector<Feature*>& Script::GetFeatures()
	{
		return features;
	}


	// ScriptManager

	ScriptManager::ScriptManager()
	{
		// do nothing...?
	}

	ScriptManager::~ScriptManager()
	{
		RemoveAll();
	}

	void ScriptManager::Add(Script *script)
	{
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			if (script == *i) return;
		}
		scripts.push_back(script);
	}

	void ScriptManager::Remove(Script *script)
	{
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			if (script == *i) {
				delete *i;
				scripts.erase(i);
				return;
			}
		}
	}

	void ScriptManager::RemoveAll()
	{
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			delete *i;
		}
		scripts.clear();
	}

	const std::vector<Script*>& ScriptManager::GetScripts() const
	{
		return scripts;
	}

	const std::vector<FeatureMacro*>& ScriptManager::GetMacros(MacroMenu menu)
	{
		macros[menu].clear();
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			std::vector<Feature*> &sfs = (*i)->GetFeatures();
			for (std::vector<Feature*>::iterator j = sfs.begin(); j != sfs.end(); ++j) {
				FeatureMacro *m = dynamic_cast<FeatureMacro*>(*j);
				if (!m) continue;
				if (menu == MACROMENU_ALL || m->GetMenu() == menu)
					macros[menu].push_back(m);
			}
		}
		return macros[menu];
	}


	// AutoloadScriptManager

	AutoloadScriptManager::AutoloadScriptManager(const wxString &_path)
		: path(_path)
	{
		Reload();
	}

	void AutoloadScriptManager::Reload()
	{
		wxDir dir(path);
		if (!dir.IsOpened()) {
			// crap
			return;
		}

		RemoveAll();

		int error_count = 0;

		wxString fn;
		wxFileName script_path(path, _T(""));
		bool more = dir.GetFirst(&fn, wxEmptyString, wxDIR_FILES);
		while (more) {
			script_path.SetName(fn);
			try {
				Add(ScriptFactory::CreateFromFile(script_path.GetFullPath()));
			}
			catch (const wchar_t *e) {
				error_count++;
				wxLogError(_T("Error loading Automation script: %s\n%s"), fn.c_str(), e);
			}
			catch (...) {
				error_count++;
				wxLogError(_T("Error loading Automation script: %s\nUnknown error."), fn.c_str());
			}
			more = dir.GetNext(&fn);
		}
		if (error_count) {
			wxLogWarning(_T("One or more scripts placed in the Automation autoload directory failed to load\nPlease review the errors above, correct them and use the Reload Autoload dir button in Automation Manager to attempt loading the scripts again."));
		}
	}



	// ScriptFactory

	std::vector<ScriptFactory*> ScriptFactory::factories;

	const wxString& ScriptFactory::GetEngineName() const
	{
		return engine_name;
	}

	const wxString& ScriptFactory::GetFilenamePattern() const
	{
		return filename_pattern;
	}

	void ScriptFactory::Register(ScriptFactory *factory)
	{
		for (std::vector<ScriptFactory*>::iterator i = factories.begin(); i != factories.end(); ++i) {
			if (*i == factory) {
				throw _T("Automation 4: Attempt to register the same script factory multiple times.");
			}
		}
		factories.push_back(factory);
	}

	void ScriptFactory::Unregister(ScriptFactory *factory)
	{
		for (std::vector<ScriptFactory*>::iterator i = factories.begin(); i != factories.end(); ++i) {
			if (*i == factory) {
				factories.erase(i);
				return;
			}
		}
	}

	Script* ScriptFactory::CreateFromFile(const wxString &filename)
	{
		for (std::vector<ScriptFactory*>::iterator i = factories.begin(); i != factories.end(); ++i) {
			Script *s = (*i)->Produce(filename);
			if (s) return s;
		}
		return new UnknownScript(filename);
	}

	const std::vector<ScriptFactory*>& ScriptFactory::GetFactories()
	{
		return factories;
	}


	// UnknownScript

	UnknownScript::UnknownScript(const wxString &filename)
		: Script(filename)
	{
		wxFileName fn(filename);
		name = fn.GetName();
		description = _("File was not recognized as a script");
		loaded = false;
	}

};
