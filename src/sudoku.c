#include "sudoku.h"

#include <getopt.h>
#include <math.h>
#include <preemptive_set.h>	
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RATIO_GRID_SIZE 3

static FILE *pFILEoutput;
static FILE *pFILEinput;
static char *progName;
static pset_t **grid;
static bool verbose;
static bool generate;
static bool strict;
static int grid_size;
static int block_size;      /*will be the square root of grid_size*/

static void usage (int status)
{
  if (status == EXIT_SUCCESS) {
    printf(
      "Usage: %s [OPTIONS] FILE...\n"
      "Solve sudoku puzzle's of variable sizes (1-4-9-16-25-36-48-64).\n\n"
      "-oFILE,\t --output=FILE\t\twrite result to FILE\n"
      "-gSIZE,\t --generate=SIZE\tgenerate a SIZE-sized grid (9 by default).\n"
      "-s,\t --strict\t\tto have only one solution\n"
      "-v,\t --verbose\t\tverbose output\n"
      "-V,\t --version\t\tdisplay version and exit\n"
      "-h,\t --help\t\t\tdisplay this help\n\n", progName);
    exit(EXIT_SUCCESS);
  }
  else {
    fprintf(stderr, "try 'sudoku --help' for more information.\n");
    exit(EXIT_FAILURE);
  }  
}


static void version(char *argv[])
{
  printf("%s v%i.%i.%i\nthis sofware is a sudoku solver\n",
          argv[0], PROG_VERSION, PROG_SUBVERSION, PROG_REVISION);
}


static void check_options (int argc, char *argv[])
{
  /*verbose, generate, strict and pFILEoutput have initial values.
    they can be change by options.*/
  generate = false;
  strict = false;
  verbose = false;
  pFILEoutput = stdout;
  struct option long_opts[] = {
    {"help",  	0, NULL, 'h'}, /* 0 means no arguments */
    {"verbose",	0, NULL, 'v'}, /* 0 means no arguments */
    {"version",	0, NULL, 'V'}, /* 0 means no arguments */
    {"output",	1, NULL, 'o'}, /* 1 means an argument is requiered */
    {"generate",2, NULL, 'g'}, /* 2 means an argument is optional */
    {"strict",	0, NULL, 's'}, /* 0 means no arguments */
    {NULL,			0, NULL, 0  }  /* this line i required. */
  };
  
  int optc;
  while ((optc=getopt_long (argc, argv, "hvVo:g::s", long_opts, NULL)) != -1) {
    switch (optc) {
      case 'h' :
        usage(EXIT_SUCCESS);
      case 'o' :
        pFILEoutput=fopen(optarg, "w");
        /*append to reset the file we want to write */
        if (pFILEoutput==NULL) {
          fprintf(stderr,"sudoku: error: file openning error.\n");
          usage(EXIT_FAILURE);
        }
        break;
      case 'g' :
      
        generate = true;
        if (optarg == NULL) {
          grid_size = 9;
        } else {
        
          /* we use strtol instead of atoi to avoid input like 15fefe*/
          char *test_function = {'\0'};
          grid_size = strtol(optarg,&test_function,0);
          if (test_function[0] != '\0') {
            fprintf(stderr,"sudoku: error: given size is not a number --"
                           "'%s'\n",test_function);
            usage(EXIT_FAILURE);
          }
          
          if (grid_size!=1 && grid_size!=4 && grid_size!=9 && grid_size!=16 && 
              grid_size!=25 && grid_size!=36 && grid_size!=49 && grid_size!=64){
            fprintf(stderr,"sudoku: error: wrong size -- '%d'\n", grid_size);
            usage(EXIT_FAILURE);
          }
        }
        
        block_size = sqrt(grid_size);
        break;
        
      case 's' :
        strict = true;
        if (!generate) {
          fprintf(stderr,"sudoku: error: can't call -s without -g.\n"
                         "try to put -g before -s.\n");
          usage(EXIT_FAILURE);
        } else {
          strict = true;
        }
        break;
      case 'v' :
        verbose = true;
        break;
      case 'V' :
        version(argv);
        exit(EXIT_SUCCESS);
      default:
        usage(EXIT_FAILURE);
    }
  }
  
  /*verifying the user put a correct argument
    it allowed only one supply argument for file name*/
  if (!generate) {
    if (argc < optind +1) {
      fprintf(stderr,"sudoku: error: file name missing.\n");
      /* we don't care the case the file name has two words*/
      usage(EXIT_FAILURE);
    }

    /*opening the input file*/
    pFILEinput=fopen(argv[optind], "r");
    if (pFILEinput==NULL) {
      fprintf(stderr,"sudoku: error: grid openning error.\n");
      usage(EXIT_FAILURE);
    }
  /* next condition means there is an argument (maybe the grid to load)*/
  } else if (argc > optind) {
    fprintf(stderr,"sudoku: error: can't generate and load a grid.\n");
    usage(EXIT_FAILURE);
  }
}


