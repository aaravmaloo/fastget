#pragma once
#include <string>
#include <vector>

namespace fastget {

class Verifier {
public:
    enum class HashType { SHA256, MD5, SHA1, SHA512 };

    static std::string ComputeHash(const std::string& filename, HashType type);
    static std::string ComputeSHA256(const std::string& filename);
    static std::string ComputeMD5(const std::string& filename);
    static std::string ComputeSHA1(const std::string& filename);
    static std::string ComputeSHA512(const std::string& filename);

    static bool Verify(const std::string& filename, const std::string& expected_hash, HashType type = HashType::SHA256);
};

}
