#ifndef NATION_HPP
#define NATION_HPP

#include <cstdint>

namespace worms_server
{
    enum class Nation : uint8_t
    {
        /// <summary>No flag.</summary>
        None,
        /// <summary>United Kingdom</summary>
        UnitedKingdom,
        /// <summary>Argentina</summary>
        Argentina,
        /// <summary>Australia</summary>
        Australia,
        /// <summary>Austria</summary>
        Austria,
        /// <summary>Belgium</summary>
        Belgium,
        /// <summary>Brazil</summary>
        Brazil,
        /// <summary>Canada</summary>
        Canada,
        /// <summary>Croatia</summary>
        Croatia,
        /// <summary>Bosnia and Herzegovina (old flag)</summary>
        BosniaAndHerzegovinaOldFlag,
        /// <summary>Cyprus</summary>
        Cyprus,
        /// <summary>Czech Republic</summary>
        CzechRepublic,
        /// <summary>Denmark</summary>
        Denmark,
        /// <summary>Finland</summary>
        Finland,
        /// <summary>France</summary>
        France,
        /// <summary>Georgia</summary>
        Georgia,
        /// <summary>Germany</summary>
        Germany,
        /// <summary>Greece</summary>
        Greece,
        /// <summary>Hong Kong SAR</summary>
        HongKong,
        /// <summary>Hungary</summary>
        Hungary,
        /// <summary>Iceland</summary>
        Iceland,
        /// <summary>India</summary>
        India,
        /// <summary>Indonesia</summary>
        Indonesia,
        /// <summary>Iran</summary>
        Iran,
        /// <summary>Iraq</summary>
        Iraq,
        /// <summary>Ireland</summary>
        Ireland,
        /// <summary>Israel</summary>
        Israel,
        /// <summary>Italy</summary>
        Italy,
        /// <summary>Japan</summary>
        Japan,
        /// <summary>Liechtenstein</summary>
        Liechtenstein,
        /// <summary>Luxembourg</summary>
        Luxembourg,
        /// <summary>Malaysia</summary>
        Malaysia,
        /// <summary>Malta</summary>
        Malta,
        /// <summary>Mexico</summary>
        Mexico,
        /// <summary>Morocco</summary>
        Morocco,
        /// <summary>Netherlands</summary>
        Netherlands,
        /// <summary>New Zealand</summary>
        NewZealand,
        /// <summary>Norway</summary>
        Norway,
        /// <summary>Poland</summary>
        Poland,
        /// <summary>Portugal</summary>
        Portugal,
        /// <summary>Puerto Rico</summary>
        PuertoRico,
        /// <summary>Romania</summary>
        Romania,
        /// <summary>Russian Federation</summary>
        RussianFederation,
        /// <summary>Singapore</summary>
        Singapore,
        /// <summary>South Africa</summary>
        SouthAfrica,
        /// <summary>Spain</summary>
        Spain,
        /// <summary>Sweden</summary>
        Sweden,
        /// <summary>Switzerland</summary>
        Switzerland,
        /// <summary>Turkey</summary>
        Turkey,
        /// <summary>United States</summary>
        UnitedStates,
        /// <summary>Custom skull flag</summary>
        CustomSkullFlag,
        /// <summary>Custom Team17 flag</summary>
        CustomTeam17Flag,
        Max
    };
}
#endif // NATION_HPP