static void out_of_memory(void)
{
  fprintf(stderr,"sudoku: error: out of memory.\n");
  usage(EXIT_FAILURE);
}


static pset_t **grid_alloc(void)
{
  pset_t** matrix = calloc (grid_size, sizeof(pset_t*));
  if (matrix == NULL) {
    out_of_memory();
  }
  for (int i=0; i<grid_size; i++) {
    matrix[i] = calloc (grid_size, sizeof(pset_t));
    if (matrix[i] == NULL) {
      out_of_memory();
    }
  }
  return matrix;
}


static void grid_free(pset_t **grid)
{
  for (int i=0; i<grid_size; i++) {
    free(grid[i]);
  }
  free(grid);
}


static bool check_input_char (char c)
{
  bool res = false;
  switch (grid_size) {
    case 64 : 
      if ((c >='o' && c <='z')||(c=='@')||(c=='&')||(c=='*')) {
        res = true;
      }
      /* it goes through the case */
    case 49 :
      if (c >='b' && c <='n') {
        res = true;
      }
      /* it goes through the case */
    case 36 :
      if ((c >='Q' && c <='Z')||(c=='a')) {
        res = true;
      }
      /* it goes through the case */
    case 25 : 
      if (c >='H' && c <= 'P') {
        res = true;
      }
      /* it goes through the case */
    case 16 :
      if (c >= 'A' && c <='G') {
        res = true;
      }
      /* it goes through the case */
    case 9 :
      if (c >= '5' && c <= '9') {
        res = true;
      }
      /* it goes through the case */
    case 4 :
      if (c >= '2' && c <= '4') {
        res = true;
      }
      /* it goes through the case */
    case 1 :
      if ((c == '1')||(c == '_')) {
        res = true;
      }
      /* it goes through the case */
  }
  return res;
}


static pset_t filter_full_pset (char c) 
{
  if (c == '_') {
    return pset_full (grid_size);
  } else {
    return char2pset(c);
  }
}


static void grid_parser(FILE *file)
{
  int current_line = 0;
  int current_row = 0;
  char current_char;
  grid_size = 0;
  char first_line[MAX_COLORS];
  bool size_is_fixed = false;
  bool reading_started = false;
  
  while ((current_char = fgetc (file)) != EOF) {
    switch (current_char) {
      /* filter out blank characters */
      case ' ':
      case '\t':
        break;
      case '#':      /* filter all the line */
        while ((current_char = fgetc (file)) != '\n' && current_char != EOF ) {}
        break;
      default :
      
        if (current_char != '\n') {
          if (size_is_fixed == false) {
          /*reading_started become true cause the first char has been read */
            if (!reading_started) {
              reading_started = true;
            }
            first_line[grid_size] = current_char;
            grid_size++;
          }
          
          /*else we continue to save the grid*/
          else {
            if (!check_input_char(current_char)) {
              fprintf(stderr,"sudoku: error: wrong character %c at line %d.\n",
                      current_char, current_line);
              usage(EXIT_FAILURE);
            }
            if (current_line == grid_size) {
              fprintf(stderr,"sudoku: error: too many lines in the grid.\n");
              usage(EXIT_FAILURE);
            }
            
            grid[current_line][current_row] = filter_full_pset (current_char);
            
            current_row++;
            if (current_row >= grid_size) {
              current_row=0;
              current_line++;
            }
          }
        }/* so, current_char is \n   <-(this is not useless for me) */
        
        else if (!reading_started){
          break;
        }
        
        else if (!size_is_fixed) {
          size_is_fixed = true;
          block_size = sqrt(grid_size);
          grid = grid_alloc();
          
          /* we copy first_line on the grid */
          for (int i = 0; i< grid_size; i++) {
            if (!check_input_char(first_line[i])) {          
              fprintf(stderr,"sudoku: error: wrong character %c at line 0.\n",
                      first_line[i]);
              usage(EXIT_FAILURE);
            }

            grid[0][i] = filter_full_pset (first_line[i]);
            
          }
          
          current_line = 1;
        }
        
        else if (current_row != 0) {
          fprintf(stderr,"sudoku: error: not a right number of cells in "
                         "line %d.\n", current_line);
          usage(EXIT_FAILURE);
        }
    }
  }

  if (!reading_started){
    fprintf(stderr,"sudoku: error: there is no grid.\n");
    usage(EXIT_FAILURE);
  }
  else if (current_line <grid_size) {
    fprintf(stderr,"sudoku: error: too few lines in the grid.\n");
    usage(EXIT_FAILURE);
  }
}


