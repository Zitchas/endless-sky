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
	// The first item in pair is plugin state on load, second item is user changing preference.
	map<string, pair<bool, bool>> settings;

	void LoadSettings(const string &path)
	{
		DataFile prefs(path);
		for(const DataNode &node : prefs)
		{
			const string &key = node.Token(0);
			if(key == "plugins")
				for(const DataNode &child : node)
					if(child.Size() == 2)
						settings[child.Token(0)].first = child.Value(1);
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

	out.Write("plugins");
	out.BeginChild();
	{
		for(const auto &it : settings)
			out.Write(it.first, it.second.second);
	}
	out.EndChild();
}



// Freeze the plugin set to determine if a setting has changed.
void Plugins::Freeze()
{
	for(auto &it : settings)
		it.second.second = it.second.first;
}



// Check if a plugin is known. It does not matter if it is enabled or disabled.
bool Plugins::Has(const string &name)
{
	auto it = settings.find(name);
	return it != settings.end();
}



// Determine if a plugin setting has changed since launching.
bool Plugins::HasChanged(const string &name)
{
	const auto it = settings.find(name);
	return (it == settings.end()) || it->second.first != it->second.second;
}



// Returns true if any plugin enabled or disabled setting has changed since
// launched via user preferences.
bool Plugins::HasChanged()
{
	for(const auto &it : settings)
		if(it.second.first != it.second.second)
			return true;
	return false;
}



// Plugins are enabled by default and disabled if the user prefers it to be
// disabled.
bool Plugins::IsEnabled(const string &name)
{
	auto it = settings.find(name);
	return (it == settings.end()) || it->second.second;
}



void Plugins::Set(const string &name, bool on)
{
	settings[name].second = on;
}



// Toggles enabling or disabling a plugin for the next game restart.
void Plugins::TogglePlugin(const string &name)
{
	settings[name].second = !IsEnabled(name);
}
