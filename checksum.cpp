#include "checksum.h"
#include <fstream>
#include <array>
#include <stdexcept>
#include <memory>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/evp.h>
#endif

std::string file_sha256(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot read download");
    std::array<unsigned char, 32> digest{};
    std::array<char, 65536> buffer{};
#ifdef _WIN32
    struct HashContext {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        ~HashContext() { if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0); }
    } context;
    if (BCryptOpenAlgorithmProvider(&context.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
        BCryptCreateHash(context.algorithm, &context.hash, nullptr, 0, nullptr, 0, 0) < 0)
        throw std::runtime_error("Cannot initialize SHA-256");
#else
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1)
        throw std::runtime_error("Cannot initialize SHA-256");
#endif
    while (input) {
        input.read(buffer.data(), buffer.size());
        auto count = input.gcount();
        if (count <= 0) break;
#ifdef _WIN32
        if (BCryptHashData(context.hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(count), 0) < 0)
#else
        if (EVP_DigestUpdate(context.get(), buffer.data(), static_cast<std::size_t>(count)) != 1)
#endif
            throw std::runtime_error("Cannot hash download");
    }
    if (input.bad()) throw std::runtime_error("Cannot read download");
#ifdef _WIN32
    if (BCryptFinishHash(context.hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
#else
    if (EVP_DigestFinal_ex(context.get(), digest.data(), nullptr) != 1)
#endif
        throw std::runtime_error("Cannot finish SHA-256");
    const char* hex = "0123456789abcdef";
    std::string result;
    for (auto byte : digest) { result += hex[byte >> 4]; result += hex[byte & 15]; }
    return result;
}
