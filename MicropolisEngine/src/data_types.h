/* data_types.h
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
 * @file data_types.h
 * @brief Commonly used data types in Micropolis game engine.
 *
 * This header file defines basic data types used throughout the
 * Micropolis game engine. These include types for bytes, pointers,
 * and quad values (both signed and unsigned). This file provides a
 * centralized definition of these types to ensure consistency and
 * readability across the game engine's codebase. By abstracting data
 * types in this manner, the code maintains flexibility and ease of
 * maintenance.
 *
 * MODERNIZATION NOTE (Phase 2):
 * - Added fixed-width types for cross-platform consistency
 * - Added Funds type (int64_t) for financial calculations to prevent overflow
 * - Legacy types (Byte, Ptr, Quad, UQuad) retained for backward compatibility
 */


#ifndef H_DATA_TYPES
#define H_DATA_TYPES

#include <cstdint>
#include <memory>
#include <vector>

// ============================================================================
// Legacy type definitions (retained for backward compatibility)
// ============================================================================

typedef unsigned char Byte;

typedef Byte *Ptr;

typedef long Quad;

typedef unsigned long UQuad;

// ============================================================================
// Modern type definitions (Phase 2 modernization)
// ============================================================================

/**
 * @brief Fixed-width integer types for cross-platform consistency
 */
using Int8   = std::int8_t;
using Int16  = std::int16_t;
using Int32  = std::int32_t;
using Int64  = std::int64_t;
using UInt8  = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;

/**
 * @brief Funds type for financial calculations.
 *
 * Using int64_t prevents overflow exploits that could occur with
 * 16-bit or 32-bit integers. A city generating $1M/year for 1000
 * years would only reach ~$1B, well within int64_t range.
 *
 * Maximum value: 9,223,372,036,854,775,807 (~$9.2 quintillion)
 */
using Funds = std::int64_t;

/**
 * @brief Population type for city population tracking.
 *
 * Using int64_t to support extremely large cities without overflow.
 */
using Population = std::int64_t;

/**
 * @brief Map tile value type.
 */
using MapTile = std::uint16_t;

/**
 * @brief Map coordinate type.
 */
using MapCoord = std::int16_t;


#endif
