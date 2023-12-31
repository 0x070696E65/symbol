import secrets

from .ByteArray import ByteArray


class Hash256(ByteArray):
	"""Represents a 256-bit hash."""

	SIZE = 32

	def __init__(self, hash256):
		"""Creates a hash from bytes or a hex string."""
		super().__init__(self.SIZE, hash256, Hash256)

	def __repr__(self):
		return f'Hash256(\'{str(self)}\')'

	@staticmethod
	def zero():
		"""Creates a zeroed hash."""
		return Hash256(bytes([0] * Hash256.SIZE))


class PrivateKey(ByteArray):
	"""Represents a private key."""

	SIZE = 32

	def __init__(self, private_key):
		"""Creates a private key from bytes or a hex string."""
		super().__init__(self.SIZE, private_key, PrivateKey)

	def __repr__(self):
		return f'PrivateKey(\'{str(self)}\')'

	@staticmethod
	def random():
		"""Generates a random private key."""
		return PrivateKey(secrets.token_bytes(PrivateKey.SIZE))


class PublicKey(ByteArray):
	"""Represents a public key."""

	SIZE = 32

	def __init__(self, public_key):
		"""Creates a public key from bytes or a hex string."""
		super().__init__(self.SIZE, public_key.bytes if isinstance(public_key, PublicKey) else public_key, PublicKey)

	def __repr__(self):
		return f'PublicKey(\'{str(self)}\')'


class SharedKey256(ByteArray):
	"""Represents 256-bit symmetric encryption key."""

	SIZE = 32

	def __init__(self, key):
		"""Creates a key from bytes or a hex string."""
		super().__init__(self.SIZE, key, SharedKey256)

	def __repr__(self):
		return f'SharedKey256(\'{str(self)}\')'


class Signature(ByteArray):
	"""Represents a signature."""

	SIZE = 64

	def __init__(self, signature):
		"""Creates a signature from bytes or a hex string."""
		super().__init__(self.SIZE, signature, Signature)

	def __repr__(self):
		return f'Signature(\'{str(self)}\')'

	@staticmethod
	def zero():
		"""Creates a zeroed signature."""
		return Signature(bytes([0] * Signature.SIZE))
