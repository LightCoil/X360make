#include "packer.h"
#include <cstdlib>

bool PackElfToXex(const std::wstring& elf, const std::wstring& xex) {
    std::wstring cmd = L"embedded\\x360make_pack.exe \"" + elf + L"\" \"" + xex + L"\"";
    return _wsystem(cmd.c_str()) == 0;
}
