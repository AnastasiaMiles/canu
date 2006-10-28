use strict;

sub createFragmentCorrectionJobs {
    my $frgCorrBatchSize  = getGlobal("frgCorrBatchSize");
    my $frgCorrThreads    = getGlobal("frgCorrThreads");
    my $frgCorrOnGrid     = getGlobal("frgCorrOnGrid") && getGlobal("useGrid");
    my $scratch           = getGlobal("scratch");

    return if (getGlobal("doFragmentCorrection") == 0);

    system("mkdir $wrk/2-frgcorr") if (! -e "$wrk/2-frgcorr");

    return if (-e "$wrk/2-frgcorr/jobsCreated.success");

    my $frgBeg = 1;
    my $frgEnd = 0;

    if ($frgCorrOnGrid) {
        open(SUB, "> $wrk/2-frgcorr/submit.sh") or die;
        print SUB "#!/bin/sh\n\n";
    }

    while ($frgBeg < $numFrags) {
        $frgEnd = $frgBeg + $frgCorrBatchSize - 1;
        $frgEnd = $numFrags if ($frgEnd > $numFrags);

        #  We put 1,000,000 fragments (~12 jobs) into a single directory,
        #  just to get them out of the way.

        my $jobName   = substr("0000000000" . $frgBeg, -8);
        my $batchName = substr($jobName, 0, 2);

        system("mkdir $wrk/2-frgcorr/$batchName") if (! -d "$wrk/2-frgcorr/$batchName");

        if ($frgCorrOnGrid) {
            #  Run the correction ob on the grid.

            open(F, "> $wrk/2-frgcorr/$batchName/$jobName.sh") or die;
            print F "#!/bin/sh\n\n";
            print F "jid=\$\$\n";
            #print F "$processStats \\\n";
            print F "$gin/correct-frags \\\n";
            print F "  -S $wrk/$asm.ovlStore \\\n";
            print F "  -o $scratch/frgcorr-$batchName-$jobName.\$jid.frgcorr \\\n";
            print F "  $wrk/$asm.frgStore \\\n";
            print F " $frgBeg $frgEnd \\\n";
            print F " > $wrk/2-frgcorr/$jobName.err 2>&1 \\\n";
            print F "&&  \\\n";
            print F "mv $scratch/frgcorr-$batchName-$jobName.\$jid.frgcorr \\\n";
            print F "   $wrk/2-frgcorr/$batchName/$jobName.frgcorr \\\n";
            print F "&& \\\n";
            print F "touch $wrk/2-frgcorr/$batchName/$jobName.success\n";
            close(F);

            chmod 0755, "$wrk/2-frgcorr/$batchName/$jobName.sh";

            my $sge                   = getGlobal("sge");
            my $sgeFragmentCorrection = getGlobal("sgeFragmentCorrection");

            print SUB "qsub $sge $sgeFragmentCorrection ";
            print SUB " -r y -N frg_${asm}_${batchName}_$jobName ";
            print SUB " -j y -o $wrk/2-frgcorr/$batchName/$jobName.grid.err ";
            print SUB "$wrk/2-frgcorr/$batchName/$jobName.sh\n";
        } else {
            #  Run the correction job right here, right now.

            open(F, "> $wrk/2-frgcorr/$batchName/$jobName.sh") or die;
            print F "#!/bin/sh\n\n";
            #print F "$processStats \\\n";
            print F "$bin/correct-frags \\\n";
            print F "  -t $frgCorrThreads \\\n";
            print F "  -S $wrk/$asm.ovlStore \\\n";
            print F "  -o $wrk/2-frgcorr/$batchName/$jobName.frgcorr \\\n";
            print F "  $wrk/$asm.frgStore \\\n";
            print F " $frgBeg $frgEnd \\\n";
            print F " > $wrk/2-frgcorr/$jobName.err 2>&1 \\\n";
            print F "&&  \\\n";
            print F "touch $wrk/2-frgcorr/$batchName/$jobName.success\n";
            close(F);

            chmod 0755, "$wrk/2-frgcorr/$batchName/$jobName.sh";

            &scheduler::schedulerSubmit("$wrk/2-frgcorr/$batchName/$jobName.sh > /dev/null 2>&1");
        }

        $frgBeg = $frgEnd + 1;
    }

    if ($frgCorrOnGrid) {
        close(SUB);
        pleaseExecute("$wrk/2-frgcorr/submit.sh");
        exit(0);
    } else {
        &scheduler::schedulerSetNumberOfProcesses($global{"frgCorrConcurrency"});
        &scheduler::schedulerFinish();
    }

    touch("$wrk/2-frgcorr/jobsCreated.success");
}

1;
