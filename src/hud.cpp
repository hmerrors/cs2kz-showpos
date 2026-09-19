#include "plugin.h"
#include "entitykeyvalues.h"
#include <cstring>

// Reuse the client assets already supplied by CS2KZ + MultiAddonManager. Each instance
// is created and owned by ShowPos; never adopt or write a CS2KZ-owned layout entity.
static constexpr const char *HudPath = "panorama/layout/custom_game/cs2kz/mhud.vxml_c";
static constexpr const char *MenuPath = "panorama/layout/custom_game/cs2kz/menu.xml";

static void Changed(CEntityInstance *ent, void *state)
{
	if (!state)
	{
		return;
	}
	NetworkStateChangedData data(true);
	using Fn = void (*)(void *, const NetworkStateChangedData *);
	reinterpret_cast<Fn>((*reinterpret_cast<void ***>(state))[engine::stateNetworkIndex])(state, &data);
}

static int Intern(CEntityInstance *ent, const Field &field, const char *s)
{
	int count = field.Count(ent);
	for (int i = 0; i < count; i++)
	{
		auto p = static_cast<CUtlString *>(field.Element(ent, i));
		if (p && !strcmp(p->Get(), s))
		{
			return i;
		}
	}
	auto p = static_cast<CUtlString *>(field.Append(ent));
	if (!p)
	{
		return -1;
	}
	*p = s;
	ent->NetworkStateChanged(NetworkStateChangedData(true));
	return count;
}

static void SetClass(CEntityInstance *ent, const char *panel, const char *name, bool on)
{
	if (!ent)
	{
		return;
	}
	int pi = Intern(ent, engine::panels, panel), ci = Intern(ent, engine::classes, name);
	void *state = engine::globalState.At<char>(ent);
	if (pi < 0 || ci < 0 || !state)
	{
		return;
	}
	int count = engine::hasClasses.Count(state);
	void *entry = nullptr;
	for (int i = 0; i < count; i++)
	{
		void *p = engine::hasClasses.Element(state, i);
		if (engine::classPanel.Read<uint16>(p) == pi && engine::className.Read<uint16>(p) == ci)
		{
			entry = p;
			break;
		}
	}
	if (entry && engine::classStatus.Read<uint32>(entry) == uint32(on))
	{
		return;
	}
	if (!entry)
	{
		entry = engine::hasClasses.Append(state);
	}
	if (!entry)
	{
		return;
	}
	*engine::classPanel.At<uint16>(entry) = uint16(pi);
	*engine::className.At<uint16>(entry) = uint16(ci);
	*engine::classStatus.At<uint32>(entry) = uint32(on);
	Changed(ent, state);
}

static void Var(CEntityInstance *ent, const char *panel, const char *name, const char *value)
{
	if (!ent)
	{
		return;
	}
	int pi = Intern(ent, engine::panels, panel), vi = Intern(ent, engine::variables, name);
	void *state = engine::globalState.At<char>(ent);
	if (pi < 0 || vi < 0 || !state)
	{
		return;
	}
	void *entry = nullptr;
	int count = engine::dialogStrings.Count(state);
	for (int i = 0; i < count; i++)
	{
		auto p = engine::dialogStrings.Element(state, i);
		if (engine::dialogPanel.Read<uint16>(p) == pi && engine::dialogName.Read<uint16>(p) == vi)
		{
			entry = p;
			break;
		}
	}
	if (entry && engine::dialogSet.Read<bool>(entry) && !strcmp(engine::dialogValue.At<CUtlString>(entry)->Get(), value))
	{
		return;
	}
	if (!entry)
	{
		entry = engine::dialogStrings.Append(state);
	}
	if (!entry)
	{
		return;
	}
	*engine::dialogPanel.At<uint16>(entry) = uint16(pi);
	*engine::dialogName.At<uint16>(entry) = uint16(vi);
	*engine::dialogValue.At<CUtlString>(entry) = value;
	*engine::dialogSet.At<bool>(entry) = true;
	Changed(ent, state);
}