static void grid_print(pset_t **grid)
{
  for (int j = 0; j<grid_size; j++) {
    for (int i = 0; i<grid_size; i++) {
      if (grid[j][i] == 0) {
        fprintf(pFILEoutput, "??\t");       
       } else if (grid[j][i] == pset_full(grid_size)) {
        fprintf(pFILEoutput, "_\t");       
       } else {
        char str[MAX_COLORS+1];
        pset2str(str, grid[j][i]);
        fprintf(pFILEoutput, "%s\t",str);
      }
    }
    fprintf(pFILEoutput, "\n");
  }
  fprintf(pFILEoutput, "\n");
}


/* return true if the grid is solved, false otherwise. */
static bool grid_solved(pset_t **grid)
{
  for (int j = 0; j<grid_size ;j++) {
    for (int i = 0; i<grid_size; i++) {
      if (!pset_is_singleton(grid[j][i])) {
        return false;
      }
    }
  }
  return true;
}


static void scan_block (int starting_column, int starting_row,
                         pset_t *subgrid[], pset_t **grid)
{
  int current_position_subgrid = 0;  
  
  for (int j = starting_column; j < (starting_column+block_size); j++) {
    for (int i = starting_row; i < (starting_row+block_size); i++) {
      subgrid[current_position_subgrid] = &grid[j][i];
      current_position_subgrid++;
    }
  }
} 


/* This function get a grid, apply func to each subgrid and return false if
   func returned at least once false. True else.*/
static bool subgrid_map (pset_t **grid, bool (*func) (pset_t *subgrid[]))
{
  pset_t *subgrid[grid_size];
  bool fixpoint = true; 
  
  /* scanning all rows : */
  for (int j = 0; j<grid_size; j++) {
    int current_position_grid = 0;
    for (int i = 0; i<grid_size; i++) {
      subgrid[current_position_grid] = &grid[j][i];
      current_position_grid++;
    }
    if (!func(subgrid)) {
      fixpoint = false;
    }
  }
  
  /* scanning all columns : */
  for (int i = 0; i<grid_size; i++) {
    int current_position_grid = 0;
    for (int j = 0; j<grid_size; j++) {
      subgrid[current_position_grid] = &grid[j][i];
      current_position_grid++;
    }
    if (!func(subgrid)) {
      fixpoint = false;
    }
  }

  /* scanning all blocks : */
  for (int j = 0; j<grid_size; j+=block_size) {
    for (int i = 0; i<grid_size; i+=block_size) {
      scan_block (j,i, subgrid, grid);
      if (!func(subgrid)) {
        fixpoint = false;
      }
    }
  }
  
  return fixpoint;
}


/* with a subgrid in input, it returns false if one of the three conditions is 
   violated : one color by subgrid ; each color present on each subgrid ;
   one color is empty */
static bool subgrid_consistency(pset_t *subgrid[])
{
  pset_t final_pset = pset_empty();
  pset_t singleton_pset = pset_empty();  
    
  for (int i = 0; i<grid_size; i++) {
    
    /* we add each present color to the final_pset */
    final_pset = pset_or (final_pset, *subgrid[i]);
        
    if (pset_is_singleton (*subgrid[i])) {
    /* that negation means that subgrid[i] was already in singleton_pset.*/
      if (pset_and(singleton_pset, *subgrid[i]) != pset_empty()) {
        return false;
      } else {
        singleton_pset = pset_or (singleton_pset, *subgrid[i]);
      }
    }
  }
  
  return (final_pset == pset_full (grid_size));
}


