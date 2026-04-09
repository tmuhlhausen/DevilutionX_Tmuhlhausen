/**
 * @file playerdat.cpp
 *
 * Implementation of all player data.
 */

#include "tables/playerdat.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <charconv>
#include <cstdint>
#include <vector>

#include <expected.hpp>
#include <fmt/format.h>
#include <magic_enum/magic_enum_utility.hpp>

#include "data/file.hpp"
#include "data/record_reader.hpp"
#include "data/value_reader.hpp"
#include "items.h"
#include "player.h"
#include "tables/textdat.h"
#include "utils/language.h"
#include "utils/static_vector.hpp"
#include "utils/str_cat.hpp"

namespace devilution {

namespace {

class ExperienceData {
	/** Specifies the experience point limit of each level. */
	std::vector<uint32_t> levelThresholds;

public:
	[[nodiscard]] uint8_t getMaxLevel() const
	{
		return static_cast<uint8_t>(std::min<size_t>(levelThresholds.size(), std::numeric_limits<uint8_t>::max()));
	}

	DVL_REINITIALIZES void clear()
	{
		levelThresholds.clear();
	}

	[[nodiscard]] uint32_t getThresholdForLevel(unsigned level) const
	{
		if (level > 0)
			return levelThresholds[std::min<unsigned>(level - 1, getMaxLevel())];

		return 0;
	}

	void setThresholdForLevel(unsigned level, uint32_t experience)
	{
		if (level > 0) {
			if (level > levelThresholds.size()) {
				// To avoid ValidatePlayer() resetting players to 0 experience we need to use the maximum possible value here
				// As long as the file has no gaps it'll get initialised properly.
				levelThresholds.resize(level, std::numeric_limits<uint32_t>::max());
			}

			levelThresholds[static_cast<size_t>(level - 1)] = experience;
		}
	}
} ExperienceData;

class NephilimExperienceData {
	/** Specifies the Nephilim experience point limit of each Nephilim level. */
	std::vector<uint64_t> levelThresholds;

public:
	[[nodiscard]] uint16_t getMaxLevel() const
	{
		return static_cast<uint16_t>(std::min<size_t>(levelThresholds.size(), std::numeric_limits<uint16_t>::max()));
	}

	DVL_REINITIALIZES void clear()
	{
		levelThresholds.clear();
	}

	[[nodiscard]] uint64_t getThresholdForLevel(unsigned level) const
	{
		if (level > 0)
			return levelThresholds[std::min<unsigned>(level - 1, getMaxLevel())];

		return 0;
	}

