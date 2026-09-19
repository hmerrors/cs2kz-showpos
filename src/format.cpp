#include "types.h"
#include "tier1/strtools.h"

#include <cmath>

#include "tier0/memdbgon.h"

static void AppendRow(char *out, i32 outLen, const char *label, i32 precision, const f32 *values, const bool *available, i32 count)
{
	i32 used = V_strlen(out);
	V_snprintf(out + used, outLen - used, "%s%s |", used ? "\n" : "", label);
	for (i32 i = 0; i < count; i++)
	{
		used = V_strlen(out);
		if ((!available || available[i]) && std::isfinite(values[i]))
		{
			V_snprintf(out + used, outLen - used, " %.*f", precision, values[i]);
		}
		else
		{
			V_snprintf(out + used, outLen - used, " n/a");
		}
	}
}

void showpos::Format(const ShowPosSnapshot &sample, const ShowPosPrefs &prefs, char *out, i32 outLen)
{
	if (!out || outLen <= 0)
	{
		return;
	}
	out[0] = '\0';
	if (prefs.mode != ShowPosMode::Simple && prefs.mode != ShowPosMode::Detailed)
	{
		return;
	}
	const i32 precision = prefs.mode == ShowPosMode::Detailed ? 6 : 2;
	if (prefs.origin)
	{
		AppendRow(out, outLen, "Pos", precision, sample.origin.Base(), nullptr, 3);
	}
	if (prefs.angles != ShowPosAngles::Disabled)
	{
		AppendRow(out, outLen, "Ang", precision, sample.angles.Base(), nullptr, prefs.angles == ShowPosAngles::PitchYawRoll ? 3 : 2);
	}
	if (prefs.velocity == ShowPosVelocity::Vector)
	{
		AppendRow(out, outLen, "Vel", precision, sample.velocity.Base(), nullptr, 3);
	}
	else if (prefs.velocity == ShowPosVelocity::Horizontal || prefs.velocity == ShowPosVelocity::Absolute)
	{
		const f32 speed = prefs.velocity == ShowPosVelocity::Horizontal ? sample.velocity.Length2D() : sample.velocity.Length();
		AppendRow(out, outLen, "Vel", precision, &speed, nullptr, 1);
	}
	if (prefs.stamina)
	{
		AppendRow(out, outLen, "Sta", precision, &sample.stamina, &sample.hasStamina, 1);
	}
	if (prefs.duck)
	{
		const f32 values[] = {sample.duckAmount, sample.duckSpeed};
		const bool available[] = {sample.hasDuckAmount, sample.hasDuckSpeed};
		AppendRow(out, outLen, "Dck", precision, values, available, 2);
	}
}
