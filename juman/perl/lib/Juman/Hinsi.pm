# $Id$
package Juman::Hinsi;
require 5.004_04; # For base pragma.
use Carp;
use English qw/ $LIST_SEPARATOR $WARNING /;
use Juman::Grammar qw/ $HINSI $BUNRUI $TYPE $FORM /;
use strict;
use base qw/ Exporter /;
use vars qw/ @EXPORT_OK %EXPORT_TAGS $ENCODING /;
@EXPORT_OK = qw/ get_hinsi get_hinsi_id get_bunrui get_bunrui_id get_type get_type_id get_form get_form_id /;
%EXPORT_TAGS = ( all => [ @EXPORT_OK ] );

=head1 NAME

Juman::Hinsi - Juman 品詞体系を扱うライブラリ

=head1 SYNOPSIS

 use Juman::Hinsi qw/ get_hinsi_id /;
 $id = &get_hinsi_id( '名詞' );

=head1 DESCRIPTION

Juman 品詞体系の情報を得るための関数を提供するライブラリである．

=head1 FUNCTIONS

=over 4

=item get_hinsi ( ID )

品詞番号から品詞を得る

=cut

$ENCODING = $JUMAN::ENCODING ? $JUMAN::ENCODING : 'utf8';

sub _zerop {
    ( $_[0] =~ /\D/ )? $_[0] eq '*' : $_[0] == 0;
}

sub _indexp {
    ( $_[0] !~ /\D/ and $_[0] >= 1 );
}

sub get_hinsi {
    if( @_ == 2 ){
	shift;
    } elsif( @_ != 1 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_hinsi(@_): requires an argument";
    }
    my( $x ) = @_;
    if( exists $HINSI->[0]->{$x} ){
	$x;
    } elsif( &_indexp($x) and defined $HINSI->[$x] ){
	$HINSI->[$x];
    } else {
	carp "Unknown hinsi ($x)" if $WARNING;
	undef;
    }
}

=item get_hinsi_id ( STR )

品詞から品詞番号を得る

=cut
sub get_hinsi_id {
    if( @_ == 2 ){
	shift;
    } elsif( @_ != 1 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_hinsi_id(@_): requires an argument";
    }
    my( $x ) = @_;

    if (utf8::is_utf8($x)) { # encode if the input has utf8_flag
	$x = Encode::encode($ENCODING, $x);
    }

    if( exists $HINSI->[0]->{$x} ){
	$HINSI->[0]->{$x};
    } elsif( &_indexp($x) and defined $HINSI->[$x] ){
	$x;
    } else {
	carp "Unknown hinsi id ($x)" if $WARNING;
	undef;
    }
}

=item get_bunrui ( HINSI, ID )

細分類番号から細分類を得る

=cut
sub get_bunrui {
    if( @_ == 3 ){
	shift;
    } elsif( @_ != 2 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_bunrui(@_): requires 2 arguments";
    }
    my( $hinsi, $x ) = @_;
    if( defined( $hinsi = &get_hinsi($hinsi) ) ){
	if( exists $BUNRUI->{$hinsi} ){
	    if( exists $BUNRUI->{$hinsi}->[0]->{$x} ){
		return $x;
	    } elsif( &_indexp($x) and defined $BUNRUI->{$hinsi}->[$x] ){
		return $BUNRUI->{$hinsi}->[$x];
	    }
	} elsif( &_zerop($x) ){
	    return '*';
	}
	carp "Unknown bunrui ($x)" if $WARNING;
    }
    undef;
}

=item get_bunrui_id ( HINSI, STR )

細分類から細分類番号を得る

=cut
sub get_bunrui_id {
    if( @_ == 3 ){
	shift;
    } elsif( @_ != 2 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_bunrui_id(@_): requires 2 arguments";
    }
    my( $hinsi, $x ) = @_;

    if (utf8::is_utf8($x)) { # encode if the input has utf8_flag
	$x = Encode::encode($ENCODING, $x);
    }

    if( defined( $hinsi = &get_hinsi($hinsi) ) ){
	if( exists $BUNRUI->{$hinsi} ){
	    if( exists $BUNRUI->{$hinsi}->[0]->{$x} ){
		return $BUNRUI->{$hinsi}->[0]->{$x};
	    } elsif( &_indexp($x) and defined $BUNRUI->{$hinsi}->[$x] ){
		return $x;
	    }
	} elsif( &_zerop($x) ){
	    return 0;
	}
	carp "Unknown bunrui id ($x)" if $WARNING;
    }
    undef;
}

=item get_type ( ID )

活用型番号から活用型を得る

=cut
sub get_type {
    if( @_ == 2 ){
	shift;
    } elsif( @_ != 1 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_type_id(@_): requires an argument";
    }
    my( $x ) = @_;
    if( &_zerop($x) ){
	'*';
    } elsif( exists $TYPE->[0]->{$x} ){
	$x;
    } elsif( &_indexp($x) and defined $TYPE->[$x] ){
	$TYPE->[$x]->[0];
    } else {
	carp "Unknown katuyou type ($x)" if $WARNING;
	undef;
    }
}

=item get_type_id ( STR )

活用型から活用型番号を得る

=cut
sub get_type_id {
    if( @_ == 2 ){
	shift;
    } elsif( @_ != 1 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_type_id(@_): requires an argument";
    }
    my( $x ) = @_;

    if (utf8::is_utf8($x)) { # encode if the input has utf8_flag
	$x = Encode::encode($ENCODING, $x);
    }

    if( &_zerop($x) ){
	0;
    } elsif( exists $TYPE->[0]->{$x} ){
	$TYPE->[0]->{$x};
    } elsif( &_indexp($x) and defined $TYPE->[$x] ){
	$x;
    } else {
	carp "Unknown katuyou id ($x)" if $WARNING;
	undef;
    }
}

=item get_form ( TYPE, ID )

活用型と活用形番号から活用形を得る

=cut
sub get_form {
    if( @_ == 3 ){
	shift;
    } elsif( @_ != 2 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_form(@_): requires 2 arguments";
    }
    my( $type, $x ) = @_;
    if( defined( $type = &get_type($type) ) ){
	if( $type eq '*' ){
	    if( &_zerop($x) ){
		return '*';
	    }
	} elsif( exists $FORM->{$type} ){
	    if( exists $FORM->{$type}->[0]->{$x} ){
		return $x;
	    } elsif( &_indexp($x) and defined $FORM->{$type}->[$x] ){
		return $FORM->{$type}->[$x]->[0];
	    }
	}
	carp "Unknown katuyou form ($x)" if $WARNING;
    }
    undef;
}

=item get_form_id ( TYPE, STR )

活用型と活用形から活用形番号を得る

=cut
sub get_form_id {
    if( @_ == 3 ){
	shift;
    } elsif( @_ != 2 ){
        local $LIST_SEPARATOR = ', ';
        croak "get_form_id(@_): requires 2 arguments";
    }
    my( $type, $x ) = @_;

    if (utf8::is_utf8($x)) { # encode if the input has utf8_flag
	$x = Encode::encode($ENCODING, $x);
    }

    if( defined( $type = &get_type($type) ) ){
	if( $type eq '*' ){
	    if( &_zerop($x) ){
		return 0;
	    }
	} elsif( exists $FORM->{$type} ){
	    if( exists $FORM->{$type}->[0]->{$x} ){
		return $FORM->{$type}->[0]->{$x};
	    } elsif( &_indexp($x) and defined $FORM->{$type}->[$x] ){
		return $x;
	    }
	}
	carp "Unknown katuyou form id ($x)" if $WARNING;
    }
    undef;
}

1;

=back

=head1 NOTES

C<Juman> オブジェクトのメソッドとして利用することもできる．

  Example:

     use Juman;
     $juman = new Juman();
     $id = $juman->get_hinsi_id( '名詞' );

=head1 SEE ALSO

=over 4

=item *

L<Juman>

=item *

L<Juman::Grammar>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=back

=cut

__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
