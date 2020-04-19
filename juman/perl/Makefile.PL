# 	$Id$	

use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'		=> 'juman-perl',
    'VERSION_FROM'	=> 'lib/Juman.pm',
    'PREREQ_PM'		=> { Time::HiRes => 1.9726, }, # e.g., Module::Name => 1.1
    ($] >= 5.005 ?    ## Add these new keywords supported since 5.005
      (ABSTRACT		=> 'segment Japanese sentences to morphemes with Juman',
       AUTHOR		=> 'TSUCHIYA Masatoshi <tsuchiya@pine.kuee.kyoto-u.ac.jp>') : ()),
);

package MY;

sub constants {
    my( $this, @args ) = @_;
    for( grep( $_ !~ /\.pm$/, keys %{$this->{PM}} ), "lib/Juman/Grammar.pm" ){
	delete $this->{PM}->{$_};
	delete $this->{MAN3PODS}->{$_};
    }
    $this->SUPER::constants( @args );
}

sub top_targets {
    my $inherited = shift->SUPER::top_targets( @_ );
    $inherited =~ s/(\npure_all\s*::.*)/$1 jumandb_convert/;
    $inherited;
}

sub install {
    my $inherited = shift->SUPER::install( @_ );
    $inherited =~ s/(\npure_install\s*::.*)/$1 jumandb_install/;
    $inherited;
}

sub postamble {
    my $dicdir;
  detect_dict:
    for my $dir ( split( /:/, $ENV{PATH}.":/share/tool/juman/bin" ) ){
	$dir =~ s!/\Z!!;
	if( -x $dir."/juman" ){
	    $dir =~ s!/bin!!;	# = prefix
	    for my $x ( map( $dir."/".$_, qw( dic share/juman/dic lib/juman/dic ) ) ){
		if( -f $x."/JUMAN.katuyou" and -f $x."/JUMAN.grammar"){
		    $dicdir = $x;
		    last detect_dict;
		}
	    }
	}
    }
    unless( $dicdir ){
	$dicdir = ".";
	print STDERR <<'__error__';
==== ERROR ====

Can't detect where Juman dictionaries are.  Specify the path of your
dictionaries as follows:

    % make DICDIR=/where/is/juman/dic
    % make DICDIR=/where/is/juman/dic test
    # make DICDIR=/where/is/juman/dic install

==== ERROR ====
__error__
    }
    my( $libdir ) = ( "@ARGV" =~ m/\bINSTALLDIRS=vendor\b/ ? "INSTALLVENDORLIB" : "INSTALLSITELIB" );
    my $grmfile = q!$(INST_LIB)/Juman/Grammar.pm!;
    my $dicfile = q!$(INST_LIB)/Juman/JUMAN.dic.db!;
    my( @targets ) = ( $grmfile );
    # push( @targets, $dicfile ) if( eval { require DB_File; } );
    sprintf( <<'__targets__', $dicdir, join(" ",@targets), $libdir, $grmfile, $dicfile );
DICDIR       = %s
JUMANTARGETS = %s

jumandb_install: $(JUMANTARGETS)
	@umask 022; cp -fp $(JUMANTARGETS) $(%s)/Juman/

jumandb_convert: $(JUMANTARGETS)

%s: mkgrammarpm $(DICDIR)/JUMAN.grammar $(DICDIR)/JUMAN.katuyou
	-rm -f $@
	$(PERL) mkgrammarpm -o $@ $(DICDIR)

%s: mkdicdb $(DICDIR)/*.dic
	-rm -f $@
	$(PERL) mkdicdb -o $@ $(DICDIR)/*.dic
__targets__
}
