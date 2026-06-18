#include <gtest/gtest.h>
#include <grid.h>
#include <base_block.h>
#include <test_utils.h>

using ::testing::FloatLE;
using ::testing::DoubleLE;
using namespace RLPlays;

TEST(UtilsTest, UtilsTest)
{
  EXPECT_RECT_EQ(GetCollisionRec({100, 100, 200, 200}, {150, 150, 100, 100}), {150, 150, 100, 100});
}


TEST(UtilsTest, Serializer)
{
  { // Convert to Hex.
    std::string s;
    EXPECT_EQ(UInt32ToString(0x1234, s), "4321");
    EXPECT_EQ(UInt32ToString(0x223344, s), "4321443322");
    s = "";
    EXPECT_EQ(UInt32ToString(0x0, s), "0");
    s = "";
    EXPECT_EQ(UInt32ToString(0xFEDCBA98, s), "89ABCDEF");
  }


  auto str = "1234,56.2,0,1.2,501";
  TSerializerOutput v;
  EXPECT_EQ(ParseInt32(str, v), 1234);
  EXPECT_EQ(ParseFloat(str, v), 56.2f);
  EXPECT_EQ(ParseDouble(str, v), double(0.0));
  EXPECT_EQ(ParseDouble(str, v), double(1.2));
  EXPECT_EQ(ParseUInt32(str, v), uint32_t(0x105));
}


TEST(UtilsTest, ParseStr)
{
  { // Len-based
    std::string s = "Hello World!";
    std::string out = "1,2,3,";
    EXPECT_EQ(EncodeStringWithLen(s, out), "1,2,3,C,Hello World!");
    out += ",";
    UInt32ToString(43, out) += ",";
    std::string s2 = "\t1\n2!,,";
    EXPECT_EQ(EncodeStringWithLen(s2, out), "1,2,3,C,Hello World!,B2,7," + s2);
    out += ",";
    UInt32ToString(42, out) += ",";


    TSerializerOutput v;
    EXPECT_EQ(ParseInt32(out, v), 1);
    EXPECT_EQ(ParseInt32(out, v), 2);
    EXPECT_EQ(ParseInt32(out, v), 3);
    EXPECT_EQ(DecodeStringWithLen(out, v), s);
    EXPECT_EQ(ParseUInt32(out, v), 43);
    EXPECT_EQ(DecodeStringWithLen(out, v), s2);
    EXPECT_EQ(ParseUInt32(out, v), 42);
  }
  { // Base64
    std::string s;
    EXPECT_EQ(UInt32ToString(0x1234, s), "4321");
    s += ",";
    EXPECT_EQ(ToBase64("Hello World!", s), "4321,SGVsbG8gV29ybGQh");

    TSerializerOutput v;

    EXPECT_EQ(ParseUInt32(s, v), 0x1234);
    auto output = FromBase64(s, v);
    EXPECT_EQ(output, "Hello World!");
  }
}