/* will apply cross-hatching to subgrid and return true if a changement has 
   occured, false else.
   if a cell is a singleton, remove all the similar color to the subgrid*/
static bool subgrid_heuristics_cross_hatching(pset_t *subgrid[])
{
  bool fixpoint = false;
  
  for (int i = 0; i<grid_size; i++) {
    if (pset_is_singleton(*subgrid[i])) {

      for (int j = 0; j<grid_size; j++) {
        /*the second part of the next condition is to not modify the pset if
          we don't need to*/
        if (j!=i && pset_is_included(*subgrid[i],*subgrid[j])) {
          *subgrid[j] = pset_discard2(*subgrid[j], *subgrid[i]);
          fixpoint = true;
        }
      }

    }
  }

  return fixpoint;
}


/* will apply lone number to subgrid and return true if a changement has 
   occured, false else.
   if a color occurs only once in a subgrid, it becomes a singleton*/
static bool subgrid_heuristics_lone_number(pset_t *subgrid[])
{
  bool fixpoint = false;
  
  for (int i = 0; i<grid_size; i++) {
    
    pset_t temp = *subgrid[i];
    
    for (int j = 0; j<grid_size; j++) {
      if (i!=j) {
        temp = pset_discard2 (temp, *subgrid[j]);
      }
    }
    
    if (pset_is_singleton(temp)) {
      *subgrid[i] = temp;
      fixpoint = true;
    }
  }
  
  return fixpoint;
}


static bool subgrid_heuristics (pset_t **grid)
{
  bool fixpoint = true;
  fixpoint=(subgrid_map (grid, subgrid_heuristics_cross_hatching) && fixpoint);
  fixpoint=(subgrid_map (grid, subgrid_heuristics_lone_number) && fixpoint);
  return fixpoint;
}


/* Will apply a heuristic on a grid. it returns 0 if the grid is solved, 1 if
   not but it's consistency and 2 if none of both.*/
static int grid_heuristics (pset_t **grid)
{
  bool fixpoint = false;

  while (!fixpoint) {
    fixpoint = true;

    fixpoint = !(subgrid_heuristics (grid));

    if (verbose && !generate) {
      grid_print(grid);
    }
  }
  
 if (grid_solved(grid)) {
    return 0;
  } else if (!subgrid_map (grid, subgrid_consistency)) {
    return 2;
  } else {
    return 1;
  }
}


/* rewrite the grid2 INTO the grid1*/
static void grid_rewrite (pset_t **grid1, pset_t **grid2) {
  for (int j = 0; j<grid_size; j++) {
    for (int i = 0; i<grid_size; i++) {	
      grid1[j][i] = grid2[j][i];
    }
  }
}


static pset_t **grid_copy (pset_t **grid) {
  pset_t **res = grid_alloc();
  grid_rewrite(res, grid);
  return res;
}


/* Will apply the heuristic to the local grid (in parameter), and for each
   choice we have to make, will call recursively grid_solver for each letter
   until we know if the grid is not consistency or it's solved.
   Depth-first traversal method is used.
   Will Return 1 if it's solved with only one solution, 2 if it's solved with*
   more than 1 solution and 0 if it's not consistent.
   using the global grid once it's resolved avoid to copy many times the grid.
   */
