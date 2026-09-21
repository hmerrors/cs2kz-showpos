#include "engine.h"
#include "KeyValues.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include "module_linux.h"
#endif
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace engine
{
	IVEngineServer2 *server;
	ISource2Server *game;
	ISource2GameClients *clients;
	ISource2GameEntities *entities;
	IFileSystem *files;
	CSchemaSystem *schemas;
	void *resource;
	void *movementAddress;
	int transmitSlot = -1, entitySystemOffset = -1, teleportIndex = -1, stateNetworkIndex = -1;
	std::string gameDir;
	CEntityInstance *(*createEntity)(const char *, int);
	void (*spawnEntity)(CEntityInstance *, CEntityKeyValues *);
	void (*removeEntity)(CEntityInstance *);
	Field life, team, pawn, currentPawn, controller, moveServices, observers, observerMode, observerTarget, stamina, duckAmount, duckSpeed, hltv;
	Field panels, classes, variables, globalState, playerStates, hasClasses, dialogStrings, capture, stateSlot;
	Field classPanel, className, classStatus, dialogPanel, dialogName, dialogValue, dialogSet;
} // namespace engine

CGameEntitySystem *GameEntitySystem()
{
	if (!engine::resource || engine::entitySystemOffset < 0)
	{
		return nullptr;
	}
	return *reinterpret_cast<CGameEntitySystem **>(static_cast<char *>(engine::resource) + engine::entitySystemOffset);
}

int Field::Count(void *p) const
{
	auto collection = At<void *>(p);
	return collection && manip ? static_cast<int>(reinterpret_cast<intptr_t>(manip(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_COUNT, collection, 0, 0)))
							   : 0;
}

void *Field::Element(void *p, int index) const
{
	return index >= 0 && index < Count(p) ? manip(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_ELEMENT, At<void *>(p), index, 0) : nullptr;
}

void *Field::Append(void *p) const
{
	int count = Count(p);
	if (!p || !manip || count < 0 || count >= 1024)
	{
		return nullptr;
	}
	manip(SCHEMA_COLLECTION_MANIPULATOR_ACTION_SET_COUNT, At<void *>(p), count + 1, 0);
	return Element(p, count);
}

static Field FindField(const char *name, const char *member, bool collection = false)
{
	auto scope = engine::schemas->FindTypeScopeForModule(SHOWPOS_SERVER_MODULE);
	auto cls = scope ? scope->FindDeclaredClass(name).Get() : nullptr;
	if (!cls)
	{
		return {};
	}
	for (int i = 0; i < cls->m_nFieldCount; i++)
	{
		auto &f = cls->m_pFields[i];
		if (strcmp(f.m_pszName, member))
		{
			continue;
		}
		Field out;
		out.offset = f.m_nSingleInheritanceOffset;
		if (collection)
		{
			if (!f.m_pType || f.m_pType->m_eTypeCategory != SCHEMA_TYPE_ATOMIC || f.m_pType->m_eAtomicCategory != SCHEMA_ATOMIC_COLLECTION_OF_T)
			{
				return {};
			}
			out.manip = static_cast<CSchemaType_Atomic_CollectionOfT *>(f.m_pType)->m_pfnManipulator;
			if (!out.manip)
			{
				return {};
			}
		}
		return out;
	}
	return {};
}

