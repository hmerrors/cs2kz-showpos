#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace linux_module
{
	using Lookup = void *(*)(void *, size_t, const char *);
	// Resolve only executable ranges belonging to the exact loaded server image.
	bool FindUnique(std::vector<char> &disk, const char *signature, Lookup lookup, uintptr_t &rva, std::string &error);
	void *Resolve(const std::filesystem::path &path, const char *signature, Lookup lookup, std::string &error);
	// Resolve named functions in the exact loaded image, including hidden ELF symbols.
	bool ResolveSymbols(const void *moduleAddress, const char *const *names, size_t count, void **addresses, std::string &error);
} // namespace linux_module
