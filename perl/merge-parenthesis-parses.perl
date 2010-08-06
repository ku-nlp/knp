#!/usr/bin/env perl

# A post-processor of parentheses

# --mrph: 形態素レベルで括弧をマージ (default)
# --bnst: 文節レベルで括弧をマージ
# --nbest-max-n integer: nbest時のnの数の制限 (デフォルト: 10)

# $Id$

use strict;
use KNP::File;
use Storable qw(dclone);
use Getopt::Long;
use encoding 'euc-jp';
binmode(STDERR, ':encoding(euc-jp)');

use FileHandle;
STDOUT->autoflush(1);

our (%opt);
&GetOptions(\%opt, 'mrph', 'bnst', 'nbest-max-n=i');
$opt{mrph} = 1 if !$opt{mrph} and !$opt{bnst};
$opt{'nbest-max-n'} = 10 if !$opt{'nbest-max-n'};

our %PAREN_MORPHEME = ('（' => '（ （ （ 特殊 1 括弧始 3 * 0 * 0 <記英数カ><英記号><記号><括弧始><括弧><接頭><非独立接頭辞>', 
		      '）' => '） ） ） 特殊 1 括弧終 4 * 0 * 0 <記英数カ><英記号><記号><括弧終><括弧><述語区切><付属>');


my $knp = new KNP::File(file => $ARGV[0]); # , encoding => 'euc-jp');
my (@main_sentences, @paren_sentences, $pre_paren_id);
my $paren_num = -1;
while (my $result = $knp->each()) {
    if ($result->comment =~ /括弧位置:(\d+)/) { # 括弧文
	my $start_pos = $1;
	my ($paren_b) = ($result->comment =~ /括弧始:(\S+)/);
	my ($paren_e) = ($result->comment =~ /括弧終:(\S+)/);
	$paren_num++ if $pre_paren_id ne $result->id; # nbest用に、入れる番号を調整
	my ($score) = ($result->comment =~ /SCORE:([\.\-\d])+/);
	push(@{$paren_sentences[$paren_num]}, {result => $result, score => $score, 
				start_pos => $start_pos, paren_b => $paren_b, paren_e => $paren_e});
	$pre_paren_id = $result->id;

	# 一文終了=EOS
	if ($result->eos eq 'EOS') {
	    my $count = 0;
	    for my $main_sentence (sort {$b->{score} <=> $a->{score}} @main_sentences) { # for nbest
		&recover_sid($main_sentence->{result}); # もとのS-IDに復元(-01削除)
		# 括弧マージ処理
		&select_merge_print_paren($main_sentence, \@paren_sentences, 0, [], 
					  ($count + 1 == scalar(@main_sentences) or $count + 1 >= $opt{'nbest-max-n'}) ? 1 : 0);
		last if ++$count >= $opt{'nbest-max-n'};
	    }
	    @main_sentences = ();
	    @paren_sentences = ();
	    $pre_paren_id = '';
	    $paren_num = -1;
	}
    }
    else { # 原文
	if ($result->comment =~ /括弧削除/) { # 後続が括弧文 -> 後で処理後にprint
	    my ($score) = ($result->comment =~ /SCORE:([\.\-\d])+/);
	    push(@main_sentences, {result => $result, score => $score, type => 'main'}); # nbest時は複数個保持
	}
	else {
	    print $result->all;
	}
    }
}

# END


sub select_merge_print_paren {
    my ($main_sentence, $paren_sentences_ar, $paren_num, $target_paren_ar, $eos_flag) = @_;

    if (defined($paren_sentences_ar->[$paren_num])) {
	# 再帰的に括弧文を選択 (for nbest)
	my $count = 0;
	for my $i (sort {$paren_sentences_ar->[$paren_num][$b]{score} <=> $paren_sentences_ar->[$paren_num][$a]{score}} 0 .. $#{$paren_sentences_ar->[$paren_num]}) {
	    push(@{$target_paren_ar}, $paren_sentences_ar->[$paren_num][$i]);
	    &select_merge_print_paren($main_sentence, $paren_sentences_ar, $paren_num + 1, $target_paren_ar, 
				      ($eos_flag and ($count + 1 == scalar(@{$paren_sentences_ar->[$paren_num]}) or $count + 1 >= $opt{'nbest-max-n'})) ? 1 : 0);
	    pop(@{$target_paren_ar});
	    last if ++$count >= $opt{'nbest-max-n'};
	}
    }
    else {
	# すべて選択が終わったので、マージして表示
	&merge_print_paren_sentences($main_sentence, $target_paren_ar, $eos_flag);
    }
}

