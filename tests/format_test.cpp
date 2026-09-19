#include "types.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

static int checks;

static void Check(bool condition, const char *message)
{
	checks++;
	if (!condition)
	{
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

int main()
{
	ShowPosSnapshot sample;
	sample.origin = Vector(1024.03125f, -512.0f, 128.03125f);
	sample.angles = QAngle(-12.3046875f, 93.427734375f, 1.5f);
	sample.velocity = Vector(3.0f, 4.0f, 12.0f);
	sample.stamina = 7.5f;
	sample.duckAmount = 0.4375f;
	sample.duckSpeed = 6.0f;
	sample.hasStamina = sample.hasDuckAmount = sample.hasDuckSpeed = true;
	const ShowPosSnapshot original = sample;
	ShowPosPrefs prefs;
	char out[1024];
	prefs.mode = ShowPosMode::Simple;
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(std::strcmp(out, "Pos | 1024.03 -512.00 128.03\nAng | -12.30 93.43\nVel | 5.00\nSta | 7.50\nDck | 0.44 6.00") == 0,
		  "Simple: exact rows, rounding, P/Y order and horizontal speed");
	prefs.mode = ShowPosMode::Detailed;
	prefs.angles = ShowPosAngles::PitchYawRoll;
	prefs.velocity = ShowPosVelocity::Absolute;
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(
		std::strcmp(
			out,
			"Pos | 1024.031250 -512.000000 128.031250\nAng | -12.304688 93.427734 1.500000\nVel | 13.000000\nSta | 7.500000\nDck | 0.437500 6.000000")
			== 0,
		"Detailed: exact six decimals, P/Y/R order and absolute speed");
	prefs.velocity = ShowPosVelocity::Vector;
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(std::strstr(out, "Vel | 3.000000 4.000000 12.000000") != nullptr, "Velocity XYZ ordering and Z retained");

	int combinations = 0;
	for (int mode = 0; mode < 3; mode++)
	{
		for (int origin = 0; origin < 2; origin++)
		{
			for (int angles = 0; angles < 3; angles++)
			{
				for (int velocity = 0; velocity < 4; velocity++)
				{
					for (int stamina = 0; stamina < 2; stamina++)
					{
						for (int duck = 0; duck < 2; duck++)
						{
							prefs.mode = static_cast<ShowPosMode>(mode);
							prefs.origin = origin != 0;
							prefs.angles = static_cast<ShowPosAngles>(angles);
							prefs.velocity = static_cast<ShowPosVelocity>(velocity);
							prefs.stamina = stamina != 0;
							prefs.duck = duck != 0;
							showpos::Format(sample, prefs, out, sizeof(out));
							const int rows = mode ? origin + (angles != 0) + (velocity != 0) + stamina + duck : 0;
							int actualRows = out[0] ? 1 : 0;
							for (const char *p = out; *p; p++)
							{
								actualRows += *p == '\n';
							}
							Check(actualRows == rows, "All 288 preference combinations: exact row count");
							Check(!std::strstr(out, "\n\n") && (!out[0] || out[std::strlen(out) - 1] != '\n'), "No empty or trailing rows");
							combinations++;
							// Guard both ends of every possible output capacity, including zero and one.
							for (int capacity = 0; capacity <= 1024; capacity++)
							{
								unsigned char guarded[1026];
								std::memset(guarded, 0xA5, sizeof(guarded));
								char *buffer = reinterpret_cast<char *>(guarded + 1);
								showpos::Format(sample, prefs, buffer, capacity);
								Check(guarded[0] == 0xA5 && guarded[capacity + 1] == 0xA5, "Bounded output preserves canaries");
								Check(capacity == 0 || std::memchr(buffer, 0, capacity), "Bounded output is terminated");
							}
						}
					}
				}
			}
		}
	}
	Check(std::memcmp(&sample, &original, sizeof(sample)) == 0, "Formatting never mutates snapshot");
	prefs = ShowPosPrefs();
	prefs.mode = ShowPosMode::Detailed;
	sample.hasStamina = sample.hasDuckAmount = false;
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(std::strstr(out, "Sta | n/a\nDck | n/a 6.000000") != nullptr, "Missing schema values stay unavailable; duck speed remains");
	sample.origin.x = std::numeric_limits<float>::quiet_NaN();
	sample.origin.y = std::numeric_limits<float>::infinity();
	sample.origin.z = -std::numeric_limits<float>::infinity();
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(std::strstr(out, "Pos | n/a n/a n/a") != nullptr, "Non-finite values are explicit");
	sample = original;
	sample.velocity.z = -12.0f;
	prefs.velocity = ShowPosVelocity::Absolute;
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(std::strstr(out, "Vel | 13.000000") != nullptr, "Falling speed magnitude includes negative Z");
	sample.origin.x = (std::numeric_limits<float>::max)();
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(std::strlen(out) < sizeof(out) - 1, "Extreme finite coordinate fits normal HUD buffer");
	prefs.mode = static_cast<ShowPosMode>(999);
	showpos::Format(sample, prefs, out, sizeof(out));
	Check(out[0] == 0, "Invalid display mode is hidden");
	showpos::Format(sample, prefs, nullptr, 1024);
	showpos::Format(sample, prefs, out, -1);
	std::printf("PASS: %d checks; %d preference combinations; capacities 0..1024; actual HL2SDK formatter.\n", checks, combinations);
	return 0;
}
