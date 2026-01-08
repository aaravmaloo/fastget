#pragma once
#include <string>
#include <vector>

namespace fastget {

class Verifier {
public:
    static std::string ComputeSHA256(const std::string& filename);
    static bool Verify(const std::string& filename, const std::string& expected_hash);
};

} // namespace fastget
