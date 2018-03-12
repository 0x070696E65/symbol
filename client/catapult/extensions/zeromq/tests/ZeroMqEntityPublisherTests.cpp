#include "zeromq/src/ZeroMqEntityPublisher.h"
#include "zeromq/src/PublisherUtils.h"
#include "catapult/model/Address.h"
#include "catapult/model/Cosignature.h"
#include "catapult/model/Elements.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionStatus.h"
#include "zeromq/tests/test/ZeroMqTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/TransactionInfoTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/TestHarness.h"

namespace catapult { namespace zeromq {

#define TEST_CLASS ZeroMqEntityPublisherTests

	namespace {
		model::TransactionInfo ToTransactionInfo(std::unique_ptr<mocks::MockTransaction>&& pTransaction) {
			model::TransactionInfo transactionInfo(std::move(pTransaction));
			transactionInfo.EntityHash = test::GenerateRandomData<Hash256_Size>();
			transactionInfo.MerkleComponentHash = test::GenerateRandomData<Hash256_Size>();
			return transactionInfo;
		}

		model::TransactionElement ToTransactionElement(const mocks::MockTransaction& transaction) {
			model::TransactionElement transactionElement(transaction);
			transactionElement.EntityHash = test::GenerateRandomData<Hash256_Size>();
			transactionElement.MerkleComponentHash = test::GenerateRandomData<Hash256_Size>();
			return transactionElement;
		}

		class EntityPublisherContext : public test::MqContext {
		public:
			void publishBlockHeader(const model::BlockElement& blockElement) {
				publisher().publishBlockHeader(blockElement);
			}

			void publishDropBlocks(Height height) {
				publisher().publishDropBlocks(height);
			}

			void publishTransaction(TransactionMarker topicMarker, const model::TransactionInfo& transactionInfo, Height height) {
				publisher().publishTransaction(topicMarker, transactionInfo, height);
			}

			void publishTransaction(TransactionMarker topicMarker, const model::TransactionElement& transactionElement, Height height) {
				publisher().publishTransaction(topicMarker, transactionElement, height);
			}

			void publishTransactionHash(TransactionMarker topicMarker, const model::TransactionInfo& transactionInfo) {
				publisher().publishTransactionHash(topicMarker, transactionInfo);
			}

			void publishTransactionStatus(const model::Transaction& transaction, const Hash256& hash, uint32_t status) {
				publisher().publishTransactionStatus(transaction, hash, status);
			}

			void publishCosignature(const model::TransactionInfo& parentTransactionInfo, const Key& signer, const Signature& signature) {
				publisher().publishCosignature(parentTransactionInfo, signer, signature);
			}
		};

		std::shared_ptr<model::AddressSet> GenerateRandomExtractedAddresses() {
			// Arrange: generate three random addresses
			return std::make_shared<model::AddressSet>(model::AddressSet{
				test::GenerateRandomData<Address_Decoded_Size>(),
				test::GenerateRandomData<Address_Decoded_Size>(),
				test::GenerateRandomData<Address_Decoded_Size>()
			});
		}
	}

	// region basic tests

	TEST(TEST_CLASS, CanDestroyPublisherWithNonEmptyQueueWithoutCrash) {
		// Arrange:
		EntityPublisherContext context;
		context.subscribe(BlockMarker::Drop_Blocks_Marker);
		Height height(123);

		// Act + Assert:
		context.publishDropBlocks(height);
		context.destroyPublisher();
	}

	// endregion

	// region publishBlockHeader

	TEST(TEST_CLASS, CanPublishBlockHeader) {
		// Arrange:
		EntityPublisherContext context;
		context.subscribe(BlockMarker::Block_Marker);
		auto pBlock = test::GenerateEmptyRandomBlock();
		auto blockElement = test::BlockToBlockElement(*pBlock);

		// Act:
		context.publishBlockHeader(blockElement);

		// Assert:
		zmq::multipart_t message;
		test::ZmqReceive(message, context.zmqSocket());

		test::AssertBlockHeaderMessage(message, blockElement);
	}

	// endregion

	// region publishDropBlocks

	TEST(TEST_CLASS, CanPublishDropBlocks) {
		// Arrange:
		EntityPublisherContext context;
		context.subscribe(BlockMarker::Drop_Blocks_Marker);
		Height height(123);

		// Act:
		context.publishDropBlocks(height);

		// Assert:
		zmq::multipart_t message;
		test::ZmqReceive(message, context.zmqSocket());

		test::AssertDropBlocksMessage(message, height);
	}

	// endregion

	// region publishTransaction

	namespace {
		constexpr TransactionMarker Marker = TransactionMarker(12);

		template<typename TAddressesGenerator>
		void AssertCanPublishTransactionInfo(TAddressesGenerator generateAddresses) {
			// Arrange:
			EntityPublisherContext context;
			auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0));
			Height height(123);
			auto addresses = generateAddresses(transactionInfo);
			context.subscribeAll(Marker, addresses);

			// Act:
			context.publishTransaction(Marker, transactionInfo, height);

			// Assert:
			auto& zmqSocket = context.zmqSocket();
			test::AssertMessages(zmqSocket, Marker, addresses, [&transactionInfo, height](const auto& message, const auto& topic) {
				test::AssertTransactionInfoMessage(message, topic, transactionInfo, height);
			});
		}

		template<typename TAddressesGenerator>
		void AssertCanPublishTransactionElement(TAddressesGenerator generateAddresses) {
			// Arrange:
			EntityPublisherContext context;
			auto pTransaction = mocks::CreateMockTransaction(0);
			auto transactionElement = ToTransactionElement(*pTransaction);
			Height height(123);
			auto addresses = generateAddresses(transactionElement);
			context.subscribeAll(Marker, addresses);

			// Act:
			context.publishTransaction(Marker, transactionElement, height);

			// Assert:
			auto& zmqSocket = context.zmqSocket();
			test::AssertMessages(zmqSocket, Marker, addresses, [&transactionElement, height](const auto& message, const auto& topic) {
				test::AssertTransactionElementMessage(message, topic, transactionElement, height);
			});
		}
	}

	TEST(TEST_CLASS, CanPublishTransaction_TransactionInfo) {
		// Assert:
		AssertCanPublishTransactionInfo([](const auto& transactionInfo) {
			return test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionToCustomAddresses_TransactionInfo) {
		// Assert:
		AssertCanPublishTransactionInfo([](auto& transactionInfo) {
			transactionInfo.OptionalExtractedAddresses = GenerateRandomExtractedAddresses();
			return *transactionInfo.OptionalExtractedAddresses;
		});
	}

	TEST(TEST_CLASS, CanPublishTransaction_TransactionElement) {
		// Assert:
		AssertCanPublishTransactionElement([](const auto& transactionElement) {
			return test::ExtractAddresses(test::ToMockTransaction(transactionElement.Transaction));
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionToCustomAddresses_TransactionElement) {
		// Assert:
		AssertCanPublishTransactionElement([](auto& transactionElement) {
			transactionElement.OptionalExtractedAddresses = GenerateRandomExtractedAddresses();
			return *transactionElement.OptionalExtractedAddresses;
		});
	}

	TEST(TEST_CLASS, PublishTransactionDeliversMessagesOnlyToRegisteredSubscribers) {
		// Arrange:
		EntityPublisherContext context;
		auto pTransaction = mocks::CreateMockTransaction(0);
		auto recipientAddress = model::PublicKeyToAddress(pTransaction->Recipient, model::NetworkIdentifier(pTransaction->Network()));
		auto transactionInfo = ToTransactionInfo(std::move(pTransaction));
		Height height(123);

		// - only subscribe to the recipient address (and not to other addresses like the sender)
		context.subscribeAll(Marker, { recipientAddress });

		// Act:
		context.publishTransaction(Marker, transactionInfo, height);

		// Assert:
		zmq::multipart_t message;
		test::ZmqReceive(message, context.zmqSocket());

		// - only a single message is sent to the recipient address (because that is the only subscribed address)
		auto topic = CreateTopic(Marker, recipientAddress);
		test::AssertTransactionInfoMessage(message, topic, transactionInfo, height);

		// - no other message is pending (e.g. to sender)
		test::AssertNoPendingMessages(context.zmqSocket());
	}

	TEST(TEST_CLASS, PublishTransactionDeliversNoMessagesIfNoAddressesAreAssociatedWithTransaction) {
		// Arrange:
		EntityPublisherContext context;
		auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0));
		Height height(123);
		auto addresses = test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		context.subscribeAll(Marker, addresses);

		// - associate no addresses with the transaction
		transactionInfo.OptionalExtractedAddresses = std::make_shared<model::AddressSet>();

		// Act:
		context.publishTransaction(Marker, transactionInfo, height);

		// Assert: no messages are pending
		test::AssertNoPendingMessages(context.zmqSocket());
	}

	// endregion

	// region publishTransactionHash

	namespace {
		template<typename TAddressesGenerator>
		void AssertCanPublishTransactionHash(TAddressesGenerator generateAddresses) {
			// Arrange:
			EntityPublisherContext context;
			auto pTransaction = mocks::CreateMockTransaction(0);
			auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0));
			auto addresses = generateAddresses(transactionInfo);
			context.subscribeAll(Marker, addresses);

			// Act:
			context.publishTransactionHash(Marker, transactionInfo);

			// Assert:
			const auto& hash = transactionInfo.EntityHash;
			test::AssertMessages(context.zmqSocket(), Marker, addresses, [&hash](const auto& message, const auto& topic) {
				test::AssertTransactionHashMessage(message, topic, hash);
			});
		}
	}

	TEST(TEST_CLASS, CanPublishTransactionHash) {
		// Assert:
		AssertCanPublishTransactionHash([](const auto& transactionInfo) {
			return test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionHashToCustomAddresses) {
		// Assert:
		AssertCanPublishTransactionHash([](auto& transactionInfo) {
			transactionInfo.OptionalExtractedAddresses = GenerateRandomExtractedAddresses();
			return *transactionInfo.OptionalExtractedAddresses;
		});
	}

	// endregion

	// region publishTransactionStatus

	TEST(TEST_CLASS, CanPublishTransactionStatus) {
		// Arrange:
		EntityPublisherContext context;
		auto pTransaction = mocks::CreateMockTransaction(0);
		auto hash = test::GenerateRandomData<Hash256_Size>();
		auto addresses = test::ExtractAddresses(*pTransaction);
		TransactionMarker marker = TransactionMarker::Transaction_Status_Marker;
		context.subscribeAll(marker, addresses);

		// Act:
		context.publishTransactionStatus(*pTransaction, hash, 123);

		// Assert:
		model::TransactionStatus expectedTransactionStatus(hash, 123, pTransaction->Deadline);
		test::AssertMessages(context.zmqSocket(), marker, addresses, [&expectedTransactionStatus](const auto& message, const auto& topic) {
			test::AssertTransactionStatusMessage(message, topic, expectedTransactionStatus);
		});
	}

	// endregion

	// region publishCosignature

	TEST(TEST_CLASS, CanPublishCosignature) {
		// Arrange:
		EntityPublisherContext context;
		auto transactionInfo = ToTransactionInfo(mocks::CreateMockTransaction(0));
		auto signer = test::GenerateRandomData<Key_Size>();
		auto signature = test::GenerateRandomData<Signature_Size>();
		auto addresses = test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		TransactionMarker marker = TransactionMarker::Cosignature_Marker;
		context.subscribeAll(marker, addresses);

		// Act:
		context.publishCosignature(transactionInfo, signer, signature);

		// Assert:
		model::DetachedCosignature expectedDetachedCosignature(signer, signature, transactionInfo.EntityHash);
		auto& zmqSocket = context.zmqSocket();
		test::AssertMessages(zmqSocket, marker, addresses, [&expectedDetachedCosignature](const auto& message, const auto& topic) {
			test::AssertDetachedCosignatureMessage(message, topic, expectedDetachedCosignature);
		});
	}

	// endregion
}}
