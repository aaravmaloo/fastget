#include "verifier.hpp"
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fastget {

std::string Verifier::ComputeHash(const std::string& filename, Verifier::HashType type) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return "";

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = nullptr;
    
    switch (type) {
        case Verifier::HashType::MD5: md = EVP_md5(); break;
        case Verifier::HashType::SHA1: md = EVP_sha1(); break;
        case Verifier::HashType::SHA512: md = EVP_sha512(); break;
        case Verifier::HashType::SHA256: 
        default: md = EVP_sha256(); break;
    }

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

std::string Verifier::ComputeSHA256(const std::string& filename) {
    return ComputeHash(filename, HashType::SHA256);
}

std::string Verifier::ComputeMD5(const std::string& filename) {
    return ComputeHash(filename, HashType::MD5);
}

std::string Verifier::ComputeSHA1(const std::string& filename) {
    return ComputeHash(filename, HashType::SHA1);
}

std::string Verifier::ComputeSHA512(const std::string& filename) {
    return ComputeHash(filename, HashType::SHA512);
}

bool Verifier::Verify(const std::string& filename, const std::string& expected_hash, Verifier::HashType type) {
    std::string actual_hash = ComputeHash(filename, type);
    return actual_hash == expected_hash;
}

} // namespace fastget
