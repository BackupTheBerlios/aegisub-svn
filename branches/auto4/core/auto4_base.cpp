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
// Contact: mailto:zeratul@cellosoft.com
//

#include "auto4_base.h"
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/thread.h>
#include <wx/sizer.h>

namespace Automation4 {


	// Feature

	Feature::Feature(ScriptFeatureClass _featureclass, wxString &_name)
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

	FeatureMacro::FeatureMacro(wxString &_name, wxString &_description, MacroMenu _menu)
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

	FeatureFilter::FeatureFilter(wxString &_name, wxString &_description, int _priority)
		: Feature(SCRIPTFEATURE_FILTER, _name)
		, AssExportFilter()
	{
		description = _description; // from AssExportFilter
		Register(_name, _priority);
	}


	// FeatureReader

	FeatureSubtitleFormat::FeatureSubtitleFormat(wxString &_name, wxString &_extension)
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
		: wxDialog(parent, -1, _T("Automation"), wxDefaultPosition, wxDefaultSize, 0)
		, cancelled(false)
	{
		// make the controls
		progress_display = new wxGauge(this, -1, 1000, wxDefaultPosition, wxSize(200, 20));
		title_display = new wxStaticText(this, -1, _T(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
		task_display = new wxStaticText(this, -1, _T(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
		cancel_button = new wxButton(this, wxID_CANCEL);

		// put it in a sizer
		// FIXME: needs borders etc
		wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(title_display, 0);
		sizer->Add(progress_display, 0);
		sizer->Add(task_display, 0);
		sizer->Add(cancel_button, 0);

		// make the title a slightly larger font
		wxFont title_font = title_display->GetFont();
		int fontsize = title_font.GetPointSize();
		title_font.SetPointSize(fontsize + fontsize >> 3);
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
		EVT_BUTTON(wxID_CANCEL, ProgressSink::OnCancel)
	END_EVENT_TABLE()

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
		// nothing to do..?
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
