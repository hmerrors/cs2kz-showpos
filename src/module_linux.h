#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace linux_module
{
	using Lookup = void *(*)(void *, size_t, const char *);
	// Kept independent of Valve/Metamod so malformed ELF images can be tested.
	bool FindUnique(std::vector<char> &disk, const char *signature, Lookup lookup, uintptr_t &rva, std::string &error);
	void *Resolve(const std::filesystem::path &path, const char *signature, Lookup lookup, std::string &error);
} // namespace linux_module
