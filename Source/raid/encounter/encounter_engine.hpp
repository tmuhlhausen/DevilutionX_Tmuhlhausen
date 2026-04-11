#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace devilution {

enum class EncounterConditionType : uint8_t {
	None,
	TimeElapsedAtLeast,
	BossHealthAtMost,
	AddsDefeatedAtLeast,
	PlayerDeathsAtLeast,
	AllPlayersDead,
};

enum class EncounterMechanicType : uint8_t {
	AoEPulse,
	AddWave,
	PositionalCheck,
	Enrage,
};

enum class EncounterFailurePolicy : uint8_t {
	Wipe,
	PartialFail,
	CheckpointRollback,
};

enum class EncounterEventType : uint8_t {
	PhaseEntered,
	PhaseExited,
	MechanicTriggered,
	FailureTriggered,
	EncounterCompleted,
};

struct EncounterCondition {
	EncounterConditionType type = EncounterConditionType::None;
	uint32_t threshold = 0;
};

struct EncounterMechanic {
	uint16_t id = 0;
	EncounterMechanicType type = EncounterMechanicType::AoEPulse;
	uint32_t periodMs = 0;
	uint32_t firstTriggerMs = 0;
	uint8_t phaseId = 0;
	bool repeat = true;
};

struct EncounterPhaseDef {
	uint8_t id = 0;
	std::optional<EncounterCondition> enterCondition;
	std::optional<EncounterCondition> exitCondition;
	EncounterFailurePolicy failurePolicy = EncounterFailurePolicy::Wipe;
};

struct EncounterDefinition {
	uint32_t encounterId = 0;
	uint8_t startPhaseId = 0;
	std::vector<EncounterPhaseDef> phases;
	std::vector<EncounterMechanic> mechanics;
	uint32_t enrageTimeMs = 0;
};

struct EncounterWorldSnapshot {
	uint32_t nowMs = 0;
	uint8_t bossHealthPercent = 100;
	uint16_t addsDefeated = 0;
	uint8_t playerDeaths = 0;
	bool allPlayersDead = false;
};

struct EncounterEvent {
	EncounterEventType type = EncounterEventType::PhaseEntered;
	uint32_t timestampMs = 0;
	uint8_t phaseId = 0;
	uint16_t mechanicId = 0;
	EncounterFailurePolicy failurePolicy = EncounterFailurePolicy::Wipe;
};

struct EncounterTelemetry {
	std::array<uint32_t, 16> phaseDurationMs {};
	uint32_t totalDeaths = 0;
	uint32_t totalWipes = 0;
};

struct EncounterHooks {
	std::function<EncounterWorldSnapshot()> readWorldSnapshot;
	std::function<void(const EncounterEvent &)> emitDeterministicEvent;
	std::function<void(const EncounterTelemetry &)> publishTelemetry;
};

class EncounterEngine {
public:
	EncounterEngine() = default;

	void Reset(const EncounterDefinition &definition);
	void Tick(bool isHost);
	[[nodiscard]] const EncounterTelemetry &GetTelemetry() const;
	[[nodiscard]] uint8_t GetActivePhaseId() const;
	void SetHooks(EncounterHooks hooks);

private:
	[[nodiscard]] bool EvaluateCondition(const EncounterCondition &condition, const EncounterWorldSnapshot &snapshot) const;
	[[nodiscard]] const EncounterPhaseDef *FindPhaseById(uint8_t phaseId) const;
	void EmitEvent(bool isHost, const EncounterEvent &event) const;
	void EnterPhase(bool isHost, uint8_t phaseId, uint32_t nowMs);
	void ExitPhase(bool isHost, uint8_t phaseId, uint32_t nowMs);
	void TriggerFailure(bool isHost, EncounterFailurePolicy policy, uint32_t nowMs);

	EncounterDefinition definition_ {};
	EncounterHooks hooks_ {};
	EncounterTelemetry telemetry_ {};
	uint8_t activePhaseId_ = 0;
	uint32_t phaseStartMs_ = 0;
	uint32_t encounterStartMs_ = 0;
	bool initialized_ = false;
	bool completed_ = false;
	std::vector<uint32_t> lastMechanicTriggerMs_;
};

} // namespace devilution
