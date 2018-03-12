#include "PtValidator.h"
#include "AggregateCosignersNotificationPublisher.h"
#include "JointValidator.h"
#include "plugins/txes/aggregate/src/validators/Results.h"
#include "catapult/model/WeakCosignedTransactionInfo.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/validators/NotificationValidatorAdapter.h"
#include "catapult/validators/ValidatingNotificationSubscriber.h"

using namespace catapult::validators;

namespace catapult { namespace chain {

	namespace {
		constexpr bool IsMissingCosignersResult(ValidationResult result) {
			return Failure_Aggregate_Missing_Cosigners == result;
		}

		CPP14_CONSTEXPR CosignersValidationResult MapToCosignersValidationResult(ValidationResult result) {
			if (IsValidationResultSuccess(result))
				return CosignersValidationResult::Success;

			// map failures (not using switch statement to workaround gcc warning)
			return Failure_Aggregate_Ineligible_Cosigners == result
					? CosignersValidationResult::Ineligible
					: Failure_Aggregate_Missing_Cosigners == result
							? CosignersValidationResult::Missing
							: CosignersValidationResult::Failure;
		}

		class DefaultPtValidator : public PtValidator {
		public:
			DefaultPtValidator(
					const cache::CatapultCache& cache,
					const TimeSupplier& timeSupplier,
					const plugins::PluginManager& pluginManager)
					: m_transactionValidator(
							CreateJointValidator(cache, timeSupplier, pluginManager, IsMissingCosignersResult),
							pluginManager.createNotificationPublisher(model::PublicationMode::Basic))
					, m_statelessTransactionValidator(
							pluginManager.createStatelessValidator(),
							pluginManager.createNotificationPublisher(model::PublicationMode::Custom))
					, m_pCosignersValidator(CreateJointValidator(cache, timeSupplier, pluginManager, [](auto) { return false; }))
			{}

		public:
			Result<bool> validatePartial(const model::WeakEntityInfoT<model::Transaction>& transactionInfo) const override {
				// notice that partial validation has two differences relative to "normal" validation
				// 1. missing cosigners failures are ignored
				// 2. custom stateful validators are ignored
				auto weakEntityInfo = transactionInfo.cast<model::VerifiableEntity>();
				auto result = m_transactionValidator.validate(weakEntityInfo);
				if (IsValidationResultSuccess(result)) {
					// - check custom stateless validators
					result = m_statelessTransactionValidator.validate(weakEntityInfo);
					if (IsValidationResultSuccess(result))
						return { result, true };
				}

				CATAPULT_LOG_LEVEL(validators::MapToLogLevel(result)) << "partial transaction failed validation with " << result;
				return { result, false };
			}

			Result<CosignersValidationResult> validateCosigners(const model::WeakCosignedTransactionInfo& transactionInfo) const override {
				validators::ValidatingNotificationSubscriber sub(*m_pCosignersValidator);
				m_aggregatePublisher.publish(transactionInfo, sub);
				return { sub.result(), MapToCosignersValidationResult(sub.result()) };
			}

		private:
			NotificationValidatorAdapter m_transactionValidator;
			NotificationValidatorAdapter m_statelessTransactionValidator;
			AggregateCosignersNotificationPublisher m_aggregatePublisher;
			std::unique_ptr<const stateless::NotificationValidator> m_pCosignersValidator;
		};
	}

	std::unique_ptr<PtValidator> CreatePtValidator(
			const cache::CatapultCache& cache,
			const TimeSupplier& timeSupplier,
			const plugins::PluginManager& pluginManager) {
		return std::make_unique<DefaultPtValidator>(cache, timeSupplier, pluginManager);
	}
}}
