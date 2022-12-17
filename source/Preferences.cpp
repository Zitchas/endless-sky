/* Preferences.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Preferences.h"

#include "Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "GameWindow.h"
#include "Logger.h"
#include "Screen.h"

#include <algorithm>
#include <map>

using namespace std;

namespace {
	map<string, bool> settings;
	int scrollSpeed = 60;

	// Strings for ammo expenditure:
	const string EXPEND_AMMO = "Escorts expend ammo";
	const string FRUGAL_ESCORTS = "Escorts use ammo frugally";

	const vector<double> ZOOMS = {.25, .35, .50, .70, 1.00, 1.40, 2.00};
	int zoomIndex = 4;
	constexpr double VOLUME_SCALE = .25;

	// Default to fullscreen.
	int screenModeIndex = 1;
	const vector<string> SCREEN_MODE_SETTINGS = {"windowed", "fullscreen"};

	// Enable standard VSync by default.
	const vector<string> VSYNC_SETTINGS = {"off", "on", "adaptive"};
	int vsyncIndex = 1;

	const vector<string> BOARDING_SETTINGS = {"proximity", "value", "mixed"};
	int boardingIndex = 0;

	const vector<string> ALERT_INDICATOR_SETTING = {"off", "audio", "visual", "both"};
	int alertIndicatorIndex = 3;
}



void Preferences::Load()
{
	// These settings should be on by default. There is no need to specify
	// values for settings that are off by default.
	settings["Automatic aiming"] = true;
	settings["Render motion blur"] = true;
	settings[FRUGAL_ESCORTS] = true;
	settings[EXPEND_AMMO] = true;
	settings["Damaged fighters retreat"] = true;
	settings["Show escort systems on map"] = true;
	settings["Show stored outfits on map"] = true;
	settings["Show mini-map"] = true;
	settings["Show planet labels"] = true;
	settings["Show hyperspace flash"] = true;
	settings["Draw background haze"] = true;
	settings["Draw starfield"] = true;
	settings["Parallax background"] = true;
	settings["Hide unexplored map regions"] = true;
	settings["Turrets focus fire"] = true;
	settings["Ship outlines in shops"] = true;

	DataFile prefs(Files::Config() + "preferences.txt");
	for(const DataNode &node : prefs)
	{
		if(node.Token(0) == "window size" && node.Size() >= 3)
			Screen::SetRaw(node.Value(1), node.Value(2));
		else if(node.Token(0) == "zoom" && node.Size() >= 2)
			Screen::SetZoom(node.Value(1));
		else if(node.Token(0) == "volume" && node.Size() >= 2)
			Audio::SetVolume(node.Value(1) * VOLUME_SCALE);
		else if(node.Token(0) == "scroll speed" && node.Size() >= 2)
			scrollSpeed = node.Value(1);
		else if(node.Token(0) == "boarding target")
			boardingIndex = max<int>(0, min<int>(node.Value(1), BOARDING_SETTINGS.size() - 1));
		else if(node.Token(0) == "view zoom")
			zoomIndex = max<int>(0, min<int>(node.Value(1), ZOOMS.size() - 1));
		else if(node.Token(0) == "vsync")
			vsyncIndex = max<int>(0, min<int>(node.Value(1), VSYNC_SETTINGS.size() - 1));
		else if(node.Token(0) == "fullscreen")
			screenModeIndex = max<int>(0, min<int>(node.Value(1), SCREEN_MODE_SETTINGS.size() - 1));
		else if(node.Token(0) == "alert indicator")
			alertIndicatorIndex = max<int>(0, min<int>(node.Value(1), ALERT_INDICATOR_SETTING.size()));
		else
			settings[node.Token(0)] = (node.Size() == 1 || node.Value(1));
	}

	// For people updating from a version before the visual red alert indicator,
	// if they have already disabled the warning siren, don't turn the audible alert back on.
	auto it = settings.find("Warning siren");
	if(it != settings.end())
	{
		if(!it->second)
			alertIndicatorIndex = 2;
		settings.erase(it);
	}
}



void Preferences::Save()
{
	DataWriter out(Files::Config() + "preferences.txt");

	out.Write("volume", Audio::Volume() / VOLUME_SCALE);
	out.Write("window size", Screen::RawWidth(), Screen::RawHeight());
	out.Write("zoom", Screen::UserZoom());
	out.Write("scroll speed", scrollSpeed);
	out.Write("boarding target", boardingIndex);
	out.Write("view zoom", zoomIndex);
	out.Write("vsync", vsyncIndex);
	out.Write("alert indicator", alertIndicatorIndex);

	for(const auto &it : settings)
		out.Write(it.first, it.second);
}



bool Preferences::Has(const string &name)
{
	auto it = settings.find(name);
	return (it != settings.end() && it->second);
}



void Preferences::Set(const string &name, bool on)
{
	settings[name] = on;
}



void Preferences::ToggleAmmoUsage()
{
	bool expend = Has(EXPEND_AMMO);
	bool frugal = Has(FRUGAL_ESCORTS);
	Preferences::Set(EXPEND_AMMO, !(expend && !frugal));
	Preferences::Set(FRUGAL_ESCORTS, !expend);
}



string Preferences::AmmoUsage()
{
	return Has(EXPEND_AMMO) ? Has(FRUGAL_ESCORTS) ? "frugally" : "always" : "never";
}



// Scroll speed preference.
int Preferences::ScrollSpeed()
{
	return scrollSpeed;
}



void Preferences::SetScrollSpeed(int speed)
{
	scrollSpeed = speed;
}



// View zoom.
double Preferences::ViewZoom()
{
	return ZOOMS[zoomIndex];
}



bool Preferences::ZoomViewIn()
{
	if(zoomIndex == static_cast<int>(ZOOMS.size() - 1))
		return false;

	++zoomIndex;
	return true;
}



bool Preferences::ZoomViewOut()
{
	if(zoomIndex == 0)
		return false;

	--zoomIndex;
	return true;
}



double Preferences::MinViewZoom()
{
	return ZOOMS[0];
}



double Preferences::MaxViewZoom()
{
	return ZOOMS[ZOOMS.size() - 1];
}



void Preferences::ToggleScreenMode()
{
	GameWindow::ToggleFullscreen();
	screenModeIndex = GameWindow::IsFullscreen();
}



const string &Preferences::ScreenModeSetting()
{
	return SCREEN_MODE_SETTINGS[screenModeIndex];
}



bool Preferences::ToggleVSync()
{
	int targetIndex = vsyncIndex + 1;
	if(targetIndex == static_cast<int>(VSYNC_SETTINGS.size()))
		targetIndex = 0;
	if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
	{
		// Not all drivers support adaptive VSync. Increment desired VSync again.
		++targetIndex;
		if(targetIndex == static_cast<int>(VSYNC_SETTINGS.size()))
			targetIndex = 0;
		if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
		{
			// Restore original saved setting.
			Logger::LogError("Unable to change VSync state");
			GameWindow::SetVSync(static_cast<VSync>(vsyncIndex));
			return false;
		}
	}
	vsyncIndex = targetIndex;
	return true;
}



// Return the current VSync setting
Preferences::VSync Preferences::VSyncState()
{
	return static_cast<VSync>(vsyncIndex);
}



const string &Preferences::VSyncSetting()
{
	return VSYNC_SETTINGS[vsyncIndex];
}



void Preferences::ToggleBoarding()
{
	int targetIndex = boardingIndex + 1;
	if(targetIndex == static_cast<int>(BOARDING_SETTINGS.size()))
		targetIndex = 0;
	boardingIndex = targetIndex;
}



Preferences::BoardingPriority Preferences::GetBoardingPriority()
{
	return static_cast<BoardingPriority>(boardingIndex);
}



const string &Preferences::BoardingSetting()
{
	return BOARDING_SETTINGS[boardingIndex];
}



void Preferences::ToggleAlert()
{
	if(++alertIndicatorIndex >= static_cast<int>(ALERT_INDICATOR_SETTING.size()))
		alertIndicatorIndex = 0;
}



Preferences::AlertIndicator Preferences::GetAlertIndicator()
{
	return static_cast<AlertIndicator>(alertIndicatorIndex);
}



const std::string &Preferences::AlertSetting()
{
	return ALERT_INDICATOR_SETTING[alertIndicatorIndex];
}



bool Preferences::PlayAudioAlert()
{
	return DoAlertHelper(AlertIndicator::AUDIO);
}



bool Preferences::DisplayVisualAlert()
{
	return DoAlertHelper(AlertIndicator::VISUAL);
}



bool Preferences::DoAlertHelper(Preferences::AlertIndicator toDo)
{
	auto value = GetAlertIndicator();
	if(value == AlertIndicator::BOTH)
		return true;
	else if(value == toDo)
		return true;
	return false;
}
