
#include "ICEConnection/ICEConnection.h"
#include "Encryptions/SHA256/sha256.h"

int main() {
    const vector<uint8_t> aa{1, 1, 3};
    const auto a = SHA256::toHashSha256(aa);
    SHA256::printHashAsString(a);
    return 0;
}
