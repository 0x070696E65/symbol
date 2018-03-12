#include "LocalNodeApiTraits.h"
#include "catapult/model/EntityHasher.h"
#include "tests/test/core/mocks/MockMemoryBasedStorage.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	namespace {
		constexpr Height Invalid_Height(123456);
	}

	// region ChainInfoApiTraits

	ChainInfoApiTraits::RequestType ChainInfoApiTraits::InitiateValidRequest(const api::RemoteChainApi& remoteChainApi) {
		return remoteChainApi.chainInfo();
	}

	void ChainInfoApiTraits::VerifyResult(const ResultType& info) {
		EXPECT_EQ(Height(1), info.Height);
		EXPECT_EQ(model::ChainScore(0), info.Score);
	}

	// endregion

	// region HashesFromApiTraits

	HashesFromApiTraits::RequestType HashesFromApiTraits::InitiateValidRequest(const api::RemoteChainApi& remoteChainApi) {
		return remoteChainApi.hashesFrom(Height(1));
	}

	HashesFromApiTraits::RequestType HashesFromApiTraits::InitiateInvalidRequest(const api::RemoteChainApi& remoteChainApi) {
		return remoteChainApi.hashesFrom(Height(0));
	}

	void HashesFromApiTraits::VerifyResult(const ResultType& hashes) {
		mocks::MockMemoryBasedStorage storage;
		auto pBlock = storage.loadBlock(Height(1));
		auto expectedHash = model::CalculateHash(*pBlock);

		// checks ONLY first returned hash...
		ASSERT_EQ(1u, hashes.size());
		EXPECT_EQ(expectedHash, *hashes.cbegin());
	}

	// endregion

	// region BlockAtApiTraits

	BlockAtApiTraits::RequestType BlockAtApiTraits::InitiateValidRequest(const api::RemoteChainApi& remoteChainApi) {
		return remoteChainApi.blockAt(Height(1));
	}

	BlockAtApiTraits::RequestType BlockAtApiTraits::InitiateInvalidRequest(const api::RemoteChainApi& remoteChainApi) {
		return remoteChainApi.blockAt(Invalid_Height);
	}

	void BlockAtApiTraits::VerifyResult(const ResultType& pBlock) {
		ASSERT_TRUE(!!pBlock);

		mocks::MockMemoryBasedStorage storage;
		auto pExpectedBlock = storage.loadBlock(Height(1));
		EXPECT_EQ(pExpectedBlock->Size, pBlock->Size);
		EXPECT_EQ(*pExpectedBlock, *pBlock);
	}

	// endregion

	// region BlockLastApiTraits

	BlockLastApiTraits::RequestType BlockLastApiTraits::InitiateValidRequest(const api::RemoteChainApi& remoteChainApi) {
		return remoteChainApi.blockLast();
	}

	void BlockLastApiTraits::VerifyResult(const ResultType& pBlock) {
		// right now this test is very artificial, it might be improved when we'll be able to push block
		ASSERT_TRUE(!!pBlock);

		mocks::MockMemoryBasedStorage storage;
		auto pExpectedBlock = storage.loadBlock(Height(1));
		EXPECT_EQ(pExpectedBlock->Size, pBlock->Size);
		EXPECT_EQ(*pExpectedBlock, *pBlock);
	}

	// endregion

	// region BlocksFromApiTraits

	BlocksFromApiTraits::RequestType BlocksFromApiTraits::InitiateValidRequest(const api::RemoteChainApi& remoteChainApi) {
		api::BlocksFromOptions options;
		options.NumBlocks = 10;
		options.NumBytes = 10 * 1024;
		return remoteChainApi.blocksFrom(Height(1), options);
	}

	void BlocksFromApiTraits::VerifyResult(const ResultType& blocks) {
		// right now this test is very artificial, it might be improved when we'll be able to push block
		EXPECT_EQ(1u, blocks.size());
	}

	// endregion
}}
