#include <preemptive_set.h>

static const char color_table[] = "123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "@&*";

pset_t char2pset (char c)
{
  int i = 0; /*we could call it position_of_the_found_color_in_color_table*/
  while (i < MAX_COLORS) {
  /*the complexity is n cause the loop stops when i reaches 64 and it
   increments each time*/
    if (c==color_table[i]) {
      return ((pset_t)1<<i);
      break;
    }
    i++;
  }
  return 0;
}


void pset2str (char string[MAX_COLORS + 1], pset_t pset)
{
  int j = 0;
  for (int i = 0; i<MAX_COLORS; i++) {
    if (pset & 1) {
      string[j] = color_table[i];
      j++;
    }
    pset = pset >> 1;
  }
  string[j] = '\0';
}


pset_t pset_full (size_t color_range)
{
  pset_t res = FULL;
  if (color_range < MAX_COLORS) {
    res = FULL >> (MAX_COLORS - color_range);
  }
  return res;
}


pset_t pset_empty(void)
{
  return 0;
}


pset_t pset_set (pset_t pset, char c)
{
  return (pset | char2pset(c));
}


pset_t pset_discard (pset_t pset, char c)
{
  return pset_and (pset, pset_negate( char2pset(c) )); 
}


pset_t pset_discard2 (pset_t pset1, pset_t pset2)
{
  return pset_and (pset1, pset_negate(pset2)); 
}


pset_t pset_negate (pset_t pset)
{
  return ~(pset);
}


pset_t pset_and (pset_t pset1, pset_t pset2)
{
  return (pset1 & pset2);
}


pset_t pset_or (pset_t pset1, pset_t pset2)
{
  return (pset1 | pset2);
}


pset_t pset_xor (pset_t pset1, pset_t pset2)
{
  return (pset1 ^ pset2);
}


bool pset_is_included (pset_t pset1, pset_t pset2)
{
  return ((pset1 - (pset1 & pset2)) == 0);
}


bool pset_is_singleton (pset_t pset)
{
  return (((pset & (pset-1)) == 0) && pset != 0);
}


size_t pset_cardinality (pset_t pset)
/*inspired on the website http://stackoverflow.com. Article by an annonymous.
  SWAR algorithm*/
{
  pset = pset - ((pset >> 1 ) & 0x5555555555555555);
  pset = (pset & 0x3333333333333333) + ((pset >> 2) & 0x3333333333333333);
  pset = (pset + (pset >> 4)) & 0x0F0F0F0F0F0F0F0F;
  pset = pset + (pset >> 8);
  pset = pset + (pset >> 16);
  pset = pset + (pset >> 32); 
  
  return (pset & 0xFF);
}


pset_t pset_leftmost (pset_t pset)
{
  pset_t res = 1;
  
  if (pset == pset_empty()) {
    return pset_empty();
  }
  
  while ( pset_and (pset, res) == pset_empty() ) {
    res = res << 1;
  }
  
  return res;
}


