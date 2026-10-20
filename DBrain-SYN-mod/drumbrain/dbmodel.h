/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef DBMODEL_H
#define DBMODEL_H
#include <stdint.h>
#define DB_NINSTR 9      /* 0 accent, then the instruments of tracks 1-8 */
/* A trie of the contexts, read backwards from the last symbol. Symbols: 0 '^', 1 '-', 2 '0',
 * 3 '1', 4 '2', 5 '5'. A node's children are contiguous from first, in symbol order, one per bit
 * of mask. th: cumulative thresholds out of 255 for the outputs 0, 1, 2 (the rest is 5). */
struct db_node { uint16_t first; uint8_t mask; uint8_t has; uint8_t th[3]; uint8_t pad; };
#define DB_NSTYLES 2     /* 0 house, 1 breaks */
extern const uint16_t db_roots[DB_NSTYLES][DB_NINSTR];
extern const struct db_node db_nodes[];
#endif
