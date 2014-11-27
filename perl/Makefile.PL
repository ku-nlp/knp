use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'		=> 'knp-perl',
    'VERSION_FROM'	=> 'lib/KNP.pm',
    'PREREQ_PM'		=> { Juman => 0.5.7, }, # e.g., Module::Name => 1.1
    ($] >= 5.005 ?    ## Add these new keywords supported since 5.005
      (ABSTRACT		=> 'analyze dependency/case structure of Japanese sentences',
       AUTHOR		=> 'TSUCHIYA Masatoshi <tsuchiya@pine.kuee.kyoto-u.ac.jp>') : ()),
);

package MY;

sub constants {
    my( $this, @args ) = @_;
    for( grep( $_ !~ /\.pm$/, keys %{$this->{PM}} ) ){
	delete $this->{PM}->{$_};
    }
    $this->SUPER::constants( @args );
}
