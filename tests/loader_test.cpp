#define META_NO_HL2SDK
#include <playerslot.h>
#include <ISmmPlugin.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::fprintf(stderr, "Usage: showpos-loader-tests /path/to/showpos.so\n");
		return 1;
	}
	void *module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (!module)
	{
		std::fprintf(stderr, "dlopen: %s\n", dlerror());
		return 1;
	}
	using Factory = void *(*)(const char *, int *);
	auto factory = reinterpret_cast<Factory>(dlsym(module, "CreateInterface"));
	int status = -1;
	auto plugin = factory ? static_cast<ISmmPlugin *>(factory(METAMOD_PLAPI_NAME, &status)) : nullptr;
	bool ok = plugin && status == META_IFACE_OK && plugin->GetApiVersion() == 18 && !std::strcmp(plugin->GetAuthor(), "hm_error and bell_meow")
			  && !std::strcmp(plugin->GetVersion(), "1.0.0-rc2");
	if (ok)
	{
		std::printf("Factory: %s / API %d / %s / %s\n", plugin->GetName(), plugin->GetApiVersion(), plugin->GetVersion(), plugin->GetAuthor());
	}
	ok = dlclose(module) == 0 && ok;
	std::printf("%s: ELF dlopen/factory metadata/dlclose (does not invoke plugin Load)\n", ok ? "PASS" : "FAIL");
	return ok ? 0 : 1;
}
