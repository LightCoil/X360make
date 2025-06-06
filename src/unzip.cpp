// src/unzip.cpp
#include "unzip.h"
#include <filesystem>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <zip.h>
#include <windows.h>

namespace fs = std::filesystem;

// UTF-8 → UTF-16 (MB_ERR_INVALID_CHARS)
static bool Utf8ToWStringSafe(const std::string& utf8, std::wstring& out) {
    if (utf8.empty()) {
        out.clear();
        return true;
    }
    int needed = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data(), (int)utf8.size(),
        nullptr, 0
    );
    if (needed <= 0) {
        out.clear();
        return false;
    }
    out.resize(needed);
    int ret = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data(), (int)utf8.size(),
        &out[0], needed
    );
    if (ret <= 0) {
        out.clear();
        return false;
    }
    return true;
}

// Проверка, что child лежит внутри base (canonical), учитывая любую регистронезависимость на Windows
static bool IsSubPath(const fs::path& base, const fs::path& child) {
    std::error_code ec;
    fs::path b = fs::weakly_canonical(base, ec);
    if (ec) return false;
    fs::path c;
    try {
        c = fs::weakly_canonical(child);
    } catch (...) {
        return false;
    }
    // Приводим к lowercase для сравнения на Windows
    auto toLower = [](const std::string& s) {
        std::string r;
        r.reserve(s.size());
        for (char ch : s) r.push_back((char)tolower(ch));
        return r;
    };
    std::string bs = toLower(b.string());
    std::string cs = toLower(c.string());
    return cs.rfind(bs, 0) == 0;
}

bool Unzip(const std::wstring& zipPath,
           const std::wstring& outDir,
           int maxThreads)
{
    std::error_code ec;
    fs::path z = fs::weakly_canonical(zipPath, ec);
    if (ec || !fs::exists(z)) {
        return false;
    }
    fs::create_directories(outDir, ec);
    if (ec) return false;
    fs::path outCan = fs::weakly_canonical(outDir, ec);
    if (ec) return false;

    int err = 0;
    zip_t* za = zip_open(std::string(z.u8string()).c_str(), ZIP_RDONLY, &err);
    if (!za) {
        return false;
    }

    zip_int64_t numEntries = zip_get_num_entries(za, 0);
    if (numEntries <= 0) {
        zip_close(za);
        return false;
    }

    struct Entry { zip_uint64_t idx; bool isDir; bool isSym; zip_uint64_t size; };
    std::vector<Entry> entries;
    entries.reserve((size_t)numEntries);

    for (zip_uint64_t i = 0; i < (zip_uint64_t)numEntries; ++i) {
        zip_stat_t st;
        zip_stat_init(&st);
        if (zip_stat_index(za, i, 0, &st) != 0) {
            zip_close(za);
            return false;
        }
        bool isDir = (st.name[strlen(st.name) - 1] == '/');
        bool isSym = (st.valid & ZIP_STAT_SYMLINK) != 0;
        if (st.size > (1ull << 30)) {
            zip_close(za);
            return false;
        }
        entries.push_back({ i, isDir, isSym, st.size });
    }

    std::atomic<int> idx(0);
    std::atomic<bool> anyExtracted(false);
    std::atomic<bool> sawSymlink(false);
    std::mutex dirMutex;

    auto worker = [&]() {
        while (true) {
            int i = idx.fetch_add(1);
            if (i >= (int)entries.size()) break;
            auto& ent = entries[i];
            if (ent.isDir) continue;
            if (ent.isSym) {
                sawSymlink.store(true, std::memory_order_release);
                continue;
            }

            zip_file_t* zf = zip_fopen_index(za, ent.idx, 0);
            if (!zf) continue;

            zip_stat_t st;
            zip_stat_init(&st);
            if (zip_stat_index(za, ent.idx, 0, &st) != 0) {
                zip_fclose(zf);
                continue;
            }
            std::string nameUtf8(st.name);
            std::wstring nameW;
            if (!Utf8ToWStringSafe(nameUtf8, nameW)) {
                zip_fclose(zf);
                continue;
            }
            fs::path destPath = fs::path(outDir) / nameW;
            fs::path parent = destPath.parent_path();

            {
                std::lock_guard<std::mutex> lock(dirMutex);
                std::error_code ec2;
                fs::create_directories(parent, ec2);
                if (ec2) {
                    zip_fclose(zf);
                    continue;
                }
            }
            if (!IsSubPath(outCan, parent)) {
                zip_fclose(zf);
                continue;
            }

            std::ofstream ofs(destPath, std::ios::binary);
            if (!ofs.is_open()) {
                zip_fclose(zf);
                continue;
            }
            const size_t BUF_SIZE = 64 * 1024;
            std::vector<char> buffer(BUF_SIZE);
            zip_int64_t bytesRead = 0;
            while ((bytesRead = zip_fread(zf, buffer.data(), BUF_SIZE)) > 0) {
                ofs.write(buffer.data(), (std::streamsize)bytesRead);
                if (!ofs.good()) {
                    break;
                }
            }
            ofs.close();
            zip_fclose(zf);
            anyExtracted.store(true, std::memory_order_release);
        }
    };

    int threadsCount = (maxThreads > 0 ? maxThreads : 4);
    threadsCount = std::min(threadsCount, (int)entries.size());
    std::vector<std::thread> threads;
    threads.reserve(threadsCount);
    for (int t = 0; t < threadsCount; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) {
        th.join();
    }

    zip_close(za);
    if (sawSymlink.load(std::memory_order_acquire)) {
        return false;
    }
    return anyExtracted.load(std::memory_order_acquire);
}
