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

#pragma once

#ifndef AUTO4_CORE_H
#define AUTO4_CORE_H

#include <wx/string.h>
#include <vector>

#include "ass_export_filter.h"
#include "subtitle_format.h"

namespace Automation4 {

	// The top-level menus a macro can appear in
	enum MacroMenu {
		MACROMENU_NONE = 0, // pseudo-index, dunno if this has any real meaning
		MACROMENU_ALL = 0, // pseudo-index, used to retrieve information about all macros loaded
		MACROMENU_EDIT,
		MACROMENU_VIDEO,
		MACROMENU_AUDIO,
		MACROMENU_TOOLS,
		MACROMENU_RIGHT, // right-click menu

		MACROMENU_MAX // must be last, one higher than the last, used for array sizing
	};
	// The class of a Feature...
	enum ScriptFeatureClass {
		SCRIPTFEATURE_MACRO = 0,
		SCRIPTFEATURE_FILTER,
		SCRIPTFEATURE_SUBFORMAT,

		SCRIPTFEATURE_MAX // must be last
	};

	// A Feature describes a function provided by a Script.
	// There are several distinct classes of features.
	class FeatureMacro;
	class FeatureFilter;
	class FeatureSubtitleFormat;
	class Feature {
	private:
		ScriptFeatureClass featureclass;
		wxString name;

	protected:
		Feature(ScriptFeatureClass _featureclass, wxString &_name);

	public:
		ScriptFeatureClass GetClass() const;
		const FeatureMacro* AsMacro() const;
		const FeatureFilter* AsFilter() const;
		const FeatureSubtitleFormat* AsSubFormat() const;

		const wxString& GetName() const;
	};

	// The Macro feature; adds a menu item that runs script code
	class FeatureMacro : public Feature {
	private:
		wxString description;
		MacroMenu menu;

	protected:
		FeatureMacro(wxString &_name, wxString &_description, MacroMenu _menu);

	public:
		const wxString& GetDescription() const;
		MacroMenu GetMenu() const;

		virtual bool Validate(/* TODO: this needs arguments */) = 0;
		virtual void Process(/* TODO: this needs arguments */) = 0;
	};

	// The Export Filter feature; adds a new export filter
	class FeatureFilter : public Feature, public AssExportFilter {
	protected:
		FeatureFilter(wxString &_name, wxString &_description, int _priority);

		// Subclasses should probably implement AssExportFilter::Init

	public:
		// Subclasses must implement the AssExportFilter virtual functions:
		//   ProcessSubs
		//   GetConfigDialogWindow
		//   LoadSettings
	};

	// The Subtitle Format feature; adds new subtitle format readers/writers
	class FeatureSubtitleFormat : public Feature, public SubtitleFormat {
	private:
		wxString extension;

	protected:
		FeatureSubtitleFormat(wxString &_name, wxString &_extension);

	public:
		const wxString& GetExtension() const;

		// Default implementations of these are provided, that just checks extension,
		// but subclasses can provide more elaborate implementations, or go back to
		// the "return false" implementation, in case of reader-only or writer-only.
		virtual bool CanWriteFile(wxString filename);
		virtual bool CanReadFile(wxString filename);

		// Subclasses should implement ReadFile and/or WriteFile here
	};

	// Base class for Scripts
	class Script {
	private:
		wxString filename;

	protected:
		wxString name;
		wxString description;
		wxString author;
		wxString version;
		bool loaded; // is the script properly loaded?

		std::vector<Feature*> features;

		Script(const wxString &_filename);

	public:
		virtual void Reload() = 0;

		const wxString& GetFilename() const;
		const wxString& GetName() const;
		const wxString& GetDescription() const;
		const wxString& GetAuthor() const;
		const wxString& GetVersion() const;
		bool GetLoadedState() const;

		const std::vector<Feature*>& GetFeatures() const;
	};

	// Manages loaded scripts; for whatever reason, multiple managers might be instantiated. In truth, this is more
	// like a macro manager at the moment, since Export Filter and Subtitle Format are already managed by other
	// classes.
	class ScriptManager {
	private:
		std::vector<Script*> scripts;

		std::vector<Feature*> macros[MACROMENU_MAX]; // array of vectors...

	public:
		ScriptManager();
		~ScriptManager();
		void Add(Script *script);
		void Remove(Script *script);

		const std::vector<Script*>& GetScripts() const;

		const std::vector<Feature*>& GetMacros(MacroMenu menu) const;
		// No need to have getters for the other kinds of features, I think.
		// They automatically register themselves in the relevant places.
	};

	// Script factory; each scripting engine should create exactly one instance of this object and register it.
	// This is used to create Script objects from a file.
	class ScriptFactory {
	private:
		static std::vector<ScriptFactory*> factories;
		ScriptFactory() { }
	protected:
		wxString engine_name;
	public:
		virtual Script* Produce(const wxString &filename) const = 0;
		const wxString& GetEngineName() const;

		static void Register(ScriptFactory *factory);
		static void Unregister(ScriptFactory *factory);
		static Script* CreateFromFile(const wxString &filename);
		static const std::vector<ScriptFactory*>& GetFactories();
	};

};

#endif