	void setThresholdForLevel(unsigned level, uint64_t experience)
	{
		if (level > 0) {
			if (level > levelThresholds.size()) {
				levelThresholds.resize(level, std::numeric_limits<uint64_t>::max());
			}

			levelThresholds[static_cast<size_t>(level - 1)] = experience;
		}
	}
} NephilimExperienceData;

enum class ExperienceColumn {
	Level,
	Experience,
	LAST = Experience
};

struct ProgressionCurveEntry {
	uint16_t minLevel = 1;
	uint16_t xpGainPercent = 100;
	uint16_t normalDropRatePermille = 1000;
	uint16_t nightmareDropRatePermille = 1000;
	uint16_t hellDropRatePermille = 1000;
	uint16_t normalAncientChancePermille = 1000;
	uint16_t nightmareAncientChancePermille = 1000;
	uint16_t hellAncientChancePermille = 1000;
};

std::vector<ProgressionCurveEntry> ProgressionCurveData;

const ProgressionCurveEntry &GetProgressionCurveForLevel(unsigned level)
{
	static const ProgressionCurveEntry fallbackCurve {};
	if (ProgressionCurveData.empty())
		return fallbackCurve;

	const ProgressionCurveEntry *curve = &ProgressionCurveData.front();
	for (const ProgressionCurveEntry &entry : ProgressionCurveData) {
		if (entry.minLevel > level)
			break;
		curve = &entry;
	}
	return *curve;
}

tl::expected<ExperienceColumn, ColumnDefinition::Error> mapExperienceColumnFromName(std::string_view name)
{
	if (name == "Level") {
		return ExperienceColumn::Level;
	}
	if (name == "Experience") {
		return ExperienceColumn::Experience;
	}
	return tl::unexpected { ColumnDefinition::Error::UnknownColumn };
}

void ReloadExperienceData()
{
	constexpr std::string_view filename = "txtdata\\Experience.tsv";
	auto dataFileResult = DataFile::load(filename);
	if (!dataFileResult.has_value()) {
		DataFile::reportFatalError(dataFileResult.error(), filename);
	}
	DataFile &dataFile = dataFileResult.value();

	constexpr unsigned ExpectedColumnCount = enum_size<ExperienceColumn>::value;

	std::array<ColumnDefinition, ExpectedColumnCount> columns;
	auto parseHeaderResult = dataFile.parseHeader<ExperienceColumn>(columns.data(), columns.data() + columns.size(), mapExperienceColumnFromName);

	if (!parseHeaderResult.has_value()) {
		DataFile::reportFatalError(parseHeaderResult.error(), filename);
	}

	ExperienceData.clear();
	for (DataFileRecord record : dataFile) {
		uint8_t level = 0;
		uint32_t experience = 0;
		bool skipRecord = false;

		FieldIterator fieldIt = record.begin();
		const FieldIterator endField = record.end();
		for (auto &column : columns) {
			fieldIt += column.skipLength;

			if (fieldIt == endField) {
				DataFile::reportFatalError(DataFile::Error::NotEnoughColumns, filename);
			}

			DataFileField field = *fieldIt;

			switch (static_cast<ExperienceColumn>(column)) {
			case ExperienceColumn::Level: {
				auto parseIntResult = field.parseInt(level);

				if (!parseIntResult.has_value()) {
					if (*field == "MaxLevel") {
						skipRecord = true;
					} else {
						DataFile::reportFatalFieldError(parseIntResult.error(), filename, "Level", field);
					}
				}
			} break;

			case ExperienceColumn::Experience: {
				auto parseIntResult = field.parseInt(experience);

				if (!parseIntResult.has_value()) {
					DataFile::reportFatalFieldError(parseIntResult.error(), filename, "Experience", field);
				}
			} break;

			default:
				break;
			}

			if (skipRecord)
				break;

			++fieldIt;
		}

		if (!skipRecord)
			ExperienceData.setThresholdForLevel(level, experience);
	}
}

void ReloadNephilimExperienceData()
{
	constexpr std::string_view filename = "txtdata\\NephilimExperience.tsv";
	auto dataFileResult = DataFile::load(filename);
	if (!dataFileResult.has_value()) {
		DataFile::reportFatalError(dataFileResult.error(), filename);
	}
	DataFile &dataFile = dataFileResult.value();

	constexpr unsigned ExpectedColumnCount = enum_size<ExperienceColumn>::value;

	std::array<ColumnDefinition, ExpectedColumnCount> columns;
	auto parseHeaderResult = dataFile.parseHeader<ExperienceColumn>(columns.data(), columns.data() + columns.size(), mapExperienceColumnFromName);

	if (!parseHeaderResult.has_value()) {
		DataFile::reportFatalError(parseHeaderResult.error(), filename);
	}

	NephilimExperienceData.clear();
	for (DataFileRecord record : dataFile) {
		uint16_t level = 0;
		uint64_t experience = 0;
		bool skipRecord = false;

		FieldIterator fieldIt = record.begin();
		const FieldIterator endField = record.end();
		for (auto &column : columns) {
			fieldIt += column.skipLength;

			if (fieldIt == endField) {
				DataFile::reportFatalError(DataFile::Error::NotEnoughColumns, filename);
			}

			DataFileField field = *fieldIt;

			switch (static_cast<ExperienceColumn>(column)) {
			case ExperienceColumn::Level: {
				auto parseIntResult = field.parseInt(level);

				if (!parseIntResult.has_value()) {
					if (*field == "MaxLevel") {
						skipRecord = true;
					} else {
						DataFile::reportFatalFieldError(parseIntResult.error(), filename, "Level", field);
					}
				}
			} break;

			case ExperienceColumn::Experience: {
				auto parseIntResult = field.parseInt(experience);

				if (!parseIntResult.has_value()) {
					DataFile::reportFatalFieldError(parseIntResult.error(), filename, "Experience", field);
				}
			} break;

			default:
				break;
			}

			if (skipRecord)
				break;

			++fieldIt;
		}

		if (!skipRecord)
			NephilimExperienceData.setThresholdForLevel(level, experience);
	}
}

void ReloadProgressionCurveData()
{
	constexpr std::string_view filename = "txtdata\\progression_curves.tsv";
	DataFile dataFile = DataFile::loadOrDie(filename);
	dataFile.skipHeaderOrDie(filename);

	ProgressionCurveData.clear();
	for (DataFileRecord record : dataFile) {
		ValueReader reader { record, filename };
		ProgressionCurveEntry &entry = ProgressionCurveData.emplace_back();
		reader.readInt("MinLevel", entry.minLevel);
		reader.readInt("XpGainPercent", entry.xpGainPercent);
		reader.readInt("NormalDropRatePermille", entry.normalDropRatePermille);
		reader.readInt("NightmareDropRatePermille", entry.nightmareDropRatePermille);
		reader.readInt("HellDropRatePermille", entry.hellDropRatePermille);
		reader.readInt("NormalAncientChancePermille", entry.normalAncientChancePermille);
		reader.readInt("NightmareAncientChancePermille", entry.nightmareAncientChancePermille);
		reader.readInt("HellAncientChancePermille", entry.hellAncientChancePermille);

		entry.xpGainPercent = std::clamp<uint16_t>(entry.xpGainPercent, 100, 500);
		entry.normalDropRatePermille = std::clamp<uint16_t>(entry.normalDropRatePermille, 100, 5000);
		entry.nightmareDropRatePermille = std::clamp<uint16_t>(entry.nightmareDropRatePermille, 100, 5000);
		entry.hellDropRatePermille = std::clamp<uint16_t>(entry.hellDropRatePermille, 100, 5000);
		entry.normalAncientChancePermille = std::clamp<uint16_t>(entry.normalAncientChancePermille, 100, 5000);
		entry.nightmareAncientChancePermille = std::clamp<uint16_t>(entry.nightmareAncientChancePermille, 100, 5000);
		entry.hellAncientChancePermille = std::clamp<uint16_t>(entry.hellAncientChancePermille, 100, 5000);
	}
}

tl::expected<PlayerClassFlag, std::string> ParsePlayerClassFlag(std::string_view value)
{
	const std::optional<PlayerClassFlag> enumValueOpt = magic_enum::enum_cast<PlayerClassFlag>(value);
	if (enumValueOpt.has_value()) {
		return enumValueOpt.value();
	}
	return tl::make_unexpected("Unknown enum value");
}

void LoadClassData(std::string_view classPath, ClassAttributes &attributes, PlayerCombatData &combat)
{
	const std::string filename = StrCat("txtdata\\classes\\", classPath, "\\attributes.tsv");
	tl::expected<DataFile, DataFile::Error> dataFileResult = DataFile::loadOrDie(filename);
	DataFile &dataFile = dataFileResult.value();
	dataFile.skipHeaderOrDie(filename);

	ValueReader reader { dataFile, filename };

	reader.readEnumList("classFlags", attributes.classFlags, ParsePlayerClassFlag);
	reader.readInt("baseStr", attributes.baseStr);
	reader.readInt("baseMag", attributes.baseMag);
	reader.readInt("baseDex", attributes.baseDex);
	reader.readInt("baseVit", attributes.baseVit);
	reader.readInt("maxStr", attributes.maxStr);
	reader.readInt("maxMag", attributes.maxMag);
	reader.readInt("maxDex", attributes.maxDex);
	reader.readInt("maxVit", attributes.maxVit);
	reader.readInt("blockBonus", combat.baseToBlock);
	reader.readDecimal("adjLife", attributes.adjLife);
	reader.readDecimal("adjMana", attributes.adjMana);
	reader.readDecimal("lvlLife", attributes.lvlLife);
	reader.readDecimal("lvlMana", attributes.lvlMana);
	reader.readDecimal("chrLife", attributes.chrLife);
	reader.readDecimal("chrMana", attributes.chrMana);
	reader.readDecimal("itmLife", attributes.itmLife);
	reader.readDecimal("itmMana", attributes.itmMana);
	reader.readInt("baseMagicToHit", combat.baseMagicToHit);
	reader.readInt("baseMeleeToHit", combat.baseMeleeToHit);
	reader.readInt("baseRangedToHit", combat.baseRangedToHit);
}

void LoadClassStartingLoadoutData(std::string_view classPath, PlayerStartingLoadoutData &startingLoadoutData)
{
	const std::string filename = StrCat("txtdata\\classes\\", classPath, "\\starting_loadout.tsv");
	tl::expected<DataFile, DataFile::Error> dataFileResult = DataFile::loadOrDie(filename);
	DataFile &dataFile = dataFileResult.value();
	dataFile.skipHeaderOrDie(filename);

	ValueReader reader { dataFile, filename };

	reader.read("skill", startingLoadoutData.skill, ParseSpellId);
	reader.read("spell", startingLoadoutData.spell, ParseSpellId);
	reader.readInt("spellLevel", startingLoadoutData.spellLevel);
	for (size_t i = 0; i < startingLoadoutData.items.size(); ++i) {
		reader.read(StrCat("item", i), startingLoadoutData.items[i], ParseItemId);
	}
	reader.readInt("gold", startingLoadoutData.gold);
}

void LoadClassSpriteData(std::string_view classPath, PlayerSpriteData &spriteData)
{
	const std::string filename = StrCat("txtdata\\classes\\", classPath, "\\sprites.tsv");
	tl::expected<DataFile, DataFile::Error> dataFileResult = DataFile::loadOrDie(filename);
	DataFile &dataFile = dataFileResult.value();
	dataFile.skipHeaderOrDie(filename);

	ValueReader reader { dataFile, filename };

	reader.readString("classPath", spriteData.classPath);
	reader.readChar("classChar", spriteData.classChar);
	reader.readString("trn", spriteData.trn);
	reader.readInt("stand", spriteData.stand);
	reader.readInt("walk", spriteData.walk);
	reader.readInt("attack", spriteData.attack);
	reader.readInt("bow", spriteData.bow);
	reader.readInt("swHit", spriteData.swHit);
	reader.readInt("block", spriteData.block);
	reader.readInt("lightning", spriteData.lightning);
	reader.readInt("fire", spriteData.fire);
	reader.readInt("magic", spriteData.magic);
	reader.readInt("death", spriteData.death);
}

void LoadClassAnimData(std::string_view classPath, PlayerAnimData &animData)
{
	const std::string filename = StrCat("txtdata\\classes\\", classPath, "\\animations.tsv");
	tl::expected<DataFile, DataFile::Error> dataFileResult = DataFile::loadOrDie(filename);
	DataFile &dataFile = dataFileResult.value();
	dataFile.skipHeaderOrDie(filename);

	ValueReader reader { dataFile, filename };

	reader.readInt("unarmedFrames", animData.unarmedFrames);
	reader.readInt("unarmedActionFrame", animData.unarmedActionFrame);
	reader.readInt("unarmedShieldFrames", animData.unarmedShieldFrames);
	reader.readInt("unarmedShieldActionFrame", animData.unarmedShieldActionFrame);
	reader.readInt("swordFrames", animData.swordFrames);
	reader.readInt("swordActionFrame", animData.swordActionFrame);
	reader.readInt("swordShieldFrames", animData.swordShieldFrames);
	reader.readInt("swordShieldActionFrame", animData.swordShieldActionFrame);
	reader.readInt("bowFrames", animData.bowFrames);
	reader.readInt("bowActionFrame", animData.bowActionFrame);
	reader.readInt("axeFrames", animData.axeFrames);
	reader.readInt("axeActionFrame", animData.axeActionFrame);
	reader.readInt("maceFrames", animData.maceFrames);
	reader.readInt("maceActionFrame", animData.maceActionFrame);
	reader.readInt("maceShieldFrames", animData.maceShieldFrames);
	reader.readInt("maceShieldActionFrame", animData.maceShieldActionFrame);
	reader.readInt("staffFrames", animData.staffFrames);
	reader.readInt("staffActionFrame", animData.staffActionFrame);
	reader.readInt("idleFrames", animData.idleFrames);
	reader.readInt("walkingFrames", animData.walkingFrames);
	reader.readInt("blockingFrames", animData.blockingFrames);
	reader.readInt("deathFrames", animData.deathFrames);
	reader.readInt("castingFrames", animData.castingFrames);
	reader.readInt("recoveryFrames", animData.recoveryFrames);
	reader.readInt("townIdleFrames", animData.townIdleFrames);
	reader.readInt("townWalkingFrames", animData.townWalkingFrames);
	reader.readInt("castingActionFrame", animData.castingActionFrame);
}

void LoadClassSounds(std::string_view classPath, ankerl::unordered_dense::map<HeroSpeech, SfxID> &sounds)
{
	const std::string filename = StrCat("txtdata\\classes\\", classPath, "\\sounds.tsv");
	tl::expected<DataFile, DataFile::Error> dataFileResult = DataFile::loadOrDie(filename);
	DataFile &dataFile = dataFileResult.value();
	dataFile.skipHeaderOrDie(filename);

	ValueReader reader { dataFile, filename };

	magic_enum::enum_for_each<HeroSpeech>([&](const HeroSpeech speech) {
		reader.read(magic_enum::enum_name(speech), sounds[speech], ParseSfxId);
	});
}

/** Contains the data related to each player class. */
std::vector<PlayerData> PlayersData;

std::vector<ClassAttributes> ClassAttributesPerClass;

std::vector<PlayerCombatData> PlayersCombatData;

std::vector<PlayerStartingLoadoutData> PlayersStartingLoadoutData;

/** Contains the data related to each player class. */
std::vector<PlayerSpriteData> PlayersSpriteData;

std::vector<PlayerAnimData> PlayersAnimData;

std::vector<ankerl::unordered_dense::map<HeroSpeech, SfxID>> herosounds;

} // namespace

void LoadClassDatFromFile(DataFile &dataFile, const std::string_view filename)
{
	dataFile.skipHeaderOrDie(filename);

	PlayersData.reserve(PlayersData.size() + dataFile.numRecords());

	for (DataFileRecord record : dataFile) {
		if (PlayersData.size() >= static_cast<size_t>(HeroClass::NUM_MAX_CLASSES)) {
			DisplayFatalErrorAndExit(_("Loading Class Data Failed"), fmt::format(fmt::runtime(_("Could not add a class, since the maximum class number of {} has already been reached.")), static_cast<size_t>(HeroClass::NUM_MAX_CLASSES)));
		}

		RecordReader reader { record, filename };

		PlayerData &playerData = PlayersData.emplace_back();

		reader.readString("className", playerData.className);
		reader.readString("folderName", playerData.folderName);
		reader.readInt("portrait", playerData.portrait);
		reader.readString("inv", playerData.inv);
	}
}

namespace {

void LoadClassDat()
{
	const std::string_view filename = "txtdata\\classes\\classdat.tsv";
	DataFile dataFile = DataFile::loadOrDie(filename);
	PlayersData.clear();
	LoadClassDatFromFile(dataFile, filename);

	PlayersData.shrink_to_fit();
}

void LoadClassesAttributes()
{
	ClassAttributesPerClass.clear();
	ClassAttributesPerClass.reserve(PlayersData.size());
	PlayersCombatData.clear();
	PlayersCombatData.reserve(PlayersData.size());
	PlayersStartingLoadoutData.clear();
	PlayersStartingLoadoutData.reserve(PlayersData.size());
	PlayersSpriteData.clear();
	PlayersSpriteData.reserve(PlayersData.size());
	PlayersAnimData.clear();
	PlayersAnimData.reserve(PlayersData.size());
	herosounds.clear();
	herosounds.reserve(PlayersData.size());

	for (const PlayerData &playerData : PlayersData) {
		LoadClassData(playerData.folderName, ClassAttributesPerClass.emplace_back(), PlayersCombatData.emplace_back());
		LoadClassStartingLoadoutData(playerData.folderName, PlayersStartingLoadoutData.emplace_back());
		LoadClassSpriteData(playerData.folderName, PlayersSpriteData.emplace_back());
		LoadClassAnimData(playerData.folderName, PlayersAnimData.emplace_back());
		LoadClassSounds(playerData.folderName, herosounds.emplace_back());
	}
}

} // namespace

const ClassAttributes &GetClassAttributes(HeroClass playerClass)
{
	return ClassAttributesPerClass[static_cast<size_t>(playerClass)];
}

void LoadPlayerDataFiles()
{
	ReloadExperienceData();
	ReloadNephilimExperienceData();
	ReloadProgressionCurveData();
	LoadClassDat();
	LoadClassesAttributes();
}

SfxID GetHeroSound(HeroClass clazz, HeroSpeech speech)
{
	const auto playerClassIndex = static_cast<size_t>(clazz);
	assert(playerClassIndex < herosounds.size());
	const auto findIt = herosounds[playerClassIndex].find(speech);
	if (findIt != herosounds[playerClassIndex].end()) {
		return findIt->second;
	}

	return SfxID::None;
}

uint32_t GetNextExperienceThresholdForLevel(unsigned level)
{
	return ExperienceData.getThresholdForLevel(level);
}

uint8_t GetMaximumCharacterLevel()
{
	return ExperienceData.getMaxLevel();
}

uint64_t GetNextNephilimThresholdForLevel(unsigned level)
{
	return NephilimExperienceData.getThresholdForLevel(level);
}

uint16_t GetMaximumNephilimLevel()
{
	return NephilimExperienceData.getMaxLevel();
}

uint16_t GetExperienceGainMultiplierForLevel(unsigned level)
{
	return GetProgressionCurveForLevel(level).xpGainPercent;
}

uint16_t GetDropRateMultiplierForLevel(unsigned level, int difficulty)
{
	const ProgressionCurveEntry &curve = GetProgressionCurveForLevel(level);
	switch (difficulty) {
	case 0:
		return curve.normalDropRatePermille;
	case 1:
		return curve.nightmareDropRatePermille;
	case 2:
		return curve.hellDropRatePermille;
	default:
		return 1000;
	}
}

uint16_t GetAncientChanceMultiplierForLevel(unsigned level, int difficulty)
{
	const ProgressionCurveEntry &curve = GetProgressionCurveForLevel(level);
	switch (difficulty) {
	case 0:
		return curve.normalAncientChancePermille;
	case 1:
		return curve.nightmareAncientChancePermille;
	case 2:
		return curve.hellAncientChancePermille;
	default:
		return 1000;
	}
}

size_t GetNumPlayerClasses()
{
	return PlayersData.size();
}

const PlayerData &GetPlayerDataForClass(HeroClass playerClass)
{
	const auto playerClassIndex = static_cast<size_t>(playerClass);
	assert(playerClassIndex < PlayersData.size());
	return PlayersData[playerClassIndex];
}

const PlayerCombatData &GetPlayerCombatDataForClass(HeroClass pClass)
{
	const auto playerClassIndex = static_cast<size_t>(pClass);
	assert(playerClassIndex < PlayersCombatData.size());
	return PlayersCombatData[playerClassIndex];
}

const PlayerStartingLoadoutData &GetPlayerStartingLoadoutForClass(HeroClass pClass)
{
	const auto playerClassIndex = static_cast<size_t>(pClass);
	assert(playerClassIndex < PlayersStartingLoadoutData.size());
	return PlayersStartingLoadoutData[playerClassIndex];
}

const PlayerSpriteData &GetPlayerSpriteDataForClass(HeroClass pClass)
{
	const auto playerClassIndex = static_cast<size_t>(pClass);
	assert(playerClassIndex < PlayersSpriteData.size());
	return PlayersSpriteData[playerClassIndex];
}

const PlayerAnimData &GetPlayerAnimDataForClass(HeroClass pClass)
{
	const auto playerClassIndex = static_cast<size_t>(pClass);
	assert(playerClassIndex < PlayersAnimData.size());
	return PlayersAnimData[playerClassIndex];
}

} // namespace devilution
