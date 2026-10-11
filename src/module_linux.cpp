#include "module_linux.h"
#include <elf.h>
#include <link.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <cstdio>
#include <cstring>
#include <limits>
#include <dlfcn.h>

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

		uintptr_t rva {}, address {}, expectedBase {};
		uint64_t length {1};
		bool found {};
	};

	static int FindLoaded(dl_phdr_info *info, size_t, void *context)
	{
		auto &module = *static_cast<LoadedModule *>(context);

		struct stat candidate
		{
		};

		const char *path = info->dlpi_name && info->dlpi_name[0] ? info->dlpi_name : "/proc/self/exe";
		if (stat(path, &candidate) || candidate.st_dev != module.file.st_dev || candidate.st_ino != module.file.st_ino
			|| (module.expectedBase && module.expectedBase != info->dlpi_addr))
		{
			return 0;
		}
		for (size_t i = 0; i < info->dlpi_phnum; ++i)
		{
			const auto &segment = info->dlpi_phdr[i];
			if (segment.p_type == PT_LOAD && (segment.p_flags & PF_X) && module.rva >= segment.p_vaddr
				&& module.rva - segment.p_vaddr < segment.p_memsz && module.length <= segment.p_memsz - (module.rva - segment.p_vaddr)
				&& module.rva <= UINTPTR_MAX - info->dlpi_addr)
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

	static bool MappedFileMatches(const void *address, const struct stat &file)
	{
		FILE *maps = fopen("/proc/self/maps", "r");
		if (!maps)
		{
			return false;
		}
		char line[4096];
		bool match = false;
		while (fgets(line, sizeof(line), maps))
		{
			unsigned long long begin, end, offset, inode;
			unsigned deviceMajor, deviceMinor;
			char permissions[5] {};
			if (sscanf(line, "%llx-%llx %4s %llx %x:%x %llu", &begin, &end, permissions, &offset, &deviceMajor, &deviceMinor, &inode) == 7
				&& reinterpret_cast<uintptr_t>(address) >= begin && reinterpret_cast<uintptr_t>(address) < end)
			{
				match = permissions[2] == 'x' && inode == file.st_ino && deviceMajor == major(file.st_dev) && deviceMinor == minor(file.st_dev);
				break;
			}
		}
		fclose(maps);
		return match;
	}

	bool ResolveSymbols(const void *moduleAddress, const char *const *names, size_t count, void **addresses, std::string &error)
	{
		error = "Cannot read the loaded Metamod ELF image";
		Dl_info info {};
		if (!moduleAddress || !names || !count || !addresses || !dladdr(moduleAddress, &info) || !info.dli_fname || !info.dli_fbase)
		{
			return false;
		}
		FILE *file = fopen(info.dli_fname, "rb");
		if (!file)
		{
			return false;
		}
		LoadedModule module;
		module.expectedBase = reinterpret_cast<uintptr_t>(info.dli_fbase);
		if (fstat(fileno(file), &module.file) || module.file.st_size < off_t(sizeof(Elf64_Ehdr)) || module.file.st_size > 512 * 1024 * 1024)
		{
			fclose(file);
			return false;
		}
		if (!MappedFileMatches(moduleAddress, module.file))
		{
			fclose(file);
			error = "Metamod ELF file differs from the mapped image; restart the server after updating Metamod";
			return false;
		}
		std::vector<char> disk(size_t(module.file.st_size));
		bool read = fread(disk.data(), 1, disk.size(), file) == disk.size();
		fclose(file);
		if (!read)
		{
			return false;
		}
		Elf64_Ehdr header;
		memcpy(&header, disk.data(), sizeof(header));
		error = "Invalid Metamod ELF symbol table";
		if (memcmp(header.e_ident, ELFMAG, SELFMAG) || header.e_ident[EI_CLASS] != ELFCLASS64 || header.e_ident[EI_DATA] != ELFDATA2LSB
			|| header.e_ident[EI_VERSION] != EV_CURRENT || header.e_version != EV_CURRENT || header.e_machine != EM_X86_64 || header.e_type != ET_DYN
			|| header.e_ehsize != sizeof(header) || header.e_shentsize != sizeof(Elf64_Shdr) || !header.e_shnum || header.e_shnum >= SHN_LORESERVE
			|| !Range(header.e_shoff, uint64_t(header.e_shnum) * sizeof(Elf64_Shdr), disk.size()))
		{
			return false;
		}
		std::vector<Elf64_Shdr> sections(header.e_shnum);
		memcpy(sections.data(), disk.data() + header.e_shoff, sections.size() * sizeof(Elf64_Shdr));
		std::vector<uint64_t> offsets(count), lengths(count);
		for (const auto &table : sections)
		{
			if (table.sh_type != SHT_SYMTAB && table.sh_type != SHT_DYNSYM)
			{
				continue;
			}
			if (table.sh_entsize != sizeof(Elf64_Sym) || table.sh_size % sizeof(Elf64_Sym) || table.sh_link >= sections.size()
				|| !Range(table.sh_offset, table.sh_size, disk.size()))
			{
				return false;
			}
			const auto &strings = sections[table.sh_link];
			if (strings.sh_type != SHT_STRTAB || !Range(strings.sh_offset, strings.sh_size, disk.size()))
			{
				return false;
			}
			for (uint64_t cursor = 0; cursor < table.sh_size; cursor += sizeof(Elf64_Sym))
			{
				Elf64_Sym symbol;
				memcpy(&symbol, disk.data() + table.sh_offset + cursor, sizeof(symbol));
				if (ELF64_ST_TYPE(symbol.st_info) != STT_FUNC || symbol.st_shndx == SHN_UNDEF || symbol.st_shndx >= sections.size()
					|| symbol.st_name >= strings.sh_size || !symbol.st_size)
				{
					continue;
				}
				const char *name = disk.data() + strings.sh_offset + symbol.st_name;
				if (!memchr(name, '\0', strings.sh_size - symbol.st_name))
				{
					return false;
				}
				for (size_t n = 0; n < count; ++n)
				{
					if (strcmp(name, names[n]))
					{
						continue;
					}
					const auto &code = sections[symbol.st_shndx];
					if (!(code.sh_flags & SHF_EXECINSTR) || code.sh_type != SHT_PROGBITS || !Range(code.sh_offset, code.sh_size, disk.size())
						|| symbol.st_value < code.sh_addr || symbol.st_value - code.sh_addr >= code.sh_size
						|| symbol.st_size > code.sh_size - (symbol.st_value - code.sh_addr) || (lengths[n] && offsets[n] != symbol.st_value))
					{
						return false;
					}
					offsets[n] = symbol.st_value;
					lengths[n] = symbol.st_size;
				}
			}
		}
		for (size_t n = 0; n < count; ++n)
		{
			if (!lengths[n])
			{
				error = std::string("Metamod is missing KHook function symbols; install the official unstripped build: ") + names[n];
				return false;
			}
			module.rva = offsets[n];
			module.length = lengths[n];
			module.found = false;
			dl_iterate_phdr(FindLoaded, &module);
			if (!module.found)
			{
				error = "KHook function does not belong to the exact loaded Metamod executable segments";
				return false;
			}
			addresses[n] = reinterpret_cast<void *>(module.address);
		}
		error.clear();
		return true;
	}
} // namespace linux_module
