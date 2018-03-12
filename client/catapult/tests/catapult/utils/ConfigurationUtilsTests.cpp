#include "catapult/utils/ConfigurationUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace utils {

#define TEST_CLASS ConfigurationUtilsTests

	// region GetIniPropertyName

	TEST(TEST_CLASS, GetIniPropertyNameThrowsIfCppVariableNameIsTooShort) {
		// Act + Assert:
		EXPECT_THROW(GetIniPropertyName(nullptr), catapult_invalid_argument);
		EXPECT_THROW(GetIniPropertyName(""), catapult_invalid_argument);
		EXPECT_THROW(GetIniPropertyName("a"), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, GetIniPropertyNameThrowsIfCppVariableNameDoesNotStartWithLetter) {
		// Act + Assert:
		EXPECT_THROW(GetIniPropertyName("0abcd"), catapult_invalid_argument);
		EXPECT_THROW(GetIniPropertyName("9abcd"), catapult_invalid_argument);
		EXPECT_THROW(GetIniPropertyName("!abcd"), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, GetIniPropertyNameCanConvertValidCppVariableNames) {
		// Act + Assert:
		// - min length
		EXPECT_EQ("aa", GetIniPropertyName("aa"));
		EXPECT_EQ("zZ", GetIniPropertyName("ZZ"));

		// - min start letter
		EXPECT_EQ("alpha", GetIniPropertyName("alpha"));
		EXPECT_EQ("alpha", GetIniPropertyName("Alpha"));

		// - max start letter
		EXPECT_EQ("zeta", GetIniPropertyName("zeta"));
		EXPECT_EQ("zeta", GetIniPropertyName("Zeta"));

		// - other
		EXPECT_EQ("fooBar", GetIniPropertyName("fooBar"));
		EXPECT_EQ("fooBar", GetIniPropertyName("FooBar"));
		EXPECT_EQ("invalid IDENTIFIER 1234!", GetIniPropertyName("Invalid IDENTIFIER 1234!"));
	}

	// endregion

	// region LoadIniProperty

	TEST(TEST_CLASS, LoadIniPropertyThrowsIfCppVariableNameIsInvalid) {
		// Arrange:
		auto bag = ConfigurationBag({{ "foo", { { "0baz", "1234" } } }});

		// Act + Assert:
		uint32_t value;
		EXPECT_THROW(LoadIniProperty(bag, "foo", "0baz", value), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, LoadIniPropertyThrowsIfBagDoesNotContainKey) {
		// Arrange:
		auto bag = ConfigurationBag({{ "foo", { { "baz", "1234" } } }});

		// Act + Assert:
		uint32_t value;
		EXPECT_THROW(LoadIniProperty(bag, "foo", "bar", value), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, LoadIniPropertyLoadsPropertyGivenValidKey) {
		// Arrange:
		auto bag = ConfigurationBag({{ "foo", { { "bar", "1234" } } }});

		// Act:
		uint32_t value;
		LoadIniProperty(bag, "foo", "bar", value);

		// Assert:
		EXPECT_EQ(1234, value);
	}

	// endregion

	// region VerifyBagSizeLte

	namespace {
		ConfigurationBag CreateBagForVerifyBagSizeTests() {
			return ConfigurationBag({
				{ "foo", { { "bar", "1234" }, { "baz", "2345" }, { "bax", "2345" } } },
				{ "greek", { { "zeta", "55" }, { "alpha", "7" } } }
			});
		}
	}

	TEST(TEST_CLASS, VerifyBagSizeLteDoesNotThrowIfBagSizeIsLessThanOrEqualToExpectedSize) {
		// Arrange:
		auto bag = CreateBagForVerifyBagSizeTests();

		// Act: no exceptions
		VerifyBagSizeLte(bag, 5);
		VerifyBagSizeLte(bag, 6);
		VerifyBagSizeLte(bag, 100);
	}

	TEST(TEST_CLASS, VerifyBagSizeLteThrowsIfBagSizeIsGreaterThanExpectedSize) {
		// Arrange:
		auto bag = CreateBagForVerifyBagSizeTests();

		// Act + Assert:
		EXPECT_THROW(VerifyBagSizeLte(bag, 0), catapult_invalid_argument);
		EXPECT_THROW(VerifyBagSizeLte(bag, 1), catapult_invalid_argument);
		EXPECT_THROW(VerifyBagSizeLte(bag, 4), catapult_invalid_argument);
	}

	// endregion

	// region ExtractSectionAsBag

	TEST(TEST_CLASS, ExtractSectionAsBagCanExtractKnownSectionAsBag) {
		// Arrange:
		auto bag = ConfigurationBag({
			{ "foo", { { "alpha", "123" } } },
			{ "bar", { { "alpha", "987" }, { "beta", "abc" } } }
		});

		// Act:
		auto fooBag = ExtractSectionAsBag(bag, "foo");
		auto barBag = ExtractSectionAsBag(bag, "bar");

		// Assert:
		EXPECT_EQ(1u, fooBag.size());
		EXPECT_EQ(1u, fooBag.size(""));
		EXPECT_EQ(123u, fooBag.get<uint64_t>({ "", "alpha" }));

		EXPECT_EQ(2u, barBag.size());
		EXPECT_EQ(2u, barBag.size(""));
		EXPECT_EQ(987u, barBag.get<uint64_t>({ "", "alpha" }));
		EXPECT_EQ("abc", barBag.get<std::string>({ "", "beta" }));
	}

	TEST(TEST_CLASS, ExtractSectionAsBagCanExtractUnknownSectionAsEmptyBag) {
		// Arrange:
		auto bag = ConfigurationBag({});

		// Act:
		auto fooBag = ExtractSectionAsBag(bag, "foo");

		// Assert:
		EXPECT_EQ(0u, fooBag.size());
	}

	// endregion

	// region ExtractSectionAsUnorderedSet

	TEST(TEST_CLASS, ExtractSectionAsUnorderedSetCanExtractKnownSectionAsUnorderedSet) {
		// Arrange:
		auto bag = ConfigurationBag({
			{ "none", { { "alpha", "false" }, { "beta", "false" }, { "gamma", "false" } } },
			{ "some", { { "alpha", "true" }, { "beta", "false" }, { "gamma", "true" } } },
			{ "all", { { "alpha", "true" }, { "beta", "true" }, { "gamma", "true" } } }
		});

		// Act:
		auto noneResultPair = ExtractSectionAsUnorderedSet(bag, "none");
		auto someResultPair = ExtractSectionAsUnorderedSet(bag, "some");
		auto allResultPair = ExtractSectionAsUnorderedSet(bag, "all");

		// Assert:
		EXPECT_TRUE(noneResultPair.first.empty());
		EXPECT_EQ(3u, noneResultPair.second);

		EXPECT_EQ(std::unordered_set<std::string>({ "alpha", "gamma" }), someResultPair.first);
		EXPECT_EQ(3u, someResultPair.second);

		EXPECT_EQ(std::unordered_set<std::string>({ "alpha", "beta", "gamma" }), allResultPair.first);
		EXPECT_EQ(3u, allResultPair.second);
	}

	TEST(TEST_CLASS, ExtractSectionAsUnorderedSetFailsIfAnyValueIsNotBoolean) {
		// Arrange:
		auto bag = ConfigurationBag({
			{ "foo", { { "alpha", "true" }, { "beta", "1" }, { "gamma", "true" } } }
		});

		// Act + Assert:
		EXPECT_THROW(ExtractSectionAsUnorderedSet(bag, "foo"), property_malformed_error);
	}

	TEST(TEST_CLASS, ExtractSectionAsUnorderedSetCanExtractUnknownSectionAsEmptyUnorderedSet) {
		// Arrange:
		auto bag = ConfigurationBag({});

		// Act:
		auto resultPair = ExtractSectionAsUnorderedSet(bag, "foo");

		// Assert:
		EXPECT_TRUE(resultPair.first.empty());
		EXPECT_EQ(0u, resultPair.second);
	}

	// endregion
}}
