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

	const FeatureMacro* Feature::AsMacro() const
	{
		if (featureclass == SCRIPTFEATURE_MACRO)
			return static_cast<const FeatureMacro*>(this);
		return 0;
	}

	const FeatureFilter* Feature::AsFilter() const
	{
		if (featureclass == SCRIPTFEATURE_FILTER)
			return static_cast<const FeatureFilter*>(this);
		return 0;
	}

	const FeatureSubtitleFormat* Feature::AsSubFormat() const
	{
		if (featureclass == SCRIPTFEATURE_SUBFORMAT)
			return static_cast<const FeatureSubtitleFormat*>(this);
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

	const std::vector<Feature*>& Script::GetFeatures() const
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
		// do nothing...?
	}

	void ScriptManager::Add(Script *script)
	{
		// TODO
	}

	void ScriptManager::Remove(Script *script)
	{
		// TODO
	}

	const std::vector<Script*>& ScriptManager::GetScripts() const
	{
		return scripts;
	}

	const std::vector<Feature*>& ScriptManager::GetMacros(MacroMenu menu) const
	{
		return macros[menu];
	}


	// ScriptFactory

	std::vector<ScriptFactory*> ScriptFactory::factories;

	const wxString& ScriptFactory::GetEngineName() const
	{
		return engine_name;
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
			Script *s = (*i)->CreateFromFile(filename);
			if (s) return s;
		}
		return 0;
	}

	const std::vector<ScriptFactory*>& ScriptFactory::GetFactories()
	{
		return factories;
	}

};
