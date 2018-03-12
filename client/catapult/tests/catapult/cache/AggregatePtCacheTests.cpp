#include "catapult/cache/AggregatePtCache.h"
#include "catapult/model/Cosignature.h"
#include "tests/catapult/cache/test/AggregateTransactionsCacheTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS AggregatePtCacheTests

	namespace {
		// region basic mocks

		class UnsupportedPtCacheModifier : public PtCacheModifier {
		public:
			bool add(const model::DetachedTransactionInfo&) override {
				CATAPULT_THROW_RUNTIME_ERROR("add - not supported in mock");
			}

			model::DetachedTransactionInfo add(const Hash256&, const Key&, const Signature&) override {
				CATAPULT_THROW_RUNTIME_ERROR("add(cosignature) - not supported in mock");
			}

			model::DetachedTransactionInfo remove(const Hash256&) override {
				CATAPULT_THROW_RUNTIME_ERROR("remove - not supported in mock");
			}

			std::vector<model::DetachedTransactionInfo> prune(Timestamp) override {
				CATAPULT_THROW_RUNTIME_ERROR("prune - not supported in mock");
			}

			std::vector<model::DetachedTransactionInfo> prune(const predicate<const Hash256&>&) override {
				CATAPULT_THROW_RUNTIME_ERROR("prune - not supported in mock");
			}
		};

		template<typename TPtCacheModifier>
		using MockPtCache = test::MockTransactionsCache<PtCache, TPtCacheModifier, PtCacheModifierProxy>;

		struct FlushInfo {
		public:
			size_t NumAdds;
			size_t NumCosignatureAdds;
			size_t NumRemoves;

		public:
			constexpr bool operator==(const FlushInfo& rhs) const {
				return NumAdds == rhs.NumAdds && NumCosignatureAdds == rhs.NumCosignatureAdds && NumRemoves == rhs.NumRemoves;
			}
		};

		class MockPtChangeSubscriber : public test::MockTransactionsChangeSubscriber<PtChangeSubscriber, FlushInfo> {
		private:
			using CosignatureInfo = std::pair<std::unique_ptr<const model::TransactionInfo>, model::Cosignature>;

		public:
			const std::vector<CosignatureInfo>& addedCosignatureInfos() const {
				return m_addedCosignatureInfos;
			}

		public:
			void notifyAddPartials(const TransactionInfos& transactionInfos) override {
				for (const auto& transactionInfo : transactionInfos)
					m_addedInfos.push_back(transactionInfo.copy());
			}

			void notifyRemovePartials(const TransactionInfos& transactionInfos) override {
				for (const auto& transactionInfo : transactionInfos)
					m_removedInfos.push_back(transactionInfo.copy());
			}

		public:
			void notifyAddCosignature(
					const model::TransactionInfo& parentTransactionInfo,
					const Key& signer,
					const Signature& signature) override {
				m_addedCosignatureInfos.emplace_back(
						std::make_unique<model::TransactionInfo>(parentTransactionInfo.copy()),
						model::Cosignature{ signer, signature });
			}

		private:
			FlushInfo createFlushInfo() const override {
				return { m_addedInfos.size(), m_addedCosignatureInfos.size(), m_removedInfos.size() };
			}

		private:
			std::vector<CosignatureInfo> m_addedCosignatureInfos;
		};

		// endregion

		struct PtTraits {
			using CacheType = PtCache;
			using ChangeSubscriberType = MockPtChangeSubscriber;
			using UnsupportedChangeSubscriberType = test::UnsupportedPtChangeSubscriber<test::UnsupportedFlushBehavior::Throw>;

			template<typename TModifier>
			using MockCacheType = MockPtCache<TModifier>;

			static constexpr auto CreateAggregateCache = CreateAggregatePtCache;
		};

		template<typename TModifier>
		using TestContext = test::TransactionsCacheTestContext<MockPtCache<TModifier>, PtTraits>;

		model::TransactionInfo StripMerkle(const model::TransactionInfo& transactionInfo) {
			// subscriber receives a TransactionInfo with a zeroed out merkle component hash
			// (because PtCache does not support merkle component hashes)
			auto infoCopy = transactionInfo.copy();
			infoCopy.MerkleComponentHash = Hash256();
			return infoCopy;
		}

		struct BasicTestsPtTraits {
		public:
			using CacheTraitsType = PtTraits;
			using UnsupportedCacheModifierType = UnsupportedPtCacheModifier;
			using TransactionInfoType = model::DetachedTransactionInfo;

			template<typename TModifier>
			using TestContextType = TestContext<TModifier>;

		public:
			static FlushInfo CreateFlushInfo(size_t numAdds, size_t numRemoves) {
				return { numAdds, 0, numRemoves };
			}

			static TransactionInfoType Copy(const TransactionInfoType& info) {
				return info.copy();
			}

			static model::TransactionInfo ToSubscriberInfo(const model::TransactionInfo& transactionInfo) {
				return StripMerkle(transactionInfo);
			}
		};
	}

	// region basic tests (add / remove / flush)

	DEFINE_AGGREGATE_TRANSACTIONS_CACHE_TESTS(TEST_CLASS, BasicTestsPtTraits);

	// endregion

	// region add(cosignature)

	namespace {
		class MockAddCosignaturePtCacheModifier : public UnsupportedPtCacheModifier {
		public:
			using CosignatureInfo = std::pair<Hash256, model::Cosignature>;

		public:
			explicit MockAddCosignaturePtCacheModifier(
					std::vector<CosignatureInfo>& cosignatureInfos,
					const model::DetachedTransactionInfo& transactionInfo)
					: m_cosignatureInfos(cosignatureInfos)
					, m_transactionInfo(transactionInfo.copy())
			{}

		public:
			model::DetachedTransactionInfo add(const Hash256& parentHash, const Key& signer, const Signature& signature) override {
				m_cosignatureInfos.emplace_back(parentHash, model::Cosignature{ signer, signature });
				return m_transactionInfo.copy();
			}

		private:
			std::vector<CosignatureInfo>& m_cosignatureInfos;
			model::DetachedTransactionInfo m_transactionInfo;
		};
	}

	TEST(TEST_CLASS, AddCosignatureDelegatesToCacheAndSubscriberOnCacheSuccess) {
		// Arrange:
		std::vector<MockAddCosignaturePtCacheModifier::CosignatureInfo> cosignatureInfos;
		auto transactionInfo = test::CreateRandomTransactionInfo();
		TestContext<MockAddCosignaturePtCacheModifier> context(cosignatureInfos, transactionInfo);

		auto parentHash = test::GenerateRandomData<Hash256_Size>();
		auto cosignature = model::Cosignature{ test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Signature_Size>() };

		// Act: add via modifier, which flushes when destroyed
		auto transactionInfoFromAdd = context.aggregate().modifier().add(parentHash, cosignature.Signer, cosignature.Signature);

		// Assert:
		test::AssertEqual(transactionInfo, transactionInfoFromAdd, "info from add");

		// - check pt cache modifier was called as expected
		ASSERT_EQ(1u, cosignatureInfos.size());
		EXPECT_EQ(parentHash, cosignatureInfos[0].first);
		EXPECT_EQ(cosignature.Signer, cosignatureInfos[0].second.Signer);
		EXPECT_EQ(cosignature.Signature, cosignatureInfos[0].second.Signature);

		// - check subscriber
		ASSERT_EQ(1u, context.subscriber().addedCosignatureInfos().size());
		const auto& addedCosignatureInfo = context.subscriber().addedCosignatureInfos()[0];
		test::AssertEqual(StripMerkle(transactionInfo), *addedCosignatureInfo.first, "info from subscriber");
		EXPECT_EQ(cosignature.Signer, addedCosignatureInfo.second.Signer);
		EXPECT_EQ(cosignature.Signature, addedCosignatureInfo.second.Signature);

		ASSERT_EQ(1u, context.subscriber().flushInfos().size());
		EXPECT_EQ(FlushInfo({ 0u, 1u, 0u }), context.subscriber().flushInfos()[0]);
	}

	TEST(TEST_CLASS, AddCosignatureDelegatesToCacheOnlyOnCacheFailure) {
		// Arrange:
		std::vector<MockAddCosignaturePtCacheModifier::CosignatureInfo> cosignatureInfos;
		TestContext<MockAddCosignaturePtCacheModifier> context(cosignatureInfos, model::TransactionInfo());

		auto parentHash = test::GenerateRandomData<Hash256_Size>();
		auto cosignature = model::Cosignature{ test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Signature_Size>() };

		// Act: add via modifier, which flushes when destroyed
		auto transactionInfoFromAdd = context.aggregate().modifier().add(parentHash, cosignature.Signer, cosignature.Signature);

		// Assert:
		EXPECT_FALSE(!!transactionInfoFromAdd);

		// - check pt cache modifier was called as expected
		ASSERT_EQ(1u, cosignatureInfos.size());
		EXPECT_EQ(parentHash, cosignatureInfos[0].first);
		EXPECT_EQ(cosignature.Signer, cosignatureInfos[0].second.Signer);
		EXPECT_EQ(cosignature.Signature, cosignatureInfos[0].second.Signature);

		// - check subscriber
		ASSERT_EQ(1u, context.subscriber().flushInfos().size());
		EXPECT_EQ(FlushInfo({ 0u, 0u, 0u }), context.subscriber().flushInfos()[0]);
	}

	// endregion

	// region prune (timestamp)

	namespace {
		class MockPruneTimestampPtCacheModifier : public UnsupportedPtCacheModifier {
		public:
			explicit MockPruneTimestampPtCacheModifier(
					std::vector<Timestamp>& timestamps,
					std::vector<model::DetachedTransactionInfo>&& transactionInfos)
					: m_timestamps(timestamps)
					, m_transactionInfos(std::move(transactionInfos))
			{}

		public:
			std::vector<model::DetachedTransactionInfo> prune(Timestamp timestamp) override {
				m_timestamps.push_back(timestamp);
				return std::move(m_transactionInfos);
			}

		private:
			std::vector<Timestamp>& m_timestamps;
			std::vector<model::DetachedTransactionInfo> m_transactionInfos;
		};

		std::vector<model::DetachedTransactionInfo> ToDetachedTransactionInfos(
				const std::vector<model::TransactionInfo>& transactionInfosWithMerkleHashes) {
			std::vector<model::DetachedTransactionInfo> transactionInfos;
			for (const auto& transactionInfo : transactionInfosWithMerkleHashes)
				transactionInfos.push_back(transactionInfo.copy());

			return transactionInfos;
		}

		std::vector<model::TransactionInfo> StripMerkles(const std::vector<model::TransactionInfo>& transactionInfosWithMerkleHashes) {
			std::vector<model::TransactionInfo> transactionInfos;
			for (auto& transactionInfo : transactionInfosWithMerkleHashes)
				transactionInfos.push_back(StripMerkle(transactionInfo));

			return transactionInfos;
		}
	}

	TEST(TEST_CLASS, PruneTimestampDelegatesToCacheOnlyWhenCacheIsEmpty) {
		// Arrange:
		std::vector<Timestamp> pruneTimestamps;
		TestContext<MockPruneTimestampPtCacheModifier> context(pruneTimestamps, std::vector<model::DetachedTransactionInfo>());

		// Act:
		auto prunedInfos = context.aggregate().modifier().prune(Timestamp(123));

		// Assert:
		EXPECT_TRUE(prunedInfos.empty());

		// - check pt cache modifier was called as expected
		ASSERT_EQ(1u, pruneTimestamps.size());
		EXPECT_EQ(Timestamp(123), pruneTimestamps[0]);

		// - check subscriber
		ASSERT_EQ(1u, context.subscriber().flushInfos().size());
		EXPECT_EQ(FlushInfo({ 0u, 0u, 0u }), context.subscriber().flushInfos()[0]);
	}

	TEST(TEST_CLASS, PruneTimestampDelegatesToCacheAndSubscriberWhenCacheIsNotEmpty) {
		// Arrange:
		std::vector<Timestamp> pruneTimestamps;
		auto transactionInfos = test::CreateTransactionInfos(5);
		auto transactionInfosWithoutMerkleHashes = ToDetachedTransactionInfos(transactionInfos);
		TestContext<MockPruneTimestampPtCacheModifier> context(pruneTimestamps, ToDetachedTransactionInfos(transactionInfos));

		// Act:
		auto prunedInfos = context.aggregate().modifier().prune(Timestamp(123));

		// Assert:
		ASSERT_EQ(5u, prunedInfos.size());
		for (auto i = 0u; i < transactionInfosWithoutMerkleHashes.size(); ++i)
			test::AssertEqual(transactionInfosWithoutMerkleHashes[i], prunedInfos[i], "info from prune " + std::to_string(i));

		// - check pt cache modifier was called as expected
		ASSERT_EQ(1u, pruneTimestamps.size());
		EXPECT_EQ(Timestamp(123), pruneTimestamps[0]);

		// - check subscriber
		ASSERT_EQ(5u, context.subscriber().removedInfos().size());
		test::AssertEquivalent(StripMerkles(transactionInfos), context.subscriber().removedInfos(), "subscriber infos");

		ASSERT_EQ(1u, context.subscriber().flushInfos().size());
		EXPECT_EQ(FlushInfo({ 0u, 0u, 5u }), context.subscriber().flushInfos()[0]);
	}

	// endregion

	// region prune (predicate)

	namespace {
		class MockPrunePredicatePtCacheModifier : public UnsupportedPtCacheModifier {
		public:
			explicit MockPrunePredicatePtCacheModifier(std::vector<model::DetachedTransactionInfo>&& transactionInfos)
					: m_transactionInfos(std::move(transactionInfos))
			{}

		public:
			std::vector<model::DetachedTransactionInfo> prune(const predicate<const Hash256&>& hashPredicate) override {
				hashPredicate(Hash256());
				return std::move(m_transactionInfos);
			}

		private:
			std::vector<model::DetachedTransactionInfo> m_transactionInfos;
		};
	}

	TEST(TEST_CLASS, PrunePredicateDelegatesToCacheOnlyWhenCacheIsEmpty) {
		// Arrange:
		auto transactionInfos = std::vector<model::DetachedTransactionInfo>();
		TestContext<MockPrunePredicatePtCacheModifier> context(std::move(transactionInfos));
		auto numPredicateCalls = 0u;

		// Act:
		auto prunedInfos = context.aggregate().modifier().prune([&numPredicateCalls](const auto&) {
			++numPredicateCalls;
			return true;
		});

		// Assert:
		EXPECT_TRUE(prunedInfos.empty());

		// - check pt cache modifier was called as expected
		EXPECT_EQ(1u, numPredicateCalls);

		// - check subscriber
		ASSERT_EQ(1u, context.subscriber().flushInfos().size());
		EXPECT_EQ(FlushInfo({ 0u, 0u, 0u }), context.subscriber().flushInfos()[0]);
	}

	TEST(TEST_CLASS, PrunePredicateDelegatesToCacheAndSubscriberWhenCacheIsNotEmpty) {
		// Arrange:
		auto transactionInfos = test::CreateTransactionInfos(5);
		auto transactionInfosWithoutMerkleHashes = ToDetachedTransactionInfos(transactionInfos);
		TestContext<MockPrunePredicatePtCacheModifier> context(ToDetachedTransactionInfos(transactionInfos));
		auto numPredicateCalls = 0u;

		// Act:
		auto prunedInfos = context.aggregate().modifier().prune([&numPredicateCalls](const auto&) {
			++numPredicateCalls;
			return true;
		});

		// Assert:
		ASSERT_EQ(5u, prunedInfos.size());
		for (auto i = 0u; i < transactionInfosWithoutMerkleHashes.size(); ++i)
			test::AssertEqual(transactionInfosWithoutMerkleHashes[i], prunedInfos[i], "info from prune " + std::to_string(i));

		// - check pt cache modifier was called as expected
		EXPECT_EQ(1u, numPredicateCalls);

		// - check subscriber
		ASSERT_EQ(5u, context.subscriber().removedInfos().size());
		test::AssertEquivalent(StripMerkles(transactionInfos), context.subscriber().removedInfos(), "subscriber infos");

		ASSERT_EQ(1u, context.subscriber().flushInfos().size());
		EXPECT_EQ(FlushInfo({ 0u, 0u, 5u }), context.subscriber().flushInfos()[0]);
	}

	// endregion
}}
