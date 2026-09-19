#include "plugin.h"
#include "KeyValues.h"
#include <Windows.h>
#include <filesystem>
#include <charconv>
#include <fstream>
#include <sstream>

void prefs::Normalize(ShowPosPrefs &p)
{
	if (int(p.mode) < 0 || int(p.mode) > 2)
	{
		p.mode = ShowPosMode::Disabled;
	}
	if (int(p.angles) < 0 || int(p.angles) > 2)
	{
		p.angles = ShowPosAngles::PitchYaw;
	}
	if (int(p.velocity) < 0 || int(p.velocity) > 3)
	{
		p.velocity = ShowPosVelocity::Horizontal;
	}
	p.x = std::clamp(p.x, 0, 70);
	p.y = std::clamp(p.y, -40, 40);
	p.size = std::clamp(p.size, 12, 32);
}

static std::filesystem::path Path(uint64 steam)
{
	return std::filesystem::u8path(engine::gameDir) / "addons/showpos/data" / (std::to_string(steam) + ".txt");
}

void prefs::Load(Player &p, uint64 steam)
{
	p.steam = steam;
	// Public API returns zero until authentication is available; bots never persist.
	if (!steam)
	{
		return;
	}
	p.prefsLoaded = true;
	std::error_code ec;
	auto path = Path(steam);
	if (!std::filesystem::exists(path, ec) || std::filesystem::file_size(path, ec) > 4096 || ec)
	{
		return;
	}
	std::ifstream file(path);
	std::string key, token;
	ShowPosPrefs loaded;
	bool version = false;
	while (file >> key >> token)
	{
		int64_t value;
		auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
		if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size())
		{
			continue;
		}
		if (key == "version")
		{
			version = value == 1;
		}
		if (key == "mode" && value >= 0 && value <= 2)
		{
			loaded.mode = ShowPosMode(value);
		}
		if (key == "origin" && value >= 0 && value <= 1)
		{
			loaded.origin = value != 0;
		}
		if (key == "angles" && value >= 0 && value <= 2)
		{
			loaded.angles = ShowPosAngles(value);
		}
		if (key == "velocity" && value >= 0 && value <= 3)
		{
			loaded.velocity = ShowPosVelocity(value);
		}
		if (key == "stamina" && value >= 0 && value <= 1)
		{
			loaded.stamina = value != 0;
		}
		if (key == "duck" && value >= 0 && value <= 1)
		{
			loaded.duck = value != 0;
		}
		if (key == "x")
		{
			loaded.x = static_cast<int>(std::clamp<int64_t>(value, 0, 70));
		}
		if (key == "y")
		{
			loaded.y = static_cast<int>(std::clamp<int64_t>(value, -40, 40));
		}
		if (key == "size")
		{
			loaded.size = static_cast<int>(std::clamp<int64_t>(value, 12, 32));
		}
	}
	// An unauthenticated user's in-session edits take precedence over disk when auth arrives.
	if (version && !p.dirty)
	{
		p.prefs = loaded;
	}
	Normalize(p.prefs);
}

bool prefs::Save(Player &p)
{
	if (!p.dirty || !p.steam || !p.prefsLoaded)
	{
		return true;
	}
	auto path = Path(p.steam), tmp = path;
	tmp += ".tmp";
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	if (ec)
	{
		return false;
	}
	std::ofstream file(tmp, std::ios::trunc);
	auto &v = p.prefs;
	file << "version 1\nmode " << int(v.mode) << "\norigin " << v.origin << "\nangles " << int(v.angles) << "\nvelocity " << int(v.velocity)
		 << "\nstamina " << v.stamina << "\nduck " << v.duck << "\nx " << v.x << "\ny " << v.y << "\nsize " << v.size << '\n';
	file.flush();
	bool ok = bool(file);
	file.close();
	if (!ok || !MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		return false;
	}
	p.dirty = false;
	return true;
}

void prefs::Cycle(ShowPosPrefs &p, int row)
{
	switch (row)
	{
		case 0:
			p.mode = ShowPosMode((int(p.mode) + 1) % 3);
			break;
		case 1:
			p.origin = !p.origin;
			break;
		case 2:
			p.angles = ShowPosAngles((int(p.angles) + 1) % 3);
			break;
		case 3:
			p.velocity = ShowPosVelocity((int(p.velocity) + 1) % 4);
			break;
		case 4:
			p.stamina = !p.stamina;
			break;
		case 5:
			p.duck = !p.duck;
			break;
		case 6:
			p.x = p.x >= 70 ? 0 : p.x + 2;
			break;
		case 7:
			p.y = p.y >= 40 ? -40 : p.y + 2;
			break;
		case 8:
			p.size = p.size >= 32 ? 12 : p.size + 2;
			break;
	}
	Normalize(p);
}
