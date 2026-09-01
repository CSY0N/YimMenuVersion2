#pragma once
#include "core/commands/Command.hpp"
#include "game/gta/Stats.hpp"
#include "game/pointers/Pointers.hpp"

namespace Mansion
{
    using namespace YimMenu;
    enum class Property : int
    {
        TongvaEstate,
        RichmanVilla,
        VinewoodResidence
    };

    enum class Trophy : int
    {
        None = 0, StarOfSanAndreas, TheBigOne, HighAchiever, Centurion, Quincentenary, Millennial,
        NineFigures, HalfABil, ThreeCommaClub, Platinum, GoingPro, CriminalMastermind,
        NimbleFingers, DoomProfiteer, CleanSweep, BeachfrontBonus, WellLiked, Entrepreneur,
        CarConnoisseur, ArenaWarLegend, Bit8Hero, Collector, FlyingAce, AdversaryAlpha,
        SpeedFreak, DistinguishedDriver, LongStoryShort, Veteran, Spotlight,
        WorldRecordHolder, GoingPlaces
    };

    static const int TrophyCabinets[3][5] =
    {
        {59802, 59803, 59804, 59805, 59806},
        {59819, 59820, 59821, 59822, 59823},
        {59836, 59837, 59838, 59839, 59840}
    };

    inline void SetTrophy(Property prop, int cabinet, Trophy value)
    {
        Stats::SetPackedInt(TrophyCabinets[(int)prop][cabinet], (int)value);
    }

    enum class Plaque : int
    {
        None = 0, KnoWayOut, MoneyFronts, OscarGuzmanFliesAgain, AgentsOfSabotage, BottomDollarBounties,
        CluckinBellFarmRaid, TheChopShop, SanAndreasMercenaries, LosSantosDrugWars, OperationPaperTrail,
        TheContract, LosSantosTuners, CayoPericoHeist, SuperyachtLife, GeraldsLastPlay,
        DiamondCasinoHeist, DiamondCasinoAndResort, PremiumDeluxeRepoWork, ArenaWar, MadrazoDispatchServices,
        DoomsdayHeist, AfterHours, SmugglersRun, Gunrunning, ImportExport, Bikers, FAIFAF, Lowriders,
        Heists, Survivals, Racing, Deathmatches, AdversaryModes, VehicleEnthusiast, WeaponsExpert
    };

    static const int Plaques[3][8] =
    {
        {59807, 59808, 59809, 59813, 59814, 59815, 59899, 59900},
        {59824, 59825, 59826, 59830, 59831, 59832, 59901, 59902},
        {59841, 59842, 59843, 59847, 59848, 59849, 59903, 59904}
    };

    inline void SetPlaque(Property prop, int plaqueIndex, Plaque value)
    {
        Stats::SetPackedInt(Plaques[(int)prop][plaqueIndex], (int)value);
    }

    struct DecorationIndices
    {
        int Entrance1, Entrance2;
        int LivingRoom1, LivingRoom2, LivingRoom3, LivingRoom4, LivingRoom5;
        int DiningRoom1, DiningRoom2;
        int Office;
        int MasterBedroom1, MasterBedroom2;
        int Staircase;
        int Gallery1, Gallery2, Gallery3, Gallery4;
        int Spa;
        int TrophyRoom;
        int GuestBedroom;
        int MediaRoom1, MediaRoom2, MediaRoom3;
    };

    static const DecorationIndices MansionIndices[3] =
    {
        {59816, 24958, 59813, 59817, 59818, 0, 0, 59811, 59865, 59866, 59810, 59862, 24959, 59864, 59863, 59860, 0, 24960, 59861, 59867, 59868, 59870, 59869},
        {59833, 7675, 59829, 59834, 59835, 0, 0, 59828, 59876, 59877, 59827, 59873, 7676, 59875, 59874, 59871, 0, 7677, 59872, 59878, 59879, 59881, 59880},
        {59850, 7678, 59846, 59851, 59852, 0, 0, 59845, 59887, 59888, 59844, 59884, 7679, 59886, 59885, 59882, 0, 7680, 59883, 59889, 59890, 59892, 59891}
    };

    inline void SetDecoration(Property prop, int roomIndex, int value)
    {
        const int* indices = reinterpret_cast<const int*>(&MansionIndices[(int)prop]);
        Stats::SetPackedInt(indices[roomIndex], value);
    }
}
namespace YimMenu::Features
{
    class Tongva_Unlock : public Command
    {
        using Command::Command;

        void OnCall() override
        {
            if (*Pointers.IsSessionStarted)
            {
                Mansion::SetTrophy(Mansion::Property::TongvaEstate, 0, Mansion::Trophy::CriminalMastermind);
            }
        }
    };

    static Tongva_Unlock _Tongva_Unlock{
        "TongvaM_Unlock",
        "Unlock Tongva Mansion",
        "Unlock Tongva Mansion"
    };
    class Richman_Unlock : public Command
    {
        using Command::Command;

        void OnCall() override
        {
            if (*Pointers.IsSessionStarted)
            {
                Mansion::SetPlaque(Mansion::Property::RichmanVilla, 2, Mansion::Plaque::SmugglersRun);
            }
        }
    };

    static Richman_Unlock _Richman_Unlock{
        "RichmanM_Unlock",
        "Unlock Richman Villa Mansion",
        "Unlock Richman Villa Mansion"
    };

   
    class Vinewood_Unlock : public Command
    {
        using Command::Command;

        void OnCall() override
        {
            if (*Pointers.IsSessionStarted)
            {
                Mansion::SetDecoration(Mansion::Property::VinewoodResidence, 0, 4);
            }
        }
    };

    static Vinewood_Unlock _Vinewood_Unlock{
        "VinewoodM_Unlock",
        "Unlock Vinewood Residence Mansion",
        "Unlock Vinewood Residence Mansion"
    };
}
