#include <gtest/gtest.h>
#include <opcuashared/bcrypt.h>

using namespace daq::opcua;

/**
 * Test BCrypt class.
 *
 * One can generate more test cases with a help of the php script: commonlib/tests/tools/bcrypt_test_cases.php
 */

struct BCryptTestCase
{
    std::string password;
    std::string salt;
    std::string hash;
    int rounds;

    BCryptTestCase(std::string password, std::string salt, std::string hash, int rounds)
        : password(std::move(password))
        , salt(std::move(salt))
        , hash(std::move(hash))
        , rounds(rounds)
    {
    }
};

static bool compareHashes(const std::string& a, const std::string& b)
{
    // we should ignore bcrypt version when comparing hashes
    return a[0] == b[0] && a[1] == b[1] && strcmp(&a[3], &b[3]) == 0;
}

class BCryptTest : public testing::Test, public ::testing::WithParamInterface<BCryptTestCase>
{
protected:
    BCrypt bcrypt;
};


class BCryptTestHash : public BCryptTest
{
};

class BCryptTestVerify : public BCryptTest
{
};

TEST_P(BCryptTestHash, HashTest)
{
    BCryptTestCase testCase = GetParam();
    std::string hash = bcrypt.hash(testCase.password, testCase.salt, testCase.rounds);
    ASSERT_TRUE(compareHashes(hash, testCase.hash));
}

TEST_P(BCryptTestVerify, VerifyTest)
{
    BCryptTestCase testCase = GetParam();
    ASSERT_TRUE(BCrypt::Verify(testCase.hash, testCase.password));
    ASSERT_FALSE(BCrypt::Verify(testCase.hash, testCase.password + "wrong"));
}

