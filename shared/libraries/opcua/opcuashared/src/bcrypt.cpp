#include <opcuashared/bcrypt.h>
#include <random>
#include <stdexcept>
#include <array>
#include <limits>
#include <sstream>
#include <string>
#include <cstring>

extern "C"
{
    #include <opcuashared/bcrypt/crypt_blowfish.h>
}

BEGIN_NAMESPACE_OPENDAQ_OPCUA

BCrypt::BCrypt()
    : randomDist(std::uniform_int_distribution<int>(std::numeric_limits<char>::min(), std::numeric_limits<char>::max()))
{
}

std::vector<char> BCrypt::randBytes(unsigned int length)
{
    std::vector<char> bytes = std::vector<char>(length);
    for (unsigned int i = 0; i < length; i++)
    {
        bytes[i] = static_cast<char>(randomDist(randomDevice));
    }
    return bytes;
}

std::string BCrypt::hash(const std::string& password, unsigned int rounds)
{
    std::string salt = generateSalt(SaltPartSize);
    return hash(password, salt, rounds);
}

std::string BCrypt::hash(const std::string& password, std::string salt, unsigned int rounds) const
{
    if (salt.size() < SaltPartSize)
        throw BCryptSaltSizeException(SaltPartSize);

    char infoSalt[SaltSize + 1];
    infoSalt[0] = '$';
    infoSalt[1] = '2';
    infoSalt[2] = 'b';
    infoSalt[3] = '$';
    infoSalt[4] = '0' + (char) (rounds / 10);
    infoSalt[5] = '0' + (char) (rounds % 10);
    infoSalt[6] = '$';
    std::memcpy(&infoSalt[InfoPartSize], salt.data(), SaltPartSize);

    char hash[HashSize + 1];
    bool ok = _crypt_blowfish_rn(password.data(), infoSalt, &hash[0], HashSize + 1) != nullptr;
    if (!ok)
        throw BCryptException("Error while hashing the password.");

    return hash;
}

bool BCrypt::Verify(const std::string& hash, const std::string& password)
{
    char test[HashSize + 1];
    bool ok = _crypt_blowfish_rn(password.data(), hash.data(), test, HashSize + 1);
    if (!ok)
        return false;

    return hash == test;
}

std::string BCrypt::generateSalt(unsigned int size)
{
    std::vector<char> bytes = randBytes(size);
    return generateSalt(bytes.data(), (unsigned int) bytes.size());
}

std::string BCrypt::generateSalt(char* bytes, int size) const
{
    if (size < SaltPartSize)
        throw BCryptSaltSizeException(SaltPartSize);

    char salt[SaltSize + 1];
    bool ok = _crypt_gensalt_blowfish_rn("$2b$", 8, bytes, size, &salt[0], SaltSize + 1);
    if (!ok)
        throw BCryptException("Error while generating bcrypt salt.");

    return std::string(&salt[InfoPartSize]);
}

BCryptException::BCryptException(const std::string& message)
    : std::runtime_error(message.c_str())
{
}

BCryptSaltSizeException::BCryptSaltSizeException(unsigned int minSaltSize)
    : BCryptException("Salt must be at least " + std::to_string(minSaltSize) + " characters long.")
{
}

END_NAMESPACE_OPENDAQ_OPCUA
