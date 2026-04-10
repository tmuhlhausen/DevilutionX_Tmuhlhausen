#include "raid/encounter/encounter_engine.hpp"

#include <algorithm>

namespace devilution {

void EncounterEngine::Reset(const EncounterDefinition &definition)
{
	definition_ = definition;
	telemetry_ = {};
	activePhaseId_ = definition.startPhaseId;
	phaseStartMs_ = 0;
	encounterStartMs_ = 0;
	initialized_ = false;
	completed_ = false;
	lastMechanicTriggerMs_.assign(definition.mechanics.size(), 0);
}

void EncounterEngine::SetHooks(EncounterHooks hooks)
{
	hooks_ = std::move(hooks);
}

const EncounterTelemetry &EncounterEngine::GetTelemetry() const
{
	return telemetry_;
}

uint8_t EncounterEngine::GetActivePhaseId() const
{
	return activePhaseId_;
}

const EncounterPhaseDef *EncounterEngine::FindPhaseById(uint8_t phaseId) const
{
	const auto it = std::find_if(definition_.phases.begin(), definition_.phases.end(), [phaseId](const EncounterPhaseDef &phase) {
		return phase.id == phaseId;
	});
	if (it == definition_.phases.end())
		return nullptr;
	return &(*it);
}

bool EncounterEngine::EvaluateCondition(const EncounterCondition &condition, const EncounterWorldSnapshot &snapshot) const
{
	switch (condition.type) {
	case EncounterConditionType::None:
		return true;
	case EncounterConditionType::TimeElapsedAtLeast:
		return snapshot.nowMs >= condition.threshold;
	case EncounterConditionType::BossHealthAtMost:
		return snapshot.bossHealthPercent <= condition.threshold;
	case EncounterConditionType::AddsDefeatedAtLeast:
		return snapshot.addsDefeated >= condition.threshold;
	case EncounterConditionType::PlayerDeathsAtLeast:
		return snapshot.playerDeaths >= condition.threshold;
	case EncounterConditionType::AllPlayersDead:
		return snapshot.allPlayersDead;
	}
	return false;
}

void EncounterEngine::EmitEvent(bool isHost, const EncounterEvent &event) const
{
	if (!isHost || !hooks_.emitDeterministicEvent)
		return;
	hooks_.emitDeterministicEvent(event);
}

void EncounterEngine::EnterPhase(bool isHost, uint8_t phaseId, uint32_t nowMs)
{
	activePhaseId_ = phaseId;
	phaseStartMs_ = nowMs;
	EmitEvent(isHost, EncounterEvent { EncounterEventType::PhaseEntered, nowMs, phaseId });
}

void EncounterEngine::ExitPhase(bool isHost, uint8_t phaseId, uint32_t nowMs)
{
	if (phaseId < telemetry_.phaseDurationMs.size())
		telemetry_.phaseDurationMs[phaseId] += nowMs - phaseStartMs_;
	EmitEvent(isHost, EncounterEvent { EncounterEventType::PhaseExited, nowMs, phaseId });
}

void EncounterEngine::TriggerFailure(bool isHost, EncounterFailurePolicy policy, uint32_t nowMs)
{
	if (policy == EncounterFailurePolicy::Wipe)
		telemetry_.totalWipes++;
	EmitEvent(isHost, EncounterEvent { EncounterEventType::FailureTriggered, nowMs, activePhaseId_, 0, policy });
}

void EncounterEngine::Tick(bool isHost)
{
	if (!hooks_.readWorldSnapshot || completed_ || definition_.phases.empty())
		return;

	const EncounterWorldSnapshot snapshot = hooks_.readWorldSnapshot();
	if (!initialized_) {
		initialized_ = true;
		encounterStartMs_ = snapshot.nowMs;
		phaseStartMs_ = snapshot.nowMs;
		EnterPhase(isHost, activePhaseId_, snapshot.nowMs);
	}

	telemetry_.totalDeaths = std::max<uint32_t>(telemetry_.totalDeaths, snapshot.playerDeaths);

	if (definition_.enrageTimeMs != 0 && snapshot.nowMs - encounterStartMs_ >= definition_.enrageTimeMs) {
		EmitEvent(isHost, EncounterEvent { EncounterEventType::MechanicTriggered, snapshot.nowMs, activePhaseId_, 0 });
	}

	for (size_t i = 0; i < definition_.mechanics.size(); ++i) {
		const EncounterMechanic &mechanic = definition_.mechanics[i];
		if (mechanic.phaseId != activePhaseId_)
			continue;
		if (snapshot.nowMs < mechanic.firstTriggerMs)
			continue;
		if (lastMechanicTriggerMs_[i] != 0 && !mechanic.repeat)
			continue;
		if (lastMechanicTriggerMs_[i] != 0 && mechanic.periodMs != 0 && snapshot.nowMs - lastMechanicTriggerMs_[i] < mechanic.periodMs)
			continue;

		lastMechanicTriggerMs_[i] = snapshot.nowMs;
		EmitEvent(isHost, EncounterEvent { EncounterEventType::MechanicTriggered, snapshot.nowMs, activePhaseId_, mechanic.id });
	}

	const EncounterPhaseDef *activePhase = FindPhaseById(activePhaseId_);
	if (activePhase == nullptr)
		return;

	if (activePhase->failurePolicy != EncounterFailurePolicy::PartialFail && snapshot.allPlayersDead) {
		TriggerFailure(isHost, activePhase->failurePolicy, snapshot.nowMs);
		if (activePhase->failurePolicy == EncounterFailurePolicy::CheckpointRollback) {
			EnterPhase(isHost, definition_.startPhaseId, snapshot.nowMs);
		}
	}

	if (activePhase->exitCondition && EvaluateCondition(*activePhase->exitCondition, snapshot)) {
		ExitPhase(isHost, activePhaseId_, snapshot.nowMs);
		const auto nextPhase = std::find_if(definition_.phases.begin(), definition_.phases.end(), [this](const EncounterPhaseDef &phase) {
			return phase.id > activePhaseId_;
		});
		if (nextPhase == definition_.phases.end()) {
			completed_ = true;
			EmitEvent(isHost, EncounterEvent { EncounterEventType::EncounterCompleted, snapshot.nowMs, activePhaseId_ });
		} else {
			EnterPhase(isHost, nextPhase->id, snapshot.nowMs);
		}
	}

	if (hooks_.publishTelemetry)
		hooks_.publishTelemetry(telemetry_);
}

} // namespace devilution
