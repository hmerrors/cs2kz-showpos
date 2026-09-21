#include "plugin.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace engine
{
	std::string gameDir;
}

static int checks;

static void Check(bool condition, const char *message)
{
	++checks;
	if (!condition)
	{
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

int main()
{
#ifdef _WIN32
	auto pid = GetCurrentProcessId();
#else
	auto pid = getpid();
#endif
	auto root = std::filesystem::temp_directory_path() / ("showpos-prefs-test-" + std::to_string(pid));
	Check(!std::filesystem::exists(root), "isolated test directory");
	engine::gameDir = root.u8string();
	const uint64 steam = 76561198000000001;
	auto path = root / "addons/showpos/data" / (std::to_string(steam) + ".txt");
	Player p;
	p.dirty = true;
	prefs::Load(p, 0);
	Check(!p.prefsLoaded, "zero identity stays session-only");
	Check(prefs::Save(p) && !std::filesystem::exists(root), "no unauthenticated write");
	prefs::Load(p, steam);
	p.prefs.mode = ShowPosMode::Detailed;
	p.prefs.x = 6;
	p.prefs.y = -20;
	Check(prefs::Save(p) && !p.dirty && std::filesystem::exists(path), "atomic preference save");
	Player loaded;
	prefs::Load(loaded, steam);
	Check(loaded.prefs.mode == ShowPosMode::Detailed && loaded.prefs.x == 6 && loaded.prefs.y == -20, "round trip");
	p.dirty = true;
	p.prefs.mode = ShowPosMode::Simple;
	Check(prefs::Save(p) && !p.dirty, "replace existing preference file");
	Player replaced;
	prefs::Load(replaced, steam);
	Check(replaced.prefs.mode == ShowPosMode::Simple, "replacement contains new settings");
	std::filesystem::remove(path);
	std::filesystem::create_directory(path);
	p.dirty = true;
	Check(!prefs::Save(p) && p.dirty && std::filesystem::is_directory(path), "failed commit retains dirty state and destination");
	std::filesystem::remove(path);
	Check(prefs::Save(p) && !p.dirty, "retry after failed commit");
	auto write = [&](const char *data) { std::ofstream(path, std::ios::trunc) << data; };
	write("version 1\nmode 999\norigin -1\nangles 999\nvelocity -1\nstamina 2\nduck -1\nx 9223372036854775807\ny -9223372036854775808\nsize "
		  "999999999999999999999999999999\n");
	Player corrupt;
	prefs::Load(corrupt, steam);
	Check(corrupt.prefs.mode == ShowPosMode::Disabled && corrupt.prefs.origin && corrupt.prefs.stamina && corrupt.prefs.duck,
		  "invalid enums and booleans default");
	Check(corrupt.prefs.x == 70 && corrupt.prefs.y == -40 && corrupt.prefs.size == 18, "overflow-safe parsing before narrowing");
	write("version 2\nmode 2\n");
	Player future;
	prefs::Load(future, steam);
	Check(future.prefs.mode == ShowPosMode::Disabled, "unknown version ignored");
	write("version 1\nmode 2\nx 8px\ny -8\n");
	Player strict;
	prefs::Load(strict, steam);
	Check(strict.prefs.mode == ShowPosMode::Detailed && strict.prefs.x == 2 && strict.prefs.y == -8, "whole-token numeric parsing");
	Player edited;
	edited.dirty = true;
	edited.prefs.mode = ShowPosMode::Simple;
	prefs::Load(edited, steam);
	Check(edited.prefs.mode == ShowPosMode::Simple, "session edits survive authentication");
	std::ofstream(path, std::ios::trunc) << "version 1\nmode 2\n" << std::string(4096, ' ');
	Player huge;
	prefs::Load(huge, steam);
	Check(huge.prefs.mode == ShowPosMode::Disabled, "oversized preference ignored");
	for (int row = 0; row < 9; row++)
	{
		ShowPosPrefs v;
		for (int i = 0; i < 200; i++)
		{
			prefs::Cycle(v, row);
			Check(int(v.mode) >= 0 && int(v.mode) <= 2 && int(v.angles) >= 0 && int(v.angles) <= 2 && int(v.velocity) >= 0 && int(v.velocity) <= 3
					  && v.x >= 0 && v.x <= 70 && v.y >= -40 && v.y <= 40 && v.size >= 12 && v.size <= 32,
				  "cycles remain bounded");
		}
	}
	// Remove only the exact per-process temporary directory created above.
	std::filesystem::remove_all(root);
	std::printf("PASS: %d preference checks; atomic save, corruption, overflow, version, session/auth merge.\n", checks);
}
