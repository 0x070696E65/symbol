#include "catapult/utils/HexParser.h"
#include "tests/TestHarness.h"
#include <array>
#include <vector>

namespace catapult { namespace utils {

#define TEST_CLASS HexParserTests

	namespace {
		struct ParseTraits {
			static uint8_t Parse(char ch1, char ch2) {
				return ParseByte(ch1, ch2);
			}

			static void AssertBadParse(char ch1, char ch2) {
				EXPECT_THROW(ParseByte(ch1, ch2), catapult_invalid_argument);
			}

			template<typename TContainer>
			static void ParseString(const char* const pHexData, size_t dataSize, TContainer& outputContainer) {
				ParseHexStringIntoContainer(pHexData, dataSize, outputContainer);
			}

			template<typename TContainer>
			static void AssertBadParse(const char* const pHexData, size_t dataSize, TContainer& outputContainer) {
				EXPECT_THROW(ParseHexStringIntoContainer(pHexData, dataSize, outputContainer), catapult_invalid_argument);
			}
		};

		struct TryParseTraits {
			static uint8_t Parse(char ch1, char ch2) {
				uint8_t by;
				EXPECT_TRUE(TryParseByte(ch1, ch2, by));
				return by;
			}

			static void AssertBadParse(char ch1, char ch2) {
				uint8_t by;
				EXPECT_FALSE(TryParseByte(ch1, ch2, by));
			}

			template<typename TContainer>
			static void ParseString(const char* const pHexData, size_t dataSize, TContainer& outputContainer) {
				EXPECT_TRUE(TryParseHexStringIntoContainer(pHexData, dataSize, outputContainer));
			}

			template<typename TContainer>
			static void AssertBadParse(const char* const pHexData, size_t dataSize, TContainer& outputContainer) {
				EXPECT_FALSE(TryParseHexStringIntoContainer(pHexData, dataSize, outputContainer));
			}
		};
	}

#define PARSE_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ParseTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Try) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<TryParseTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	PARSE_TRAITS_BASED_TEST(CanConvertAllValidHexCharCombinationsToByte) {
		// Arrange:
		std::vector<std::pair<char, uint8_t>> charToValueMappings;
		for (char ch = '0'; ch <= '9'; ++ch) charToValueMappings.push_back(std::make_pair(ch, static_cast<uint8_t>(ch - '0')));
		for (char ch = 'a'; ch <= 'f'; ++ch) charToValueMappings.push_back(std::make_pair(ch, static_cast<uint8_t>(ch - 'a' + 10)));
		for (char ch = 'A'; ch <= 'F'; ++ch) charToValueMappings.push_back(std::make_pair(ch, static_cast<uint8_t>(ch - 'A' + 10)));

		// Act:
		auto numTests = 0;
		for (const auto& pair1 : charToValueMappings) {
			for (const auto& pair2 : charToValueMappings) {
				// Act:
				uint8_t byte = TTraits::Parse(pair1.first, pair2.first);

				// Assert:
				uint8_t expected = pair1.second * 16 + pair2.second;
				EXPECT_EQ(expected, byte) << "input: " << pair1.first << pair2.first;
				++numTests;
			}
		}

		// Sanity:
		EXPECT_EQ(22 * 22, numTests);
	}

	PARSE_TRAITS_BASED_TEST(CannotConvertInvalidHexCharsToByte) {
		// Assert:
		TTraits::AssertBadParse('G', '6');
		TTraits::AssertBadParse('7', 'g');
		TTraits::AssertBadParse('*', '8');
		TTraits::AssertBadParse('9', '!');
	}

	PARSE_TRAITS_BASED_TEST(CanParseValidHexStringIntoContainer) {
		// Act:
		using ArrayType = std::array<uint8_t, 6>;
		ArrayType array;
		TTraits::ParseString("026ee415fc15", 12, array);

		// Assert:
		ArrayType expected{ { 0x02, 0x6E, 0xE4, 0x15, 0xFC, 0x15 } };
		EXPECT_EQ(expected, array);
	}

	PARSE_TRAITS_BASED_TEST(CanParseValidHexStringContainingAllValidHexCharsIntoContainer) {
		// Act:
		using ArrayType = std::array<uint8_t, 11>;
		ArrayType array;
		TTraits::ParseString("abcdef0123456789ABCDEF", 22, array);

		// Assert:
		ArrayType expected{ { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF } };
		EXPECT_EQ(expected, array);
	}

	PARSE_TRAITS_BASED_TEST(CannotParseHexStringWithInvalidHexCharsIntoContainer) {
		// Assert:
		std::array<uint8_t, 11> array;
		TTraits::AssertBadParse("abcdef012345G789ABCDEF", 22, array);
	}

	PARSE_TRAITS_BASED_TEST(CannotParseValidHexStringWithInvalidSizeIntoContainer) {
		// Assert: the only allowable size is 2 * 10 == 20
		std::array<uint8_t, 10> array;
		TTraits::AssertBadParse("abcdef0123456789ABCDEF", 18, array);
		TTraits::AssertBadParse("abcdef0123456789ABCDEF", 19, array);
		TTraits::AssertBadParse("abcdef0123456789ABCDEF", 21, array);
		TTraits::AssertBadParse("abcdef0123456789ABCDEF", 22, array);
	}
}}
