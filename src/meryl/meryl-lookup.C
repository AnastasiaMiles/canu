
/******************************************************************************
 *
 *  This file is part of canu, a software program that assembles whole-genome
 *  sequencing reads into contigs.
 *
 *  This software is based on:
 *    'Celera Assembler' (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' (http://kmer.sourceforge.net)
 *  both originally distributed by Applera Corporation under the GNU General
 *  Public License, version 2.
 *
 *  Canu branched from Celera Assembler at its revision 4587.
 *  Canu branched from the kmer project at its revision 1994.
 *
 *  Modifications by:
 *
 *    Brian P. Walenz beginning on 2018-JUL-21
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *  File 'README.licenses' in the root directory of this distribution contains
 *  full conditions and disclaimers for each license.
 */

#include "kmers.H"
#include "sequence.H"
#include "bits.H"


#define OP_NONE       0
#define OP_EXISTENCE  1


void
reportExistence(dnaSeqFile           *sf,
                kmerCountExactLookup *kl) {
  uint64   nameMax = 0;
  char    *name    = NULL;
  uint64   seqLen  = 0;
  uint64   seqMax  = 0;
  char    *seq     = NULL;
  uint8   *qlt     = NULL;

  while (sf->loadSequence(name, nameMax, seq, qlt, seqMax, seqLen)) {
    kmerIterator  kiter(seq, seqLen);

    uint64   nKmer      = 0;
    uint64   nKmerFound = 0;

    while (kiter.nextMer()) {
      nKmer++;

      if ((kl->value(kiter.fmer()) > 0) ||
          (kl->value(kiter.rmer()) > 0))
        nKmerFound++;
    }
      
    fprintf(stdout, "%s\t%lu\t%lu\t%lu\n", name, nKmer, kl->nKmers(), nKmerFound);
  }

  delete [] name;
  delete [] seq;
  delete [] qlt;
}



int
main(int argc, char **argv) {
  char   *inputSeqName = NULL;
  char   *inputDBname  = NULL;
  uint32  minV         = 0;
  uint32  maxV         = UINT32_MAX;
  uint32  threads      = 1;
  uint32  reportType   = OP_NONE;

  argc = AS_configure(argc, argv);

  vector<char *>  err;
  int             arg = 1;
  while (arg < argc) {
    if        (strcmp(argv[arg], "-sequence") == 0) {   //  INPUT READS and RANGE TO PROCESS
      inputSeqName = argv[++arg];

    } else if (strcmp(argv[arg], "-mers") == 0) {
      inputDBname = argv[++arg];

    } else if (strcmp(argv[arg], "-min") == 0) {
      minV = strtouint32(argv[++arg]);

    } else if (strcmp(argv[arg], "-max") == 0) {
      maxV = strtouint32(argv[++arg]);

    } else if (strcmp(argv[arg], "-threads") == 0) {
      threads = strtouint32(argv[++arg]);

    } else if (strcmp(argv[arg], "-existence") == 0) {
      reportType = OP_EXISTENCE;

    } else {
      char *s = new char [1024];
      snprintf(s, 1024, "Unknown option '%s'.\n", argv[arg]);
      err.push_back(s);
    }

    arg++;
  }

  if (inputSeqName == NULL)
    err.push_back("No query meryl database (-mers) supplied.\n");
  if (inputDBname == NULL)
    err.push_back("No input sequences (-sequence) supplied.\n");
  if (reportType == OP_NONE)
    err.push_back("No report-type (-existence, etc) supplied.\n");

  if (err.size() > 0) {
    fprintf(stderr, "usage: %s <report-type> -sequence <input.fasta> -mers <input.meryl>\n", argv[0]);
    fprintf(stderr, "  Query the kmers in meryl database <input.meryl> with the sequences\n");
    fprintf(stderr, "  in <input.fasta> (both FASTA and FASTQ supported, file can be compressed).\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  The meryl database can be filtered by value.  More advanced filtering\n");
    fprintf(stderr, "  requires a new database to be constructed using meryl.\n");
    fprintf(stderr, "    -min   m    Ignore kmers with value below m\n");
    fprintf(stderr, "    -max   m    Ignore kmers with value above m\n");
    fprintf(stderr, "    -threads t  Number of threads to use when constructing lookup table.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Exactly one report type must be specified.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -existence     Report a tab-delimited line for each sequence showing\n");
    fprintf(stderr, "                 the number of kmers in the sequence, in the database,\n");
    fprintf(stderr, "                 and in both.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "     output:  seqName <tab> mersInSeq <tab> mersInDB <tab> mersInBoth\n");
    fprintf(stderr, "         seqName    - name of the sequence\n");
    fprintf(stderr, "         mersInSeq  - number of mers in the sequence\n");
    fprintf(stderr, "         mersInDB   - number of mers in the meryl database\n");
    fprintf(stderr, "         mersInBoth - number of mers in the sequence that are\n");
    fprintf(stderr, "                      also in the database\n");
    fprintf(stderr, "\n");

    for (uint32 ii=0; ii<err.size(); ii++)
      if (err[ii])
        fputs(err[ii], stderr);

    exit(1);
  }

  omp_set_num_threads(threads);

  //  Open the kmers, build a lookup table.

  fprintf(stderr, "-- Loading kmers from '%s' into lookup table.\n", inputDBname);

  kmerCountFileReader   *merylDB    = new kmerCountFileReader(inputDBname);
  kmerCountExactLookup  *kmerLookup = new kmerCountExactLookup(merylDB, minV, maxV);

  delete merylDB;   //  Not needed anymore.

  //  Open sequences.

  fprintf(stderr, "-- Opening sequences in '%s'.\n", inputSeqName);

  dnaSeqFile            *seqFile    = new dnaSeqFile(inputSeqName);


  //  Do something.

  if (reportType == OP_EXISTENCE) {
    reportExistence(seqFile, kmerLookup);
  }

#if 0
  rr = new kmerCountFileReader(queryDBname);

  uint64  tested    = 0;
  uint64  kmerFound = 0;
  uint64  valuFound = 0;

  while (rr->nextMer()) {
    kmer    k = rr->theFMer();
    uint32  c = rr->theCount();
    uint32  v = ll->value(k);

    tested++;

    if (v > 0)
      kmerFound++;

    if (c == v)
      valuFound++;
  }
#endif

  delete seqFile;
  delete kmerLookup;

  //fprintf(stderr, "Tested %12lu kmers.\n", tested);
  //fprintf(stderr, "Found  %12lu kmers.                     %12lu missed.\n", kmerFound, tested-kmerFound);
  //fprintf(stderr, "Found  %12lu kmers with correct value.  %12lu missed.\n", valuFound, tested-valuFound);

  exit(0);
}



