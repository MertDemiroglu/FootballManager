#pragma once

#include<cstdint>

class GameInteraction {
public: 
	enum class Kind : std::uint8_t {
		PostMatch,
		TransferOfferDecision
	};

	virtual ~GameInteraction() = default;

	Kind getKind() const;
	bool isBlocking() const;
	bool isResolved() const;

	void resolve();

protected:
	GameInteraction(Kind kind, bool blocking);

private:
	Kind kind;
	bool blocking;
	bool resolved;
};