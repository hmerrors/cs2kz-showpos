#include "module_linux.h"
#include <elf.h>
#include <link.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <limits>

namespace linux_module
{
	static bool Range(uint64_t start, uint64_t count, size_t total)
	{
		return start <= total && count <= total - start;
	}

	bool FindUnique(std::vector<char> &disk, const char *signature, Lookup lookup, uintptr_t &rva, std::string &error)
	{
		error = "Invalid ELF64 server image";
		if (!signature || !*signature || !lookup || disk.size() < sizeof(Elf64_Ehdr))
		{
			return false;
		}
		Elf64_Ehdr header;
		memcpy(&header, disk.data(), sizeof(header));
		if (memcmp(header.e_ident, ELFMAG, SELFMAG) || header.e_ident[EI_CLASS] != ELFCLASS64 || header.e_ident[EI_DATA] != ELFDATA2LSB
			|| header.e_ident[EI_VERSION] != EV_CURRENT || header.e_version != EV_CURRENT || header.e_machine != EM_X86_64
			|| (header.e_type != ET_DYN && header.e_type != ET_EXEC) || header.e_ehsize != sizeof(header) || header.e_phentsize != sizeof(Elf64_Phdr)
			|| !header.e_phnum || header.e_phnum == PN_XNUM || !Range(header.e_phoff, uint64_t(header.e_phnum) * sizeof(Elf64_Phdr), disk.size()))
		{
			return false;
		}
		unsigned matches = 0;
		for (size_t i = 0; i < header.e_phnum; ++i)
		{
			Elf64_Phdr segment;
			memcpy(&segment, disk.data() + header.e_phoff + i * sizeof(segment), sizeof(segment));
			if (segment.p_type != PT_LOAD || !(segment.p_flags & PF_X))
			{
				continue;
			}
			if (!Range(segment.p_offset, segment.p_filesz, disk.size()) || segment.p_filesz > segment.p_memsz
				|| segment.p_memsz > std::numeric_limits<uintptr_t>::max() - segment.p_vaddr)
			{
				return false;
			}
			char *begin = disk.data() + segment.p_offset;
			char *end = begin + segment.p_filesz;
			for (char *cursor = begin; cursor < end;)
			{
				auto hit = static_cast<char *>(lookup(cursor, size_t(end - cursor), signature));
				if (!hit)
				{
					break;
				}
				if (hit < cursor || hit >= end)
				{
					return false;
				}
				rva = segment.p_vaddr + size_t(hit - begin);
				if (++matches > 1)
				{
					error = "ELF signature is not unique";
					return false;
				}
				cursor = hit + 1;
			}
		}
		error = matches == 1 ? "" : "Signature not found in executable ELF segments";
		return matches == 1;
	}

	struct LoadedModule
	{
		struct stat file
		{
		};

		uintptr_t rva {}, address {};
		bool found {};
	};

	static int FindLoaded(dl_phdr_info *info, size_t, void *context)
	{
		auto &module = *static_cast<LoadedModule *>(context);

		struct stat candidate
		{
		};

		const char *path = info->dlpi_name && info->dlpi_name[0] ? info->dlpi_name : "/proc/self/exe";
		if (stat(path, &candidate) || candidate.st_dev != module.file.st_dev || candidate.st_ino != module.file.st_ino)
		{
			return 0;
		}
		for (size_t i = 0; i < info->dlpi_phnum; ++i)
		{
			const auto &segment = info->dlpi_phdr[i];
			if (segment.p_type == PT_LOAD && (segment.p_flags & PF_X) && module.rva >= segment.p_vaddr
				&& module.rva - segment.p_vaddr < segment.p_memsz && module.rva <= UINTPTR_MAX - info->dlpi_addr)
			{
				module.address = info->dlpi_addr + module.rva;
				module.found = true;
				return 1;
			}
		}
		return 0;
	}

	void *Resolve(const std::filesystem::path &path, const char *signature, Lookup lookup, std::string &error)
	{
		error = "Cannot read loaded Linux server module";
		FILE *file = fopen(path.c_str(), "rb");
		if (!file)
		{
			return nullptr;
		}
		LoadedModule module;
		if (fstat(fileno(file), &module.file) || module.file.st_size < off_t(sizeof(Elf64_Ehdr)) || module.file.st_size > 512 * 1024 * 1024)
		{
			fclose(file);
			return nullptr;
		}
		std::vector<char> disk(size_t(module.file.st_size));
		bool read = fread(disk.data(), 1, disk.size(), file) == disk.size();
		fclose(file);
		if (!read || !FindUnique(disk, signature, lookup, module.rva, error))
		{
			return nullptr;
		}
		dl_iterate_phdr(FindLoaded, &module);
		if (!module.found)
		{
			error = "ELF address does not belong to the exact loaded server module";
			return nullptr;
		}
		return reinterpret_cast<void *>(module.address);
	}
} // namespace linux_module
