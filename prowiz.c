/*
 * Pro-Wizard_1.c
 *
 * 1997-2016 (c) Sylvain "Asle" Chipaux
 *
 */

#include "globals.h"
#include "extern.h"
#include "vars.h"

void DisplayBanner(FILE *out)
{
  fprintf(out, "-<([ Pro-Wizard v1.71 (demoscene.au modified) ])>-\n\n");
}

void DisplayUsage(char *name, char *errorMessage)
{
  DisplayBanner(stderr);
  if (errorMessage != NULL)
  {
    fprintf(stderr, "%s\n\n", errorMessage);
  }
  fprintf(stderr, "Usage: %s <flag> <input file> <output file>\n", name);
  fprintf(stderr, "Flags:\n");
  fprintf(stderr, "  -r   : Rip Mode (Identify and depack modules to .mod)\n");
  fprintf(stderr, "  -d   : Data Mode (Identify and rebuild executables for data crunchers)\n");
  fprintf(stderr, "  -rd  : Both modes\n");
  fprintf(stderr, "  -j   : JSON mode (clean stdout, machine readable JSON)\n");
  fprintf(stderr, "  -s   : Scan only (Identify but don't write anything)\n");
  fprintf(stderr, "  -f <index> : specify which found module to write (default 0)\n");
  fprintf(stderr, "\nCheck the documentation for more info !\n");
}

void DisplayFooter(void)
{
  printf("\n 1997-2016 (c) Sylvain \"Asle\" Chipaux (asle@free.fr)\n\n");
}

int main(int ac, char **av)
{

  if (ac < 3)
  {
    DisplayUsage(av[0], NULL);
    exit(0);
  }

  int positional_arg_count = 0;
  char *input_file = NULL;

  for (PW_j = 1; PW_j < ac; PW_j++)
  {
    if (strcmp(av[PW_j], "-f") == 0)
    {
      if (PW_j + 1 < ac)
      {
        Requested_Index = atoi(av[PW_j + 1]);
        Filter_By_Index = GOOD;
        PW_j++; /* skip index */
      }
      continue;
    }

    if (av[PW_j][0] == '-')
    {
      if (strstr(av[PW_j], "j") != NULL)
        Script_Mode = GOOD;
      if (strstr(av[PW_j], "s") != NULL)
      {
        Scan_Only = GOOD;
        Do_Depack = BAD;
      }
      if (strstr(av[PW_j], "r") != NULL)
      {
        Do_Rip = GOOD;
        Do_Depack = GOOD;
        Do_Module_Mode = GOOD;
      }
      if (strstr(av[PW_j], "d") != NULL)
      {
        Do_Rip = GOOD;
        Do_Data_Mode = GOOD;
      }
      continue;
    }

    /* Positional arguments */
    if (positional_arg_count == 0)
    {
      input_file = av[PW_j];
    }
    else if (positional_arg_count == 1)
    {
      strcpy(User_OutName, av[PW_j]);
    }
    positional_arg_count++;
  }

  if (input_file == NULL)
  {
    DisplayUsage(av[0], NULL);
    exit(0);
  }

  if (Scan_Only == GOOD)
  {
    Do_Depack = BAD;
  }

  if (Scan_Only == BAD && positional_arg_count < 2)
  {
    DisplayUsage(av[0], "Missing output file !");
    exit(0);
  }

  if (Scan_Only == GOOD && Do_Rip == BAD && Do_Data_Mode == BAD)
  {
    /* if only -s is specified, assume we want to rip modules at least */
    Do_Rip = GOOD;
    Do_Module_Mode = GOOD;
  }

  if (positional_arg_count < 2)
  {
    User_OutName[0] = '\0';
  }

  if (Script_Mode != GOOD)
  {
    DisplayBanner(stdout);
  }

  PW_in = fopen(input_file, "rb");
  if (PW_in == NULL)
  {
    fprintf(stderr, "cant find \"%s\" !\n", input_file);
    exit(0);
  }

  /* take care of the editable extensions */
  Support_Types();

  /* get input file size */
  PW_in_size = PWGetFileSize(input_file);
  fseek(PW_in, 0, 0); /* probably useless */
  if (Script_Mode != GOOD)
  {
    printf("input file size : %lld\n", (long long)PW_in_size);
  }
  if (PW_in_size < MINIMAL_FILE_LENGHT)
  {
    fprintf(stderr, "! input file size is too small ...\n");
    fclose(PW_in);
    exit(1);
  }

  /* alloc mem */
  in_data = (uint8_t *)malloc(PW_in_size);
  if (in_data == NULL)
  {
    perror("Couldn't allocate memory");
    exit(0);
  }
  fread(in_data, PW_in_size, 1, PW_in);
  fclose(PW_in);

  PW_ScanData();
  free(in_data);
  if (Script_Mode != GOOD)
  {
    DisplayFooter();
  }
  exit(0);
}


