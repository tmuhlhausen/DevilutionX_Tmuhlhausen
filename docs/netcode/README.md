# Netcode

## Packet encryption salts

Packet encryption now supports caller-provided password salts through `packet_factory(std::string password, salt_t salt)`.

Use this path for new secure sessions:

1. Generate a fresh salt with `packet_factory::GenerateSalt()` during session setup.
2. Exchange or persist that salt as part of the session/protocol handshake metadata.
3. Construct packet factories on every peer with the same password and salt.
4. Never reuse a generated salt for unrelated sessions.

The legacy `packet_factory(std::string password)` constructor is intentionally preserved for compatibility. It still derives the key from the historical fixed salt so older callers keep working until the wire handshake can exchange per-session salts automatically.

Regression coverage:

- Same password plus same salt decrypts successfully.
- Same password plus different salt rejects encrypted packets with `DecryptionFailed`.

Follow-up work:

- Add explicit wire-version negotiation for salt exchange.
- Include the generated salt in join/session setup packets.
- Remove fixed-salt dependency once compatibility policy allows it.
