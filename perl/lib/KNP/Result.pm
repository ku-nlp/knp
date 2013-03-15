# $Id$
package KNP::Result;
require 5.004_04; # For base pragma.
use KNP::Bunsetsu;
use KNP::Morpheme;
use KNP::Tag;
use KNP::SynNodes;
use KNP::SynNode;
use strict;
use base qw/ KNP::BList /;
use vars qw/ %DEFAULT $ENCODING /;

=head1 NAME

KNP::Result - 構文解析結果オブジェクト

=head1 SYNOPSIS

  $result = new KNP::Result( "* -1D <BGH:解析>\n...\nEOS\n" );

=head1 DESCRIPTION

構文解析結果を保持するオブジェクト．

=head1 CONSTRUCTOR

=over 4

=item new ( RESULT )

KNP の出力文字列，または，その文字列を行を単位として格納されたリストに
対するリファレンス RESULT を引数として呼び出すと，その構文解析結果を表
すオブジェクトを生成する．

=item new ( OPTIONS )

以下の拡張オプションを指定してコンストラクタを呼び出す．

=over 4

=item result => RESULT

KNP の出力文字列，または，その文字列を行を単位として格納されたリストに
対するリファレンスを指定する．

=item pattern => STRING

構文解析結果を終端するためのパターンを指定する．

=item bclass => NAME

文節オブジェクトを指定する．無指定の場合は，C<KNP::Bunsetsu> を用いる．

=item mclass => NAME

形態素オブジェクトを指定する．無指定の場合は，C<KNP::Morpheme> を用いる．

=item tclass => NAME

タグオブジェクトを指定する．無指定の場合は，C<KNP::Tag> を用いる．

=back

=cut

$ENCODING = $KNP::ENCODING ? $KNP::ENCODING : 'utf8';

%DEFAULT = ( pattern => '^EO[SP]$',
	     bclass  => 'KNP::Bunsetsu',
	     mclass  => 'KNP::Morpheme',
	     tclass  => 'KNP::Tag' );

sub new {
    my $class = shift;

    my( %opt ) = %DEFAULT;
    if( @_ == 1 ){
	$opt{result} = shift;
    } else {
	while( @_ ){
	    my $key = shift;
	    my $val = shift;
	    $key =~ s/^-+//;
	    $opt{lc($key)} = $val;
	}
    }
    my $result  = $opt{result};
    my $pattern = $opt{pattern};
    my $bclass  = $opt{bclass};
    my $mclass  = $opt{mclass};
    my $tclass  = $opt{tclass};
    return undef unless( $result and $pattern and $bclass and $mclass and $tclass );

    # 文字列が直接指定された場合
    $result = [ map( "$_\n", split( /\n/, $result ) ) ] unless ref $result;

    my $this = { all => join( '', @$result ) };
    bless $this, $class;
    return $this unless $pattern;

    # 構文解析結果の先頭に含まれているコメントとエラーの部分を取り除く
    my( $str, $comment, $error );
    while( defined( $str = shift @$result ) ){
	if( $str =~ /^#/ ){
	    $comment .= $str;
	} elsif( $str =~ m!^;;! ){
	    $error .= $str;
	} else {
	    unshift( @$result, $str );
	    last;
	}
    }

    # for mrphtab
    my $input_mrphtab = 0;
    my ($mrph_parent_id, $mrph_dpndtype);
    while( defined( $str = shift @$result ) ){
	if( $str =~ m!$pattern! and @$result == 0 ){
	    $this->{_eos} = $str;
	    last;
	} elsif( $str =~ m!^;;! ){
	    $error .= $str;
	} elsif( $str =~ m!^\*! ){
	    my $bnst = $bclass->new( $str, scalar($this->bnst) );
	    return undef if !defined $bnst;
	    $this->push_bnst( $bnst );
	} elsif( $str =~ m!^\+! ){
	    $this->push_tag( $tclass->new( $str, scalar($this->tag) ) );
	} elsif( $str =~ m!^\- (-?\d+)(.+)$! ){
	    $input_mrphtab = 1;
	    $mrph_parent_id = $1;
	    $mrph_dpndtype = $2;
	} elsif( $str =~ m/^!!/ ){
	    my $synnodes = KNP::SynNodes->new($str);
	    push @{ ( $this->tag ) [-1]->{synnodes} }, $synnodes;
	} elsif( $str =~ m/^!/ ){
	    my $synnode = KNP::SynNode->new($str);
	    my $last_tag = ( $this->tag ) [-1];
	    # synnodesがある場合
	    if ( defined $last_tag->{synnodes} ){
		$last_tag->{synnodes}[-1]->push_synnode( $synnode );
	    }
	    # synnodesがない場合 (tagにpushする)
	    else {
		$last_tag->push_synnode( $synnode );
	    }
	} else {
	    $this->push_mrph( $mclass->new( $str, scalar($this->mrph), $mrph_parent_id, $mrph_dpndtype ) );
	    my $fstring = ( $this->mrph )[-1]->fstring;
	    while ( $fstring =~ /<(ALT-[^>]+)>/g ){ # ALT
		( $this->mrph )[-1]->push_doukei( $mclass->new( $1, scalar($this->mrph) ) );
	    }
	}
    }

    # 係り受け情報を取り出す
    my( @bnst ) = $this->bnst;
    for my $bnst ( @bnst ){
	$bnst->make_reference( \@bnst );
    }
    my( @tag ) = $this->tag;
    for my $tag ( @tag ){
	$tag->make_reference( \@tag );
    }
    if ($input_mrphtab) {
	my( @mrph ) = $this->mrph;
	for my $mrph ( @mrph ){
	    $mrph->make_reference( \@mrph );
	}
    }

    # 書き込みを禁止する
    $this->set_readonly();

    $this->{comment} = $comment;
    $this->{error}   = $error;
    $this;
}