sub recover_sid {
    my ($result) = @_;

    my $sid = $result->id;
    $sid =~ s/-01$//;
    $result->id($sid);

    $result->{comment} =~ s/ 括弧削除//;
}

sub get_score {
    my ($result) = @_;

    if ($result->{comment} =~ /SCORE:([-\.\d]+)/) {
	return $1;
    }
    else {
	return 0;
    }
}

sub update_score {
    my ($result, $paren_score) = @_;

    my $new_score = sprintf("%.5f", &get_score($result) + $paren_score);
    $result->{comment} =~ s/SCORE:([-\.\d]+)/SCORE:$new_score/;
}

# main: 括弧マージ処理
sub merge_print_paren_sentences {
    my ($orig_sentence, $paren_sentences_ar, $eos_flag) = @_;

    my $orig_modified_sentence_result = dclone($orig_sentence->{result}); # まるごとcopy (for nbnst)
    my @bnsts = $orig_modified_sentence_result->bnst;
    my $paren_score = 0;

    for my $j (0 .. $#{$paren_sentences_ar}) { # 括弧文loop
	my $paren_sentence = dclone($paren_sentences_ar->[$j]); # まるごとcopy
	$paren_score += &get_score($paren_sentence->{result});
	my @paren_bnsts = $paren_sentence->{result}->bnst;
	my $pos = 0;
	my $merged_flag = 0;
	for my $i (0 .. $#bnsts) {
	    if ($opt{bnst} and $pos >= $paren_sentence->{start_pos}) { # 括弧開始点を越えた最初の文節
		&insert_paren_bnst(\@bnsts, $i, \@paren_bnsts, $paren_sentence->{paren_b}, $paren_sentence->{paren_e});
		$merged_flag = 1;
		last;
	    }

	    my @mrphs = $bnsts[$i]->mrph;
	    for my $m (0 .. $#mrphs) {
		my $mrph = $mrphs[$m];
		if ($opt{mrph} and $pos >= $paren_sentence->{start_pos}) { # 括弧開始点を越えた最初の形態素
		    &insert_paren_mrph(\@bnsts, $i, \@paren_bnsts, $paren_sentence->{paren_b}, $paren_sentence->{paren_e}, $m);
		    $merged_flag = 1;
		    last;
		}
		$pos += utf8::is_utf8($mrph->midasi) ? length($mrph->midasi) : length($mrph->midasi) / 2; # 字数
	    }
	    if ($opt{mrph} and $merged_flag == 0 and $pos >= $paren_sentence->{start_pos}) { # 括弧が文節末
		&insert_paren_mrph(\@bnsts, $i, \@paren_bnsts, $paren_sentence->{paren_b}, $paren_sentence->{paren_e}, -1);
		$merged_flag = 1;
	    }
	    last if $merged_flag;
	}
	if ($opt{bnst} and $merged_flag == 0 and $pos >= $paren_sentence->{start_pos}) { # 括弧が文末の場合
	    &insert_paren_bnst(\@bnsts, scalar(@bnsts), \@paren_bnsts, $paren_sentence->{paren_b}, $paren_sentence->{paren_e});
	}
    }

    $orig_modified_sentence_result->{bnst} = \@bnsts;
    &update_score($orig_modified_sentence_result, $paren_score);

    # 表示
    $orig_modified_sentence_result->set_eos('EOS') if $eos_flag; # EOP -> EOS
    print $orig_modified_sentence_result->all_dynamic;
}

sub insert_paren_bnst {
    my ($bnsts_ar, $b_pos, $paren_bnsts_ar, $paren_b, $paren_e) = @_;

    # $b_pos: 挿入位置文節
    my $paren_b_num = scalar(@{$paren_bnsts_ar});   # 括弧内の文節数
    my $t_pos = &count_tag($bnsts_ar, $b_pos);      # 挿入位置基本句
    my $paren_t_num = &count_tag($paren_bnsts_ar);  # 括弧内の基本句数
    my $m_pos = &count_mrph($bnsts_ar, $b_pos);     # 挿入位置形態素
    my $paren_m_num = &count_mrph($paren_bnsts_ar); # 括弧内の形態素数

    # 括弧始と括弧終を括弧文に追加
    if (&insert_paren_to_tag($paren_bnsts_ar, $paren_b, $paren_e, $paren_m_num)) { # 成功
	$paren_m_num += 2; # 括弧自体の分
    }

    # 原文の$b_posから後側のidを更新
    for my $i ($b_pos .. $#{$bnsts_ar}) {
	my $bnst = $bnsts_ar->[$i];
	$bnst->{id} = $bnst->id + $paren_b_num;
	for my $tag ($bnst->tag) {
	    $tag->{id} = $tag->id + $paren_t_num;
	    for my $mrph ($tag->mrph) {
		$mrph->{id} = $mrph->id + $paren_m_num;
	    }
	}
    }

    if ($b_pos == 0) { # 括弧が先頭のとき
	# 括弧文の最後の文節の親 = 原文の文末の文節
	$paren_bnsts_ar->[$#{$paren_bnsts_ar}]->parent($bnsts_ar->[-1]);
	$bnsts_ar->[-1]->child($bnsts_ar->[-1]->child, $paren_bnsts_ar->[$#{$paren_bnsts_ar}]);
	# 基本句
	($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->parent(($bnsts_ar->[-1]->tag)[-1]);
	($bnsts_ar->[-1]->tag)[-1]->child(($bnsts_ar->[-1]->tag)[-1]->child, ($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]);
	# 形態素
	my $head_mrph = &find_head_mrph(($bnsts_ar->[-1]->tag)[-1]);
	(($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-1]->parent($head_mrph);
	$head_mrph->child($head_mrph->child, (($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-1]);
    }
    else {
	# 括弧文のIDを更新
	for my $bnst (@{$paren_bnsts_ar}) {
	    $bnst->{id} = $bnst->id + $b_pos;
	    for my $tag ($bnst->tag) {
		$tag->{id} = $tag->id + $t_pos;
		for my $mrph ($tag->mrph) {
		    $mrph->{id} = $mrph->id + $m_pos;
		}
	    }
	}

	# 括弧文の最後の文節の親 = 原文の括弧の前の文節
	$paren_bnsts_ar->[$#{$paren_bnsts_ar}]->parent($bnsts_ar->[$b_pos - 1]);
	$bnsts_ar->[$b_pos - 1]->child($bnsts_ar->[$b_pos - 1]->child, $paren_bnsts_ar->[$#{$paren_bnsts_ar}]);
	# 基本句
	($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->parent(($bnsts_ar->[$b_pos - 1]->tag)[-1]);
	($bnsts_ar->[$b_pos - 1]->tag)[-1]->child(($bnsts_ar->[$b_pos - 1]->tag)[-1]->child, ($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]);
	# 形態素
	my $head_mrph = &find_head_mrph(($bnsts_ar->[$b_pos - 1]->tag)[-1]);
	(($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-1]->parent($head_mrph);
	$head_mrph->child($head_mrph->child, (($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-1]);
    }

    # 括弧文を挿入
    splice(@{$bnsts_ar}, $b_pos, 0, @{$paren_bnsts_ar});
}

sub insert_paren_mrph {
    my ($bnsts_ar, $b_pos, $paren_bnsts_ar, $paren_b, $paren_e, $m_pos_in_bnst) = @_;

    # $b_pos: 挿入位置文節
    my $paren_m_num = &count_mrph($paren_bnsts_ar); # 括弧内の形態素数
    my ($m_pos, $target_mrph);
    if ($m_pos_in_bnst == -1) { # 文節末のとき
	$m_pos = $bnsts_ar->[$b_pos]->mrph(-1)->id + 1; # 挿入位置形態素
	$target_mrph = $bnsts_ar->[$b_pos]->mrph(-1); # 括弧をくっつける親形態素
    }
    else {
	$m_pos = $bnsts_ar->[$b_pos]->mrph($m_pos_in_bnst)->id; # 挿入位置形態素
	$target_mrph = $bnsts_ar->[$b_pos]->mrph($m_pos_in_bnst - 1); # 括弧をくっつける親形態素
    }

    # 括弧始と括弧終を括弧文に追加
    if (&insert_paren_to_tag($paren_bnsts_ar, $paren_b, $paren_e, $paren_m_num)) { # 成功
	$paren_m_num += 2; # 括弧自体の分
    }

    # 原文の$b_posから後側のidを更新
    for my $i ($b_pos .. $#{$bnsts_ar}) {
	my $bnst = $bnsts_ar->[$i];
	for my $tag ($bnst->tag) {
	    for my $mrph ($tag->mrph) {
		if ($mrph->id >= $m_pos) {
		    $mrph->{id} = $mrph->id + $paren_m_num;
		}
	    }
	}
    }

    if ($m_pos == 0) { # 括弧が先頭のとき
	(($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-2]->parent((($bnsts_ar->[0]->tag)[0]->mrph)[0]);
	(($bnsts_ar->[0]->tag)[0]->mrph)[0]->child((($bnsts_ar->[0]->tag)[0]->mrph)[0]->child, (($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-2]);
    }
    else {
	# 括弧文のIDを更新
	for my $bnst (@{$paren_bnsts_ar}) {
	    for my $tag ($bnst->tag) {
		for my $mrph ($tag->mrph) {
		    $mrph->{id} = $mrph->id + $m_pos;
		}
	    }
	}

	(($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-2]->parent($target_mrph);
	$target_mrph->child($target_mrph->child, (($paren_bnsts_ar->[$#{$paren_bnsts_ar}]->tag)[-1]->mrph)[-2]);
    }

    # 括弧文を挿入
    my ($target_tag, $m_pos_in_tag) = &find_target_tag($bnsts_ar->[$b_pos], $m_pos_in_bnst); # 対象の基本句
    my $added_mrph_count = 0;
    for my $bnst (@{$paren_bnsts_ar}) {
	for my $tag ($bnst->tag) {
	    splice(@{$target_tag->{mrph}}, $m_pos_in_tag + $added_mrph_count, 0, $tag->mrph);
	    $added_mrph_count += scalar($tag->mrph);
	}
    }
}

sub insert_paren_to_tag {
    my ($paren_bnsts_ar, $paren_b, $paren_e, $paren_m_num) = @_;

    if (defined($PAREN_MORPHEME{$paren_b})) {
	my $new_mrph = new KNP::Morpheme($PAREN_MORPHEME{$paren_b}, 0, $paren_m_num, 'D');
	for my $bnst (@{$paren_bnsts_ar}) { # 形態素IDを+1
	    for my $tag ($bnst->tag) {
		for my $mrph ($tag->mrph) {
		    $mrph->{id} = $mrph->id + 1;
		}
	    }
	}
	$new_mrph->parent((($paren_bnsts_ar->[-1]->tag)[-1]->mrph)[-1]);
	(($paren_bnsts_ar->[-1]->tag)[-1]->mrph)[-1]->child($new_mrph);
 	unshift(@{($paren_bnsts_ar->[0]->tag)[0]->{mrph}}, $new_mrph);
    }
    else {
	warn("Unknown parenthesis: $paren_b\n");
	return 0;
    }

    if (defined($PAREN_MORPHEME{$paren_e})) {
	my $new_mrph = new KNP::Morpheme($PAREN_MORPHEME{$paren_e}, $paren_m_num + 1, $paren_m_num, 'D');
	$new_mrph->parent((($paren_bnsts_ar->[-1]->tag)[-1]->mrph)[-1]);
	(($paren_bnsts_ar->[-1]->tag)[-1]->mrph)[-1]->child($new_mrph);
	push(@{($paren_bnsts_ar->[-1]->tag)[-1]->{mrph}}, $new_mrph);
    }
    else {
	warn("Unknown parenthesis: $paren_e\n");
	return 0;
    }

    return 1;
}

sub count_tag {
    my ($bnsts_ar, $b_pos) = @_;

    my $count = 0;
    for my $i (0 .. $#{$bnsts_ar}) {
	last if defined($b_pos) and $i >= $b_pos;
	for my $tag ($bnsts_ar->[$i]->tag) {
	    $count++;
	}
    }

    return $count;
}

sub count_mrph {
    my ($bnsts_ar, $b_pos) = @_;

    my $count = 0;
    for my $i (0 .. $#{$bnsts_ar}) {
	last if defined($b_pos) and $i >= $b_pos;
	for my $tag ($bnsts_ar->[$i]->tag) {
	    for my $mrph ($tag->mrph) {
		$count++;
	    }
	}
    }

    return $count;
}

sub find_head_mrph {
    my ($tag_r) = @_;

    for my $mrph (reverse($tag_r->mrph)) {
	if ($mrph->fstring =~ /<(?:準)?内容語>/) {
	    return $mrph;
	}
    }

    return ($tag_r->mrph)[-1];
}

# 文節とその文節中の形態素位置から基本句とその基本句の形態素位置を返す
sub find_target_tag {
    my ($bnst_r, $m_pos_in_bnst) = @_;

    my $m_pos = 0;
    my $pre_m_pos;
    for my $tag ($bnst_r->tag) {
	$pre_m_pos = $m_pos;
	$m_pos += scalar($tag->mrph);
	if ($m_pos >= $m_pos_in_bnst and $m_pos_in_bnst != -1) {
	    return ($tag, $m_pos_in_bnst - $pre_m_pos);
	}
    }

    if ($m_pos_in_bnst == -1) { # 末尾指定
	return ($bnst_r->tag(-1), $m_pos - $pre_m_pos);
    }
}
