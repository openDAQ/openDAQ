#include <gtest/gtest.h>
#include <memory>
#include <bcrypt/BCrypt.hpp>

using BcryptTest = testing::Test;

TEST_F(BcryptTest, VerifyEmptyThirdParty)
{
    ASSERT_TRUE(BCrypt::validatePassword("", "$2a$12$PxQWlpp1oS81OvQ/1YX82eAwfBkbVdySNrZNRpSTiKzn3xfbAcrHC"));
    ASSERT_FALSE(BCrypt::validatePassword("wrong", "$2a$12$PxQWlpp1oS81OvQ/1YX82eAwfBkbVdySNrZNRpSTiKzn3xfbAcrHC"));

    ASSERT_TRUE(BCrypt::validatePassword("", "$2a$08$fR99UepkZlTqQCkqrH/Ssu4Z0cG/0WRmcjhjXsU0xaMvS0VAjUyLi"));
    ASSERT_FALSE(BCrypt::validatePassword("wrong", "$2a$08$fR99UepkZlTqQCkqrH/Ssu4Z0cG/0WRmcjhjXsU0xaMvS0VAjUyLi"));

    ASSERT_TRUE(BCrypt::validatePassword("", "$2a$14$JchEIKAoBx7wEpYUnmaAYO5LZYJ2m2CmWUEAsMdknyYe50L0.FF1G"));
    ASSERT_FALSE(BCrypt::validatePassword("wrong", "$2a$14$JchEIKAoBx7wEpYUnmaAYO5LZYJ2m2CmWUEAsMdknyYe50L0.FF1G"));
}

TEST_F(BcryptTest, VerifySimpleThirdParty)
{
    ASSERT_TRUE(BCrypt::validatePassword("password", "$2a$10$ypr17Uvai2oVnRtYM8UzRO/0EbF7/IwEPctzOWfdI7pW6SfLQieZq"));
    ASSERT_FALSE(BCrypt::validatePassword("password_wrong", "$2a$10$ypr17Uvai2oVnRtYM8UzRO/0EbF7/IwEPctzOWfdI7pW6SfLQieZq"));

    ASSERT_TRUE(BCrypt::validatePassword("Hakuna Matata", "$2a$10$ZC9tsb.AScI80zXdm5jfMeNlRzB318L/0j3hIPfctK10bOVQNDEWu"));
    ASSERT_FALSE(BCrypt::validatePassword("Matata Hakuna", "$2a$10$ZC9tsb.AScI80zXdm5jfMeNlRzB318L/0j3hIPfctK10bOVQNDEWu"));

    ASSERT_TRUE(BCrypt::validatePassword("guujBFSgF9phbLubaA3N", "$2a$10$VwVeE.IvLLeHTp.ahaNBBuEfvb.Rz9sXs48Wzlh0WBCuLC.181FVW"));
    ASSERT_FALSE(BCrypt::validatePassword("guujBFSgF9phbLubaA3N_wrong", "$2a$10$VwVeE.IvLLeHTp.ahaNBBuEfvb.Rz9sXs48Wzlh0WBCuLC.181FVW"));
}

TEST_F(BcryptTest, VerifyLongThirdParty)
{
    // Passwords over 72 characters get truncated as a limitation of Bcrypt

    ASSERT_TRUE(BCrypt::validatePassword("YiiS062PLKe9cVWZuT7Bkeucfwhh4xy9gKM1Xp9p4armXNYY26FiMqK2X0F0CEtBJ5Qk2vA4",
                                         "$2a$12$W1W99tBTeBrjRo/nuxuDXei5MvN3yBJMglEvrE3uJXcce1cM30IoO"));
    ASSERT_TRUE(BCrypt::validatePassword("YiiS062PLKe9cVWZuT7Bkeucfwhh4xy9gKM1Xp9p4armXNYY26FiMqK2X0F0CEtBJ5Qk2vA4_wrong",
                                         "$2a$12$W1W99tBTeBrjRo/nuxuDXei5MvN3yBJMglEvrE3uJXcce1cM30IoO"));
    ASSERT_FALSE(BCrypt::validatePassword("wrong_YiiS062PLKe9cVWZuT7Bkeucfwhh4xy9gKM1Xp9p4armXNYY26FiMqK2X0F0CEtBJ5Qk2vA4",
                                          "$2a$12$W1W99tBTeBrjRo/nuxuDXei5MvN3yBJMglEvrE3uJXcce1cM30IoO"));

    ASSERT_TRUE(BCrypt::validatePassword("Guy)eyJ+}6SuBdVe0TpD%H]7j.B,PZjicW}qyMbSpf..*LvuFS,g9HS6x?mRq.bWmAy?_4a73d@k6*AD",
                                         "$2a$12$kHPG8ZFJG8k/OqdLGWC31u0sT/.pozQ81Ef90KwfxfgkJgwbLo2a."));
    ASSERT_TRUE(BCrypt::validatePassword("Guy)eyJ+}6SuBdVe0TpD%H]7j.B,PZjicW}qyMbSpf..*LvuFS,g9HS6x?mRq.bWmAy?_4a73d@k6*AD_wrong",
                                         "$2a$12$kHPG8ZFJG8k/OqdLGWC31u0sT/.pozQ81Ef90KwfxfgkJgwbLo2a."));
    ASSERT_FALSE(BCrypt::validatePassword("wrong_Guy)eyJ+}6SuBdVe0TpD%H]7j.B,PZjicW}qyMbSpf..*LvuFS,g9HS6x?mRq.bWmAy?_4a73d@k6*AD",
                                          "$2a$12$kHPG8ZFJG8k/OqdLGWC31u0sT/.pozQ81Ef90KwfxfgkJgwbLo2a."));
}

