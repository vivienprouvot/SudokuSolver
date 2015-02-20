#ifndef PREEMPTIVE_SET
#define PREEMPTIVE_SET

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_COLORS 64
#define FULL UINT64_MAX /* thank you Edon */

typedef uint64_t pset_t;

/*Convert a char into pset_t.
  If the char isn't a known color, it returns 0.*/
pset_t char2pset (char c);

/*Convert a pset into string.
  Assuming that the memory zone given as argument has enough space to handle
  the full string.*/
void pset2str (char string[MAX_COLORS + 1], pset_t pset);

/*Return the pset full corresponding to the maximum of the color range
  an integer can be placed in the input.*/
pset_t pset_full (size_t color_range);

/*Return the empty set.*/
pset_t pset_empty();

/*Set to 1 the color encoded by the character 'c'.*/
pset_t pset_set (pset_t pset, char c);

/*Set to 0 the color encoded by the character 'c'.*/
pset_t pset_discard (pset_t pset, char c);

/*Set to 0 the color corresponding to pset2.
  pset_2 might be a singleton*/
pset_t pset_discard2 (pset_t pset1, pset_t pset2);

/*Bitwise negate pset.*/
pset_t pset_negate (pset_t pset);

/*Compute  the intersection of the two psets.*/
pset_t pset_and (pset_t pset1, pset_t pset2);

/*Compute the union of the two psets.*/
pset_t pset_or (pset_t pset1, pset_t pset2);

/*Compute the xor of the two psets.*/
pset_t pset_xor (pset_t pset1, pset_t pset2);

/*Tests if pset1 is included IN pset2.*/
bool pset_is_included (pset_t pset1, pset_t pset2);

/*Test if pset is a singleton.*/
bool pset_is_singleton (pset_t pset);

/*It returns a size_t like it has be asked, but i'd prefer return a int.*/
/*Return the number of colors enclosed in the set.*/
size_t pset_cardinality (pset_t pset);

/*return the left most color of a set. */
pset_t pset_leftmost (pset_t pset);

#endif