static int grid_solver (pset_t **grid)
{
  int result = 0;
  int result_heuristic = grid_heuristics (grid);

  if (result_heuristic == 0) {
    return 1;
  } else if (result_heuristic == 2) {
    return 0;
  } else {
  
    /* choice function in hardcode (cause we use the x and the y later) */
    int cardinality_chosen_cell = MAX_COLORS+1;
    int x_chosen_cell;
    int y_chosen_cell;
  
    for (int j = 0; j<grid_size; j++) {
      for (int i = 0; i<grid_size; i++) {
      
        if (!pset_is_singleton(grid[j][i])) {
        
          int current_cardinality = pset_cardinality(grid[j][i]);
          if (current_cardinality < cardinality_chosen_cell) {
            x_chosen_cell = j;
            y_chosen_cell = i;
            cardinality_chosen_cell = current_cardinality;
          }
        }
      }
    }

    /* at this position, we can't have no choice do to (there is no embiguity) :
       the grid is consistent and is not solved*/
    
    pset_t chosen_cell = grid[x_chosen_cell][y_chosen_cell];
    pset_t **reference_grid = grid_copy(grid);
    
    /*for each letter in the chosen cell : */
    while (chosen_cell != pset_empty()) {

      /* we copy the grid and place in it one of the element of chosen cell*/
      pset_t left_most_element = pset_leftmost(chosen_cell);
      pset_t **temporary_grid = grid_copy(reference_grid);
      
      temporary_grid[x_chosen_cell][y_chosen_cell] = left_most_element;
      chosen_cell =  pset_discard2 (chosen_cell, left_most_element);
      
      /*recursive call*/
      int temp = grid_solver(temporary_grid);

      result += temp;
      if (temp>0) {
        grid_rewrite(grid, temporary_grid);
      }
      grid_free(temporary_grid);

      /*this if saves time but it wont compute the right number of solution*/
      if (result > 1) {
        grid_free(reference_grid);
        return 2;
      }
    }
    
    grid_free(reference_grid);
    /* we don't need to check the others cells with multiple choices cause the
       recursive call will care about it */
  }
  return result;
}


static void remove_random_cell (pset_t **grid)
{
  int x_generated;
  int y_generated;

  do {
    x_generated = rand() % grid_size;
    y_generated = rand() % grid_size;
    /*this while can't loop ifinitely cause number_generated is less than 
      the number of cases in the grid */

  } while (grid[y_generated][x_generated] == pset_full(grid_size));
  
  grid[y_generated][x_generated] = pset_full(grid_size);
}


static void place_a_singleton (pset_t **grid)
{
  int x_generated = rand() % grid_size;
  int y_generated = rand() % grid_size;
  int z_generated = rand() % grid_size;
  pset_t pset = 1;
  for(int i = 0; i<z_generated; i++) {
    pset=pset<<1;
  }
  grid[y_generated][x_generated] = pset;
}


static bool only_one_solution (pset_t **grid)
{
  pset_t **temporary_grid = grid_copy(grid);
  int result = grid_solver(temporary_grid);
  grid_free(temporary_grid);  
  return (result==1);
}


static void generate_grid (void)
{
  srand(time(NULL));

  grid = grid_alloc();
  /*fill the grid with full*/
  for (int j = 0; j<grid_size; j++) {
    for (int i = 0; i<grid_size; i++) {
      grid[j][i] = pset_full(grid_size);
    }
  }
  
  /* we place randomly a number to guide to a random grid */
  place_a_singleton(grid);
  
  grid_solver(grid);
  

  int cells_to_remove = ((grid_size*grid_size) / RATIO_GRID_SIZE);
  /*we remove two third of all the cells*/
 
  while (cells_to_remove > 0) {

    pset_t **temporary_grid = grid_copy(grid);

    for (int i = 0; i<grid_size; i++) {
      remove_random_cell(grid);
    }

    if (strict && !only_one_solution(grid)) {
      grid_rewrite(grid, temporary_grid);
    } else {
      cells_to_remove -= grid_size;/*we remove grid_size cells each time*/
      if (verbose) {
        grid_print(grid);
      }    
    }

    grid_free(temporary_grid);
  }
}
 

static void close_and_check(FILE *pFILE)
{
  if (fclose(pFILE)!=0) {
    fprintf(stderr, "File closing error.\n");
    exit(EXIT_FAILURE);  
  }
}



int main (int argc, char *argv[])
{ 
  progName = argv[0];

  check_options (argc,argv);
  
  if (!generate) {
    grid_parser(pFILEinput);

    int temp = grid_solver(grid);

    if (temp>=2) {
      printf("The grid has been solved. There is >%d solutions\n", temp);
    } else if (temp==1) {
      printf("The grid has been solved. There is only one solution\n");
    } else {
      printf("The grid isn't consistant.\n");
    }
    
    grid_print(grid);
    grid_free(grid);

    close_and_check(pFILEinput);

  } else {
    generate_grid();
    grid_print(grid);
    grid_free(grid);
  }

  /*warning : the standard output may close there.*/
  close_and_check(pFILEoutput);
  return 0;
}

