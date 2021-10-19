#!/usr/bin/perl -w

#
#  smoke_tests.pl - Create and run smoke tests for TECO text editor.
#
#  @copyright 2021 Franklin P. Johnston / Nowwith Treble Software
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIA-
#  BILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.
#
################################################################################

use strict;
use warnings;
use version; our $VERSION = '1.0.0';

use Carp;
use Cwd qw(getcwd abs_path);
use English qw( -no_match_vars );
use autodie qw( open close );
use Getopt::Long;
use File::Basename;
use File::Copy;
use File::Find;
use File::Glob;
use File::Slurp;

# Command-line options

my $clean;
my $execute = 1;
my $target  = 'TECO-64';
my $verbose = q{};

my $indir;
my $outdir;
my $nscripts = 0;
my $nskipped = 0;
my $nfiles   = 0;
my $ntests   = 0;
my %scripts;
my @tecfiles = ();
my %tokens   = (
    'FAIL'  => q{[[error]] ^C},
    '^T'    => q{10^T},
    '-8'    => q{4096,0 ET},
    '"E'    => q{"E [[FAIL]] '},
    '"L'    => q{"L [[FAIL]] '},
    '"N'    => q{"N [[FAIL]] '},
    '"S'    => q{"S [[FAIL]] '},
    '"U'    => q{"U [[FAIL]] '},
    'I'     => q{10@I//},
    'PASS'  => qq{@^A/!PASS!/ [[^T]]},
    'exit'  => q{^D EK HK [[PASS]] EX},
    'enter' => q{HK 0,128ET},
    'error' => qq{@^A/!FAIL!/ [[^T]]},
    'in1'   => q{in_1.tmp},
    'in2'   => q{in_2.tmp},
    'out1'  => q{out_1.tmp},
    'out2'  => q{out_2.tmp},
    'cmd1'  => q{cmd_1.tmp},
    'cmd2'  => q{cmd_2.tmp},
    'log1'  => q{log_1.tmp},
    'bad'   => q{/dev/teco},
);

#
#  Main program start
#

initialize();

find( { wanted => \&wanted }, $indir );

# If we found any .tec files in the input directory tree, then they're helper
# files for the test scripts, so copy them to the output directory.

while ( defined( my $file = pop @tecfiles ) )
{
    copy( $file, $outdir )
      or croak "Failed to copy '$file' to '$outdir': $OS_ERROR";
}

# Sort all of the input files by file name, then create a test script for
# each one, translating any tokens as needed, then run the script, capture
# the output, and check to see whether we got the desired result.

foreach my $file ( sort { uc $a cmp uc $b } keys %scripts )
{
    my $infile = "$scripts{$file}/$file.test";
    my @input  = read_file($infile);

    next if ( $#input + 1 == 0 );

    my $outfile = "$file.tec";
    my ( $report, $text, $expects, $redirect ) =
      parse_script( $infile, $outfile, @input );

    next unless $report;

    # Found a real test script, so count it

    ++$nscripts;

    $text = translate_tokens( $infile, $text, $redirect );

    write_test( "$outdir$outfile", $text );

    if ($execute)
    {
        chdir $outdir or croak "Can't change directory to $outdir";

        my $actual   = run_test( $outfile, $report, $expects, $redirect );
        my $expected = find_log($outfile);

        check_test( $report, $expects, $actual, $expected );

        chdir q{..} or croak 'Can\'t change directory to \'..\'';
    }
}

# Clean up any temp. files we may have created.

my @files = glob "$outdir" . '*.tmp';

foreach my $file (@files)
{
    unlink $file or croak "Can't delete file: $file";
}

$nscripts += $nskipped;

print "\n" if $verbose;
printf "Found %u script%s in $indir, ", $nscripts, $nscripts == 1 ? q{} : 's';
printf "skipped %u\n", $nskipped;
printf "Created %u file%s in $outdir, ", $nfiles, $nfiles == 1 ? q{} : 's';
printf "executed $ntests test%s\n", $ntests == 1 ? q{} : 's';

exit;


sub check_error
{
    my ( $report, $expects, $actual ) = @_;
    my $match = '! Expects: ' . quotemeta $expects . ' !';

    $actual =~ s/$match//ms;

    # Here if test was supposed to abort with TECO error

    if ( index( $actual, $expects ) < 0 )
    {
        if ( $actual =~ /( \N{QUESTION MARK} \w\w\w )/ms )
        {
            # Test encountered unexpected TECO error

            printf "%s %s\n", $report, $1;
        }
        elsif ( $actual =~ /!FAIL!/ms )
        {
            # Test failed when it should have issued TECO error

            printf "%s FAIL\n", $report;
        }
        elsif ( $actual =~ /!PASS!/ms )
        {
            # Test succeeded when it should have failed

            printf "%s PASS\n", $report;
        }
        else
        {
            # Test encountered unknown error

            printf "%s UNKNOWN\n", $report;
        }
    }
    elsif ($verbose)
    {
        # Test encountered expected TECO error

        printf "%s OK\n", $report;
    }

    return;
}


sub check_fail
{
    my ( $report, $expected, $actual ) = @_;

    # Here if test was supposed to fail

    if ( $actual !~ /!FAIL\d+!/ms )
    {
        if ( $actual =~ / ( \N{QUESTION MARK} \w\w\w )/ms )
        {
            # Test encountered TECO error

            printf "%s %s\n", $report, $1;
        }
        elsif ( $actual =~ /!PASS!/ms )
        {
            # Test succeeded when it should have failed

            printf "%s PASS\n", $report;
        }
        else
        {
            # Test encountered unknown error

            printf "%s UNKNOWN\n", $report;
        }
    }
    elsif ($verbose)
    {
        printf "%s OK\n", $report;
    }

    return;
}


sub check_pass
{
    my ( $report, $expected, $actual ) = @_;

    # Here if test was supposed to succeed

    if ( $actual !~ /!PASS!/ms )
    {
        if ( $actual =~ / ( \N{QUESTION MARK} \w\w\w )/ms )
        {
            # Test encountered TECO error

            printf "%s %s\n", $report, $1;
        }
        elsif ( $actual =~ /!FAIL\d+!/ms )
        {
            # Test failed when it should have succeeded

            printf "%s FAIL\n", $report;
        }
        else
        {
            # Test encountered unknown error

            printf "%s UNKNOWN\n", $report;
        }
    }
    elsif ( defined $expected && $expected ne $actual )
    {
        printf "%s DIFF\n", $report;
    }
    elsif ($verbose)
    {
        printf "%s OK\n", $report;
    }

    return;
}


sub check_teco
{
    my ( $infile, $teco ) = @_;

    if (   ( $teco eq 'TECO C' && $target eq 'TECO-32' )
        || ( $teco eq 'TECO-10' && $target ne 'TECO-64' )
        || ( $teco eq 'TECO-32' && $target eq 'TECO C' )
        || ( $teco eq 'TECO-64' && $target ne 'TECO-64' ) )
    {
        ++$nskipped;

        print "Skipping file: $infile\n" if $verbose;

        return;
    }

    if (   $teco ne 'TECO'
        && $teco ne 'TECO C'
        && $teco ne 'TECO-10'
        && $teco ne 'TECO-32'
        && $teco ne 'TECO-64' )
    {
        croak "Unknown requirement $teco in script: $infile";
    }

    return $teco;
}


sub check_test
{
    my ( $report, $expects, $actual, $expected ) = @_;

    if ( $actual =~ / Aborted /ms )
    {
        # This is handled specially without regard to the desired result,
        # because it was found that some tests could complete successfully,
        # but abort on exit, due to such things as stack smashing. So we
        # first check to see if there was an abort in the output, and only
        # then check for desired success or failure.

        printf "%s ABORTED\n", $report;
    }
    elsif ( $expects eq 'PASS' )
    {
        check_pass( $report, $expected, $actual );
    }
    elsif ( $expects eq 'FAIL' )
    {
        check_fail( $report, $expected, $actual );
    }
    else    # Here if test failed or should fail
    {
        check_error( $report, $expects, $actual );
    }

    return;
}


sub find_log
{
    my ($file) = @_;

    $file =~ s/ [.] tec $ /.log/msx;

    if ( !-e $file )
    {
        return;
    }

    my $expected = read_file($file);

    if ($expected)
    {
        $expected =~ s/\r//msg;
    }

    return $expected;
}


sub initialize
{
    #
    #  Parse our command-line options
    #

    GetOptions(
        'clean!'   => \$clean,
        'execute!' => \$execute,
        'target=s' => \$target,
        'verbose'  => \$verbose,
    );

    croak 'Missing input directory'  if $#ARGV + 1 == 0;
    croak 'Missing output directory' if $#ARGV + 1 == 1;

    my %targets = (
        'TECOC'   => 'TECO C',
        'TECO32'  => 'TECO-32',
        'TECO-32' => 'TECO-32',
        'TECO64'  => 'TECO-64',
        'TECO-64' => 'TECO-64',
        'VMS'     => 'TECO-32',
    );

    my $orig = $target;

    $target = $targets{ uc $orig };

    croak "Invalid TECO version: $orig" unless $target;

    $indir  = $ARGV[0] =~ s{ \$ }{}mrs;
    $outdir = $ARGV[1] =~ s{ \$ }{}mrs;

    # If requested, delete any existing .tec files in output directory

    if ($clean)
    {
        my @outfiles = glob "$outdir/*.tec";

        foreach my $file (@outfiles)
        {
            unlink $file or croak "Can't delete file: $file";
        }
    }

    return;
}


sub parse_script
{
    my ( $infile, $outfile, @lines ) = @_;
    my $header      = 1;
    my $local_tests = 0;
    my $function;
    my $command;
    my $teco;
    my $redirect;
    my $expects;
    my $text;

    foreach my $line (@lines)
    {
        if ( $line =~ /\[\[enter\]\]/ms )
        {
            $header = 0;
        }
        elsif ($header)
        {
            if ( $line =~ /Function:\s(.+)\s!/ms )
            {
                $function = $1;
            }
            elsif ( $line =~ /Command:\s(.+)\s!/ms )
            {
                $command = $1;
            }
            elsif ( $line =~ /!\s+(<)?(TECO.*):\s(.+)\s!/ms )
            {
                if ( !$teco || $target eq 'TECO C' )
                {
                    $redirect = $1;
                    $expects  = $3;

                    $teco = check_teco( $infile, $2 );
                }

                return ( undef, undef, undef, undef ) unless $teco;
            }

            next;
        }

        $text .= $line;

        if ( $line =~ / Test: /ms )
        {
            ++$local_tests;
        }
    }

    croak "No function found in script: $infile"     unless $function;
    croak "No command found in script: $infile"      unless $command;
    croak "No TECO version found in script: $infile" unless $teco;
    croak "No expectations found in script: $infile" unless $expects;

    $ntests += $local_tests;

    $outfile =~ s/ (.+) [.]tec /$1/msx;

    my $report = sprintf '%-11s %-45s %-13s  (%2d)  %s ->', "$outfile",
      $function, $command, $local_tests, $expects;

    return ( $report, $text, $expects, $redirect );
}


sub run_test
{
    my ( $file, $report, $expects, $redirect ) = @_;
    my $fh;

    unlink 'out_1.tmp', 'out_2.tmp';

    open $fh, '>', 'in_1.tmp' or die "Can't create in_1.tmp: $OS_ERROR\n";

    print {$fh} "hello, world!\n";

    close $fh;

    open $fh, '>', 'cmd_1.tmp' or die "Can't create cmd_1.tmp: $OS_ERROR\n";

    print {$fh} "@^A/hello, world!/ 10^T";

    close $fh;

    my $teco;
    my $actual;

    if ( $target eq 'TECO C' )
    {
        if ($redirect)
        {
            $teco = 'tecoc < ' . $file . ' 2>&1';
        }
        else
        {
            $teco = 'tecoc mung ' . $file . ' 2>&1';
        }

        $actual = qx/$teco/;    # Execute command and capture output

        if ( $file =~ /^EG_\d\d[.]tec$/ms )
        {
            local $ENV{TEC_INIT}    = 'TECO_INIT';
            local $ENV{TEC_LIBRARY} = 'TECO_LIBRARY';
            local $ENV{TEC_MEMORY}  = 'TECO_MEMORY';
            local $ENV{TEC_VTEDIT}  = 'TECO_VTEDIT';

            $actual = qx/$teco/;    # Execute command and capture output
        }
        else
        {
            $actual = qx/$teco/;    # Execute command and capture output
        }
    }
    elsif ( $target eq 'TECO-32' )
    {
        croak "$target target not implemented yet";
    }
    elsif ( $target eq 'TECO-64' )
    {
        if ($redirect)
        {
            $teco = 'teco -n -I"\'0,128ET\'" < ' . $file . ' 2>&1';
        }
        else
        {
            $teco = 'mungx -n -I"\'0,128ET\'" ' . $file . ' 2>&1';
        }

        if ( $file =~ /^EG_\d\d[.]tec$/ms )
        {
            local $ENV{TECO_INIT}    = 'TECO_INIT';
            local $ENV{TECO_LIBRARY} = 'TECO_LIBRARY';
            local $ENV{TECO_MEMORY}  = 'TECO_MEMORY';
            local $ENV{TECO_VTEDIT}  = 'TECO_VTEDIT';

            $actual = qx/$teco/;    # Execute command and capture output
        }
        else
        {
            $actual = qx/$teco/;    # Execute command and capture output
        }
    }
    else
    {
        croak "Unknown target: $target";
    }

    $actual =~ s/\r//gms;

    if ( $actual =~ /!START!.(.+)/msx )
    {
        $actual = $1;
    }

    return $actual;
}


sub translate_tokens
{
    my ( $file, $text, $redirect ) = @_;
    my $sequence = 0;

    while ( $text =~ / (.*) \[ \[ (.+) \] \] (.*) /msx )
    {
        croak "Can't translate token $2 in script: $file"
          unless exists $tokens{$2};

        my $before = $1;
        my $token  = $2;
        my $middle = $tokens{$token};
        my $after  = $3;

        if ( $middle =~ /(.+!FAIL)(!.+)/ms )
        {
            $middle = $1 . sprintf '%u', ++$sequence;
            $middle .= $2;
        }

        if ( $target eq 'TECO C' )
        {
            if ( $token eq '^T' )
            {
                #                $middle = " 13^T $middle";
            }
            elsif ( $token eq 'FF' || $token eq '-8' )
            {
                $middle = q{};
            }
            elsif ( $token eq 'I' )
            {
                $middle = ' 13@I// ' . $middle;
            }
        }
        elsif ( $target eq 'TECO-64' && $token eq 'enter' )
        {
            $middle .= ' 0E1 1,0E3';
        }

        if ($redirect)
        {
            if ( $token eq 'enter' )
            {
                $middle .= "\e\e\n";
            }
            elsif ( $token eq 'exit' )
            {
                $middle = "\e\e\n$middle";
            }
        }

        $text = $before . $middle . $after;
    }

    $text .= "\e\e";

    return $text;
}


sub wanted
{
    my $infile = $_;

    # Only look for TECO script files

    if ( $infile =~ / (.+) [.] test $ /msx )
    {
        $scripts{$1} = $File::Find::dir;
    }
    elsif ( $infile =~ / [.] tec $ /msx && $outdir )
    {
        $infile = getcwd . q{/} . $infile;

        push @tecfiles, $infile;
    }

    return;
}


sub write_test
{
    my ( $file, $text ) = @_;

    $file =~ s{[.]test$}{.tec}ms;

    open my $fh, '>', $file or croak "Can't open output file: $file";

    print {$fh} $text;

    close $fh;

    ++$nfiles;

    return;
}