TEST_F(BcryptTest, VerifyUnicode)
{
    ASSERT_TRUE(BCrypt::validatePassword("open 🦆", "$2a$12$obqgprlebChvPKud9/P.uuiZtMPCRaqOkIwCXbxdAhRGCUpQrYBXq"));
    ASSERT_FALSE(BCrypt::validatePassword("open 🦆!", "$2a$12$obqgprlebChvPKud9/P.uuiZtMPCRaqOkIwCXbxdAhRGCUpQrYBXq"));

    ASSERT_TRUE(BCrypt::validatePassword("choco 🍌", "$2a$08$W7QE8LIAyPWzuxdAllSQuOo7EI7Q4zT/gCwnWCn5cnVkKUta4RZli"));
    ASSERT_FALSE(BCrypt::validatePassword("choco 🍌🍌", "$2a$08$W7QE8LIAyPWzuxdAllSQuOo7EI7Q4zT/gCwnWCn5cnVkKUta4RZli"));
}


TEST_F(BcryptTest, Verify2b)
{
    ASSERT_TRUE(BCrypt::validatePassword("hello world", "$2b$10$zIkqRe5A/lwIGwYzjmeWr.khpFZMo3N22qZhjoOokAdYn8413G.6q"));
    ASSERT_FALSE(BCrypt::validatePassword("hello world!", "$2b$10$zIkqRe5A/lwIGwYzjmeWr.khpFZMo3N22qZhjoOokAdYn8413G.6q"));

    ASSERT_TRUE(BCrypt::validatePassword("opendaq", "$2b$08$QtgqcJSJCBRPuFcsTrVXYeqtWBEwv9SKHWHghm4Avive6UUobtaTq"));
    ASSERT_FALSE(BCrypt::validatePassword("wrong", "$2b$08$QtgqcJSJCBRPuFcsTrVXYeqtWBEwv9SKHWHghm4Avive6UUobtaTq"));
}

TEST_F(BcryptTest, Generate2b)
{
    const std::string hash = BCrypt::generateHash("hello");
    ASSERT_EQ(hash.substr(0, 3), "$2b");
    ASSERT_TRUE(BCrypt::validatePassword("hello", hash));
    ASSERT_FALSE(BCrypt::validatePassword("hello!", hash));
}

TEST_F(BcryptTest, GenerateCheckEmpty)
{
    std::string password = "";

    {
        std::string hash = BCrypt::generateHash(password);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
    }

    {
        std::string hash = BCrypt::generateHash(password, 10);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
    }

    {
        std::string hash = BCrypt::generateHash(password, 8);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
    }
}

TEST_F(BcryptTest, GenerateCheckRandom)
{
    std::string password = "CVDtVBP4y17kSmJnDndc";

    {
        std::string hash = BCrypt::generateHash(password);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword(password + "!", hash));
    }

    {
        std::string hash = BCrypt::generateHash(password, 10);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword(password + "!", hash));

    }

    {
        std::string hash = BCrypt::generateHash(password, 8);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword(password + "!", hash));
    }
}

TEST_F(BcryptTest, GenerateCheckSimple)
{
    {
        std::string password = "passwrod";
        std::string hash = BCrypt::generateHash(password);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword(password + "!", hash));
    }

    {
        std::string password = "Hakuna Matata";
        std::string hash = BCrypt::generateHash(password);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword(password + "!", hash));
    }

    {
        std::string password = "It might work";
        std::string hash = BCrypt::generateHash(password);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword(password + "!", hash));
    }
}

TEST_F(BcryptTest, GenerateCheckLong)
{
    // Passwords over 72 characters get truncated as a limitation of Bcrypt

    std::string password = "SvXUbrVXc9ezCxKSYD4DT1nXHAcveAEFpXfJ6pQ2BTU3LCy8MMPu2Gtkcf0ETtR8rWr77wUHpF1uhwn6";
    std::string hash = BCrypt::generateHash(password);

    ASSERT_TRUE(BCrypt::validatePassword(password, hash));
    ASSERT_TRUE(BCrypt::validatePassword(password + "!", hash));
    ASSERT_FALSE(BCrypt::validatePassword("!" + password, hash));
}


TEST_F(BcryptTest, GenerateCheckUnicode)
{
    {
        auto password = u8"open 🦆";
        auto hash = BCrypt::generateHash(password);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword("open", hash));
    }

    {
        auto password = u8"choco 🍌";
        auto hash = BCrypt::generateHash(password);
        ASSERT_TRUE(BCrypt::validatePassword(password, hash));
        ASSERT_FALSE(BCrypt::validatePassword("choco 🍌🍌", hash));
    }
}