bool engine::SchemaReady()
{
	bool ok = true;
	auto get = [&](Field &f, const char *cls, const char *name, bool collection = false, bool required = true)
	{
		f = FindField(cls, name, collection);
		if (required && f.offset < 0)
		{
			Warning("[ShowPos] Missing schema %s::%s\n", cls, name);
			ok = false;
		}
	};
	get(life, "CBaseEntity", "m_lifeState");
	get(team, "CBaseEntity", "m_iTeamNum");
	get(pawn, "CCSPlayerController", "m_hPlayerPawn");
	get(currentPawn, "CCSPlayerController", "m_hObserverPawn");
	get(hltv, "CBasePlayerController", "m_bIsHLTV");
	get(controller, "CBasePlayerPawn", "m_hController");
	get(moveServices, "CBasePlayerPawn", "m_pMovementServices");
	get(observers, "CBasePlayerPawn", "m_pObserverServices");
	get(observerMode, "CPlayer_ObserverServices", "m_iObserverMode");
	get(observerTarget, "CPlayer_ObserverServices", "m_hObserverTarget");
	get(stamina, "CCSPlayer_MovementServices", "m_flStamina", false, false);
	get(duckAmount, "CCSPlayer_MovementServices", "m_flDuckAmount", false, false);
	get(duckSpeed, "CCSPlayer_MovementServices", "m_flDuckSpeed", false, false);
	get(panels, "CCSCustomHudLayout", "m_vecPanelIds", true);
	get(classes, "CCSCustomHudLayout", "m_vecClassNames", true);
	get(variables, "CCSCustomHudLayout", "m_vecDialogVariableNames", true);
	get(globalState, "CCSCustomHudLayout", "m_globalLayoutState");
	get(playerStates, "CCSCustomHudLayout", "m_vecPlayerLayoutStates", true);
	get(hasClasses, "CCSCustomHudLayoutState", "m_vecHasClasses", true);
	get(dialogStrings, "CCSCustomHudLayoutState", "m_vecDialogVariableStrings", true);
	get(capture, "CCSCustomHudLayoutState", "m_bInputCaptureEnabled");
	get(stateSlot, "CCSCustomHudLayoutState", "m_playerSlot");
	get(classPanel, "HUDPanelHasClass_t", "m_nPanelIdIndex");
	get(className, "HUDPanelHasClass_t", "m_nClassNameIndex");
	get(classStatus, "HUDPanelHasClass_t", "m_eClassStatus");
	get(dialogPanel, "HUDPanelDialogVariableString_t", "m_nPanelIdIndex");
	get(dialogName, "HUDPanelDialogVariableString_t", "m_nDialogVariableIndex");
	get(dialogValue, "HUDPanelDialogVariableString_t", "m_sValue");
	get(dialogSet, "HUDPanelDialogVariableString_t", "m_bIsSet");
	return ok;
}

// Resolve the signature in the exact loaded module's on-disk image. KHook can already have
// replaced its in-memory prologue. Verify the entry through KHook's original trampoline.
// No patching, guessed function addresses, or private CS2KZ object layouts are used.
void *engine::Resolve(const char *signature, char *error, size_t maxlen)
{
	if (!signature || !signature[0])
	{
		V_snprintf(error, maxlen, "Missing signature in ShowPos gamedata");
		return nullptr;
	}
#ifdef _WIN32
	auto modulePath = std::filesystem::u8path(gameDir) / "bin/win64/server.dll";
	HMODULE module = GetModuleHandleW(modulePath.c_str());
	wchar_t filename[32768];
	if (!module || !GetModuleFileNameW(module, filename, 32768))
	{
		return nullptr;
	}
	std::ifstream file(std::filesystem::path(filename), std::ios::binary | std::ios::ate);
	if (!file)
	{
		return nullptr;
	}
	auto size = file.tellg();
	if (size < static_cast<std::streamoff>(sizeof(IMAGE_DOS_HEADER)) || size > 512 * 1024 * 1024)
	{
		return nullptr;
	}
	std::vector<char> disk(static_cast<size_t>(size));
	file.seekg(0);
	file.read(disk.data(), size);
	auto dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(disk.data());
	if (!file || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || size_t(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) > disk.size())
	{
		return nullptr;
	}
	auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(disk.data() + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		return nullptr;
	}
	auto sections = IMAGE_FIRST_SECTION(nt);
	if (reinterpret_cast<const char *>(sections + nt->FileHeader.NumberOfSections) > disk.data() + disk.size())
	{
		return nullptr;
	}
	void *result = nullptr;
	int matches = 0;
	for (int i = 0; i < nt->FileHeader.NumberOfSections; i++)
	{
		auto &section = sections[i];
		if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) || size_t(section.PointerToRawData) + section.SizeOfRawData > disk.size())
		{
			continue;
		}
		char *begin = disk.data() + section.PointerToRawData;
		char *end = begin + section.SizeOfRawData;
		for (char *cursor = begin; cursor < end;)
		{
			auto hit = static_cast<char *>(KHook::LookupSignature(cursor, end - cursor, signature));
			if (!hit)
			{
				break;
			}
			size_t rva = section.VirtualAddress + (hit - begin);
			if (rva < nt->OptionalHeader.SizeOfImage)
			{
				result = reinterpret_cast<char *>(module) + rva;
				matches++;
			}
			cursor = hit + 1;
		}
	}
	if (matches != 1)
	{
		V_snprintf(error, maxlen, "Signature is not unique: %s (%d matches)", signature, matches);
		return nullptr;
	}
	// This pattern contains no relative instructions in its first eleven bytes.
	if (!strcmp(signature, "40 57 41 57 48 81 EC B8 00 00 00"))
	{
		void *original = KHook::FindOriginal(result);
		if (KHook::LookupSignature(original, 11, signature) != original)
		{
			V_snprintf(error, maxlen, "ProcessMovement disk/memory mismatch");
			return nullptr;
		}
	}
	return result;
