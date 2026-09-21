#include "module_linux.h"
#include <elf.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unistd.h>

static int checks;

static void Check(bool ok, const char *message)
{
	++checks;
	if (!ok)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

// Unique, relocation-free executable bytes; never invoked.
__attribute__((naked, noinline, used)) static void Marker()
{
	asm volatile(".byte 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x5f,0x5e,0x5d,0x5c,0x5b,0x5a,0x59,0x58,0xc3");
}

static void *Lookup(void *start, size_t length, const char *text)
{
	std::vector<unsigned char> bytes;
	for (const char *cursor = text; *cursor;)
	{
		char *end;
		auto value = strtoul(cursor, &end, 16);
		if (end == cursor || value > 255)
		{
			return nullptr;
		}
		bytes.push_back(static_cast<unsigned char>(value));
		cursor = end;
		while (*cursor == ' ')
		{
			++cursor;
		}
	}
	for (size_t i = 0; !bytes.empty() && bytes.size() <= length && i <= length - bytes.size(); ++i)
	{
		if (!memcmp(static_cast<char *>(start) + i, bytes.data(), bytes.size()))
		{
			return static_cast<char *>(start) + i;
		}
	}
	return nullptr;
}

int main()
{
	constexpr const char *pattern = "50 51 52 53 54 55 56 57 5f 5e 5d 5c 5b 5a 59 58 c3";
	std::string error;
	Check(linux_module::Resolve("/proc/self/exe", pattern, Lookup, error) == reinterpret_cast<void *>(&Marker), "loaded ELF virtual address");
	Check(!linux_module::Resolve("/nonexistent-showpos-module", pattern, Lookup, error), "missing module");
	std::ifstream input("/proc/self/exe", std::ios::binary);
	std::vector<char> disk((std::istreambuf_iterator<char>(input)), {});
	uintptr_t rva;
	Check(linux_module::FindUnique(disk, pattern, Lookup, rva, error), "unique executable match");
	auto copy = std::filesystem::temp_directory_path() / ("showpos-elf-test-" + std::to_string(getpid()));
	Check(!std::filesystem::exists(copy), "isolated image path");
	{
		std::ofstream out(copy, std::ios::binary);
		out.write(disk.data(), disk.size());
		Check(bool(out), "copy image");
	}
	Check(!linux_module::Resolve(copy, pattern, Lookup, error), "on-disk copy is not the loaded module");
	std::filesystem::remove(copy);

	Elf64_Ehdr eh {};
	memcpy(eh.e_ident, ELFMAG, SELFMAG);
	eh.e_ident[EI_CLASS] = ELFCLASS64;
	eh.e_ident[EI_DATA] = ELFDATA2LSB;
	eh.e_ident[EI_VERSION] = eh.e_version = EV_CURRENT;
	eh.e_type = ET_DYN;
	eh.e_machine = EM_X86_64;
	eh.e_ehsize = sizeof(eh);
	eh.e_phentsize = sizeof(Elf64_Phdr);
	eh.e_phnum = 1;
	eh.e_phoff = sizeof(eh);
	Elf64_Phdr ph {};
	ph.p_type = PT_LOAD;
	ph.p_flags = PF_R | PF_X;
	ph.p_offset = sizeof(eh) + sizeof(ph);
	ph.p_filesz = ph.p_memsz = 8;
	ph.p_vaddr = 0x4000;
	auto fixture = [&]()
	{
		std::vector<char> data(sizeof(eh) + sizeof(ph) + 8);
		memcpy(data.data(), &eh, sizeof(eh));
		memcpy(data.data() + sizeof(eh), &ph, sizeof(ph));
		data[sizeof(eh) + sizeof(ph) + 1] = char(0xaa);
		return data;
	};
	auto data = fixture();
	Check(linux_module::FindUnique(data, "aa", Lookup, rva, error) && rva == 0x4001, "file offset mapped through segment virtual address");
	Check(!linux_module::FindUnique(data, "bb", Lookup, rva, error), "missing signature");
	data.back() = char(0xaa);
	Check(!linux_module::FindUnique(data, "aa", Lookup, rva, error), "ambiguous signature rejected");
	ph.p_flags = PF_R;
	data = fixture();
	Check(!linux_module::FindUnique(data, "aa", Lookup, rva, error), "non-executable data excluded");
	ph.p_flags = PF_R | PF_X;
	ph.p_offset = UINT64_MAX;
	data = fixture();
	Check(!linux_module::FindUnique(data, "aa", Lookup, rva, error), "overflowing segment offset");
	ph.p_offset = sizeof(eh) + sizeof(ph);
	ph.p_vaddr = UINT64_MAX;
	data = fixture();
	Check(!linux_module::FindUnique(data, "aa", Lookup, rva, error), "overflowing virtual address");
	ph.p_vaddr = 0x4000;
	eh.e_phoff = UINT64_MAX;
	data = fixture();
	Check(!linux_module::FindUnique(data, "aa", Lookup, rva, error), "overflowing program header offset");
	eh.e_phoff = sizeof(eh);
	eh.e_machine = EM_AARCH64;
	data = fixture();
	Check(!linux_module::FindUnique(data, "aa", Lookup, rva, error), "wrong architecture");
	eh.e_machine = EM_X86_64;
	data = fixture();
	while (!data.empty())
	{
		data.pop_back();
		Check(!linux_module::FindUnique(data, "aa", Lookup, rva, error), "truncated ELF rejected");
	}
	printf("PASS: %d ELF/module checks\n", checks);
}
