#pragma once
#include "catapult/plugins.h"
#include <memory>

namespace catapult { namespace model { class TransactionPlugin; } }

namespace catapult { namespace plugins {

	/// Creates a secret lock transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateSecretLockTransactionPlugin();
}}