=back

=head1 METHODS

構文解析結果は，対象となる文を文節単位に分解したリストと見ることができ
る．そのため，本クラスは C<KNP::BList> クラスを継承するように実装され
ており，以下のメソッドが利用可能である．

=over 4

=item bnst

文節列を取り出す．

=item tag

タグ列を取り出す．

=item mrph

形態素列を取り出す．

=back

これらのメソッドの詳細については，L<KNP::BList> を参照のこと．

加えて，以下のメソッドが定義されている．

=over 4

=item all

構文解析結果の全文字列を返す．

=cut
sub all {
    my( $this ) = @_;
    $this->{all} || undef;
}

=item all_dynamic

構文解析結果の全文字列を動的に作って返す．
関数push_featureなどを使って内部情報を書き換えた時に，allでは変更が反映されないため，この関数を使う．

=cut
sub all_dynamic {
    my( $this, $option ) = @_;

    my $MIDASI = '見出し';
    my $SCORE = 'スコア';

    if (utf8::is_utf8($this->{all})) {
	require Encode;
	$MIDASI = Encode::decode($ENCODING, $MIDASI);
	$SCORE = Encode::decode($ENCODING, $SCORE);
    }

    my $ret;
    $ret .= $this->{comment};
    # 文節を順番に
    foreach my $bnst ($this->bnst) {
	$ret .= '* ';
	$ret .= defined $bnst->parent ? $bnst->parent->id : -1;
	$ret .=  $bnst->dpndtype . ' ' . $bnst->fstring . "\n";

	# 基本句を順番に
	foreach my $tag ($bnst->tag) {
	    $ret .=  '+ ';
	    $ret .= defined $tag->parent ? $tag->parent->id : -1;
	    $ret .= $tag->dpndtype . ' ' . $tag->fstring . "\n";

	    # 形態素を順番に
	    for my $mrph ($tag->mrph) {
		if (defined $mrph->dpndtype) {
		    $ret .=  '- ';
		    $ret .= defined $mrph->parent ? $mrph->parent->id : -1;
		    $ret .= $mrph->dpndtype . "\n";
		}

		$ret .= $mrph->midasi . ' ' . $mrph->yomi . ' ' . $mrph->genkei . ' ' . $mrph->hinsi . ' ' . $mrph->hinsi_id . ' ' . $mrph->bunrui . ' ' . $mrph->bunrui_id. ' ' . $mrph->katuyou1 . ' ' . $mrph->katuyou1_id . ' ' . $mrph->katuyou2 . ' ' . $mrph->katuyou2_id . ' ' . $mrph->imis . ' ' . $mrph->fstring . "\n";
	    }

	    # SynGraph
	    for my $synnodes ($tag->synnodes) {
		$ret .= '!! ';
		$ret .= $synnodes->tagid . ' ' . $synnodes->parent . $synnodes->dpndtype . " <$MIDASI:" . $synnodes->midasi . '>' . $synnodes->feature . "\n";

		for my $synnode ($synnodes->synnode) {
		    $ret .= '! ';
		    $ret .= $synnode->tagid . ' <SYNID:' . $synnode->synid . "><$SCORE:" . $synnode->score . '>' . $synnode->feature . "\n";
		}
	    }
	}
    }
    $ret .= $this->{_eos};

    return $ret;
}

=item comment

構文解析結果中のコメントを返す．

=cut
sub comment {
    my( $this ) = @_;
    $this->{comment} || undef;
}

=item error

