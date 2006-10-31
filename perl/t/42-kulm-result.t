# -*- perl -*-

use strict;
use Test;

BEGIN { plan tests => 17 }

unless( eval { require KULM::KNP::Result; } ){
    print STDERR "KULM::KNP::Result is missing.  Skip all tests.\n";
    for( 1 .. 17 ){
	print "ok $_\n";
    }
    exit 0;
}
use KNP::Result;

my $str = "構文解析の実例を示す．";
my $result = <<'__result__';
# S-ID:123
* 1D <BGH:解析><SM:解析:711006601***71100650****><文頭><サ変><助詞><体言><係:ノ格><区切:0-4><RID:992><抽象>
構文 こうぶん 構文 名詞 6 普通名詞 1 * 0 * 0 NIL <文頭><漢字><かな漢字><自立><名詞相当語>
解析 かいせき 解析 名詞 6 サ変名詞 2 * 0 * 0 NIL <漢字><かな漢字><サ変><自立><←複合><名詞相当語>
の の の 助詞 9 接続助詞 3 * 0 * 0 NIL <かな漢字><ひらがな><付属>
* 2D <BGH:実例><SM:実例:31212*******312232******311006b0****><ヲ><助詞><体言><係:ヲ格><区切:0-0><RID:1031><抽象>
実例 じつれい 実例 名詞 6 普通名詞 1 * 0 * 0 NIL <漢字><かな漢字><自立><名詞相当語>
を を を 助詞 9 格助詞 1 * 0 * 0 NIL <かな漢字><ひらがな><付属>
* -1D <BGH:示す><文末><句点><用言:動><レベル:C><区切:5-5><ID:（文末）><RID:110><提題受:30>
示す しめす 示す 動詞 2 * 0 子音動詞サ行 5 基本形 2 NIL <表現文末><かな漢字><自立><活用語>
． ． ． 特殊 1 句点 1 * 0 * 0 NIL <文末><付属>
EOS
__result__

my $x = KNP::Result->new( $result );
my $y = KULM::KNP::Result->new( input => $str );
$y->setup( [ map( "$_\n", split(/\n/, $result) ) ],
	   { PATTERN => '^EOS$',
	     OPTION => { bclass => "KULM::KNP::B",
			 mclass => "KULM::KNP::M",
			 option => "-tab", } } );

ok( defined $x );
ok( defined $y );
ok( $x->mrph_list == $y->mrph_list );
ok( $x->bnst_list == $y->bnst_list );
ok( &midasi($x) eq &midasi($y) );

sub midasi {
    my( $ml ) = @_;
    join( "", map( $_->midasi, $ml->mrph_list ) );
}

my $p = ( $x->bnst_list )[1];
my $q = ( $y->bnst_list )[1];
ok( $p->string() eq $q->string() );
ok( $p->string(undef, "all") eq $q->string(undef, "all") );
ok( $p->get('ID') == $q->get('ID') );
ok( $p->get('P') and $q->get('P') and $p->get('P')->string(undef, "all") eq $q->get('P')->string(undef, "all") );
ok( $p->get('D') eq $q->get('D') );
ok( &attr_midasi($p, 'C') eq &attr_midasi($q, 'C') );
ok( &attr_midasi($p, 'ML') eq &attr_midasi($q, 'ML') );
ok( $p->get('FS') eq $q->get('FS') );
ok( join("\n", @{$p->get('F')}) eq join("\n", @{$q->get('F')}) );
ok( $p->get( [ F => 1 ] ) eq $q->get( [ F => 1 ] ) );
ok( $p->get('string') eq $q->get('string') );
ok( $p->get('p_id') == $q->get('p_id') );

sub attr_midasi {
    my( $b, $attr ) = @_;
    join( "\n", map( $_->string(undef, "all"), @{$b->get($attr)} ) );
}
