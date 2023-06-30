
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
#include "dataset.h"
#include "dchmod.h"
#include "dchmodcli.h"

static void syntax()
{
  fprintf(stderr, 
    "dchmod [OPTION]... MODE[,MODE]... DATASET...\n"
    "dchmod [OPTION]... --reference=RDATASET DATASET...\n"
      "Change the mode of each DATASET to MODE.\n"
      "  With --reference, change the mode of each DATASET to that of RDATASET.\n"
    "OPTION is one or more of:\n"
    "  -c, --changes\n"
    "         like verbose but report only when a change is made\n"
    "  -f, --silent, --quiet\n"
    "         suppress most error messages\n"
    "  -v, --verbose\n"
    "         output a diagnostic for every file processed\n"
    "  --no-preserve-hlq\n"
    "         do not treat root hlq specially\n"
    "  --preserve-hlq\n"
    "         fail to operate recursively on hlq\n" 
    "  --reference=RDATASET\n"
    "         use RDATASET's mode instead of specifying MODE values.\n" 
    "  -R, --recursive\n"
    "         change partitioned and sequential datasets that match root pattern.\n"
    "  --help display this help and exit\n"
    "  --version\n"
    "         output version information and exit\n"
    "  Each MODE is of the form\n"
    "  '[ugoa]*([-+=]([rwx]*))+'.\n"
  );
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

static ReferenceDataset* update_reference(DatasetChangeMode* dcm, const char* reference) 
{
  DatasetError err = check_dataset(reference);
  if (err) {
    pdataseterror(err);
    return NULL;
  }
  dcm->r.reference = reference;
  return &dcm->r;
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

  return 0;
}

static Option* options(DatasetChangeMode* dcm, ParameterState* ps, const char* argv[], int entry, Option* o) 
{
  const char* p;

  if ((argv[entry][0] != '-') || ps->opt_done) {
    ps->opt_done = 1;
    return NULL;
  }

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
        if (!update_reference(dcm, p)) {
          syntax();
        }
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
    default:
      syntax();
      break;
  }
  return o;
}

static void update_bits(ModeChange* users, ModeBits* mb) 
{
  if (mb->plus || mb->minus) {
    int sign = (mb->plus) ? +1 : -1;
    if (mb->read) {
      users->read = sign;
    }
    if (mb->write) {
      users->write = sign;
    }
    if (mb->exec) {
      users->exec = sign;
    }
  }
}

static Mode* update_mode(Mode* mode, ModeBits* mb) 
{
  if (mb->all) {
    mb->user = mb->group = mb->others = 1;
  }
  if (mb->user) {
    update_bits(&mode->user, mb);
  } 
  if (mb->group) {
    update_bits(&mode->group, mb);
  } 
  if (mb->others) {
    update_bits(&mode->others, mb);
  } 

  return mode;
}

static Mode* mode(ParameterState* ps, const char* argv[], int entry, Mode* m) 
{
  ModeBits mb = { 0 };

  char c = argv[entry][0];
  if (ps->mode_done) {
    return NULL;
  }
  int i=0;

  do {
    switch (argv[entry][i]) {
      case 'u':
        mb.user=1;
        break;
      case 'g':
        mb.group=1;
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
        mb.minus=1;
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

static void propagate_mode(ModeChange* result, ModeChange* next)
{
  if (next->read) {
    result->read = next->read;
  }
  if (next->write) {
    result->write = next->write;
  }
  if (next->exec) {
    result->exec = next->exec;
  }
}

static int add_mode(DatasetChangeMode* dcm, Mode* mode) 
{
  propagate_mode(&dcm->m.user, &mode->user);
  propagate_mode(&dcm->m.group, &mode->group);
  propagate_mode(&dcm->m.others, &mode->others);
  return 0;
}

static int compute_modes(DatasetChangeMode* dcm) 
{
  return 0;
}

static Dataset* dataset(ParameterState* ps, const char* argv[], int entry, Dataset* ds) 
{
  DatasetError err = check_dataset(argv[entry]);
  if (err) {
    pdataseterror(err);
    return NULL;
  }
  ds->name = argv[entry];
  return ds;
}

static void print_attribute_change(const char* title, int bits) 
{
  fprintf(stderr, " %s", title);
  switch (bits) {
    case -1: 
      fputc('-', stderr); 
      break;
    case +1: 
      fputc('+', stderr); 
      break;
    case 0: 
      fputc('=', stderr); 
      break;
  }
}

static void print_mode_change(const char* title, ModeChange* cm) 
{
  fprintf(stderr, "%s:", title);
  print_attribute_change("r", cm->read);
  print_attribute_change("w", cm->write);
  print_attribute_change("x", cm->exec);
  fputs("\n", stderr);
}

static int change_mode_cli(DatasetChangeMode* dcm, Dataset* dataset)
{

  if (dcm->o.changes) {
    fprintf(stderr, "changes not implemented yet\n");
    return 1;
  }
  if (dcm->o.recursive) {
    fprintf(stderr, "recursive not implemented yet\n");
    return 1;
  }
  if (dcm->o.no_preserve_hlq) {
    fprintf(stderr, "no_preserve_hlq not implemented yet\n");
    return 1;
  }
  if (dcm->o.preserve_hlq) {
    fprintf(stderr, "preserve_hlq not implemented yet\n");
    return 1;
  }

  if (dcm->o.debug) {
    fputs("Options:\n", stderr);
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
    fputs("Mode change:\n", stderr);
    print_mode_change("user", &dcm->m.user);
    print_mode_change("group", &dcm->m.group);
    print_mode_change("others", &dcm->m.others);

    fputs("Reference Dataset:", stderr);
    if (dcm->r.reference) {
      fprintf(stderr, "%s\n", dcm->r.reference);
    } else {
      fputs("not specified\n", stderr);
    }
    fputs("Dataset:", stderr);
    fprintf(stderr, "%s\n", dataset->name);
  }

  return dchmod(&dcm->m, dataset);
}

int main(int argc, const char* argv[])
{
  DatasetChangeMode dcm = { 0 };
  ParameterState ps = { 0 };
  Option o = {0};
  Mode m = { 0 } ;
  ReferenceDataset r = { 0 };
  Dataset d = { 0 };
  int i;

  for (i=1; i<argc && !ps.err && !ps.mode_done; ++i) {
    if (options(&dcm, &ps, argv, i, &o)) {
      add_option(&dcm, &o);
    } else if (mode(&ps, argv, i, &m)) {
      add_mode(&dcm, &m);
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

  if (dcm.o.reference) {
    if (compute_modes(&dcm)) {
      return 8;
    }
  }

  while (i < argc) {
    if (!dataset(&ps, argv, i, &d)) {
        return 8;
    } else {
      if (change_mode_cli(&dcm, &d)) {
        return 8;
      }
    }
    ++i;
  }
}


  