INSTANTIATE_TEST_SUITE_P(
    BCryptTestHashAll,
    BCryptTestHash,
    ::testing::Values(
        BCryptTestCase("", "thisisasaltwhichis22ch", "$2y$06$thisisasaltwhichis22ceiWyJJQuCqQ6K/VMVNgl3WR2TnKke93O", 6),
        BCryptTestCase("", "thisisasaltwhichis22ch", "$2y$08$thisisasaltwhichis22cetIGsXYPjot9MEiTlOQSjSqsUhO/ncum", 8),
        BCryptTestCase("hello", "thisisasaltwhichis22ch", "$2y$06$thisisasaltwhichis22cenrvQa8HoAzcd4uM4IFyFZCnCW/K/Z8W", 6),
        BCryptTestCase("hello", "thisisasaltwhichis22ch", "$2y$08$thisisasaltwhichis22ceGSq7Gjk/l24CRv3kIGqHqO1FkjVJqfa", 8),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "thisisasaltwhichis22ch", "$2y$06$thisisasaltwhichis22cewx9N0qHf5lH1qhen4ZSkh9G.9vF7bpC", 6),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "thisisasaltwhichis22ch", "$2y$08$thisisasaltwhichis22cehPvOT6ZlqH0g2TnuEL4wXbzDyVzqtJK", 8),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "thisisasaltwhichis22ch", "$2y$06$thisisasaltwhichis22ceeL2tbWfYYl6VVS.6kwSgf4Afnv5UVWu", 6),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "thisisasaltwhichis22ch", "$2y$08$thisisasaltwhichis22cejWbLJJzXmMWFyRkQhHGqvFWPPnh1era", 8),
        BCryptTestCase("", "thisisasaltwhichis22chHello", "$2y$06$thisisasaltwhichis22ceiWyJJQuCqQ6K/VMVNgl3WR2TnKke93O", 6),
        BCryptTestCase("", "thisisasaltwhichis22chHello", "$2y$08$thisisasaltwhichis22cetIGsXYPjot9MEiTlOQSjSqsUhO/ncum", 8),
        BCryptTestCase("hello", "thisisasaltwhichis22chHello", "$2y$06$thisisasaltwhichis22cenrvQa8HoAzcd4uM4IFyFZCnCW/K/Z8W", 6),
        BCryptTestCase("hello", "thisisasaltwhichis22chHello", "$2y$08$thisisasaltwhichis22ceGSq7Gjk/l24CRv3kIGqHqO1FkjVJqfa", 8),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "thisisasaltwhichis22chHello", "$2y$06$thisisasaltwhichis22cewx9N0qHf5lH1qhen4ZSkh9G.9vF7bpC", 6),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "thisisasaltwhichis22chHello", "$2y$08$thisisasaltwhichis22cehPvOT6ZlqH0g2TnuEL4wXbzDyVzqtJK", 8),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "thisisasaltwhichis22chHello", "$2y$06$thisisasaltwhichis22ceeL2tbWfYYl6VVS.6kwSgf4Afnv5UVWu", 6),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "thisisasaltwhichis22chHello", "$2y$08$thisisasaltwhichis22cejWbLJJzXmMWFyRkQhHGqvFWPPnh1era", 8),
        BCryptTestCase("", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$06$MkjCFcpnaHo.ngQW8hDv1OE5e7EEsNBkPqjKjbeiAAGDZ32tMeoxi", 6),
        BCryptTestCase("", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$08$MkjCFcpnaHo.ngQW8hDv1Oq3XwM.TU1ux4xZH9SWTvEeO6PaRKJhq", 8),
        BCryptTestCase("hello", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$06$MkjCFcpnaHo.ngQW8hDv1OKSXItY4YgdUhR1XnCCIdVTG6/Z9Q8lq", 6),
        BCryptTestCase("hello", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$08$MkjCFcpnaHo.ngQW8hDv1OI06YNkstj1fnz7Hb7hMALmyKde/b6XC", 8),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$06$MkjCFcpnaHo.ngQW8hDv1OWBmePzneCphnun8DXjkg0xSyFELeGQK", 6),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$08$MkjCFcpnaHo.ngQW8hDv1Opd3VPU7yrnM2bcYLxKXnz5GseN./Ihe", 8),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$06$MkjCFcpnaHo.ngQW8hDv1O2bkSBvdwGDuLKRRKAKDrJjDaEHkvbbS", 6),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "MkjCFcpnaHo.ngQW8hDv1Y", "$2y$08$MkjCFcpnaHo.ngQW8hDv1Ow2pDUKWKd3hQQKpiPMMzJY12zfFs1OG", 8),
        BCryptTestCase("", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$06$V0bNN/hb14yIaLqiGzRq8.0q2LbRd5Bky7G/FobYdfIDZDt.3GfuS", 6),
        BCryptTestCase("", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$08$V0bNN/hb14yIaLqiGzRq8.F9Eco7RM.M4t2WJoqcHpbKg9GmpD0BK", 8),
        BCryptTestCase("hello", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$06$V0bNN/hb14yIaLqiGzRq8.h96biwYKLx4XkUR6uIq4MvxrGmMBo9q", 6),
        BCryptTestCase("hello", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$08$V0bNN/hb14yIaLqiGzRq8./LDV//ottPlTJbrHPKM9GPrnc9VvHky", 8),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$06$V0bNN/hb14yIaLqiGzRq8.Rvv5BWMw9VGPlDfCkMY9CgANdtm.cki", 6),
        BCryptTestCase("MpxzNrkTT)0OTBlU5TTY", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$08$V0bNN/hb14yIaLqiGzRq8.SiqvUcvyIVUuPmBOyv/h8iCzGwuzG2G", 8),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$06$V0bNN/hb14yIaLqiGzRq8.hRrCzpywIiN0DtynMdE4yrXf3cOwCJS", 6),
        BCryptTestCase("2xNRPxj8EJfcN.Yxghkyt54Z39}Ylz0X4LwanXJB3i2mk<2Qs<9gWivWhpUe", "V0bNN/hb14yIaLqiGzRq8HBnUg/f7Ord6Q2ENsN.X3RmPgQR8wCfFDwKXJ3E", "$2y$08$V0bNN/hb14yIaLqiGzRq8.fKNrgl454jT6cqUTmCNI67wil/qB.Ya", 8)
));

INSTANTIATE_TEST_SUITE_P(
    BCryptTestVerifyAll,
    BCryptTestVerify,
    ::testing::Values(
        BCryptTestCase("", "", "$2y$06$vHGBnsrghALFuir57FW6g.eelIYPnAvZCqMcixaPD0ge7dxFg6Ldy", 6),
        BCryptTestCase("", "", "$2y$08$RkQxUBJv0fMV32wHx1MWeu/79CQCvxNZbP.a2KkZG5D5PHsxguaM6", 8),
        BCryptTestCase("hello", "", "$2y$06$kM1HuyiWFB0RUnI5e2tlCuhNeO.NxUE/QIyBpWoC0Av7xSJ0CNgci", 6),
        BCryptTestCase("hello", "", "$2y$08$Q7KWcZNJfhG92UyuKj9aNesCozbY2nPde7vnNUrYbwW8cyISekTQu", 8),
        BCryptTestCase("QOzQ-xgq(4gF(aw1", "", "$2y$06$JOoIFeD2H6uLtcfZ7y1Nv.7086L6d.QFRL90e7tx4x7/dy0q4oNJ2", 6),
        BCryptTestCase("QOzQ-xgq(4gF(aw1", "", "$2y$08$bIbtUgz8hhye4Iw.gX2fc.yYQOik10qMMmue6i6ld6fJrr8VLMZn6", 8),
        BCryptTestCase("8dK>paPE26/9{Zb29VyU", "", "$2y$06$znnkg1eVKFniEnL0QacK6OMi60p67ynDrLmi7vTOcSu0pFiD8MUbm", 6),
        BCryptTestCase("8dK>paPE26/9{Zb29VyU", "", "$2y$08$nqwBRmGip.fSASDshXNKcOkHqa7BLndfZMjPAJT3CuKjxYUonTjbi", 8),
        BCryptTestCase("LYb>ycmZvhM.(u{U76HVMoXW)Sf7g<Rox{hT<nNt}}C0AU)(AB/WyiM{RVM0Mwfk", "", "$2y$06$rE0ZdcLzvZkI8eG1jBO6NOOGmJYfL7fdIxOKzXEwfIKBzztwmf9aa", 6),
        BCryptTestCase("LYb>ycmZvhM.(u{U76HVMoXW)Sf7g<Rox{hT<nNt}}C0AU)(AB/WyiM{RVM0Mwfk", "", "$2y$08$kAQ1s0GRZwyYMNYqKVdoHeG2KQtI2wT4Vi97o.yCOQVJJ7prfpj8i", 8)
));
