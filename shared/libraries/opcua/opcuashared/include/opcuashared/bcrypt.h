/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    bcrypt.h
 * @author  Jan Mikolič
 * @date    23/04/2021
 * @version 1.0
 *
 * @brief Wrapper class for BCrypt password hashing algorithm.
 *
 */

#pragma once
#include <string>
#include <random>
#include <stdexcept>
#include <opcuashared/opcua.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

/**
 * Wrapper class for BCrypt password hashing algorithm.
 */
class BCrypt
{
private:
    static const int InfoPartSize = 7;
    static const int SaltPartSize = 22;
    static const int HashPartSize = 31;
    static const int SaltSize = InfoPartSize + SaltPartSize;
    static const int HashSize = InfoPartSize + SaltPartSize + HashPartSize;

    std::random_device randomDevice;
    std::uniform_int_distribution<int> randomDist;

    std::vector<char> randBytes(unsigned int length);

public:
    BCrypt();

    /**
     * Compute BCrypt hash of a password using random salt.
     * 
     * @param password The password in plain text to be hashed
     * @param rounds Base-2 logarithm of the iteration count for the underlying Blowfish-based hashing algorithm
     * @returns Hash string of the provided password
     * @throws {BCryptException} if blowfish algorithm failed to generate the hash
     */
    std::string hash(const std::string& password, unsigned int rounds = 10);

    /**
     * Compute BCrypt hash of a password using static salt. Try to avoid using this method, but if you must have a static salt, then
     * generate it with generateSalt(int size) method.
     * 
     * @param password The password in plain text to be hashed
     * @param salt The salt to be used for hashing the password
     * @param rounds Base-2 logarithm of the iteration count for the underlying Blowfish-based hashing algorithm
     * @returns Hash string of the provided password
     * @throws {BCryptException} if blowfish algorithm failed to generate the hash
     * @throws {BCryptSaltSizeException} if salt size is too small
     */
    std::string hash(const std::string& password, std::string salt, unsigned int rounds = 10) const;

    /**
     * Verifies if the password matches a hash.
     *
     * @param hash BCrypt hash of the password
     * @param password The password in plain text
     * @returns true if password matches the hash
     */
    static bool Verify(const std::string& hash, const std::string& password);

    /**
     * Generate a random salt string of the specified size.
     *
     * @param size The size of the salt to be generated
     * @returns Generated salt string
     * @throws {BCryptException} if blowfish algorithm failed to generate the salt
     * @throws {BCryptSaltSizeException} if size is too small
     */
    std::string generateSalt(unsigned int size);

    /**
     * Generate a salt string from provided byte array.
     *
     * @param bytes Byte array from which to generate the salt
     * @param size The size of the provided byte arry
     * @returns Generated salt string
     * @throws {BCryptException} if blowfish algorithm failed to generate the salt
     * @throws {BCryptSaltSizeException} if size is too small
     */
    std::string generateSalt(char* bytes, int size) const;
};

class BCryptException : public std::runtime_error
{
public:
    explicit BCryptException(const std::string& message);
};

class BCryptSaltSizeException : public BCryptException
{
public:
    explicit BCryptSaltSizeException(unsigned int minSaltSize);
};

END_NAMESPACE_OPENDAQ_OPCUA
