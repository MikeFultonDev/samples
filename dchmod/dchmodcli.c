
/*
 * dchmod: code to update dataset file 'mode' bits
 *         interface inspired by 'chmod':
 *          - supporting ugo for symbolic pattern (u: user, g: group, o: universal access)
 *          - support + or - to add or remove access for specified symbolic pattern 
 *         Unlike chmod, you need to explicitly specify the user(s) and group(s) you want to
 *         make changes to.
 *         Also, unlike chmod, there is no -r or --recursive option. Instead you should
 *         end your dataset pattern with .* to indicate that a generic rule should be created.
 * Notes:
 *   If a non-RACF security subsystem is in use, it will report an error.
 *   Help would be greatly appreciated for TopSecret support to be added in.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dataset.h"
#include "accessid.h"
#include "dchmod.h"
#include "dchmodcli.h"

static void syntax()
{
  fprintf(stderr, 
    "dchmod [OPTION]... MODE[,MODE]... USER|GROUP[,USER|GROUP]... DATASET-PATTERN...\n"
    "dchmod [OPTION]... --reference=RDATASET-PATTERN DATASET-PATTERN...\n"
      "Change the mode of each DATASET-PATTERN to MODE for each specified USER and GROUP.\n"
      "  With --reference, change the mode of each DATASET-PATTERN to that of RDATASET-PATTERN.\n"
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
    "         fail to operate on just an hlq being specified\n" 
    "  --reference=RDATASET-PATTERN\n"
    "         use RDATASET-PATTERN's mode instead of specifying MODE values.\n" 
    "  --help display this help and exit\n"
    "  --version\n"
    "         output version information and exit\n"
    "  Each MODE is of the form\n"
    "  '[ugoa]*([-+]([rwx]*))+'.\n"
    "Examples\n"
    "  Give the group DEPT237 read access to all datasets that start with FULTON.DEPT237.*\n"
    "    dchmod g+r dept237 fulton.dept237.*\n"
    "  Give Dewayne read/write access to the dataset FULTON.COMMON.DATA\n"
    "    dchmod u+rw dewayne FULTON.COMMON.DATA\n"

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

static Dataset* update_reference(DatasetChangeMode* dcm, const char* reference) 
{
  DatasetError err = check_dataset(reference, 1);
  if (err) {
    fprintf(stderr, "Reference dataset: %s is invalid\n", reference);
    pdataseterror(err);
    return NULL;
  }
  normalize_dataset(reference, dcm->r.name);
  return &dcm->r;
}

static int add_option(DatasetChangeMode* dcm, Option* option) 
{
  dcm->o.changes |= option->changes;
  dcm->o.quiet |= option->quiet;
  dcm->o.verbose |= option->verbose;
  dcm->o.debug |= option->debug;
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
      } else if (!strcmp(argv[entry], "--help")) {
        o->help = 1;
      } else if ((p=startswith(argv[entry], "--reference="))) {
        if (!update_reference(dcm, p)) {
          syntax();
        }
        o->reference = 1;
        ps->mode_done = 1;
        ps->ids_done = 1;
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
  int i=0;

  char c = argv[entry][0];
  if (ps->mode_done) {
    return NULL;
  }

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

static AccessID* ids(ParameterState* ps, const char* argv[], int entry, AccessID** ids) 
{
  int i;
  size_t idnum=1;
  size_t idlen;
  const char* start=argv[entry];
  const char* next;

  if (ps->ids_done) {
    return NULL;
  }

  for (i=0; i<strlen(start); ++i) {
    if (start[i] == ',') {
      ++idnum;
    }
  }

  *ids = calloc(sizeof(AccessID), idnum);
  if (! (*ids)) {
    fprintf(stderr, "Error: Unable to acquire storage for access ids\n");
    return NULL;
  }

  for (i=0; i<idnum; ++i) {
    AccessIDError err = accessid_check(start, ',', &next);
    if (err != AccessIDOK) {
      fprintf(stderr, "User or Group list: %s is invalid\n", argv[entry]);
      paccessiderror(err);
      return NULL;
    }
    idlen = next-start;
    memcpy((*ids)[i].name, start, idlen);
    (*ids)[i].name[idlen] = '\0';
    start = next+1;
  }
  if (*next != '\0') {
    fprintf(stderr, "Internal Error: Unexpected mismatch in commas and number of entries\n");
    return NULL;
  }

  ps->ids_done = 1;

  return (*ids);
}

static int add_ids(DatasetChangeMode* dcm, AccessID* id)
{
  dcm->id = id;
  return 0;
}

static Dataset* dataset(ParameterState* ps, const char* argv[], int entry, Dataset* ds) 
{
  DatasetError err = check_dataset(argv[entry], 1);
  if (err) {
    fprintf(stderr, "Dataset: %s is invalid\n", argv[entry]);
    pdataseterror(err);
    return NULL;
  }
  normalize_dataset(argv[entry], ds->name);
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

static int change_mode_cli(DatasetChangeMode* dcm, Dataset* dataset, void* work)
{

  if (dcm->o.changes) {
    fprintf(stderr, "changes not implemented yet\n");
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
      "reference:%d\n"
      "help:%d\n"
      "version:%d\n"
      "no_preserve_hlq:%d\n"
      "preserve_hlq:%d\n", 
      dcm->o.changes,
      dcm->o.quiet,
      dcm->o.verbose,
      dcm->o.debug,
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
    fprintf(stderr, "%s\n", dcm->r.name);
    fputs("Dataset:", stderr);
    fprintf(stderr, "%s\n", dataset->name);
  }

  return dchmod(&dcm->m, dataset, work);
}

int main(int argc, const char* argv[])
{
  DatasetChangeMode dcm = { 0 };
  ParameterState ps = { 0 };
  Option o = {0};
  Mode m = { 0 } ;
  Dataset r = { 0 };
  Dataset d = { 0 };
  AccessID* id;
  int i;
  void* work;

  for (i=1; i<argc && !ps.err && !ps.ids_done; ++i) {
    if (options(&dcm, &ps, argv, i, &o)) {
      add_option(&dcm, &o);
    } else if (mode(&ps, argv, i, &m)) {
      add_mode(&dcm, &m);
    } else if (ids(&ps, argv, i, &id)) {
      add_ids(&dcm, id);
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

  work = dchmod_init(NULL, &dcm.m, (dcm.o.reference) ? &dcm.r : NULL, dcm.o.verbose);
  if (!work) {
    fprintf(stderr, "Error: Unable to retrieve RACF base information\n");
    return 8;
  }

  while (i < argc) {
    if (!dataset(&ps, argv, i, &d)) {
        return 8;
    } else {
      if (change_mode_cli(&dcm, &d, work)) {
        return 8;
      }
    }
    ++i;
  }
  dchmod_term(work);

  return 0;
}


  
