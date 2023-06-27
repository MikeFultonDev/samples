/*
 * dchmod: code to update dataset file 'mode' bits
 *         reduced interface from 'chmod':
 *          - supporting ug for symbolic pattern (u: user, g: group)
 *          - support + or - to add or remove access for specified symbolic pattern 
 * Notes:
 *   If 'g' is specified and more than one group is associated with the dataset, it will report an error
 *   If a non-RACF security subsystem is in use, it will report an error
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
  unsigned int changes:1; 
  unsigned int quiet:1; 
  unsigned int verbose:1; 
  unsigned int debug:1; 
  unsigned int recursive:1; 
  unsigned int reference:1; 
  unsigned int help:1; 
  unsigned int version:1; 
  unsigned int no_preserve_hlq:1; 
  unsigned int preserve_hlq:1; 
} Option;

typedef struct {
  unsigned int user:1;
  unsigned int group:1;
  unsigned int others:1;
  unsigned int all:1;
  unsigned int plus:1;
  unsigned int read:1;
  unsigned int write:1;
  unsigned int exec:1;
} ModeBits; 

typedef struct {
  int tbd;
} Mode;
typedef struct {
  const char* reference;
} ReferenceDataset;
typedef struct {
  int tbd;
} Dataset;

typedef struct {
  Option o;
  Mode m;
  ReferenceDataset r;
  Dataset d;
} DatasetChangeMode;

typedef struct {
  unsigned int opt_done:1;
  unsigned int mode_done:1;
  unsigned int err:1;
} ParameterState;

static void syntax()
{
  fprintf(stderr, "dchmod [OPTION]... MODE[,MODE]... DATASET...\n");
  fprintf(stderr, "dchmod [OPTION]... --reference=RDATASET DATASET...\n");
  exit(4);
}

static const char* startswith(const char* str, const char* prefix)
{
  size_t prefix_len = strlen(prefix);
  if (strlen(str) > prefix_len && !memcmp(str, prefix, prefix_len)) {
    return &str[prefix_len];
  } else {
    return NULL;
  }
}

static void update_reference(DatasetChangeMode* dcm, const char* reference) 
{
  dcm->r.reference = reference;
}

static int add_option(DatasetChangeMode* dcm, Option* option) 
{
  dcm->o.changes |= option->changes;
  dcm->o.quiet |= option->quiet;
  dcm->o.verbose |= option->verbose;
  dcm->o.debug |= option->debug;
  dcm->o.recursive |= option->recursive;
  dcm->o.reference |= option->reference;
  dcm->o.help |= option->help;
  dcm->o.version |= option->version;
  dcm->o.no_preserve_hlq |= option->no_preserve_hlq;
  dcm->o.preserve_hlq |= option->preserve_hlq;

  if (option->debug) {
    fprintf(stderr, "Options:\n");
    fprintf(stderr, 
      "changes:%d\n"
      "quiet:%d\n"
      "verbose:%d\n"
      "debug:%d\n"
      "recursive:%d\n"
      "reference:%d\n"
      "help:%d\n"
      "version:%d\n"
      "no_preserve_hlq:%d\n"
      "preserve_hlq:%d\n", 
      dcm->o.changes,
      dcm->o.quiet,
      dcm->o.verbose,
      dcm->o.debug,
      dcm->o.recursive,
      dcm->o.reference,
      dcm->o.help,
      dcm->o.version,
      dcm->o.no_preserve_hlq,
      dcm->o.preserve_hlq
    );
  }
    
  return 0;
}

static Option* options(DatasetChangeMode* dcm, ParameterState* ps, const char* argv[], int entry) 
{
  Option* o = NULL;
  const char* p;

  if ((argv[entry][0] != '-') || ps->opt_done) {
    ps->opt_done = 1;
    return o;
  }

  o = calloc(sizeof(Option), 1);
  switch (argv[entry][1]) {
    case '-':
      if (!strcmp(argv[entry], "--changes")) {
        o->changes = 1;
      } else if (!strcmp(argv[entry], "--silent") || !strcmp(argv[entry], "--quiet")) {
        o->quiet = 1;
      } else if (!strcmp(argv[entry], "--verbose")) {
        o->verbose = 1;
      } else if (!strcmp(argv[entry], "--debug")) {
        o->debug = 1;
      } else if (!strcmp(argv[entry], "--version")) {
        o->version = 1;
      } else if (!strcmp(argv[entry], "--no-preserve-hlq")) {
        o->no_preserve_hlq = 1;
      } else if (!strcmp(argv[entry], "--preserve-hlq")) {
        o->preserve_hlq = 1;
      } else if (!strcmp(argv[entry], "--recursive")) {
        o->recursive = 1;
      } else if (!strcmp(argv[entry], "--help")) {
        o->help = 1;
      } else if ((p=startswith(argv[entry], "--reference="))) {
        o->reference = 1;
        update_reference(dcm, p);
      } else {
        syntax();
      }
      break;

    case 'c':
      if (argv[entry][2] == '\0') {
        o->changes = 1;
      } else {
        syntax();
      }
      break;
    case 'f':
      if (argv[entry][2] == '\0') {
        o->quiet = 1;
      } else {
        syntax();
      }
      break;
    case 'v':
      if (argv[entry][2] == '\0') {
        o->verbose = 1;
      } else {
        syntax();
      }
      break;
    case 'R':
      if (argv[entry][2] == '\0') {
        o->recursive = 1;
      } else {
        syntax();
      }
      break;
  }
  return o;
}

static Mode* update_mode(Mode* mode, ModeBits* mb) 
{
  if (!mode) {
    mode = calloc(sizeof(Mode), 1);
  }
  return mode;
}

static Mode* mode(ParameterState* ps, const char* argv[], int entry) 
{
  Mode* m = NULL;
  ModeBits mb = { 0 };

  char c = argv[entry][0];
  if (ps->mode_done) {
    return m;
  }
  int i=0;

  do {
    switch (argv[entry][i]) {
      case 'u':
        mb.user=1;
        break;
      case 'g':
        mb.user=1;
        break;
      case 'o':
        mb.others=1;
        break;
      case 'a':
        mb.all=1;
        break;
      case 'r':
        mb.read=1;
        break;
      case 'w':
        mb.write=1;
        break;
      case 'x':
        mb.exec=1;
        break;
      case '-':
        mb.plus=0;
        break;
      case '+':
        mb.plus=1;
        break;
      case ',':
      case '\0':
        m = update_mode(m, &mb);
        memset(&mb, 0, sizeof(ModeBits));
        break;
      default:
        fprintf(stderr, "dchmod: Invalid file mode: %s\n", argv[entry]);
        ps->err = 1;
        return NULL;
    }
  } while (argv[entry][i++] != '\0');
  ps->mode_done = 1;
  return m;
}

static int add_mode(DatasetChangeMode* dcm, Mode* mode) 
{
  return 0;
}
static ReferenceDataset* reference(ParameterState* ps, const char* argv[], int entry) 
{
  return NULL;
}
static int compute_modes(DatasetChangeMode* dcm, ReferenceDataset* ref) 
{
  return 0;
}
static Dataset* dataset(ParameterState* ps, const char* argv[], int entry) 
{
  return NULL;
}
static int add_dataset(DatasetChangeMode* dcm, Dataset* dataset) 
{
  return 0;
}

static int change_mode(DatasetChangeMode* dcm)
{

  if (dcm->o.changes) {
    fprintf(stderr, "changes not implemented yet\n");
    return 1;
  }
  if (dcm->o.recursive) {
    fprintf(stderr, "recursive not implemented yet\n");
    return 1;
  }
  if (dcm->o.reference) {
    fprintf(stderr, "no_preserve_hlq not implemented yet\n");
    return 1;
  }
  return 0;
}

int main(int argc, const char* argv[])
{
  DatasetChangeMode dcm = { 0 };
  ParameterState ps = { 0 };
  Option* o;
  Mode* m;
  ReferenceDataset* r;
  Dataset* d;
  int i;

  for (i=1; i<argc && !ps.err; ++i) {
    if ((o=options(&dcm, &ps, argv, i))) {
      add_option(&dcm, o);
    } else if ((m=mode(&ps, argv, i))) {
      add_mode(&dcm, m);
    } else if ((r=reference(&ps, argv, i))) {
      compute_modes(&dcm, r);
    } else if ((d=dataset(&ps, argv, i))) {
      add_dataset(&dcm, d);
    }
  }

  if (ps.err) {
    return 8;
  }

  if (dcm.o.version) {
    puts("dchmod 1.0.0");
    return 0;
  }
  if (dcm.o.help) {
    syntax();
  }
  
  return change_mode(&dcm);
}


  