static void CaptureInput(CEntityInstance *ent, int slot, bool on)
{
	if (!ent)
	{
		return;
	}
	auto state = engine::playerStates.Element(ent, slot);
	if (!state)
	{
		return;
	}
	*engine::stateSlot.At<CPlayerSlot>(state) = CPlayerSlot(slot);
	*engine::capture.At<bool>(state) = on;
	Changed(ent, state);
}

static CEntityInstance *Create(int slot, bool menu)
{
	auto &p = plugin.players[slot];
	auto &handle = menu ? p.menu : p.hud;
	if (auto ent = handle.Get())
	{
		return ent;
	}
	if (!plugin.Ready() || !plugin.Assets())
	{
		return nullptr;
	}
	auto ent = engine::createEntity("custom_hud_layout", -1);
	if (!ent)
	{
		return nullptr;
	}
	auto kv = new CEntityKeyValues();
	kv->SetString("layout", menu ? MenuPath : HudPath);
	char name[48];
	V_snprintf(name, sizeof(name), "showpos_%s_%d", menu ? "menu" : "hud", slot);
	kv->SetString("targetname", name);
	engine::spawnEntity(ent, kv);
	handle = ent->GetRefEHandle();
	if (!menu)
	{
		p.lastText[0] = 0;
		p.hudVisible = false;
		p.lastX = p.lastY = p.lastSize = -999;
		SetClass(ent, "mhud_timer", "align-left", true);
		SetClass(ent, "mhud_timer", "outline", true);
		SetClass(ent, "mhud_timer", "font-family--stratum2-bold-monodigit", true);
		SetClass(ent, "mhud_timer", "pal-fg-9", true);
	}
	return ent;
}

static void ValueClass(CEntityInstance *ent, const char *prefix, int &cached, int value, bool percent)
{
	if (cached == value)
	{
		return;
	}
	char name[64];
	if (cached != -999)
	{
		V_snprintf(name, sizeof(name), "%s--%s%d%s", prefix, cached < 0 ? "neg" : "", abs(cached), percent ? "pct" : "px");
		SetClass(ent, "mhud_timer", name, false);
	}
	V_snprintf(name, sizeof(name), "%s--%s%d%s", prefix, value < 0 ? "neg" : "", abs(value), percent ? "pct" : "px");
	SetClass(ent, "mhud_timer", name, true);
	cached = value;
}

bool ShowPosPlugin::Assets() const
{
	return engine::files && engine::files->FileExists(HudPath, "GAME")
		   && engine::files->FileExists("panorama/layout/custom_game/cs2kz/menu.vxml_c", "GAME");
}

void hud::Draw(int slot, const ShowPosSnapshot *sample)
{
	auto &p = plugin.players[slot];
	char text[1024] {};
	if (sample)
	{
		showpos::Format(*sample, p.prefs, text, sizeof(text));
	}
	auto ent = p.hud.Get();
	bool visible = text[0] != 0;
	if (!visible && !ent)
	{
		return;
	}
	if (!ent)
	{
		ent = Create(slot, false);
	}
	if (!ent)
	{
		return;
	}
	if (p.hudVisible != visible)
	{
		SetClass(ent, "mhud_timer", "hidden", !visible);
		p.hudVisible = visible;
	}
	if (!visible)
	{
		return;
	}
	ValueClass(ent, "x", p.lastX, p.prefs.x, true);
	ValueClass(ent, "y", p.lastY, p.prefs.y, true);
	ValueClass(ent, "font-size", p.lastSize, p.prefs.size, false);
	if (strcmp(text, p.lastText))
	{
		Var(ent, "mhud_timer", "timer", text);
		V_strncpy(p.lastText, text, sizeof(p.lastText));
		plugin.renders++;
	}
}

