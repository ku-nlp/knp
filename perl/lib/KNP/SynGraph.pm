# $Id$
package KNP::SynGraph;
require 5.004_04; # For base pragma.
use strict;
use KNP::SynNode;
use Encode;

=head1 NAME

KNP::SynGraph - SynGraph in KNP

=head1 DESCRIPTION

SynGraphの各種情報を保持するオブジェクト．

=cut
sub new {
    my( $class, $str ) = @_;
    my $this = {};

    my $midasi_pat = '見出し';
    if( utf8::is_utf8( $str ) ){
	$midasi_pat = decode('euc-jp', $midasi_pat);
    }

    # !! 0 1D <見出し:国際化が><格解析結果:ガ格>
    my ($tagid, $dpnd, $string) = (split(' ', $str))[1,2,3];
    my @tagids = split(',', $tagid);

    my ($parent, $dpndtype, @parentids);
    if ($dpnd =~ /^([\-\,\/\d]+)([DPIA])$/) {
	$parent = $1;
	$dpndtype = $2;

	# 係り先が複数ある場合がある
	for (split('/', $parent)) {
	    # 対応する基本句IDが複数ある場合がある
	    push @parentids, [split(',', $_)];
	}
    }
    else {
	die "KNP::SynGraph: Illegal dpnd = $dpnd\n";
    }

    my ($midasi, @features);
    while ($string =~ /(<.+?>)/g) {
	my $s = $1;
	# 見出し
	if ($s =~ /<$midasi_pat:(.+?)>/) {
	    $midasi = $1;
	}
	# その他はfeature
	else {
	    push @features, $s;
	}
    }

    $this->{tagid} = $tagid;
    $this->{tagids} = \@tagids;
    $this->{parent} = $parent;
    $this->{parentids} = \@parentids;
    $this->{dpndtype} = $dpndtype;
    $this->{midasi} = $midasi;
    $this->{feature} = join('', @features);

    bless $this, $class;
}

=head1 METHODS

以下のメソッドが利用可能である。

=over 4

=item push_synnode

Synノードを追加する

=cut
sub push_synnode {
    my ($this, $str) = @_;

    my $synnode = KNP::SynNode->new($str);

    push @{$this->{synnodes}}, $synnode;
}

=item synnode

全てのSynノードを返す

=cut
sub synnode {
    my ($this) = @_;

    if( defined $this->{synnodes} ){
	@{$this->{synnodes}};
    } else {
	wantarray ? () : 0;
    }
}

=item tagid

対応する基本句IDを返す。

=cut
sub tagid {
    my ($this) = @_;
    $this->{tagid};
}

=item tagids

対応する基本句ID(配列)を返す。

=cut
sub tagids {
    my ($this) = @_;
    @{$this->{tagids}};
}

=item parent

係り先の基本句IDを返す。

=cut
sub parent {
    my ($this) = @_;
    $this->{parent};
}

=item parent

係り先の基本句ID(配列の配列)を返す。

=cut
sub parentids {
    my ($this) = @_;
    @{$this->{parentids}};
}

=item dpndtype

依存関係の種類(D,P,I,A)を返す。

=cut
sub dpndtype {
    my ($this) = @_;
    $this->{dpndtype};
}

=item midasi

見出しを返す。

=cut
sub midasi {
    my ($this) = @_;
    $this->{midasi};
}

=item feature

文法素性を返す。

=cut
sub feature {
    my ($this) = @_;
    $this->{feature};
}

=back

=head1 AUTHOR

=over 4

=item
柴田 知秀 <shibata@nlp.kuee.kyoto-u.ac.jp>

=cut

1;
__END__
# Local Variables:
# mode: perl
# coding: euc-japan
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:

