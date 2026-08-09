#include "security/PasswordHasher.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cstring>
#include <sstream>

namespace healthiq::security {

namespace {
const int kIterations = 150000;
const size_t kSaltLen = 16;
const size_t kHashLen = 32;

std::string toHex(const unsigned char* data, size_t len) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(hex[data[i] >> 4]);
        out.push_back(hex[data[i] & 0x0F]);
    }
    return out;
}

bool fromHex(const std::string& s, unsigned char* out, size_t outLen) {
    if (s.size() != outLen * 2) return false;
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < outLen; ++i) {
        int hi = nibble(s[i * 2]);
        int lo = nibble(s[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = static_cast<unsigned char>((hi << 4) | lo);
    }
    return true;
}
}  // namespace

std::string PasswordHasher::hash(const std::string& password) {
    std::array<unsigned char, kSaltLen> salt{};
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        return {};
    }
    std::array<unsigned char, kHashLen> key{};
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                          salt.data(), static_cast<int>(salt.size()),
                          kIterations, EVP_sha256(),
                          static_cast<int>(key.size()), key.data()) != 1) {
        return {};
    }
    return "pbkdf2$" + std::to_string(kIterations) + "$" +
           toHex(salt.data(), salt.size()) + "$" + toHex(key.data(), key.size());
}

bool PasswordHasher::verify(const std::string& password,
                            const std::string& stored) {
    const std::string prefix = "pbkdf2$";
    if (stored.rfind(prefix, 0) != 0) return false;
    const auto itersEnd = stored.find('$', prefix.size());
    if (itersEnd == std::string::npos) return false;
    const auto saltEnd = stored.find('$', itersEnd + 1);
    if (saltEnd == std::string::npos) return false;
    const int iterations = std::stoi(stored.substr(prefix.size(), itersEnd - prefix.size()));
    const std::string saltHex = stored.substr(itersEnd + 1, saltEnd - itersEnd - 1);
    const std::string keyHex = stored.substr(saltEnd + 1);
    if (saltHex.size() != kSaltLen * 2 || keyHex.size() != kHashLen * 2) return false;
    std::array<unsigned char, kSaltLen> salt{};
    std::array<unsigned char, kHashLen> key{};
    if (!fromHex(saltHex, salt.data(), salt.size())) return false;
    if (!fromHex(keyHex, key.data(), key.size())) return false;
    std::array<unsigned char, kHashLen> computed{};
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                          salt.data(), static_cast<int>(salt.size()),
                          iterations, EVP_sha256(),
                          static_cast<int>(computed.size()), computed.data()) != 1) {
        return false;
    }
    return CRYPTO_memcmp(computed.data(), key.data(), key.size()) == 0;
}

}  // namespace healthiq::security
