#pragma once
#include <cstdint>
using i32 = int32_t;
using f32 = float;
#include "mathlib/vector.h"

enum class ShowPosMode
{
	Disabled,
	Simple,
	Detailed,
};

enum class ShowPosAngles
{
	Disabled,
	PitchYaw,
	PitchYawRoll,
};

enum class ShowPosVelocity
{
	Disabled,
	Horizontal,
	Absolute,
	Vector,
};

struct ShowPosPrefs
{
	ShowPosMode mode {ShowPosMode::Disabled};
	bool origin {true};
	ShowPosAngles angles {ShowPosAngles::PitchYaw};
	ShowPosVelocity velocity {ShowPosVelocity::Horizontal};
	bool stamina {true};
	bool duck {true};
	i32 x {2};
	i32 y {-24};
	i32 size {18};

	bool HasRows() const
	{
		return this->origin || this->angles != ShowPosAngles::Disabled || this->velocity != ShowPosVelocity::Disabled || this->stamina || this->duck;
	}
};

struct ShowPosSnapshot
{
	Vector origin {0.0f, 0.0f, 0.0f};
	QAngle angles {0.0f, 0.0f, 0.0f};
	Vector velocity {0.0f, 0.0f, 0.0f};
	f32 stamina {};
	f32 duckAmount {};
	f32 duckSpeed {};
	bool hasStamina {};
	bool hasDuckAmount {};
	bool hasDuckSpeed {};
};

namespace showpos
{
	// Fixed-capacity, NUL-terminated plain text. Unavailable values are explicitly n/a.
	void Format(const ShowPosSnapshot &sample, const ShowPosPrefs &prefs, char *out, i32 outLen);
} // namespace showpos
