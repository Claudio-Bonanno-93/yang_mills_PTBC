#ifndef YM_TUBE_DISC_C
#define YM_TUBE_DISC_C

#include"../include/macro.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#ifdef OPENMP_MODE
  #include<omp.h>
#endif

#include"../include/function_pointers.h"
#include"../include/gauge_conf.h"
#include"../include/geometry.h"
#include"../include/gparam.h"
#include"../include/random.h"

void real_main(char *in_file)
    {
    Gauge_Conf GC;
    Geometry geo;
    GParam param;

    int count;
    FILE *datafilep, *chiprimefilep, *topchar_tprof_filep;
    time_t time1, time2;

    // to disable nested parallelism
    #ifdef OPENMP_MODE
      omp_set_nested(0);
    #endif

    // read input file
    readinput(in_file, &param);

    int tmp=param.d_size[1];
    for(count=2; count<STDIM; count++)
       {
       if(tmp!= param.d_size[count])
         {
         fprintf(stderr, "When using yang_mills_tube_disc all the spatial sizes have to be of equal length.\n");
         exit(EXIT_FAILURE);
         }
       }

    // initialize random generator
    initrand(param.d_randseed);

    // open data_file
    init_data_file(&datafilep, &chiprimefilep, &topchar_tprof_filep, &param);

    // initialize geometry
    init_indexing_lexeo();
    init_geometry(&geo, &param);

    // initialize gauge configuration
    init_gauge_conf(&GC, &param);

    // allocate ml_polycorr and ml_polyplaq arrays
    alloc_tube_disc_stuff(&GC, &param);

    // montecarlo
    time(&time1);
    // count starts from 1 to avoid problems using %
    for(count=1; count < param.d_sample + 1; count++)
       {
       update(&GC, &geo, &param);

       if(count % param.d_measevery ==0 && count >= param.d_thermal)
         {
         perform_measures_tube_disc(&GC, &geo, &param, datafilep);
         }

       // save configuration for backup
       if(param.d_saveconf_back_every!=0)
         {
         if(count % param.d_saveconf_back_every == 0 )
           {
           // simple
           write_conf_on_file(&GC, &param);

           // backup copy
           write_conf_on_file_back(&GC, &param);
           }
         }
       }
    time(&time2);
    // montecarlo end

    // close data file
    fclose(datafilep);

    // save configuration
    if(param.d_saveconf_back_every!=0)
      {
      write_conf_on_file(&GC, &param);
      }

    // print simulation details
    print_parameters_tube_disc(&param, time1, time2);

    // free gauge configuration
    free_gauge_conf(&GC, &param);

    // free ml_polycorr and ml_polyplaq
    free_tube_disc_stuff(&GC, &param);

    // free geometry
    free_geometry(&geo, &param);
    }


void print_template_input(void)
  {
  FILE *fp;

  fp=fopen("template_input.in", "w");

  if(fp==NULL)
    {
    fprintf(stderr, "Error in opening the file template_input.in (%s, %d)\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
    }
  else
    {
    fprintf(fp, "size 4 4 4 4\n");
    fprintf(fp,"\n");
    fprintf(fp, "beta 5.705\n");
    fprintf(fp, "theta 1.5\n");
    fprintf(fp,"\n");
    fprintf(fp, "sample    10\n");
    fprintf(fp, "thermal   0\n");
    fprintf(fp, "overrelax 5\n");
    fprintf(fp, "measevery 1\n");
    fprintf(fp,"\n");
    fprintf(fp, "start                   0  # 0=ordered  1=random  2=from saved configuration\n");
    fprintf(fp, "saveconf_back_every     5  # if 0 does not save, else save backup configurations every ... updates\n");
    fprintf(fp,"\n");
    fprintf(fp, "#for multilevel\n");
    fprintf(fp, "multihit         10  # number of multihit step\n");
    fprintf(fp, "ml_step          2   # timeslices for multilevel (from largest to smallest)\n");
    fprintf(fp, "ml_upd           10  # number of updates for various levels\n");
    fprintf(fp, "dist_poly        2   # distance between the polyakov loop\n");
    fprintf(fp, "transv_dist      2   # transverse distance from the polyakov correlator\n");
    fprintf(fp, "plaq_dir         1 0 # plaquette orientation for flux tube\n");
    fprintf(fp,"\n");
    fprintf(fp, "#output files\n");
    fprintf(fp, "conf_file  conf.dat\n");
    fprintf(fp, "data_file  dati.dat\n");
    fprintf(fp, "log_file   log.dat\n");
    fprintf(fp, "\n");
    fprintf(fp, "randseed 0    #(0=time)\n");
    fclose(fp);
    }
  }


int main (int argc, char **argv)
    {
    char in_file[50];

    if(argc != 2)
      {
      printf("\nPackage %s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
      printf("Claudio Bonati %s\n", PACKAGE_BUGREPORT);
      printf("Usage: %s input_file\n\n", argv[0]);

      printf("Compilation details:\n");
      printf("\tN_c (number of colors): %d\n", NCOLOR);
      printf("\tST_dim (space-time dimensionality): %d\n", STDIM);
      printf("\tNum_levels (number of levels): %d\n", NLEVELS);
      printf("\n");
      printf("\tINT_ALIGN: %s\n", QUOTEME(INT_ALIGN));
      printf("\tDOUBLE_ALIGN: %s\n", QUOTEME(DOUBLE_ALIGN));

      #ifdef DEBUG
        printf("\n\tDEBUG mode\n");
      #endif

      #ifdef OPENMP_MODE
        printf("\n\tusing OpenMP with %d threads\n", NTHREADS);
      #endif

      #ifdef THETA_MODE
        printf("\n\tusing imaginary theta\n");
      #endif


      printf("\n");

      #ifdef __INTEL_COMPILER
        printf("\tcompiled with icc\n");
      #elif defined(__clang__)
        printf("\tcompiled with clang\n");
      #elif defined( __GNUC__ )
        printf("\tcompiled with gcc version: %d.%d.%d\n",
                __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
      #endif

      print_template_input();

      return EXIT_SUCCESS;
      }
    else
      {
      if(strlen(argv[1]) >= STD_STRING_LENGTH)
        {
        fprintf(stderr, "File name too long. Increse STD_STRING_LENGTH in include/macro.h\n");
        }
      else
        {
        strcpy(in_file, argv[1]);
        }
      }

    real_main(in_file);

    return EXIT_SUCCESS;
    }

#endif