void hud::RenderMenu(int slot)
{
	auto &p = plugin.players[slot];
	auto ent = p.menu.Get();
	if (!ent)
	{
		return;
	}
	bool zh = plugin.chinese;
	const char *labelsEN[] = {"ShowPos", "Origin", "Angles", "Velocity", "Stamina", "Duck", "Horizontal position", "Vertical position", "Text size"};
	const char *labelsZH[] = {"ShowPos", "位置", "视角", "速度", "体力", "蹲伏", "水平位置", "垂直位置", "字号"};
	const char *modes[] = {zh ? "关闭" : "Disabled", zh ? "简洁（2 位小数）" : "Simple (2)", zh ? "详细（6 位小数）" : "Detailed (6)"};
	const char *angles[] = {zh ? "关闭" : "Disabled", zh ? "俯仰 / 偏航" : "Pitch & Yaw", zh ? "俯仰 / 偏航 / 滚转" : "Pitch & Yaw & Roll"};
	const char *velocity[] = {zh ? "关闭" : "Disabled", zh ? "水平速率" : "Horizontal", zh ? "三维速率" : "Absolute", zh ? "速度向量" : "Vector"};
	const char *enabled = zh ? "开启" : "Enabled", *disabled = zh ? "关闭" : "Disabled";
	auto &v = p.prefs;
	const char *values[] = {modes[int(v.mode)],        v.origin ? enabled : disabled,  angles[int(v.angles)],
							velocity[int(v.velocity)], v.stamina ? enabled : disabled, v.duck ? enabled : disabled};
	Var(ent, "menu_title", "title", zh ? "ShowPos 设置" : "ShowPos settings");
	SetClass(ent, "menu_root", "font-family--stratum2", true);
	SetClass(ent, "menu_root", "pal-fg-9", true);
	SetClass(ent, "cat0", "hidden", false);
	SetClass(ent, "cat0", "selected", true);
	Var(ent, "cat_lbl0", "cl0", "ShowPos");
	for (int i = 0; i < 9; i++)
	{
		char id[40], var[20], number[40];
		V_snprintf(id, sizeof(id), "item%d", i);
		SetClass(ent, id, "hidden", false);
		SetClass(ent, id, "type-choice", true);
		V_snprintf(id, sizeof(id), "item_lbl%d", i);
		V_snprintf(var, sizeof(var), "il%d", i);
		Var(ent, id, var, zh ? labelsZH[i] : labelsEN[i]);
		V_snprintf(id, sizeof(id), "item_sub%d", i);
		V_snprintf(var, sizeof(var), "is%d", i);
		Var(ent, id, var, zh ? "点击切换" : "Click to cycle");
		V_snprintf(id, sizeof(id), "item_val%d", i);
		V_snprintf(var, sizeof(var), "iv%d", i);
		if (i >= 6)
		{
			V_snprintf(number, sizeof(number), "%d%s", i == 6 ? v.x : i == 7 ? v.y : v.size, i == 8 ? " px" : "%");
		}
		Var(ent, id, var, i < 6 ? values[i] : number);
	}
	SetClass(ent, "menu_root", "hidden", false);
}

bool hud::OpenMenu(int slot)
{
	auto ent = Create(slot, true);
	if (!ent)
	{
		return false;
	}
	auto &p = plugin.players[slot];
	p.menuOpen = true;
	p.menuExpires = engine::Now() + 60;
	RenderMenu(slot);
	CaptureInput(ent, slot, true);
	return true;
}

void hud::CloseMenu(int slot)
{
	auto &p = plugin.players[slot];
	CaptureInput(p.menu.Get(), slot, false);
	SetClass(p.menu.Get(), "menu_root", "hidden", true);
	p.menuOpen = false;
}

void hud::Destroy(int slot)
{
	auto &p = plugin.players[slot];
	if (GameEntitySystem())
	{
		CloseMenu(slot);
		if (auto ent = p.menu.Get())
		{
			engine::removeEntity(ent);
		}
		if (auto ent = p.hud.Get())
		{
			engine::removeEntity(ent);
		}
	}
	p.menu = CEntityHandle();
	p.hud = CEntityHandle();
	p.hudVisible = false;
	p.menuOpen = false;
}
