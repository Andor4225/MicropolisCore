/* allocate.cpp
 *
 * Micropolis, Unix Version.  This game was released for the Unix platform
 * in or about 1990 and has been modified for inclusion in the One Laptop
 * Per Child program.  Copyright (C) 1989 - 2007 Electronic Arts Inc.  If
 * you need assistance with this program, you may contact:
 *   http://wiki.laptop.org/go/Micropolis  or email  micropolis@laptop.org.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.  You should have received a
 * copy of the GNU General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 *             ADDITIONAL TERMS per GNU GPL Section 7
 *
 * No trademark or publicity rights are granted.  This license does NOT
 * give you any right, title or interest in the trademark SimCity or any
 * other Electronic Arts trademark.  You may not distribute any
 * modification of this program using the trademark SimCity or claim any
 * affliation or association with Electronic Arts Inc. or its employees.
 *
 * Any propagation or conveyance of this program must include this
 * copyright notice and these terms.
 *
 * If you convey this program (or any modifications of it) and assume
 * contractual liability for the program to recipients of it, you agree
 * to indemnify Electronic Arts for any liability that those contractual
 * assumptions impose on Electronic Arts.
 *
 * You may not misrepresent the origins of this program; modified
 * versions of the program must be marked as such and not identified as
 * the original program.
 *
 * This disclaimer supplements the one included in the General Public
 * License.  TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, THIS
 * PROGRAM IS PROVIDED TO YOU "AS IS," WITH ALL FAULTS, WITHOUT WARRANTY
 * OF ANY KIND, AND YOUR USE IS AT YOUR SOLE RISK.  THE ENTIRE RISK OF
 * SATISFACTORY QUALITY AND PERFORMANCE RESIDES WITH YOU.  ELECTRONIC ARTS
 * DISCLAIMS ANY AND ALL EXPRESS, IMPLIED OR STATUTORY WARRANTIES,
 * INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY, SATISFACTORY QUALITY,
 * FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS, AND WARRANTIES (IF ANY) ARISING FROM A COURSE OF DEALING,
 * USAGE, OR TRADE PRACTICE.  ELECTRONIC ARTS DOES NOT WARRANT AGAINST
 * INTERFERENCE WITH YOUR ENJOYMENT OF THE PROGRAM; THAT THE PROGRAM WILL
 * MEET YOUR REQUIREMENTS; THAT OPERATION OF THE PROGRAM WILL BE
 * UNINTERRUPTED OR ERROR-FREE, OR THAT THE PROGRAM WILL BE COMPATIBLE
 * WITH THIRD PARTY SOFTWARE OR THAT ANY ERRORS IN THE PROGRAM WILL BE
 * CORRECTED.  NO ORAL OR WRITTEN ADVICE PROVIDED BY ELECTRONIC ARTS OR
 * ANY AUTHORIZED REPRESENTATIVE SHALL CREATE A WARRANTY.  SOME
 * JURISDICTIONS DO NOT ALLOW THE EXCLUSION OF OR LIMITATIONS ON IMPLIED
 * WARRANTIES OR THE LIMITATIONS ON THE APPLICABLE STATUTORY RIGHTS OF A
 * CONSUMER, SO SOME OR ALL OF THE ABOVE EXCLUSIONS AND LIMITATIONS MAY
 * NOT APPLY TO YOU.
 */

/** 
 * @file allocate.cpp
 * @brief Implementation of map array initialization and destruction
 * in Micropolis.
 *
 * This source file contains functions for initializing and destroying
 * map arrays in the Micropolis game engine. It includes the
 * allocation of memory for various map-related data structures such
 * as the main city map, history data, and various density maps. The
 * functions handle the setup of necessary resources when the game
 * starts and ensure proper cleanup of these resources upon
 * termination, maintaining efficient memory management throughout the
 * lifecycle of the game.
 */


////////////////////////////////////////////////////////////////////////


#include "micropolis.h"


////////////////////////////////////////////////////////////////////////

/**
 * Allocate and initialize arrays for the maps.
 *
 * MODERNIZATION (Phase 2):
 * - Replaced malloc/newPtr with std::vector for automatic memory management
 * - History arrays now use std::vector<short> with proper element counts
 * - Map storage uses std::vector but maintains pointer array for compatibility
 */
void Micropolis::initMapArrays()
{
    // Allocate map storage using vectors (RAII - automatic cleanup)
    if (mapBaseStorage.empty()) {
        mapBaseStorage.resize(WORLD_W * WORLD_H, 0);
    }

    if (mopBaseStorage.empty()) {
        mopBaseStorage.resize(WORLD_W * WORLD_H, 0);
    }

    // Set up pointer array to access map storage by column
    // This maintains compatibility with existing map[x][y] access pattern
    for (int i = 0; i < WORLD_W; i++) {
        map[i] = mapBaseStorage.data() + (i * WORLD_H);
        mop[i] = mopBaseStorage.data() + (i * WORLD_H);
    }

    // Initialize history vectors with proper element counts
    // HISTORY_ELEMENT_COUNT = 240 elements (480 bytes / 2 bytes per short)
    // MISC_HISTORY_ELEMENT_COUNT = 120 elements (240 bytes / 2 bytes per short)
    resHist.resize(HISTORY_ELEMENT_COUNT, 0);
    comHist.resize(HISTORY_ELEMENT_COUNT, 0);
    indHist.resize(HISTORY_ELEMENT_COUNT, 0);
    moneyHist.resize(HISTORY_ELEMENT_COUNT, 0);
    pollutionHist.resize(HISTORY_ELEMENT_COUNT, 0);
    crimeHist.resize(HISTORY_ELEMENT_COUNT, 0);
    miscHist.resize(MISC_HISTORY_ELEMENT_COUNT, 0);
}


/**
 * Free all map arrays.
 *
 * MODERNIZATION (Phase 2):
 * - Vectors automatically free memory when cleared (RAII)
 * - No more manual null checks or freePtr calls
 * - Memory safety guaranteed by std::vector
 */
void Micropolis::destroyMapArrays()
{
    // Clear map storage vectors (automatic memory deallocation)
    mapBaseStorage.clear();
    mapBaseStorage.shrink_to_fit();

    mopBaseStorage.clear();
    mopBaseStorage.shrink_to_fit();

    // Clear pointer arrays
    memset(map, 0, sizeof(unsigned short *) * WORLD_W);
    memset(mop, 0, sizeof(unsigned short *) * WORLD_W);

    // Clear density maps (these were already vectors)
    populationDensityMap.clear();
    trafficDensityMap.clear();
    pollutionDensityMap.clear();
    landValueMap.clear();
    crimeRateMap.clear();

    tempMap1.clear();
    tempMap2.clear();
    tempMap3.clear();

    terrainDensityMap.clear();

    // Clear history vectors (automatic memory deallocation)
    resHist.clear();
    resHist.shrink_to_fit();

    comHist.clear();
    comHist.shrink_to_fit();

    indHist.clear();
    indHist.shrink_to_fit();

    moneyHist.clear();
    moneyHist.shrink_to_fit();

    pollutionHist.clear();
    pollutionHist.shrink_to_fit();

    crimeHist.clear();
    crimeHist.shrink_to_fit();

    miscHist.clear();
    miscHist.shrink_to_fit();
}


////////////////////////////////////////////////////////////////////////
