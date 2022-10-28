/* Plugins.cpp
Copyright (c) 2022 by Sam Gleske (samrocketman on GitHub)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Plugins.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"

#include <algorithm>
#include <map>

using namespace std;

namespace {
	// Plugin enabled state is used to compare initial settings from startup
	// with changes made by user in plugins UI.
	struct EnabledState
	{
		bool initial = true;
		bool current = true;
		EnabledState(bool startValue = true)
		{
			initial = startValue;
			current = startValue;
		}
	};

	// The first item in pair is plugin state on load, second item is user changing preference.
	map<string, EnabledState> settings;

	void LoadSettings(const string &path)
	{
		DataFile prefs(path);
		for(const DataNode &node : prefs)
		{
			// TODO: remove code migration for old plugins.txt format
			if(node.Size() == 2)
			{
				settings[node.Token(0)] = EnabledState(node.Value(1));
				continue;
			}
			// end code migration
			const string &key = node.Token(0);
			// TODO: remove code migration || key == "plugins"
			if(key == "state" || key == "plugins")
				for(const DataNode &child : node)
					if(child.Size() == 2)
						settings[child.Token(0)] = EnabledState(child.Value(1));
		}
	}
}



void Plugins::Load()
{
	// Global plugin settings
	LoadSettings(Files::Resources() + "plugins.txt");
	// Local plugin settings
	LoadSettings(Files::Config() + "plugins.txt");
}



void Plugins::Save()
{
	if(settings.empty())
		return;
	DataWriter out(Files::Config() + "plugins.txt");

	out.Write("state");
	out.BeginChild();
	{
		for(const auto &it : settings)
			out.Write(it.first, it.second.current);
	}
	out.EndChild();
}



// Check if a plugin is known. It does not matter if it is enabled or disabled.
bool Plugins::Has(const string &name)
{
	auto it = settings.find(name);
	return it != settings.end();
}



// Returns true if any plugin enabled or disabled setting has changed since
// launched via user preferences.
bool Plugins::HasChanged()
{
	for(const auto &it : settings)
		if(it.second.initial != it.second.current)
			return true;
	return false;
}



// Plugins are enabled by default and disabled if the user prefers it to be
// disabled.
bool Plugins::IsEnabled(const string &name)
{
	auto it = settings.find(name);
	return (it == settings.end()) || it->second.current;
}



// Enable or disable a plugin from loading next time the game is restarted.
void Plugins::SetPlugin(const string &name, bool on)
{
	settings[name].current = on;
}



// Enable a plugin for loading next time the game is restarted.
void Plugins::SetPlugin(const string &name)
{
	settings[name] = EnabledState();
}



// Toggles enabling or disabling a plugin for the next game restart.
void Plugins::TogglePlugin(const string &name)
{
	settings[name].current = !IsEnabled(name);
}



// Get the state of the plugin from when the game was first loaded.
bool Plugins::InitialPluginState(const string &name)
{
	const auto it = settings.find(name);
	return (it == settings.end()) || it->second.initial;
}
