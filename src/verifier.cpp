#include "verifier.hpp"
#include <openssl/evp.h>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fastget {

std::string Verifier::ComputeSHA256(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return "";

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    EVP_DigestInit_ex(ctx, md, nullptr);

    char buffer[32768];
    while (file.read(buffer, sizeof(buffer))) {
        EVP_DigestUpdate(ctx, buffer, file.gcount());
    }
    EVP_DigestUpdate(ctx, buffer, file.gcount());

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool Verifier::Verify(const std::string& filename, const std::string& expected_hash) {
    std::string actual_hash = ComputeSHA256(filename);
    return actual_hash == expected_hash;
}

} // namespace fastget
