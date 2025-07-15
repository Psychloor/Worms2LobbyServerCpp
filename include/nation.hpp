//
// Created by blomq on 2025-06-24.
//

#ifndef NATION_HPP
#define NATION_HPP

#include <cstdint>

namespace worms_server
{
    enum class nation : uint8_t
    {
        /// <summary>No flag.</summary>
        none,
        /// <summary>United Kingdom</summary>
        uk,
        /// <summary>Argentina</summary>
        ar,
        /// <summary>Australia</summary>
        au,
        /// <summary>Austria</summary>
        at,
        /// <summary>Belgium</summary>
        be,
        /// <summary>Brazil</summary>
        br,
        /// <summary>Canada</summary>
        ca,
        /// <summary>Croatia</summary>
        hr,
        /// <summary>Bosnia and Herzegovina (old flag)</summary>
        ba,
        /// <summary>Cyprus</summary>
        cy,
        /// <summary>Czech Republic</summary>
        cz,
        /// <summary>Denmark</summary>
        dk,
        /// <summary>Finland</summary>
        fi,
        /// <summary>France</summary>
        fr,
        /// <summary>Georgia</summary>
        ge,
        /// <summary>Germany</summary>
        de,
        /// <summary>Greece</summary>
        gr,
        /// <summary>Hong Kong SAR</summary>
        hk,
        /// <summary>Hungary</summary>
        hu,
        /// <summary>Iceland</summary>
        is,
        /// <summary>India</summary>
        in,
        /// <summary>Indonesia</summary>
        id,
        /// <summary>Iran</summary>
        ir,
        /// <summary>Iraq</summary>
        iq,
        /// <summary>Ireland</summary>
        ie,
        /// <summary>Israel</summary>
        il,
        /// <summary>Italy</summary>
        it,
        /// <summary>Japan</summary>
        jp,
        /// <summary>Liechtenstein</summary>
        li,
        /// <summary>Luxembourg</summary>
        lu,
        /// <summary>Malaysia</summary>
        my,
        /// <summary>Malta</summary>
        mt,
        /// <summary>Mexico</summary>
        mx,
        /// <summary>Morocco</summary>
        ma,
        /// <summary>Netherlands</summary>
        nl,
        /// <summary>New Zealand</summary>
        nz,
        /// <summary>Norway</summary>
        no,
        /// <summary>Poland</summary>
        pl,
        /// <summary>Portugal</summary>
        pt,
        /// <summary>Puerto Rico</summary>
        pr,
        /// <summary>Romania</summary>
        ro,
        /// <summary>Russian Federation</summary>
        ru,
        /// <summary>Singapore</summary>
        sg,
        /// <summary>South Africa</summary>
        za,
        /// <summary>Spain</summary>
        es,
        /// <summary>Sweden</summary>
        se,
        /// <summary>Switzerland</summary>
        ch,
        /// <summary>Turkey</summary>
        tr,
        /// <summary>United States</summary>
        us,
        /// <summary>Custom skull flag</summary>
        skull,
        /// <summary>Custom Team17 flag</summary>
        team17,
        max
    };
}
#endif //NATION_HPP
