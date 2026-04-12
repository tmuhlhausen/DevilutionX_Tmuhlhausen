#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include "raid/encounter/encounter_engine.hpp"

namespace devilution {
namespace {

TEST(EncounterEnginePhaseTest, ScriptedTransitionUsesNextPhaseId)
{
	EncounterDefinition def;
	def.startPhaseId = 0;
	def.phases = {
		EncounterPhaseDef { .id = 0, .exitCondition = EncounterCondition { EncounterConditionType::TimeElapsedAtLeast, 10 }, .nextPhaseId = 2 },
		EncounterPhaseDef { .id = 1, .exitCondition = EncounterCondition { EncounterConditionType::TimeElapsedAtLeast, 999 } },
		EncounterPhaseDef { .id = 2, .exitCondition = EncounterCondition { EncounterConditionType::TimeElapsedAtLeast, 999 } },
	};

	EncounterWorldSnapshot snapshot;
	snapshot.nowMs = 0;
	EncounterEngine engine;
	engine.Reset(def);
	engine.SetHooks(EncounterHooks { [&]() { return snapshot; }, nullptr, nullptr });

	engine.Tick(true);
	snapshot.nowMs = 12;
	engine.Tick(true);

	EXPECT_EQ(engine.GetActivePhaseId(), 2);
}

TEST(EncounterEnginePhaseTest, MechanicConditionBlocksUntilSatisfied)
{
	EncounterDefinition def;
	def.startPhaseId = 0;
	def.phases = { EncounterPhaseDef { .id = 0 } };
	EncounterMechanic mechanic;
	mechanic.id = 99;
	mechanic.phaseId = 0;
	mechanic.periodMs = 1;
	mechanic.repeat = true;
	mechanic.triggerCondition = EncounterCondition { EncounterConditionType::BossHealthAtMost, 50 };
	def.mechanics = { mechanic };

	EncounterWorldSnapshot snapshot;
	std::vector<EncounterEvent> events;
	EncounterEngine engine;
	engine.Reset(def);
	engine.SetHooks(EncounterHooks {
		[&]() { return snapshot; },
		[&](const EncounterEvent &event) { events.push_back(event); },
		nullptr,
	});

	snapshot.nowMs = 1;
	snapshot.bossHealthPercent = 80;
	engine.Tick(true);
	snapshot.nowMs = 2;
	snapshot.bossHealthPercent = 45;
	engine.Tick(true);

	auto it = std::find_if(events.begin(), events.end(), [](const EncounterEvent &event) {
		return event.type == EncounterEventType::MechanicTriggered && event.mechanicId == 99;
	});
	EXPECT_NE(it, events.end());
}

TEST(EncounterEnginePhaseTest, WipeFailureEmitsFailStateAndTracksWipes)
{
	EncounterDefinition def;
	def.startPhaseId = 0;
	def.phases = { EncounterPhaseDef { .id = 0, .failurePolicy = EncounterFailurePolicy::Wipe } };

	EncounterWorldSnapshot snapshot;
	std::vector<EncounterEvent> events;
	EncounterEngine engine;
	engine.Reset(def);
	engine.SetHooks(EncounterHooks {
		[&]() { return snapshot; },
		[&](const EncounterEvent &event) { events.push_back(event); },
		nullptr,
	});

	snapshot.nowMs = 5;
	snapshot.allPlayersDead = true;
	engine.Tick(true);

	auto failure = std::find_if(events.begin(), events.end(), [](const EncounterEvent &event) {
		return event.type == EncounterEventType::FailureTriggered && event.failurePolicy == EncounterFailurePolicy::Wipe;
	});
	EXPECT_NE(failure, events.end());
	EXPECT_EQ(engine.GetTelemetry().totalWipes, 1u);
}

TEST(EncounterEnginePhaseTest, CheckpointRollbackFailureReturnsToStartPhase)
{
	EncounterDefinition def;
	def.startPhaseId = 0;
	def.phases = {
		EncounterPhaseDef { .id = 0, .exitCondition = EncounterCondition { EncounterConditionType::TimeElapsedAtLeast, 1 }, .nextPhaseId = 1 },
		EncounterPhaseDef { .id = 1, .failurePolicy = EncounterFailurePolicy::CheckpointRollback },
	};

	EncounterWorldSnapshot snapshot;
	std::vector<EncounterEvent> events;
	EncounterEngine engine;
	engine.Reset(def);
	engine.SetHooks(EncounterHooks {
		[&]() { return snapshot; },
		[&](const EncounterEvent &event) { events.push_back(event); },
		nullptr,
	});

	snapshot.nowMs = 0;
	engine.Tick(true);
	snapshot.nowMs = 2;
	engine.Tick(true);
	ASSERT_EQ(engine.GetActivePhaseId(), 1);

	snapshot.nowMs = 3;
	snapshot.allPlayersDead = true;
	engine.Tick(true);

	EXPECT_EQ(engine.GetActivePhaseId(), 0);
	auto rollbackEnter = std::find_if(events.begin(), events.end(), [](const EncounterEvent &event) {
		return event.type == EncounterEventType::PhaseEntered && event.phaseId == 0 && event.timestampMs == 3;
	});
	EXPECT_NE(rollbackEnter, events.end());
}

} // namespace
} // namespace devilution
