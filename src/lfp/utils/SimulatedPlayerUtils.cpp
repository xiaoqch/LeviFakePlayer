
#include "SimulatedPlayerUtils.h"
#include "md5.h"

namespace lfp::utils::sp_utils {


std::string xuidFromUuid(mce::UUID uuid) {
    int64_t v = ((uuid.a >> 16) ^ uuid.b) ^ ((uuid.b >> 16) * uuid.a);
    return std::to_string(-abs(v));
}
mce::UUID uuidFromName(std::string_view name) {
    return JAVA_nameUUIDFromBytes(fmt::format("LFP_{}", name));
}

// TODO: test needed
mce::UUID JAVA_nameUUIDFromBytes(std::string_view name) {
    MD5 md5;
    md5.add(name.data(), name.size());

    auto hash = md5.getHash();
    assert(hash.size() == 32);
    // hash[6] &= 0x0f;  /* clear version        */
    // hash[6] |= 0x30;  /* set to version 3     */
    // hash[8] &= 0x3f;  /* clear variant        */
    // hash[8] |= 0x80;  /* set to IETF variant  */

    hash[12] = '3'; /* set to version 3     */
    auto c   = (unsigned char)hash[16];
    assert(c >= '0');
    auto b = c - '0';
    if (b > 9) b = 10 + c - 'A';
    if (b > 15) b = 10 + c - 'a';
    b &= 0x3; /* clear variant        */
    b |= 0x8; /* set to IETF variant  */

    hash[16] = "0123456789abcdef"[b];

    hash.insert(20, 1, '-');
    hash.insert(16, 1, '-');
    hash.insert(12, 1, '-');
    hash.insert(8, 1, '-');
    return mce::UUID::fromString(hash);
}
} // namespace lfp::utils::sp_utils