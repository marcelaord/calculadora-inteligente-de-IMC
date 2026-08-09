#include "security/JwtService.h"

#include <openssl/hmac.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <sstream>
#include <utility>

namespace healthiq::security {

namespace {
const char* kB64Url =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
}  // namespace

JwtService::JwtService(std::string secret) : secret_(std::move(secret)) {}

std::string JwtService::base64UrlEncode(const std::string& data) {
    if (data.empty()) return {};
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < data.size()) {
        uint32_t n = (static_cast<uint8_t>(data[i]) << 16) |
                     (static_cast<uint8_t>(data[i + 1]) << 8) |
                     static_cast<uint8_t>(data[i + 2]);
        out.push_back(kB64Url[(n >> 18) & 63]);
        out.push_back(kB64Url[(n >> 12) & 63]);
        out.push_back(kB64Url[(n >> 6) & 63]);
        out.push_back(kB64Url[n & 63]);
        i += 3;
    }
    size_t rem = data.size() - i;
    if (rem == 1) {
        uint32_t n = static_cast<uint8_t>(data[i]) << 16;
        out.push_back(kB64Url[(n >> 18) & 63]);
        out.push_back(kB64Url[(n >> 12) & 63]);
    } else if (rem == 2) {
        uint32_t n = (static_cast<uint8_t>(data[i]) << 16) |
                     (static_cast<uint8_t>(data[i + 1]) << 8);
        out.push_back(kB64Url[(n >> 18) & 63]);
        out.push_back(kB64Url[(n >> 12) & 63]);
        out.push_back(kB64Url[(n >> 6) & 63]);
    }
    return out;
}

std::string JwtService::base64UrlDecode(const std::string& data) {
    auto isB64 = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_';
    };
    std::string clean;
    clean.reserve(data.size());
    for (char c : data) {
        if (isB64(c)) clean.push_back(c);
    }
    int table[256];
    std::memset(table, -1, sizeof(table));
    for (int i = 0; i < 64; ++i) {
        table[static_cast<unsigned char>(kB64Url[i])] = i;
    }
    std::string out;
    uint32_t acc = 0;
    int bits = 0;
    for (char c : clean) {
        int v = table[static_cast<unsigned char>(c)];
        if (v < 0) continue;
        acc = (acc << 6) | static_cast<uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((acc >> bits) & 0xFF));
        }
    }
    return out;
}

std::string JwtService::hmacSha256(const std::string& data) const {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(), secret_.data(), static_cast<int>(secret_.size()),
         reinterpret_cast<const unsigned char*>(data.data()),
         data.size(), digest, &len);
    return std::string(reinterpret_cast<char*>(digest), len);
}

std::string JwtService::nowEpoch() {
    return std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::string JwtService::createToken(int64_t userId,
                                    const std::string& role,
                                    long ttlSeconds) const {
    const long iat = std::stol(nowEpoch());
    const long exp = iat + ttlSeconds;
    const std::string header = R"({"alg":"HS256","typ":"JWT"})";
    const std::string payload =
        "{\"uid\":" + std::to_string(userId) +
        ",\"role\":\"" + role +
        "\",\"iat\":" + std::to_string(iat) +
        ",\"exp\":" + std::to_string(exp) + "}";
    const std::string signingInput =
        base64UrlEncode(header) + "." + base64UrlEncode(payload);
    return signingInput + "." + base64UrlEncode(hmacSha256(signingInput));
}

std::optional<JwtService::Payload> JwtService::verify(const std::string& token) const {
    const auto dot1 = token.find('.');
    const auto dot2 = token.find('.', dot1 + 1);
    if (dot1 == std::string::npos || dot2 == std::string::npos) {
        return std::nullopt;
    }
    const std::string signingInput = token.substr(0, dot2);
    const std::string signature = token.substr(dot2 + 1);
    const std::string expected = hmacSha256(signingInput);
    if (expected != base64UrlDecode(signature)) {
        return std::nullopt;
    }
    const std::string payloadJson = base64UrlDecode(token.substr(dot1 + 1, dot2 - dot1 - 1));
    Payload p;
    auto getInt = [&](const std::string& key, long& out) {
        const std::string k = "\"" + key + "\":";
        auto pos = payloadJson.find(k);
        if (pos == std::string::npos) return false;
        pos += k.size();
        out = std::stol(payloadJson.substr(pos));
        return true;
    };
    auto getString = [&](const std::string& key, std::string& out) {
        const std::string k = "\"" + key + "\":\"";
        auto pos = payloadJson.find(k);
        if (pos == std::string::npos) return false;
        pos += k.size();
        auto end = payloadJson.find('"', pos);
        if (end == std::string::npos) return false;
        out = payloadJson.substr(pos, end - pos);
        return true;
    };
    if (!getInt("uid", p.userId) || !getString("role", p.role) ||
        !getInt("iat", p.issuedAt) || !getInt("exp", p.expiresAt)) {
        return std::nullopt;
    }
    const long now = std::stol(nowEpoch());
    if (now >= p.expiresAt || now < p.issuedAt) {
        return std::nullopt;
    }
    return p;
}

}  // namespace healthiq::security