構文解析結果中のエラーメッセージを返す．

=cut
sub error {
    my( $this ) = @_;
    $this->{error} || undef;
}

=item id

構文解析結果IDを得る．

=cut
sub id {
    my $this = shift;
    if( @_ ){
	$this->set_id( @_ );
    } else {
	unless( defined $this->{_id} ){
	    $this->{_id} = $this->{comment} =~ m/# S-ID:(\S+)/ ? $1 : -1;
	}
	$this->{_id};
    }
}

=item set_id ( ID )

構文解析結果IDを設定する．

=cut
sub set_id {
    my( $this, $id ) = @_;
    if( defined $this->{comment} ){
	( $this->{comment} =~ s/# S-ID:\S+/# S-ID:$id/ )
	    or ( $this->{comment} = "S-ID:$id\n" . $this->{comment} );
    } else {
	$this->{comment} = "S-ID:$id\n";
    }
    $this->{_id} = $id;
}

=item version

バージョンを得る．

=cut
sub version {
    my $this = shift;
    unless( defined $this->{_version} ){
	$this->{_version} = $this->{comment} =~ m/ KNP:(\S+)/ ? $1 : -1;
    }
    $this->{_version};
}

=item eos

解析結果終了区切りを得る

=cut
sub eos {
    my $this = shift;
    if( @_ ){
	$this->set_eos( @_ );
    }
    else {
	my $eos = $this->{_eos};
	chomp($eos); # delete newline for return
	$eos;
    }
}

=item set_eos ( EOS )

解析結果終了区切りを設定する．

=cut
sub set_eos {
    my( $this, $eos ) = @_;
    $eos .= "\n" unless $eos =~ /\n$/; # append newline if unavailable
    $this->{_eos} = $eos;
    chomp($eos); # delete newline for return
    $eos;
}

=item spec

構文解析結果を表現する文字列を生成する．
SynGraph部分は除かれる．

=cut
sub spec {
    my( $this ) = @_;
    sprintf( "%s%s%s",
	     $this->{comment},
	     $this->KNP::BList::spec(),
	     $this->{_eos} );
}

=item make_ss

標準構造(Standard Structure)を返す．

=cut
sub make_ss {
    my ( $this ) = @_;

    my %ss;

    $ss{sentence}{id} = $this->id;
    $ss{sentence}{comment} = $this->comment;
    chomp $ss{sentence}{comment};

    $ss{sentence}{phrase} = [];

    my $phrase = $ss{sentence}{phrase};

    # 基本句を順番に
    foreach my $tag ( $this->tag ) {
	# 子供
	my @child_ids;
	if (defined $tag->child) {
	    foreach my $ctag ($tag->child) {
		push @child_ids, $ctag->id;
	    }
	}

	push @{$phrase}, { id => $tag->id,
			   fstring => $tag->fstring,
			   dpndtype => $tag->dpndtype,
			   parent => defined $tag->parent ? $tag->parent->id : -1,
			   child => join('/', @child_ids)
		       };

	push @{$phrase->[-1]{node}}, { type => 'base' };

	# 形態素を順番に
	foreach my $mrph ( $tag->mrph ) {
	    my $repname = $mrph->repname ? $mrph->repname : $mrph->genkei . '/' .  $mrph->yomi;

	    push @{$phrase->[-1]{node}[0]{word}}, { fstring => $mrph->fstring,
						    content => $mrph->midasi, # <word ...>(ここに入る)</word> 
						    katuyou1 => $mrph->katuyou1 eq '*' ? '' : $mrph->katuyou1,
						    katuyou2 => $mrph->katuyou2 eq '*' ? '' : $mrph->katuyou2,
						    repname => $repname,
						    imis => $mrph->imis eq 'NIL' ? '' : $mrph->imis,
						    yomi => $mrph->yomi,
						    hinsi => $mrph->hinsi,
						    bunrui => $mrph->bunrui eq '*' ? '' : $mrph->bunrui
						    };
	}
    }

    return \%ss;
}

=item all_xml

XMLを返す．

=cut
sub all_xml {
    my ( $this ) = @_;

    require XML::Simple;

    my $xs = new XML::Simple;

    my $ss = $this->make_ss;

    my $xml = $xs->XMLout($ss, KeepRoot => 1);

    return $xml;
}

=back

=head1 SEE ALSO

=over 4

=item *

L<KNP::BList>

=back

=head1 AUTHOR

=over 4

=item
土屋 雅稔 <tsuchiya@pine.kuee.kyoto-u.ac.jp>

=cut

1;
__END__
# Local Variables:
# mode: perl
# use-kuten-for-period: nil
# use-touten-for-comma: nil
# End:
