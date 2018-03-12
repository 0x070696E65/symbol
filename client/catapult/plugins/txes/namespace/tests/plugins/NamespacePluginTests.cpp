#include "src/plugins/NamespacePlugin.h"
#include "src/model/MosaicEntityType.h"
#include "src/model/NamespaceEntityType.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct NamespacePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::BlockChainConfiguration::Uninitialized();
				config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(1);
				config.BlockPruneInterval = 150;
				config.Plugins.emplace("catapult.plugins.namespace", utils::ConfigurationBag({{
					"",
					{
						{ "maxNameSize", "0" },
						{ "maxNamespaceDuration", "0h" },
						{ "namespaceGracePeriodDuration", "0h" },
						{ "reservedRootNamespaceNames", "reserved" },

						{ "namespaceRentalFeeSinkPublicKey", "0000000000000000000000000000000000000000000000000000000000000000" },
						{ "rootNamespaceRentalFeePerBlock", "0" },
						{ "childNamespaceRentalFee", "0" },

						{ "maxChildNamespaces", "0" },
						{ "maxMosaicsPerAccount", "0" },

						{ "maxMosaicDuration", "0h" },

						{ "isMosaicLevyUpdateAllowed", "false" },
						{ "maxMosaicDivisibility", "0" },
						{ "maxMosaicDivisibleUnits", "0" },

						{ "mosaicRentalFeeSinkPublicKey", "0000000000000000000000000000000000000000000000000000000000000000" },
						{ "mosaicRentalFee", "0" }
					}
				}}));

				PluginManager manager(config);
				RegisterNamespaceSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_Register_Namespace,
					model::Entity_Type_Mosaic_Definition,
					model::Entity_Type_Mosaic_Supply_Change
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "NamespaceCache", "MosaicCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Mosaic_Infos, ionet::PacketType::Namespace_Infos };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "NS C", "NS C AS", "NS C DS", "MOSAIC C", "MOSAIC C DS" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"NamespaceTypeValidator",
					"NamespaceNameValidator",
					"RootNamespaceValidator",
					"MosaicNameValidator",
					"MosaicPropertiesValidator",
					"MosaicSupplyChangeValidator"
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"RootNamespaceAvailabilityValidator",
					"ChildNamespaceAvailabilityValidator",
					"RootNamespaceMaxChildrenValidator",
					"MosaicChangeAllowedValidator",
					"NamespaceMosaicConsistencyValidator",
					"MosaicAvailabilityValidator",
					"MosaicTransferValidator",
					"MaxMosaicsBalanceTransferValidator",
					"MaxMosaicsSupplyChangeValidator",
					"MosaicSupplyChangeAllowedValidator"
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"RegisterNamespaceMosaicPruningObserver",
					"RootNamespaceObserver",
					"ChildNamespaceObserver",
					"NamespacePruningObserver",
					"MosaicDefinitionObserver",
					"MosaicSupplyChangeObserver",
					"MosaicPruningObserver"
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return {
					"RegisterNamespaceMosaicPruningObserver",
					"RootNamespaceObserver",
					"ChildNamespaceObserver",
					"NamespacePruningObserver",
					"MosaicDefinitionObserver",
					"MosaicSupplyChangeObserver",
					"MosaicPruningObserver"
				};
			}
		};
	}

	DEFINE_PLUGIN_TESTS(NamespacePluginTests, NamespacePluginTraits);
}}
