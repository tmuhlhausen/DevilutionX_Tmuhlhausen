#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "dvlnet/runtime_hooks.hpp"
#include "utils/endian_swap.hpp"

namespace devilution {

inline bool NetReadBufferedCmdSize(const TCmd *cmd, size_t maxCmdSize, size_t &messageSize)
{
	if (maxCmdSize < sizeof(TCmd))
		return false;

	switch (cmd->bCmd) {
	case CMD_STAND:
	case CMD_PLRALIVE:
	case CMD_DEACTIVATEPORTAL:
	case CMD_RETOWN:
	case CMD_FRIENDLYMODE:
	case CMD_CHEAT_EXPERIENCE:
	case CMD_DEBUG:
	case CMD_SETSHIELD:
	case CMD_REMSHIELD:
	case CMD_NAKRUL:
	case CMD_OPENHIVE:
	case CMD_OPENGRAVE:
		messageSize = sizeof(TCmd);
		return true;
	case CMD_WALKXY:
	case CMD_RATTACKXY:
	case CMD_OPOBJXY:
	case CMD_DISARMXY:
	case CMD_OPOBJT:
	case CMD_OPENDOOR:
	case CMD_CLOSEDOOR:
	case CMD_OPERATEOBJ:
	case CMD_BREAKOBJ:
	case CMD_SATTACKXY:
		messageSize = sizeof(TCmdLoc);
		return maxCmdSize >= messageSize;
	case CMD_ADDSTR:
	case CMD_ADDMAG:
	case CMD_ADDDEX:
	case CMD_ADDVIT:
	case CMD_ATTACKID:
	case CMD_ATTACKPID:
	case CMD_RATTACKID:
	case CMD_RATTACKPID:
	case CMD_RESURRECT:
	case CMD_KNOCKBACK:
	case CMD_PLRDEAD:
	case CMD_DELINVITEMS:
	case CMD_DELBELTITEMS:
	case CMD_PLRLEVEL:
	case CMD_HEALOTHER:
	case CMD_SETSTR:
	case CMD_SETMAG:
	case CMD_SETDEX:
	case CMD_SETVIT:
	case CMD_SETREFLECT:
		messageSize = sizeof(TCmdParam1);
		return maxCmdSize >= messageSize;
	case CMD_NEWLVL:
	case CMD_CHANGE_SPELL_LEVEL:
		messageSize = sizeof(TCmdParam2);
		return maxCmdSize >= messageSize;
	case CMD_SPELLID:
	case CMD_SPELLPID:
		messageSize = sizeof(TCmdParam4);
		return maxCmdSize >= messageSize;
	case CMD_MONSTDAMAGE:
		messageSize = sizeof(TCmdMonDamage);
		return maxCmdSize >= messageSize;
	case CMD_SPELLXY:
	case CMD_PLAYER_JOINLEVEL:
	case CMD_ACTIVATEPORTAL:
		messageSize = sizeof(TCmdLocParam3);
		return maxCmdSize >= messageSize;
	case CMD_GOTOGETITEM:
	case CMD_GOTOAGETITEM:
	case CMD_MONSTDEATH:
	case CMD_REQUESTSPAWNGOLEM:
	case CMD_TALKXY:
		messageSize = sizeof(TCmdLocParam1);
		return maxCmdSize >= messageSize;
	case CMD_SPELLXYD:
		messageSize = sizeof(TCmdLocParam5);
		return maxCmdSize >= messageSize;
	case CMD_CHANGEPLRITEMS:
	case CMD_CHANGEINVITEMS:
	case CMD_CHANGEBELTITEMS:
		messageSize = sizeof(TCmdChItem);
		return maxCmdSize >= messageSize;
	case CMD_DELPLRITEMS:
		messageSize = sizeof(TCmdDelItem);
		return maxCmdSize >= messageSize;
	case CMD_PLRDAMAGE:
		messageSize = sizeof(TCmdDamage);
		return maxCmdSize >= messageSize;
	case CMD_PUTITEM:
	case CMD_SPAWNITEM:
	case CMD_SYNCPUTITEM:
	case CMD_DROPITEM:
		messageSize = sizeof(TCmdPItem);
		return maxCmdSize >= messageSize;
	case CMD_REQUESTGITEM:
	case CMD_REQUESTAGITEM:
	case CMD_GETITEM:
	case CMD_AGETITEM:
	case CMD_ITEMEXTRA:
		messageSize = sizeof(TCmdGItem);
		return maxCmdSize >= messageSize;
	case CMD_SYNCQUEST:
		messageSize = sizeof(TCmdQuest);
		return maxCmdSize >= messageSize;
	case CMD_SPAWNMONSTER:
		messageSize = sizeof(TCmdSpawnMonster);
		return maxCmdSize >= messageSize;
	case CMD_GUILD_CREATE:
		messageSize = sizeof(TCmdGuildCreate);
		return maxCmdSize >= messageSize;
	case CMD_GUILD_INVITE:
	case CMD_GUILD_JOIN:
	case CMD_GUILD_LEAVE:
	case CMD_GUILD_PROMOTE:
	case CMD_GUILD_KICK:
		messageSize = sizeof(TCmdGuildAction);
		return maxCmdSize >= messageSize;
	case CMD_GUILD_STATE:
		messageSize = sizeof(TCmdGuildState);
		return maxCmdSize >= messageSize;
	case CMD_RAID_CREATE:
	case CMD_RAID_INVITE:
	case CMD_RAID_JOIN:
	case CMD_RAID_LEAVE:
	case CMD_RAID_READY_TOGGLE:
	case CMD_RAID_READY:
	case CMD_RAID_START:
	case CMD_RAID_RESET:
		messageSize = sizeof(TCmdRaidAction);
		return maxCmdSize >= messageSize;
	case CMD_RAID_EVENT:
	case CMD_RAID_CHECKPOINT:
		messageSize = sizeof(TCmdRaidEvent);
		return maxCmdSize >= messageSize;
	case CMD_RAID_STATE_SYNC:
		messageSize = sizeof(TCmdRaidState);
		return maxCmdSize >= messageSize;
	case CMD_RAID_SNAPSHOT:
		messageSize = sizeof(TCmdRaidSnapshot);
		return maxCmdSize >= messageSize;
	case CMD_ACK_PLRINFO:
	case CMD_SEND_PLRINFO:
	case CMD_DLEVEL:
	case CMD_DLEVEL_JUNK:
	case CMD_DLEVEL_END: {
		if (maxCmdSize < sizeof(TCmdPlrInfoHdr))
			return false;
		const auto &hdr = reinterpret_cast<const TCmdPlrInfoHdr &>(*cmd);
		messageSize = sizeof(TCmdPlrInfoHdr) + Swap16LE(hdr.wBytes);
		return maxCmdSize >= messageSize;
	}
	case CMD_STRING: {
		const auto &strCmd = reinterpret_cast<const TCmdString &>(*cmd);
		const size_t headerSize = sizeof(strCmd) - sizeof(strCmd.str);
		const size_t capped = std::min<size_t>(MAX_SEND_STR_LEN, maxCmdSize - headerSize);
		const char *end = static_cast<const char *>(memchr(strCmd.str, '\0', capped));
		messageSize = headerSize + (end != nullptr ? static_cast<size_t>(end - strCmd.str) + 1 : capped);
		return maxCmdSize >= messageSize;
	}
	case CMD_SYNCDATA: {
		if (maxCmdSize < sizeof(TSyncHeader))
			return false;
		const auto &hdr = reinterpret_cast<const TSyncHeader &>(*cmd);
		messageSize = sizeof(TSyncHeader) + Swap16LE(hdr.wLen);
		return maxCmdSize >= messageSize;
	}
	default:
		return false;
	}
}

inline uint8_t NetActionBucketForCmd(_cmd_id cmd)
{
	switch (cmd) {
	case CMD_ATTACKID:
	case CMD_ATTACKPID:
	case CMD_SATTACKXY:
		return 1;
	case CMD_RATTACKID:
	case CMD_RATTACKPID:
	case CMD_RATTACKXY:
		return 2;
	case CMD_SPELLID:
	case CMD_SPELLPID:
	case CMD_SPELLXY:
	case CMD_SPELLXYD:
	case CMD_HEALOTHER:
	case CMD_RESURRECT:
		return 3;
	case CMD_OPOBJXY:
	case CMD_DISARMXY:
	case CMD_OPOBJT:
	case CMD_OPENDOOR:
	case CMD_CLOSEDOOR:
	case CMD_OPERATEOBJ:
	case CMD_BREAKOBJ:
		return 4;
	case CMD_TALKXY:
		return 5;
	case CMD_GOTOGETITEM:
	case CMD_GOTOAGETITEM:
	case CMD_GETITEM:
	case CMD_AGETITEM:
	case CMD_REQUESTGITEM:
	case CMD_REQUESTAGITEM:
	case CMD_PUTITEM:
	case CMD_DROPITEM:
	case CMD_SPAWNITEM:
	case CMD_SYNCPUTITEM:
		return 6;
	default:
		return 7;
	}
}

inline bool NetValidateBufferedCommands(uint8_t playerId, const std::byte *data, size_t size)
{
	for (size_t offset = 0; offset < size;) {
		const auto *cmd = reinterpret_cast<const TCmd *>(data + offset);
		size_t messageSize = 0;
		if (!NetReadBufferedCmdSize(cmd, size - offset, messageSize) || messageSize == 0)
			return false;

		if (cmd->bCmd == CMD_WALKXY) {
			const auto &walk = reinterpret_cast<const TCmdLoc &>(*cmd);
			if (!NetOnRemoteWalkCommand(playerId, { walk.x, walk.y }))
				return false;
		} else {
			switch (cmd->bCmd) {
			case CMD_RATTACKXY:
			case CMD_OPOBJXY:
			case CMD_DISARMXY:
			case CMD_OPOBJT:
			case CMD_ATTACKID:
			case CMD_ATTACKPID:
			case CMD_RATTACKID:
			case CMD_RATTACKPID:
			case CMD_SPELLID:
			case CMD_SPELLPID:
			case CMD_RESURRECT:
			case CMD_KNOCKBACK:
			case CMD_GOTOGETITEM:
			case CMD_GOTOAGETITEM:
			case CMD_OPENDOOR:
			case CMD_CLOSEDOOR:
			case CMD_OPERATEOBJ:
			case CMD_BREAKOBJ:
			case CMD_SATTACKXY:
			case CMD_SPELLXY:
			case CMD_SPELLXYD:
			case CMD_HEALOTHER:
			case CMD_TALKXY:
			case CMD_GETITEM:
			case CMD_AGETITEM:
			case CMD_REQUESTGITEM:
			case CMD_REQUESTAGITEM:
			case CMD_PUTITEM:
			case CMD_DROPITEM:
			case CMD_SPAWNITEM:
			case CMD_SYNCPUTITEM:
				if (!NetOnRemoteActionCommand(playerId, NetActionBucketForCmd(cmd->bCmd)))
					return false;
				break;
			default:
				break;
			}
		}

		offset += messageSize;
	}

	return !NetShouldDropPlayer(playerId);
}

} // namespace devilution
