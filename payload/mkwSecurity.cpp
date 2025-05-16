#if RMC

#  include "import/mkw/net/eventHandler.hpp"
#  include "import/mkw/net/itemHandler.hpp"
#  include "import/mkw/net/matchHeaderHandler.hpp"
#  include "import/mkw/net/roomHandler.hpp"
#  include "import/mkw/net/selectHandler.hpp"
#  include "import/mkw/net/userHandler.hpp"
#  include "import/mkw/registry.hpp"
#  include "import/mkw/system/raceConfig.hpp"
#  include "import/mkw/system/system.hpp"
#  include "wwfcLibC.hpp"

namespace wwfc::mkw::Security
{

using namespace Net;

static std::size_t s_vanillaPacketBufferSizes[sizeof(RacePacket::sizes)] = {
    sizeof(RacePacket),
    sizeof(MatchHeaderHandler::Packet),
    0x28,
    sizeof(SelectHandler::Packet),
    0x40 << 1,
    sizeof(UserHandler::Packet),
    sizeof(ItemHandler::Packet) << 1,
    sizeof(EventHandler::Packet),
};

static bool IsPacketSizeValid(RacePacket::EType packetType, u8 packetSize)
{
    if (packetType >= RacePacket::MatchHeader &&
        packetType <= RacePacket::Event) {
        if (packetSize == 0) {
            return true;
        }
    }

    std::size_t* packetBufferSizesPointer;
    if (!NetController::Instance()->inVanillaMatch()) {
        extern std::size_t packetBufferSizes[sizeof(RacePacket::sizes)] AT(
            RMCXD_PORT(0x8089A194, 0x80895AC4, 0x808992F4, 0x808885CC)
        );

        packetBufferSizesPointer = packetBufferSizes;
    } else {
        packetBufferSizesPointer = s_vanillaPacketBufferSizes;
    }

    switch (packetType) {
    case RacePacket::Header: {
        if (packetSize < sizeof(RacePacket) ||
            packetSize > packetBufferSizesPointer[RacePacket::Header]) {
            return false;
        }

        return true;
    }
    case RacePacket::MatchHeader: {
        if (packetSize < sizeof(MatchHeaderHandler::Packet) ||
            packetSize > packetBufferSizesPointer[RacePacket::MatchHeader]) {
            return false;
        }

        return true;
    }
    case RacePacket::MatchData: {
        if (packetSize < 0x28 ||
            packetSize > packetBufferSizesPointer[RacePacket::MatchData]) {
            return false;
        }

        return true;
    }
    case RacePacket::RoomSelect: {
        // 'Room' packet
        if (packetSize < sizeof(SelectHandler::Packet)) {
            return packetSize == sizeof(RoomHandler::Packet);
        }

        // 'Select' packet
        if (packetSize > packetBufferSizesPointer[RacePacket::RoomSelect]) {
            return false;
        }

        return true;
    }
    case RacePacket::PlayerData: {
        if (packetSize < 0x40 ||
            packetSize > packetBufferSizesPointer[RacePacket::PlayerData]) {
            return false;
        }

        return true;
    }
    case RacePacket::User: {
        if (packetSize < sizeof(UserHandler::Packet) ||
            packetSize > packetBufferSizesPointer[RacePacket::User]) {
            return false;
        }

        return true;
    }
    case RacePacket::Item: {
        if (packetSize < sizeof(ItemHandler::Packet) ||
            packetSize > packetBufferSizesPointer[RacePacket::Item]) {
            return false;
        }

        return true;
    }
    case RacePacket::Event: {
        if (packetSize < sizeof(EventHandler::Packet::eventInfo) ||
            packetSize > packetBufferSizesPointer[RacePacket::Event]) {
            return false;
        }

        return true;
    }
    default: {
        return false;
    }
    }
}

static bool IsHeaderPacketDataValid(
    const void* /* packet */, u8 /* packetSize */, u8 /* playerAid */
)
{
    // This packet is validated by the function 'IsRacePacketValid'
    return true;
}

static bool IsMatchHeaderPacketDataValid(
    const void* packet, u8 /* packetSize */, u8 /* playerAid */
)
{
    using namespace mkw::Net;
    using namespace mkw::Registry;
    using namespace mkw::System;

    const MatchHeaderHandler::Packet* matchHeaderPacket =
        reinterpret_cast<const MatchHeaderHandler::Packet*>(packet);

    if (!NetController::Instance()->inVanillaRaceScene()) {
        return true;
    }

    RaceConfig::Scenario* scenario = &RaceConfig::Instance()->raceScenario();

    for (std::size_t n = 0;
         n < ARRAY_ELEMENT_COUNT(MatchHeaderHandler::Packet::player); n++) {
        MatchHeaderHandler::Packet::Player player =
            matchHeaderPacket->player[n];

        MatchHeaderHandler::Packet::Vehicle playerVehicle = player.vehicle;
        MatchHeaderHandler::Packet::Character playerCharacter =
            player.character;
        if (playerVehicle == MatchHeaderHandler::Packet::Vehicle::None &&
            playerCharacter == MatchHeaderHandler::Packet::Character::None) {
            continue;
        }

        Vehicle vehicle = static_cast<Vehicle>(playerVehicle);
        Character character = static_cast<Character>(playerCharacter);
        if (scenario->isOnlineVersusRace()) {
            if (!IsCombinationValidVS(character, vehicle)) {
                return false;
            }
        } else /* if (scenario->isOnlineBattle()) */ {
            if (!IsCombinationValidBT(character, vehicle)) {
                return false;
            }
        }
    }

    MatchHeaderHandler::Packet::Course currentCourse =
        matchHeaderPacket->course;
    if (currentCourse != MatchHeaderHandler::Packet::Course::None) {
        Course course = static_cast<Course>(currentCourse);
        if (scenario->isOnlineVersusRace()) {
            if (!IsRaceCourse(course)) {
                return false;
            }
        } else /* if (scenario->isOnlineBattle()) */ {
            if (!IsBattleCourse(course)) {
                return false;
            }
        }
    }

    return true;
}

static bool IsMatchDataPacketDataValid(
    const void* /* packet */, u8 /* packetSize */, u8 /* playerAid */
)
{
    return true;
}

static bool
IsRoomSelectPacketDataValid(const void* packet, u8 packetSize, u8 playerAid)
{
    // 'Room' packet
    if (packetSize == sizeof(RoomHandler::Packet)) {
        const RoomHandler::Packet* roomPacket =
            reinterpret_cast<const RoomHandler::Packet*>(packet);

        switch (roomPacket->event) {
        case RoomHandler::Packet::Event::StartRoom: {
            // Ensure that guests can't start rooms
            if (!NetController::Instance()->isAidTheServer(playerAid)) {
                return false;
            }
            break;
        }
        default: {
            break;
        }
        }
    }
    // 'Select' packet
    else {
        if (!NetController::Instance()->inVanillaMatch()) {
            return true;
        }

        System::RaceConfig::Scenario* scenario;
        if (static_cast<System::Scene::SceneID>(
                System::System::Instance().sceneManager()->getCurrentSceneID()
            ) == System::Scene::SceneID::Race) {
            scenario = &System::RaceConfig::Instance()->raceScenario();
        } else {
            scenario = &System::RaceConfig::Instance()->menuScenario();
        }

        const SelectHandler::Packet* selectPacket =
            reinterpret_cast<const SelectHandler::Packet*>(packet);
        for (std::size_t n = 0;
             n < ARRAY_ELEMENT_COUNT(SelectHandler::Packet::player); n++) {
            SelectHandler::Packet::Player player = selectPacket->player[n];

            SelectHandler::Packet::Player::Character selectedCharacter =
                player.character;
            SelectHandler::Packet::Player::Vehicle selectedVehicle =
                player.vehicle;
            if (selectedCharacter !=
                    SelectHandler::Packet::Player::Character::NotSelected ||
                selectedVehicle !=
                    SelectHandler::Packet::Player::Vehicle::NotSelected) {
                Registry::Character character =
                    static_cast<Registry::Character>(selectedCharacter);
                Registry::Vehicle vehicle =
                    static_cast<Registry::Vehicle>(selectedVehicle);
                if (scenario->isOnlineVersusRace()) {
                    if (!IsCombinationValidVS(character, vehicle)) {
                        return false;
                    }
                } else /* if (scenario->isOnlineBattle()) */ {
                    if (!IsCombinationValidBT(character, vehicle)) {
                        return false;
                    }
                }
            }

            SelectHandler::Packet::Player::CourseVote courseVote =
                selectPacket->player[n].courseVote;
            if (courseVote ==
                    SelectHandler::Packet::Player::CourseVote::NotSelected ||
                courseVote ==
                    SelectHandler::Packet::Player::CourseVote::Random) {
                continue;
            }
            Registry::Course course = static_cast<Registry::Course>(courseVote);
            if (scenario->isOnlineVersusRace()) {
                if (!IsRaceCourse(course)) {
                    return false;
                }
            } else /* if (scenario->isOnlineBattle()) */ {
                if (!IsBattleCourse(course)) {
                    return false;
                }
            }
        }

        if (selectPacket->selectedCourse !=
            SelectHandler::Packet::SelectedCourse::NotSelected) {
            Registry::Course course =
                static_cast<Registry::Course>(selectPacket->selectedCourse);
            if (scenario->isOnlineVersusRace()) {
                if (!IsRaceCourse(course)) {
                    return false;
                }
            } else /* if (scenario->isOnlineBattle()) */ {
                if (!IsBattleCourse(course)) {
                    return false;
                }
            }
        }
    }

    return true;
}

static bool IsPlayerDataPacketDataValid(
    const void* /* packet */, u8 /* packetSize */, u8 /* playerAid */
)
{
    return true;
}

static bool IsUserPacketDataValid(
    const void* packet, u8 /* packetSize */, u8 /* playerAid */
)
{
    const UserHandler::Packet* userPacket =
        reinterpret_cast<const UserHandler::Packet*>(packet);

    if (!userPacket->isValid()) {
        return false;
    }

    return true;
}

static bool
IsItemPacketDataValid(const void* packet, u8 packetSize, u8 /* playerAid */)
{
    using namespace mkw::Item;
    using namespace mkw::System;

    if (!NetController::Instance()->inVanillaRaceScene()) {
        return true;
    }

    RaceConfig::Scenario* scenario = &RaceConfig::Instance()->raceScenario();

    for (u8 n = 0; n < (packetSize >> 3); n++) {
        const ItemHandler::Packet* itemPacket =
            reinterpret_cast<const ItemHandler::Packet*>(
                reinterpret_cast<const char*>(packet) +
                (sizeof(ItemHandler::Packet) * n)
            );

        ItemBox heldItem = static_cast<ItemBox>(itemPacket->heldItem);
        ItemBox trailedItem = static_cast<ItemBox>(itemPacket->trailedItem);

        if (scenario->isOnlineVersusRace()) {
            if (heldItem != ItemBox::NoItem && !IsHeldItemValidVS(heldItem)) {
                return false;
            }
            if (trailedItem != ItemBox::NoItem &&
                !IsTrailedItemValidVS(trailedItem)) {
                return false;
            }
        } else /* if (scenario->isOnlineBattle()) */ {
            if (scenario->isBalloonBattle()) {
                if (heldItem != ItemBox::NoItem &&
                    !IsHeldItemValidBB(heldItem)) {
                    return false;
                }
                if (trailedItem != ItemBox::NoItem &&
                    !IsTrailedItemValidBB(trailedItem)) {
                    return false;
                }
            } else /* if (scenario->isCoinRunners()) */ {
                if (heldItem != ItemBox::NoItem &&
                    !IsHeldItemValidCR(heldItem)) {
                    return false;
                }
                if (trailedItem != ItemBox::NoItem &&
                    !IsTrailedItemValidCR(trailedItem)) {
                    return false;
                }
            }
        }

        if (!itemPacket->isHeldPhaseValid()) {
            return false;
        }
        if (!itemPacket->isTrailPhaseValid()) {
            return false;
        }
    }

    return true;
}

static bool
IsEventPacketDataValid(const void* packet, u8 packetSize, u8 playerAid)
{
    if (static_cast<System::Scene::SceneID>(
            System::System::Instance().sceneManager()->getCurrentSceneID()
        ) != System::Scene::SceneID::Race) {
        return true;
    }

    const EventHandler::Packet* eventPacket =
        reinterpret_cast<const EventHandler::Packet*>(packet);

    // Always ensure that the packet does not contain any invalid item
    // objects, as this can cause a buffer overflow to occur.
    if (eventPacket->containsInvalidItemObject()) {
        return false;
    }

    if (!NetController::Instance()->inVanillaMatch()) {
        return true;
    }

    if (!eventPacket->isValid(packetSize, playerAid)) {
        return false;
    }

    return true;
}

typedef bool (*IsPacketDataValid)(
    const void* packet, u8 packetSize, u8 playerAid
);

static std::array<IsPacketDataValid, sizeof(RacePacket::sizes)>
    s_isPacketDataValid{
        IsHeaderPacketDataValid,     IsMatchHeaderPacketDataValid,
        IsMatchDataPacketDataValid,  IsRoomSelectPacketDataValid,
        IsPlayerDataPacketDataValid, IsUserPacketDataValid,
        IsItemPacketDataValid,       IsEventPacketDataValid,
    };

bool IsRacePacketValid(
    const RacePacket* racePacket, u32 racePacketSize, u8 playerAid
)
{
    if (racePacketSize < sizeof(RacePacket)) {
        return false;
    }

    u32 expectedPacketSize = 0;
    for (std::size_t n = 0; n < ARRAY_ELEMENT_COUNT(RacePacket::sizes); n++) {
        RacePacket::EType packetType = static_cast<RacePacket::EType>(n);
        u8 packetSize = racePacket->sizes[n];

        if (!IsPacketSizeValid(packetType, packetSize)) {
            return false;
        }

        expectedPacketSize += packetSize;
    }

    if (racePacketSize < expectedPacketSize) {
        return false;
    }

    expectedPacketSize = 0;
    for (std::size_t n = 0; n < ARRAY_ELEMENT_COUNT(RacePacket::sizes); n++) {
        const IsPacketDataValid isPacketDataValid = s_isPacketDataValid[n];
        const void* packet =
            reinterpret_cast<const char*>(racePacket) + expectedPacketSize;
        u8 packetSize = racePacket->sizes[n];

        if (packetSize != 0 &&
            !isPacketDataValid(packet, packetSize, playerAid)) {
            return false;
        }

        expectedPacketSize += packetSize;
    }

    return true;
}

} // namespace wwfc::mkw::Security

#endif // RMC