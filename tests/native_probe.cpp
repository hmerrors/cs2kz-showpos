#include "native_hook.h"
#include <Windows.h>
#include <cstdio>
#include "entityidentity.h"
#include "cstrike15_usermessages.pb.h"
static ConCommand *testMenuCommand, *testClickCommand;
static void *testResource;
static ISource2GameClients *testClients;

CGameEntitySystem *GameEntitySystem()
{
	return testResource ? *reinterpret_cast<CGameEntitySystem **>(static_cast<char *>(testResource) + 88) : nullptr;
}

static void TestClick(const CCommandContext &, const CCommand &args)
{
	auto es = GameEntitySystem();
	if (!es)
	{
		return;
	}
	for (int i = 0; i < 16384; i++)
	{
		auto ent = es->GetEntityInstance(CEntityIndex(i));
		if (!ent || V_strcmp(ent->GetClassname(), "custom_hud_layout") || V_strcmp(ent->m_pEntity->m_name.String(), "showpos_menu_0"))
		{
			continue;
		}
		CCSUsrMsg_CustomHudClicked message;
		message.set_custom_hud_layout(ent->GetRefEHandle().ToPackedInt());
		message.set_button_id(args.ArgC() > 1 ? args[1] : "item0");
		std::string bytes = message.SerializeAsString();
		testClients->ClientSvcUserMessage(CPlayerSlot(0), CS_UM_CustomHudClicked, uint32(bytes.size()), bytes.data());
		Msg("[ShowPos] Test-only native HUD click delivered: %s\n", message.button_id().c_str());
		return;
	}
	Warning("[ShowPos] Test-only menu entity not found.\n");
}

static void TestMenu(const CCommandContext &, const CCommand &)
{
	CCommand args;
	args.Tokenize("kz_showpos");
	ConCommandRef command("kz_showpos");
	if (command.IsValidRef())
	{
		command.Dispatch(CCommandContext(CT_NO_TARGET, CPlayerSlot(0)), args);
		Msg("[ShowPos] Test-only player slot 0 menu request sent.\n");
	}
}

static LONG CALLBACK TraceException(EXCEPTION_POINTERS *e)
{
	if (e->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	FILE *f = fopen("showpos-test-fault.txt", "a");
	if (!f)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	fprintf(f, "FAULT ip=%p address=%p\n", e->ExceptionRecord->ExceptionAddress, (void *)e->ExceptionRecord->ExceptionInformation[1]);
	void *frames[40];
	USHORT count = CaptureStackBackTrace(0, 40, frames, nullptr);
	for (USHORT i = 0; i < count; i++)
	{
		HMODULE mod = nullptr;
		char path[MAX_PATH] = {};
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)frames[i], &mod);
		if (mod)
		{
			GetModuleFileNameA(mod, path, MAX_PATH);
		}
		fprintf(f, "FRAME %s + %llx\n", path, (unsigned long long)frames[i] - (unsigned long long)mod);
	}
	fclose(f);
	return EXCEPTION_CONTINUE_SEARCH;
}

class Probe
{
public:
	__declspec(noinline) void Move(CMoveData *p)
	{
		p->m_vecVelocity.x += 10;
	}
};

static float observed;

static void Observe(void *, CMoveData *p)
{
	observed = p->m_vecVelocity.x;
}

static void CoreCorrection(void *, CMoveData *p)
{
	p->m_vecVelocity.x += 1;
}

static bool SelfTest()
{
	bool ok = true;
	for (bool observerFirst : {false})
	{
		MovementHook core, observer;
		void *addr = KHook::ExtractMFP(&Probe::Move);
		bool a, b;
		if (observerFirst)
		{
			a = observer.Install(addr, Observe);
			b = core.Install(addr, CoreCorrection, true);
		}
		else
		{
			a = core.Install(addr, CoreCorrection, true);
			b = observer.Install(addr, Observe);
		}
		CMoveData data;
		data.m_vecVelocity.Init(5, 0, 0);
		Probe p;
		observed = -1;
		auto volatile fn = KHook::BuildMFP<void (Probe::*)(CMoveData *)>(addr);
		(p.*fn)(&data);
		Msg("[ShowPos] Hook probe first=%d registered=%d/%d observed=%.0f final=%.0f\n", observerFirst, a, b, observed, data.m_vecVelocity.x);
		ok &= a && b && observed == 5 && data.m_vecVelocity.x == 16;
		observer.Clear();
		data.m_vecVelocity.x = 5;
		(p.*fn)(&data);
		ok &= data.m_vecVelocity.x == 16;
		Msg("[ShowPos] Hook probe observer removed=%.0f\n", data.m_vecVelocity.x);
		core.Clear();
		data.m_vecVelocity.x = 5;
		(p.*fn)(&data);
		ok &= data.m_vecVelocity.x == 15;
		Msg("[ShowPos] Hook probe all removed=%.0f\n", data.m_vecVelocity.x);
	}
	Msg("[ShowPos] Native API 18 pre-order/unload self-test: %s\n", ok ? "PASS" : "FAIL");
	return ok;
}

class NativeProbePlugin final : public ISmmPlugin
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override
	{
		PLUGIN_SAVEVARS();
		GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
		GET_V_IFACE_CURRENT(GetServerFactory, testClients, ISource2GameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
		testResource = ismm->GetEngineFactory()(GAMERESOURCESERVICESERVER_INTERFACE_VERSION, nullptr);
		testMenuCommand = new ConCommand("showpos_test_menu", TestMenu, "Test-only slot 0 menu trigger");
		testClickCommand = new ConCommand("showpos_test_click", TestClick, "Test-only slot 0 HUD click");
		META_CONVAR_REGISTER(FCVAR_NONE);
		// Test-only DLL: KHook retains detour capsules until Metamod shuts down. Pin this
		// probe's code so those capsules cannot target an unloaded test module at exit.
		HMODULE module;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&SelfTest), &module);
		AddVectoredExceptionHandler(1, TraceException);
		bool ok = SelfTest();
		if (!ok)
		{
			V_snprintf(error, maxlen, "Hook probe failed");
		}
		return ok;
	}

	bool Unload(char *, size_t) override
	{
		for (auto p : {testMenuCommand, testClickCommand})
		{
			g_SMAPI->UnregisterConCommand(g_PLAPI, p);
			p->InvalidateRef();
			delete p;
		}
		testMenuCommand = testClickCommand = nullptr;
		ConVar_Unregister();
		return true;
	}

	const char *GetAuthor() override
	{
		return "hm_error and bell_meow";
	}

	const char *GetName() override
	{
		return "ShowPos native hook test (test only)";
	}

	const char *GetDescription() override
	{
		return "API 18 native integration test";
	}

	const char *GetURL() override
	{
		return "";
	}

	const char *GetLicense() override
	{
		return "AGPL-3.0";
	}

	const char *GetVersion() override
	{
		return "1";
	}

	const char *GetDate() override
	{
		return __DATE__;
	}

	const char *GetLogTag() override
	{
		return "ShowPosTest";
	}
};

NativeProbePlugin probe;
PLUGIN_EXPOSE(NativeProbePlugin, probe);
