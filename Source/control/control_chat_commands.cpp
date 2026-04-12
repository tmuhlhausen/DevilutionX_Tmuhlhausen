#include "control_chat_commands.hpp"
#include "control.hpp"

#include <algorithm>
#include <bit>

#ifdef USE_SDL3
#include <SDL3/SDL_timer.h>
#else
#include <SDL.h>
#endif

#include "diablo_msg.hpp"
#include "engine/backbuffer_state.hpp"
#include "inv.h"
#include "levels/setmaps.h"
#include "msg.h"
#include "guild/guild_mod_api.hpp"
#include "raid/raid.hpp"
#include "raid/raid_mod_api.hpp"
#include "raid/raid_progression.hpp"
#include "storm/storm_net.hpp"
#include "utils/algorithm/container.hpp"
#include "utils/log.hpp"
#include "utils/parse_int.hpp"
#include "utils/str_case.hpp"
#include "utils/str_cat.hpp"

#ifdef _DEBUG
#include "debug.h"
#endif

namespace devilution {

namespace {

constexpr uint8_t RaidMinimumMembersToStart = 2;
uint32_t NextRaidChatSequence = 1;

[[nodiscard]] uint32_t ConsumeRaidChatSequence()
{
	return NextRaidChatSequence++;
}

[[nodiscard]] bool RequiresHostAuthority(const std::string_view action)
{
	return action == "create" || action == "invite" || action == "start" || action == "reset";
}

[[nodiscard]] std::string GetRaidPhaseLabel(RaidPhase phase)
{
	switch (phase) {
	case RaidPhase::Inactive:
		return _("Inactive");
	case RaidPhase::Forming:
		return _("Forming");
	case RaidPhase::InProgress:
		return _("In Progress");
	case RaidPhase::Completed:
		return _("Completed");
	case RaidPhase::Failed:
		return _("Failed");
	case RaidPhase::LockedOut:
		return _("Locked Out");
	}
	return _("Unknown");
}

struct TextCmdItem {
	const std::string text;
	const std::string description;
	const std::string requiredParameter;
	std::string (*actionProc)(const std::string_view);
};

extern std::vector<TextCmdItem> TextCmdList;

std::string TextCmdHelp(const std::string_view parameter)
{
	if (parameter.empty()) {
		std::string ret;
		StrAppend(ret, _("Available Commands:"));
		for (const TextCmdItem &textCmd : TextCmdList) {
			StrAppend(ret, " ", _(textCmd.text));
		}
		return ret;
	}
	auto textCmdIterator = c_find_if(TextCmdList, [&](const TextCmdItem &elem) { return elem.text == parameter; });
	if (textCmdIterator == TextCmdList.end())
		return StrCat(_("Command "), parameter, _(" is unknown."));
	auto &textCmdItem = *textCmdIterator;
	if (textCmdItem.requiredParameter.empty())
		return StrCat(_("Description: "), _(textCmdItem.description), _("\nParameters: No additional parameter needed."));
	return StrCat(_("Description: "), _(textCmdItem.description), _("\nParameters: "), _(textCmdItem.requiredParameter));
}

void AppendArenaOverview(std::string &ret)
{
	for (int arena = SL_FIRST_ARENA; arena <= SL_LAST; arena++) {
		StrAppend(ret, "\n", arena - SL_FIRST_ARENA + 1, " (", QuestLevelNames[arena], ")");
	}
}

std::string TextCmdArena(const std::string_view parameter)
{
	std::string ret;
	if (!gbIsMultiplayer) {
		StrAppend(ret, _("Arenas are only supported in multiplayer."));
		return ret;
	}

	if (parameter.empty()) {
		StrAppend(ret, _("What arena do you want to visit?"));
		AppendArenaOverview(ret);
		return ret;
	}

	const ParseIntResult<int> parsedParam = ParseInt<int>(parameter, /*min=*/0);
	const _setlevels arenaLevel = parsedParam.has_value() ? static_cast<_setlevels>(parsedParam.value() - 1 + SL_FIRST_ARENA) : _setlevels::SL_NONE;
	if (!IsArenaLevel(arenaLevel)) {
		StrAppend(ret, _("Invalid arena-number. Valid numbers are:"));
		AppendArenaOverview(ret);
		return ret;
	}

	if (!MyPlayer->isOnLevel(0) && !MyPlayer->isOnArenaLevel()) {
		StrAppend(ret, _("To enter a arena, you need to be in town or another arena."));
		return ret;
	}

	setlvltype = GetArenaLevelType(arenaLevel);
	StartNewLvl(*MyPlayer, WM_DIABSETLVL, arenaLevel);
	return ret;
}

std::string TextCmdArenaPot(const std::string_view parameter)
{
	std::string ret;
	if (!gbIsMultiplayer) {
		StrAppend(ret, _("Arenas are only supported in multiplayer."));
		return ret;
	}
	const int numPots = ParseInt<int>(parameter, /*min=*/1).value_or(1);

	Player &myPlayer = *MyPlayer;

	for (int potNumber = numPots; potNumber > 0; potNumber--) {
		Item item {};
		InitializeItem(item, IDI_ARENAPOT);
		GenerateNewSeed(item);
		item.updateRequiredStatsCacheForPlayer(myPlayer);

		if (!AutoPlaceItemInBelt(myPlayer, item, true, true) && !AutoPlaceItemInInventory(myPlayer, item, true)) {
			break; // inventory is full
		}
	}

	return ret;
}

std::string TextCmdInspect(const std::string_view parameter)
{
	std::string ret;
	if (!gbIsMultiplayer) {
		StrAppend(ret, _("Inspecting only supported in multiplayer."));
		return ret;
	}

	if (parameter.empty()) {
		StrAppend(ret, _("Stopped inspecting players."));
		InspectPlayer = MyPlayer;
		return ret;
	}

	const std::string param = AsciiStrToLower(parameter);
	auto it = c_find_if(Players, [&param](const Player &player) {
		return AsciiStrToLower(player._pName) == param;
	});
	if (it == Players.end()) {
		it = c_find_if(Players, [&param](const Player &player) {
			return AsciiStrToLower(player._pName).find(param) != std::string::npos;
		});
	}
	if (it == Players.end()) {
		StrAppend(ret, _("No players found with such a name"));
		return ret;
	}

	Player &player = *it;
	InspectPlayer = &player;
	StrAppend(ret, _("Inspecting player: "));
	StrAppend(ret, player._pName);
	OpenCharPanel();
	if (!SpellbookFlag)
		invflag = true;
	RedrawEverything();
	return ret;
}

bool IsQuestEnabled(const Quest &quest)
{
	switch (quest._qidx) {
	case Q_FARMER:
		return gbIsHellfire && !sgGameInitInfo.bCowQuest;
	case Q_JERSEY:
		return gbIsHellfire && sgGameInitInfo.bCowQuest;
	case Q_GIRL:
		return gbIsHellfire && sgGameInitInfo.bTheoQuest;
	case Q_CORNSTN:
		return gbIsHellfire && !gbIsMultiplayer;
	case Q_GRAVE:
	case Q_DEFILER:
	case Q_NAKRUL:
		return gbIsHellfire;
	case Q_TRADER:
		return false;
	default:
		return quest._qactive != QUEST_NOTAVAIL;
	}
}

std::string TextCmdLevelSeed(const std::string_view /*parameter*/)
{
	const std::string_view levelType = setlevel ? "set level" : "dungeon level";

	char gameId[] = {
		static_cast<char>((sgGameInitInfo.programid >> 24) & 0xFF),
		static_cast<char>((sgGameInitInfo.programid >> 16) & 0xFF),
		static_cast<char>((sgGameInitInfo.programid >> 8) & 0xFF),
		static_cast<char>(sgGameInitInfo.programid & 0xFF),
		'\0'
	};

	const std::string_view mode = gbIsMultiplayer ? "MP" : "SP";
	const std::string_view questPool = UseMultiplayerQuests() ? "MP" : "Full";

	uint32_t questFlags = 0;
	for (const Quest &quest : Quests) {
		questFlags <<= 1;
		if (IsQuestEnabled(quest))
			questFlags |= 1;
	}

	return StrCat(
	    "Seedinfo for ", levelType, " ", currlevel, "\n",
	    "seed: ", DungeonSeeds[currlevel], "\n",
#ifdef _DEBUG
	    "Mid1: ", glMid1Seed[currlevel], "\n",
	    "Mid2: ", glMid2Seed[currlevel], "\n",
	    "Mid3: ", glMid3Seed[currlevel], "\n",
	    "End: ", glEndSeed[currlevel], "\n",
#endif
	    "\n",
	    gameId, " ", mode, "\n",
	    questPool, " quests: ", questFlags, "\n",
	    "Storybook: ", DungeonSeeds[16]);
}

std::string TextCmdPing(const std::string_view parameter)
{
	std::string ret;
	const std::string param = AsciiStrToLower(parameter);
	auto it = c_find_if(Players, [&param](const Player &player) {
		return AsciiStrToLower(player._pName) == param;
	});
	if (it == Players.end()) {
		it = c_find_if(Players, [&param](const Player &player) {
			return AsciiStrToLower(player._pName).find(param) != std::string::npos;
		});
	}
	if (it == Players.end()) {
		StrAppend(ret, _("No players found with such a name"));
		return ret;
	}

	Player &player = *it;
	DvlNetLatencies latencies = DvlNet_GetLatencies(player.getId());

	StrAppend(ret, fmt::format(fmt::runtime(_(/* TRANSLATORS: {:s} means: Character Name */ "Latency statistics for {:s}:")), player.name()));

	StrAppend(ret, "\n", fmt::format(fmt::runtime(_(/* TRANSLATORS: Network connectivity statistics */ "Echo latency: {:d} ms")), latencies.echoLatency));

	if (latencies.providerLatency) {
		if (latencies.isRelayed && *latencies.isRelayed) {
			StrAppend(ret, "\n", fmt::format(fmt::runtime(_(/* TRANSLATORS: Network connectivity statistics */ "Provider latency: {:d} ms (Relayed)")), *latencies.providerLatency));
		} else {
			StrAppend(ret, "\n", fmt::format(fmt::runtime(_(/* TRANSLATORS: Network connectivity statistics */ "Provider latency: {:d} ms")), *latencies.providerLatency));
		}
	}

	return ret;
}

std::string TextCmdRaid(const std::string_view parameter)
{
	if (!gbIsMultiplayer)
		return _("Raids are only supported in multiplayer.");

	const std::string param = AsciiStrToLower(parameter);
	const size_t splitPos = param.find(' ');
	const std::string action = splitPos == std::string::npos ? param : param.substr(0, splitPos);
	const std::string actionParam = splitPos == std::string::npos ? "" : param.substr(splitPos + 1);
	if (action.empty())
		return _("Usage: /raid <create|invite|join|ready|start|status|reset> [player]");

	if (RequiresHostAuthority(action) && MyPlayerId != 0) {
		return StrCat(_("Raid action '/raid "), action, _("' is host-only. Ask the game host to run it."));
	}

	RaidInstanceState state = GetActiveRaidState();
	const uint32_t currentRaidId = state.raidId.IsValid() ? state.raidId.value : (SDL_GetTicks() | 1U);

	if (action == "create") {
		NetSendCmdRaidCreate(true, currentRaidId, RaidDifficulty::Normal, state.snapshotRevision, ConsumeRaidChatSequence());
		return _("Raid create request sent.");
	}

	if (action == "invite") {
		if (actionParam.empty())
			return _("Usage: /raid invite <player name>");
		auto it = c_find_if(Players, [&](const Player &player) {
			return player.plractive && AsciiStrToLower(player._pName).find(actionParam) != std::string::npos;
		});
		if (it == Players.end())
			return _("No players found with such a name");
		NetSendCmdRaidInvite(true, currentRaidId, it->getId(), state.snapshotRevision, ConsumeRaidChatSequence());
		return StrCat(_("Raid invite request sent to "), it->_pName, ".");
	}

	if (action == "join") {
		NetSendCmdRaidJoin(true, currentRaidId, state.snapshotRevision, ConsumeRaidChatSequence());
		return _("Raid join request sent.");
	}

	if (action == "ready") {
		NetSendCmdRaidReadyToggle(true, currentRaidId, state.snapshotRevision, ConsumeRaidChatSequence());
		return _("Raid readiness toggle sent.");
	}

	if (action == "start") {
		NetSendCmdRaidStart(true, currentRaidId, state.snapshotRevision, ConsumeRaidChatSequence());
		return _("Raid start request sent.");
	}

	if (action == "reset") {
		const uint32_t resetRaidId = (SDL_GetTicks() | 1U);
		NetSendCmdRaidCreate(true, resetRaidId, state.difficulty == RaidDifficulty::None ? RaidDifficulty::Normal : state.difficulty, state.snapshotRevision, ConsumeRaidChatSequence());
		return _("Raid reset request sent.");
	}

	if (action == "status") {
		const RaidLobbyUiState lobbyState = GetActiveRaidLobbyUiState();
		const int readyCount = static_cast<int>(std::popcount(lobbyState.readyMask));
		const int joinedCount = static_cast<int>(std::popcount(lobbyState.joinedMask));
		return StrCat(
		    _("Raid status"),
		    "\n",
		    _("Phase: "), GetRaidPhaseLabel(state.phase),
		    "\n",
		    _("Members: "), joinedCount, "/8",
		    "\n",
		    _("Ready: "), readyCount, "/", std::max(joinedCount, static_cast<int>(RaidMinimumMembersToStart)),
		    "\n",
		    _("Attempts left: "), static_cast<int>(lobbyState.attemptsLeft));
	}

	return _("Usage: /raid <create|invite|join|ready|start|status|reset> [player]");
}

std::string TextCmdOps(const std::string_view parameter)
{
	const std::string param = AsciiStrToLower(parameter);
	if (param.empty())
		return _("Usage: /ops <snapshot|raiddiag|repairprog>");

	if (param == "snapshot") {
		const RaidInstanceState raidState = GetActiveRaidState();
		const GuildHallState guildState = GetGuildHallState();
		const RaidModCompatibilityStatus raidCompat = GetRaidModCompatibilityStatus();
		const GuildModCompatibilityStatus guildCompat = GetGuildModCompatibilityStatus();
		return StrCat(
		    _("Ops snapshot"),
		    "\n",
		    _("Raid phase: "), GetRaidPhaseLabel(raidState.phase),
		    "\n",
		    _("Raid sequence: "), raidState.sequence,
		    "\n",
		    _("Guild id: "), guildState.guildId.value,
		    "\n",
		    _("Guild members: "), static_cast<int>(guildState.memberCount),
		    "\n",
		    _("Raid API compatible: "), raidCompat.compatible ? "yes" : "no", " (v", raidCompat.requestedVersion, " -> v", raidCompat.activeVersion, ")",
		    "\n",
		    _("Guild API compatible: "), guildCompat.compatible ? "yes" : "no", " (v", guildCompat.requestedVersion, " -> v", guildCompat.activeVersion, ")");
	}

	if (param == "raiddiag") {
		const RaidInstanceState state = GetActiveRaidState();
		return StrCat(
		    _("Raid diagnostics"),
		    "\n",
		    _("Raid id: "), state.raidId.value,
		    "\n",
		    _("Difficulty: "), static_cast<int>(state.difficulty),
		    "\n",
		    _("Members joined: "), static_cast<int>(GetRaidMemberCount(state)),
		    "\n",
		    _("Members ready: "), static_cast<int>(GetRaidReadyCount(state)),
		    "\n",
		    _("Revision/Sequence: "), state.snapshotRevision, "/", state.sequence,
		    "\n",
		    _("Lockout expiration tick: "), state.lockoutExpirationTick);
	}

	if (param == "repairprog") {
		ClearRaidCheckpoint(RaidDifficulty::Normal);
		ClearRaidCheckpoint(RaidDifficulty::Nightmare);
		ClearRaidCheckpoint(RaidDifficulty::Hell);
		return _("Progression repair complete: cleared raid checkpoints for all difficulties.");
	}

	return _("Usage: /ops <snapshot|raiddiag|repairprog>");
}

std::vector<TextCmdItem> TextCmdList = {
	{ "/help", N_("Prints help overview or help for a specific command."), N_("[command]"), &TextCmdHelp },
	{ "/arena", N_("Enter a PvP Arena."), N_("<arena-number>"), &TextCmdArena },
	{ "/arenapot", N_("Gives Arena Potions."), N_("<number>"), &TextCmdArenaPot },
	{ "/inspect", N_("Inspects stats and equipment of another player."), N_("<player name>"), &TextCmdInspect },
	{ "/seedinfo", N_("Show seed infos for current level."), "", &TextCmdLevelSeed },
	{ "/ping", N_("Show latency statistics for another player."), N_("<player name>"), &TextCmdPing },
	{ "/raid", N_("Manage raid lobby actions and status."), N_("<create|invite|join|ready|start|status|reset> [player]"), &TextCmdRaid },
	{ "/ops", N_("Operator tools for telemetry and raid/guild diagnostics."), N_("<snapshot|raiddiag|repairprog>"), &TextCmdOps },
};

} // namespace

bool CheckChatCommand(const std::string_view text)
{
	if (text.size() < 1 || text[0] != '/')
		return false;

	auto textCmdIterator = c_find_if(TextCmdList, [&](const TextCmdItem &elem) { return text.find(elem.text) == 0 && (text.length() == elem.text.length() || text[elem.text.length()] == ' '); });
	if (textCmdIterator == TextCmdList.end()) {
		InitDiabloMsg(StrCat(_("Command "), "\"", text, "\"", _(" is unknown.")));
		return true;
	}

	const TextCmdItem &textCmd = *textCmdIterator;
	std::string_view parameter = "";
	if (text.length() > (textCmd.text.length() + 1))
		parameter = text.substr(textCmd.text.length() + 1);
	const std::string result = textCmd.actionProc(parameter);
	if (result != "")
		InitDiabloMsg(result);
	return true;
}

} // namespace devilution