#else
	std::string diagnostic;
	auto lookup = [](void *start, size_t length, const char *pattern) { return KHook::LookupSignature(start, length, pattern); };
	void *result = linux_module::Resolve(std::filesystem::u8path(gameDir) / "bin/linuxsteamrt64/libserver.so", signature, lookup, diagnostic);
	if (!result)
	{
		V_snprintf(error, maxlen, "%s", diagnostic.c_str());
		return nullptr;
	}
	// SafetyHook's 5-byte near jump relocates only these first 6 instruction bytes.
	// A longer comparison would read its generated return jump, not original code.
	constexpr const char *prologue = "55 48 89 E5 41 57";
	if (!strncmp(signature, prologue, strlen(prologue)))
	{
		void *original = KHook::FindOriginal(result);
		if (!original || KHook::LookupSignature(original, 6, prologue) != original)
		{
			V_snprintf(error, maxlen, "ProcessMovement disk/memory mismatch");
			return nullptr;
		}
	}
	return result;
#endif
}

bool engine::Open(ISmmAPI *ismm, char *error, size_t maxlen)
{
	V_snprintf(error, maxlen, "Could not resolve required ShowPos engine interfaces/gamedata");
	GET_V_IFACE_CURRENT(GetEngineFactory, server, IVEngineServer2, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetServerFactory, game, ISource2Server, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetServerFactory, clients, ISource2GameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetServerFactory, entities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, schemas, CSchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, files, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	resource = ismm->GetEngineFactory()(GAMERESOURCESERVICESERVER_INTERFACE_VERSION, nullptr);
	gameDir = std::string(Plat_GetGameDirectory()) + "/csgo";
	auto kv = new KeyValues("Games");
	if (!resource || !kv->LoadFromFile(files, "addons/showpos/gamedata/showpos.games.txt", "GAME"))
	{
		delete kv;
		V_snprintf(error, maxlen, "Cannot load ShowPos gamedata");
		return false;
	}
	auto gameKV = kv->FindKey("csgo");
	auto offsets = gameKV ? gameKV->FindKey("Offsets") : nullptr;
	auto signatures = gameKV ? gameKV->FindKey("Signatures") : nullptr;
	auto offset = [&](const char *name)
	{
		auto k = offsets ? offsets->FindKey(name) : nullptr;
		return k ? k->GetInt(SHOWPOS_PLATFORM, -1) : -1;
	};
	entitySystemOffset = offset("GameEntitySystem");
	transmitSlot = offset("QuietPlayerSlot");
	teleportIndex = offset("Teleport");
	stateNetworkIndex = offset("HudStateNetworkChanged");
	auto address = [&](const char *name)
	{
		auto k = signatures ? signatures->FindKey(name) : nullptr;
		return k ? Resolve(k->GetString(SHOWPOS_PLATFORM, ""), error, maxlen) : nullptr;
	};
	movementAddress = address("ProcessMovement");
	createEntity = reinterpret_cast<decltype(createEntity)>(address("CreateEntityByName"));
	spawnEntity = reinterpret_cast<decltype(spawnEntity)>(address("DispatchSpawn"));
	removeEntity = reinterpret_cast<decltype(removeEntity)>(address("RemoveEntity"));
	delete kv;
	if (!movementAddress || !createEntity || !spawnEntity || !removeEntity || entitySystemOffset < 0 || transmitSlot < 0 || teleportIndex < 0
		|| stateNetworkIndex < 0)
	{
		return false;
	}
	if (!SchemaReady())
	{
		V_snprintf(error, maxlen, "Unsupported game schema; inspect ShowPos log");
		return false;
	}
	return true;
}

CEntityInstance *engine::Controller(int slot)
{
	auto es = GameEntitySystem();
	auto ent = es && slot >= 0 && slot < 64 ? es->GetEntityInstance(CEntityIndex(slot + 1)) : nullptr;
	return ent && !strcmp(ent->GetClassname(), "cs_player_controller") ? ent : nullptr;
}

CEntityInstance *engine::Pawn(int slot, bool current)
{
	auto c = Controller(slot);
	auto handle = (current ? currentPawn : pawn).At<CEntityHandle>(c);
	return handle ? handle->Get() : nullptr;
}

bool engine::Alive(CEntityInstance *p)
{
	return p && !strcmp(p->GetClassname(), "player") && life.Read<uint8>(p, 255) == 0;
}

double engine::Now()
{
	auto globals = server ? server->GetServerGlobals() : nullptr;
	return globals ? globals->curtime : 0;
}

int engine::Slot(CEntityInstance *p)
{
	auto handle = controller.At<CEntityHandle>(p);
	auto c = handle ? handle->Get() : nullptr;
	return c ? c->GetRefEHandle().GetEntryIndex() - 1 : -1;
}
